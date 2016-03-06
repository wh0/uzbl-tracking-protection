#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct table {
	uint32_t num_buckets;
	uint32_t buckets[];
};

struct table_entry {
	uint32_t key;
	uint32_t value;
};

static uint32_t table_lookup(const uint32_t *base, const uint8_t *string_base, uint32_t table_index, uint32_t key_hash, const uint8_t *key) {
	const struct table *table = (const void *) &base[table_index];
	if (table -> num_buckets == 0) return 0;
	unsigned int bucket = key_hash % table->num_buckets;
	uint32_t entry_index = table->buckets[bucket];
	const struct table_entry *entry = (const void *) &base[entry_index];
	while (entry->key != 0) {
		if (strcmp(key, &string_base[entry->key]) == 0) return entry->value;
	}
	return 0;
}

#define MAX_PARTS 5

struct suffix {
	uint32_t hash;
	const uint8_t *pos;
};

static size_t reverse_hash_parts(struct suffix *out, const uint8_t *start, const uint8_t *end) {
	size_t count = 0;
	uint32_t hash = 0;
	while (count != MAX_PARTS && end-- != start) {
		hash = 33 * hash + *end;
		if (end == start || *(end - 1) == '.') out[count++] = (struct suffix) {hash, end};
	}
	return count;
}

static uint32_t suffix_lookup(const uint32_t *base, const uint8_t *string_base, uint32_t table_index, const uint8_t *start, const uint8_t *end) {
	struct suffix suffixes[MAX_PARTS];
	size_t count = reverse_hash_parts(suffixes, start, end);
	while (count-- > 0) {
		uint32_t value = table_lookup(base, string_base, table_index, suffixes[count].hash, suffixes[count].pos);
		if (value != 0) return value;
	}
	return 0;
}

static void isolate_hostname(const uint8_t **start, const uint8_t **end, uint8_t *uri) {
	uint8_t *scheme_end = strchr(uri, ':');
	if (scheme_end == NULL) return;
	uint8_t *authority_start = scheme_end + 3;
	uint8_t *authority_end = strchr(uri, '/');
	if (authority_end == NULL) return;
	*authority_end = '\0';
	uint8_t *userinfo_end = strchr(authority_start, '@');
	uint8_t *hostname_start = userinfo_end == NULL ? authority_start : userinfo_end + 1;
	uint8_t *hostname_end = strchr(hostname_start, ':');
	hostname_end = hostname_end == NULL ? authority_end : hostname_end;
	*hostname_end = '\0';
	*start = hostname_start;
	*end = hostname_end;
}

#define CLASS_ALLOW 0
#define CLASS_BLOCK 1

static int classify_request(const uint32_t *base, const uint8_t *string_base, uint32_t root_index, const uint8_t *resource_hostname_start, const uint8_t *resource_hostname_end, const uint8_t *page_hostname_start, const uint8_t *page_hostname_end) {
	uint32_t blacklist_index = suffix_lookup(base, string_base, root_index, resource_hostname_start, resource_hostname_end);
	if (blacklist_index == 0) return CLASS_ALLOW;
	uint32_t whitelist_index = suffix_lookup(base, string_base, blacklist_index, page_hostname_start, page_hostname_end);
	return whitelist_index == 0 ? CLASS_BLOCK : CLASS_ALLOW;
}

struct header {
	uint32_t root_index;
	uint32_t string_index;
};

int main(int argc, char **argv) {
	if (argc < 2) return 1;
	char *resource_uri = argv[1];
	char *page_uri = getenv("UZBL_URI");
	if (page_uri == NULL) return 1;

	int fd = open("cooked.bin", O_RDONLY);
	if (fd == -1) return perror("cooked.bin"), 1;
	struct stat stat;
	if (fstat(fd, &stat) == -1) return perror("stat"), 1;
	const void *data = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) return perror("mmap"), 1;
	const struct header *header = data;
	const uint32_t *base = data;
	const uint8_t *string_base = (const void *) &base[header->string_index];

	const uint8_t *resource_hostname_start = NULL, *resource_hostname_end = NULL;
	isolate_hostname(&resource_hostname_start, &resource_hostname_end, resource_uri);
	const uint8_t *page_hostname_start = NULL, *page_hostname_end = NULL;
	isolate_hostname(&page_hostname_start, &page_hostname_end, page_uri);
	int classification = classify_request(base, string_base, header->root_index, resource_hostname_start, resource_hostname_end, page_hostname_start, page_hostname_end);
	if (classification == CLASS_BLOCK) printf("about:blank\n");

	return 0;
}
