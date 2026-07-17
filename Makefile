PACKAGE := ncmpcpp
VERSION := 0.10.2_dev

BUILD_DIR ?= build
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DOCDIR ?= $(PREFIX)/share/doc/ncmpcpp
MANDIR ?= $(PREFIX)/share/man
DESTDIR ?=

PKG_CONFIG ?= pkg-config
CC ?= cc
CLANG_ANALYZER ?= clang

CPPFLAGS ?=
CFLAGS ?= -O0 -g3
LDFLAGS ?=
LDLIBS ?= -lm

PKG_DEPS := 'libmpdclient >= 2.8' ncursesw 'fftw3 >= 3' libcurl taglib_c
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS) 2>/dev/null)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_DEPS) 2>/dev/null)
READLINE_CFLAGS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --cflags readline; fi)
READLINE_LIBS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --libs readline; else printf '%s' '-lreadline -lhistory'; fi)
CSTD ?= $(shell if printf 'int main(void) { return 0; }\n' | $(CC) -std=c23 -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c23; elif printf 'int main(void) { return 0; }\n' | $(CC) -std=c2x -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c2x; fi)
ANALYZER_CSTD ?= $(shell if printf 'int main(void) { return 0; }\n' | $(CLANG_ANALYZER) -std=c23 -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c23; elif printf 'int main(void) { return 0; }\n' | $(CLANG_ANALYZER) -std=c2x -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c2x; fi)

COMMON_CPPFLAGS := \
	-I$(BUILD_DIR) \
	-I. \
	-Isrc \
	-D_GNU_SOURCE \
	-D_DEFAULT_SOURCE \
	$(CPPFLAGS) \
	$(PKG_CFLAGS) \
	$(READLINE_CFLAGS)
WARNINGS := -Wall -Wextra -Wfatal-errors
THREAD_FLAGS := -pthread

OBJ_DIR := $(BUILD_DIR)/obj
TOOLS_STAMP := $(BUILD_DIR)/.tools-ok
BINARY := $(BUILD_DIR)/ncmpcpp
NCMPCPP_SOURCE := src/main.c
NCMPCPP_OBJECT := $(OBJ_DIR)/src/main.c.o
DEPS := $(NCMPCPP_OBJECT:.o=.d)

