PREFIX ?= /openresty
LUA_LIB_DIR ?= $(PREFIX)/lualib
CFLAGS := -std=c++11 -Wall -O3 -g -fPIC
INSTALL := install
CC := g++

all: libchash.so

libchash.so: chash.o city.o
	$(CC) $(CFLAGS) -shared -o $@ $^

chash.o: consistent_hash_map.cc consistent_hash_map.h
	$(CC) $(CFLAGS) -o $@ -c $<

city.o: city.cc city.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean test

clean:
	@rm -vf *.o *.so

install: all
	$(INSTALL) -d $(DESTDIR)$(LUA_LIB_DIR)/resty/chash
	$(INSTALL) -m0644 *.lua $(DESTDIR)$(LUA_LIB_DIR)/resty/chash
	$(INSTALL) libchash.so $(DESTDIR)$(LUA_LIB_DIR)/libchash.so

test:
	resty chash_test.lua