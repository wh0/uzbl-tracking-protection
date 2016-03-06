CFLAGS=-std=c11 -Ofast

all: check-tracking cooked.bin

cooked.bin: stove.py disconnect-blacklist.json disconnect-entitylist.json
	python $<

disconnect-%.json:
	curl -LO https://github.com/mozilla-services/shavar-prod-lists/raw/master/$@

.PHONY: all