TEST_SOURCES := $(sort $(wildcard tests/*.c))
TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,\
    $(TEST_SOURCES))
TEST_RUNS := $(TEST_BINARIES:%=%.run)
TEST_CPPFLAGS ?= \
    -I$(BUILD_DIR) \
    -Itests \
    -I. \
    -Isrc \
    -D_GNU_SOURCE \
    -D_DEFAULT_SOURCE \
    $(CPPFLAGS)
TEST_CFLAGS ?= -Wno-psabi -Wno-constant-logical-operand
TEST_LDLIBS ?= $(LDLIBS) $(THREAD_FLAGS)

.PHONY: all check test install clean help check-no-foreign-sources FORCE
.DELETE_ON_ERROR:
.SECONDARY: $(NCMPCPP_OBJECT) $(TEST_BINARIES)

all: check-no-foreign-sources $(BINARY)

$(TOOLS_STAMP): FORCE
	@mkdir -p $(BUILD_DIR)
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { printf 'missing command: %s\n' '$(PKG_CONFIG)' >&2; exit 1; }
	@$(PKG_CONFIG) --exists $(PKG_DEPS) || { printf 'missing pkg-config package(s): %s\n' "$(PKG_DEPS)" >&2; exit 1; }
	@test -n "$(CSTD)" || { printf '%s\n' 'C compiler does not support C23 or C2x' >&2; exit 1; }
	@touch $@

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) \
		$(COMMON_CPPFLAGS) \
		$(CSTD) \
		$(WARNINGS) \
		$(CFLAGS) \
		$(THREAD_FLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(BINARY): $(NCMPCPP_OBJECT)
	$(CC) $(LDFLAGS) -o $@ \
		$(NCMPCPP_OBJECT) \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)

$(BUILD_DIR)/tests/%: tests/%.c
	@mkdir -p $(@D)
	$(CC) \
		$(TEST_CPPFLAGS) \
		$(CSTD) \
		$(WARNINGS) \
		$(CFLAGS) \
		$(TEST_CFLAGS) \
		$(THREAD_FLAGS) \
		$< \
		-o $@ \
		$(TEST_LDLIBS)

$(BUILD_DIR)/tests/%.run: $(BUILD_DIR)/tests/%
	$<

test: $(TEST_RUNS)

check-no-foreign-sources:
	@bad_files=$$(find src -type f \
		| awk '/[.](cc|c[p]p|cxx)$$/ { print }'); \
	if test -n "$$bad_files"; then \
		printf '%s\n' 'Non-C source files are not allowed:' >&2; \
		printf '%s\n' "$$bad_files" >&2; \
		exit 1; \
	fi

check: check-no-foreign-sources
	@command -v $(CLANG_ANALYZER) >/dev/null 2>&1 || { \
		printf 'missing command: %s\n' '$(CLANG_ANALYZER)' >&2; \
		exit 1; \
	}
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { \
		printf 'missing command: %s\n' '$(PKG_CONFIG)' >&2; \
		exit 1; \
	}
	@$(PKG_CONFIG) --exists $(PKG_DEPS) || { \
		printf 'missing pkg-config package(s): %s\n' "$(PKG_DEPS)" >&2; \
		exit 1; \
	}
	@test -n "$(ANALYZER_CSTD)" || { \
		printf '%s\n' 'clang analyzer does not support C23 or C2x' >&2; \
		exit 1; \
	}
	$(CLANG_ANALYZER) \
		$(COMMON_CPPFLAGS) \
		$(ANALYZER_CSTD) \
		$(WARNINGS) \
		$(CFLAGS) \
		$(THREAD_FLAGS) \
		--analyze \
		-Xanalyzer -analyzer-output=text \
		-fno-color-diagnostics \
		$(NCMPCPP_SOURCE)

install: $(BINARY)
	install -d '$(DESTDIR)$(BINDIR)'
	install -m 755 '$(BINARY)' '$(DESTDIR)$(BINDIR)/ncmpcpp'
	install -d '$(DESTDIR)$(DOCDIR)'
	install -m 644 AUTHORS COPYING '$(DESTDIR)$(DOCDIR)'
	install -m 644 doc/bindings doc/config '$(DESTDIR)$(DOCDIR)'
	install -d '$(DESTDIR)$(MANDIR)/man1'
	install -m 644 doc/ncmpcpp.1 '$(DESTDIR)$(MANDIR)/man1/ncmpcpp.1'

clean:
	rm -rf '$(BUILD_DIR)'

help:
	@printf '%s\n' 'usage: make [all|check|test|install|clean|help]'
	@printf '%s\n' ''
	@printf '%s\n' 'Common variables:'
	@printf '%s\n' '  CC                 compiler command'
	@printf '%s\n' '  CLANG_ANALYZER     clang command for make check'
	@printf '%s\n' '  CFLAGS            extra compiler flags'
	@printf '%s\n' '  CPPFLAGS, LDFLAGS  extra preprocessor and linker flags'
	@printf '%s\n' '  LDLIBS             extra libraries, default: -lm'
	@printf '%s\n' '  TEST_CPPFLAGS      preprocessor flags for make test'
	@printf '%s\n' '  TEST_CFLAGS        extra compiler flags for make test'
	@printf '%s\n' '  TEST_LDLIBS        libraries for make test'
	@printf '%s\n' '  BUILD_DIR          build output directory, default: build'
	@printf '%s\n' '  PREFIX             install prefix, default: /usr/local'
	@printf '%s\n' '  BINDIR             binary install directory, default: PREFIX/bin'
	@printf '%s\n' '  DOCDIR             docs install directory, default: PREFIX/share/doc/ncmpcpp'
	@printf '%s\n' '  MANDIR             man install directory, default: PREFIX/share/man'

-include $(DEPS)
