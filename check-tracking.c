#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tables.h"

static const void *table_lookup(const struct table *table, uint32_t key_hash, const char *key) {
	if (table->num_buckets == 0) return 0;
	size_t bucket = key_hash % table->num_buckets;
	const struct table_entry *entry = table->buckets[bucket];
	while (entry->key != NULL) {
		if (strcmp(key, entry->key) == 0) return entry->value;
		entry++;
	}
	return NULL;
}

struct start_end {
	const char *start;
	const char *end;
};

#define MAX_PARTS 5

struct suffix {
	uint32_t hash;
	const char *pos;
};

static size_t reverse_hash_parts(struct suffix *out, struct start_end hostname) {
	size_t count = 0;
	uint32_t hash = 0;
	const char *pos = hostname.end;
	while (count != MAX_PARTS && pos-- != hostname.start) {
		hash = 33 * hash + *pos;
		if (pos == hostname.start || *(pos - 1) == '.') out[count++] = (struct suffix) {hash, pos};
	}
	return count;
}

static const void *suffix_lookup(const struct table *table, struct start_end hostname) {
	struct suffix suffixes[MAX_PARTS];
	size_t count = reverse_hash_parts(suffixes, hostname);
	while (count-- != 0) {
		const void *value = table_lookup(table, suffixes[count].hash, suffixes[count].pos);
		if (value != NULL) return value;
	}
	return 0;
}

static void isolate_hostname(struct start_end *hostname, char *uri) {
	char *scheme_end = strstr(uri, "://");
	if (scheme_end == NULL) return;
	char *authority_start = scheme_end + 3;
	char *authority_end = strchr(authority_start, '/');
	if (authority_end == NULL) return;
	*authority_end = '\0';
	char *userinfo_end = strchr(authority_start, '@');
	char *hostname_start = userinfo_end == NULL ? authority_start : userinfo_end + 1;
	char *hostname_end = strchr(hostname_start, ':');
	hostname_end = hostname_end == NULL ? authority_end : hostname_end;
	*hostname_end = '\0';
	hostname->start = hostname_start;
	hostname->end = hostname_end;
}

enum classification {
	CLASS_ALLOW,
	CLASS_BLOCK,
};

static enum classification classify_request(const struct table *root, struct start_end resource, struct start_end page) {
	if (page.start == page.end) {
		fprintf(stderr, "arbitrary\n");
		return CLASS_ALLOW;
	}
	const void *blacklist_value = suffix_lookup(root, resource);
	if (blacklist_value == NULL) {
		fprintf(stderr, "allow\n");
		return CLASS_ALLOW;
	}
	const void *whitelist_value = suffix_lookup(blacklist_value, page);
	if (whitelist_value == NULL) {
		fprintf(stderr, "blacklist\n");
		return CLASS_BLOCK;
	} else {
		fprintf(stderr, "whitelist\n");
		return CLASS_ALLOW;
	}
}

int main(int argc, char **argv) {
	if (argc < 2) return 1;
	char *resource_uri = argv[1];
	char *page_uri = getenv("UZBL_URI");
	if (page_uri == NULL) return 1;
	fprintf(stderr, "%s <- %s = ", page_uri, resource_uri);

	struct start_end resource = {};
	isolate_hostname(&resource, resource_uri);
	struct start_end page = {};
	isolate_hostname(&page, page_uri);
	enum classification classification = classify_request(&root, resource, page);
	if (classification == CLASS_BLOCK) printf("about:blank\n");

	return 0;
}
