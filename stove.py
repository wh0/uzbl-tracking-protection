import json

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
					root[domain] = empty_whitelist

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
			root[domain] = whitelist

import struct

data = struct.pack('II', 0, 0)

def current_index():
	return len(data) / 4

string_data = b''
string_table = {}

def get_string_index(s):
	global string_data
	if s not in string_table:
		string_table[s] = len(string_data)
		string_data += s.encode('utf-8') + b'\000'
	return string_table[s]

def write_strings():
	global data
	index = current_index()
	data += string_data
	return index

empty_string_index = get_string_index(u'')

def reverse_hash(s):
	h = 0
	for c in s[::-1]:
		h = (h * 33 + ord(c)) & 0xffffffff
	return h

def write_bucket(b):
	global data
	index = current_index()
	data += b''.join(struct.pack('II', k, v) for k, v in b) + struct.pack('II', 0, 0)
	return index

empty_bucket_index = write_bucket([])

def next_power(n):
	p = 1
	while p < n:
		p *= 2
	return p

worst_bucket = None
worst_bucket_size = 0

def write_table(t):
	global data, worst_bucket, worst_bucket_size
	num_buckets = next_power(len(t))
	buckets = [[] for i in range(num_buckets)]
	for k, v in t.items():
		k_index = get_string_index(k)
		k_hash = reverse_hash(k)
		buckets[k_hash % num_buckets].append((k_index, v))
	bucket_indices = []
	for b in buckets:
		if len(b) == 0:
			bucket_indices.append(empty_bucket_index)
		else:
			b_size = len(b)
			if b_size > worst_bucket_size:
				worst_bucket = (num_buckets, t, b)
				worst_bucket_size = b_size
			bucket_indices.append(write_bucket(b))
	index = current_index()
	data += struct.pack('I', num_buckets) + b''.join(struct.pack('I', bucket_index) for bucket_index in bucket_indices)
	return index

empty_table_index = write_table({})

root_table = {}
for resource_domain in root:
	if root[resource_domain] is empty_whitelist:
		root_table[resource_domain] = empty_table_index
	else:
		whitelist_table = {}
		for property_domain in root[resource_domain]:
			property_domain_index = get_string_index(property_domain)
			whitelist_table[property_domain] = property_domain_index
		root_table[resource_domain] = write_table(whitelist_table)
root_index = write_table(root_table)
string_index = write_strings()
data = struct.pack('II', root_index, string_index) + data[8:]
open('cooked.bin', 'wb').write(data)

reverse = {index: string for string, index in string_table.items()}

print 'worst bucket has %d entries' % worst_bucket_size
print '%d / %d' % (len(worst_bucket[1]), worst_bucket[0])
print [reverse[key] for key, value in worst_bucket[2]]
