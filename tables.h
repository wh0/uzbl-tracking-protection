#include <stddef.h>

struct table_entry {
	const char *key;
	const void *value;
};

struct table {
	size_t num_buckets;
	const struct table_entry *buckets[];
};

extern const struct table_entry empty_bucket[];
extern const struct table empty_table;

extern const struct table root;
