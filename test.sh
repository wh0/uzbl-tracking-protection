#!/bin/sh -e
test -z "$(UZBL_URI=http://www.example.com/ ./check-tracking http://www.example.org/)"
test "$(UZBL_URI=http://www.example.com/ ./check-tracking http://ad.doubleclick.net/)" = about:blank
test -z "$(UZBL_URI=https://www.google.com/ ./check-tracking http://ad.doubleclick.net/)"
test "$(UZBL_URI=https://www.google.com/ ./check-tracking https://www.facebook.com/)" = about:blank
test -z "$(UZBL_URI=about:blank ./check-tracking https://www.facebook.com/)"
