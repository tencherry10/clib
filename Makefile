
CC      = gcc
PREFIX ?= /usr/local

ifeq ($(shell uname -o),Cygwin)
BINS    = clib.exe clib-install.exe clib-search.exe clib-search-github.exe
LDFLAGS = -lcurl
CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p
else ifeq ($(OS),Windows_NT)
BINS    = clib.exe clib-install.exe clib-search.exe clib-search-github.exe
LDFLAGS = -lcurldll
CP      = copy /Y
RM      = del /Q /S
MKDIR   = mkdir
else
BINS    = clib clib-install clib-search clib-search-github
LDFLAGS = -lcurl
CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p
endif

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard deps/*/*.c)
OBJS = $(DEPS:.c=.o)

CFLAGS  = -std=c99 -Ideps -Wall -Wpedantic -Wno-unused-function -Werror  -U__STRICT_ANSI__
CFLAGS += -Wextra -Wno-unused-parameter -Wno-sign-compare

all: $(BINS)

$(BINS): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/$(@:.exe=).c $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ $(CFLAGS)

clean:
	$(foreach c, $(BINS), $(RM) $(c);)
	$(RM) $(OBJS)

install: $(BINS)
	$(MKDIR) $(PREFIX)/bin
	$(foreach c, $(BINS), $(CP) $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), $(RM) $(PREFIX)/bin/$(c);)

test:
	@./test.sh

.PHONY: test all clean install uninstall
