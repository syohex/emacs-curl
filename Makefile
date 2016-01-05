EMACS_ROOT ?= ../..
EMACS ?= emacs

CC      = gcc
LD      = gcc
CPPFLAGS = -I$(EMACS_ROOT)/src
CFLAGS = -std=gnu99 -ggdb3 -O2 -Wall -fPIC $(CPPFLAGS)

.PHONY : clean test

all: curl-core.so

curl-core.so: curl-core.o
	$(LD) -shared $(LDFLAGS) -o $@ $^ -lcurl

curl-core.o: curl-core.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f curl-core.o curl-core.so

test:
	$(EMACS) -Q -batch -L . \
		-l test/test.el \
		-f ert-run-tests-batch-and-exit
