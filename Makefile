CFLAGS := -std=c11 -Ofast
LDFLAGS := -s

all: check-tracking

disconnect-%.json:
	curl -LO https://github.com/mozilla-services/shavar-prod-lists/raw/master/$@

cooked.c: export PYTHONIOENCODING := UTF-8
cooked.c: stove.py disconnect-blacklist.json disconnect-entitylist.json
	python $< > $@

cooked.o: tables.h

check-tracking.o: tables.h

check-tracking: cooked.o

test: check-tracking
	./test.sh

.PHONY: all test
