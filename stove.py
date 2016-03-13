import json
import re
import sys

root = {}
empty_whitelist = set()

blacklist = json.load(open('disconnect-blacklist.json'))
for name, category in blacklist['categories'].items():
	if 'Legacy' in name or 'Content' in name:
		continue
	for item in category:
		for org_name, top_domains in item.items():
			for top_domain, domains in top_domains.items():
				for domain in domains:
					host_parts = domain.split('.')
					if len(host_parts) > 5:
						raise Exception('%s is too long' % domain)
					root[domain] = (None, empty_whitelist)

entitylist = json.load(open('disconnect-entitylist.json'))
for name, entity in entitylist.items():
	for domain in entity['properties']:
		host_parts = domain.split('.')
		if len(host_parts) > 5:
			raise Exception('%s is too long' % domain)
	whitelist = set(entity['properties'])
	for domain in entity['resources']:
		host_parts = domain.split('.')
		if len(host_parts) > 5:
			raise Exception('%s is too long' % domain)
		if domain in root:
			prev_name, prev_whitelist = root[domain]
			if prev_whitelist is empty_whitelist:
				root[domain] = (name, whitelist)
			else:
				print >> sys.stderr, '%s has multiple resource lists' % domain
				root[domain] = ('%s and %s' % (prev_name, name), prev_whitelist.union(whitelist))

print '#include "tables.h"'
print ''

def to_literal(s):
	return json.dumps(s)

def to_slug(s):
	return re.sub(r'[^A-Za-z0-9]+', '_', s)

def reverse_hash(s):
	h = 0
	for c in s[::-1]:
		h = (h * 33 + ord(c)) & 0xffffffff
	return h

def write_bucket(b, name, linkage='static '):
	print '%sconst struct table_entry %s[] = {' % (linkage, name)
	for kv in b:
		print '\t{%s, %s},' % kv
	print '\t{},'
	print '};'
	print ''
	return name

empty_bucket_name = write_bucket([], 'empty_bucket', '')

def next_power(n):
	if n == 0:
		return 0
	p = 1
	while p < n:
		p *= 2
	return p

worst_bucket = (0, None, None)

def write_table(t, name, linkage='static '):
	global worst_bucket
	num_buckets = next_power(len(t))
	buckets = [[] for i in range(num_buckets)]
	for k, v in t.items():
		k_literal = to_literal(k)
		k_hash = reverse_hash(k)
		buckets[k_hash % num_buckets].append((k_literal, v))
	bucket_names = []
	for bi, b in enumerate(buckets):
		if len(b) == 0:
			bucket_names.append(empty_bucket_name)
		else:
			b_size = len(b)
			if b_size > worst_bucket[0]:
				worst_bucket = (b_size, bi, num_buckets, name, t, b)
			bucket_names.append(write_bucket(b, '%s_bucket_%d' % (name, bi)))
	print '%sconst struct table %s = {%d, {' % (linkage, name, num_buckets)
	for bn in bucket_names:
		print '\t%s,' % bn
	print '}};'
	print ''
	return name

empty_table_name = write_table({}, 'empty_table', '')

whitelist_names = {id(empty_whitelist): empty_table_name}
root_table = {}
for resource_domain, (name, whitelist) in root.items():
	whitelist_id = id(whitelist)
	if whitelist_id not in whitelist_names:
		whitelist_table = {}
		for property_domain in whitelist:
			property_domain_literal = to_literal(property_domain)
			whitelist_table[property_domain] = property_domain_literal
		print '// %s' % name
		whitelist_names[whitelist_id] = write_table(whitelist_table, 'properties_%s' % to_slug(name))
	root_table[resource_domain] = '&%s' % whitelist_names[whitelist_id]
root_name = write_table(root_table, 'root', '')

print >> sys.stderr, 'worst bucket has %d entries' % worst_bucket[0]
print >> sys.stderr, '%d/%d in %s' % (worst_bucket[1], worst_bucket[2], worst_bucket[3])
print >> sys.stderr, [k for k, v in worst_bucket[5]]
