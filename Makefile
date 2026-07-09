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
CXX ?= c++
AR ?= ar

CPPFLAGS ?=
CFLAGS ?= -O0 -g3
CXXFLAGS ?= -O0 -g3
LDFLAGS ?=
LDLIBS ?= -lm
CXXSTD ?= -std=c++20

PKG_DEPS := 'libmpdclient >= 2.8' ncursesw 'fftw3 >= 3' libcurl taglib_c
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS) 2>/dev/null)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_DEPS) 2>/dev/null)
READLINE_CFLAGS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --cflags readline; fi)
READLINE_LIBS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --libs readline; else printf '%s' '-lreadline -lhistory'; fi)
CSTD ?= $(shell if printf 'int main(void) { return 0; }\n' | $(CC) -std=c23 -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c23; elif printf 'int main(void) { return 0; }\n' | $(CC) -std=c2x -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c2x; fi)

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
CONFIG_H := $(BUILD_DIR)/config.h
BINARY := $(BUILD_DIR)/ncmpcpp
CBASE_LIB := $(BUILD_DIR)/libcbase.a
NCMPCPP_C_LIB := $(BUILD_DIR)/libncmpcpp_c.a
NCMPCPP_APP_C_LIB := $(BUILD_DIR)/libncmpcpp_app_c.a

CBASE_SRCS := cbase/cbase.c
NCMPCPP_C_SRCS := $(shell find src/c -type f -name '*.c' | sort)
APP_C_SRCS := $(shell find src -type f -name '*.c' ! -path 'src/c/*' | sort)
APP_CXX_SRCS := \
	src/actions_legacy.cpp \
	src/configuration_legacy.cpp \
	src/mpdpp.cpp \
	src/settings_legacy.cpp \
	src/status_legacy.cpp
TEST_SRCS := $(sort $(wildcard tests/*_test.c))

CBASE_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(CBASE_SRCS))
NCMPCPP_C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(NCMPCPP_C_SRCS))
APP_C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(APP_C_SRCS))
APP_CXX_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.cpp.o,$(APP_CXX_SRCS))
TEST_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(TEST_SRCS))
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRCS))
DEPS := \
	$(CBASE_OBJS:.o=.d) \
	$(NCMPCPP_C_OBJS:.o=.d) \
	$(APP_C_OBJS:.o=.d) \
	$(APP_CXX_OBJS:.o=.d) \
	$(TEST_OBJS:.o=.d)

.PHONY: all check install clean help FORCE
.DELETE_ON_ERROR:
.SECONDARY: $(CBASE_OBJS) $(NCMPCPP_C_OBJS) $(APP_C_OBJS) $(APP_CXX_OBJS) $(TEST_OBJS)

all: $(BINARY)

$(TOOLS_STAMP): FORCE
	@mkdir -p $(BUILD_DIR)
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { printf 'missing command: %s\n' '$(PKG_CONFIG)' >&2; exit 1; }
	@$(PKG_CONFIG) --exists $(PKG_DEPS) || { printf 'missing pkg-config package(s): %s\n' "$(PKG_DEPS)" >&2; exit 1; }
	@test -n "$(CSTD)" || { printf '%s\n' 'C compiler does not support C23 or C2x' >&2; exit 1; }
	@printf 'int main() { return 0; }\n' | $(CXX) $(CXXSTD) -x c++ -c -o /dev/null - >/dev/null 2>&1 || { printf '%s\n' 'C++ compiler does not support C++20' >&2; exit 1; }
	@touch $@

$(CONFIG_H): | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@{ \
		printf '%s\n' '#ifndef NCMPCPP_CONFIG_H'; \
		printf '%s\n' '#define NCMPCPP_CONFIG_H'; \
		printf '\n'; \
		printf '%s\n' '#define ENABLE_OUTPUTS 1'; \
		printf '%s\n' '#define ENABLE_VISUALIZER 1'; \
		printf '%s\n' '#define HAVE_CURL_CURL_H 1'; \
		printf '%s\n' '#define HAVE_CURSES_H 1'; \
		printf '%s\n' '#define HAVE_FFTW3_H 1'; \
		printf '%s\n' '#define HAVE_LIBNCURSESW 1'; \
		printf '%s\n' '#define HAVE_LIBREADLINE 1'; \
		printf '%s\n' '#define HAVE_MPD_CLIENT_H 1'; \
		printf '%s\n' '#define HAVE_NETINET_IN_H 1'; \
		printf '%s\n' '#define HAVE_NETINET_TCP_H 1'; \
		printf '%s\n' '#define HAVE_READLINE_HISTORY 1'; \
		printf '%s\n' '#define HAVE_READLINE_HISTORY_H 1'; \
		printf '%s\n' '#define HAVE_READLINE_READLINE_H 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_FILE_NEW 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_FILE_SAVE 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_H 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_GET 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_SET 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_SET_APPEND 1'; \
		printf '%s\n' '#define PACKAGE "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_NAME "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_STRING "$(PACKAGE) $(VERSION)"'; \
		printf '%s\n' '#define PACKAGE_TARNAME "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_VERSION "$(VERSION)"'; \
		printf '%s\n' '#define VERSION "$(VERSION)"'; \
		printf '\n'; \
		printf '%s\n' '#endif'; \
	} >$@

$(OBJ_DIR)/%.c.o: %.c $(CONFIG_H) | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@printf 'CC  %s\n' '$<'
	@$(CC) \
		$(COMMON_CPPFLAGS) \
		$(CSTD) \
		$(WARNINGS) \
		$(CFLAGS) \
		$(THREAD_FLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(OBJ_DIR)/%.cpp.o: %.cpp $(CONFIG_H) | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@printf 'CXX %s\n' '$<'
	@$(CXX) \
		$(COMMON_CPPFLAGS) \
		$(CXXSTD) \
		$(WARNINGS) \
		$(CXXFLAGS) \
		$(THREAD_FLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(CBASE_LIB): $(CBASE_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(NCMPCPP_C_LIB): $(NCMPCPP_C_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(NCMPCPP_APP_C_LIB): $(APP_C_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(BINARY): $(APP_C_OBJS) $(APP_CXX_OBJS) $(NCMPCPP_C_LIB) $(CBASE_LIB)
	@printf 'LD  %s\n' '$@'
	@$(CXX) $(LDFLAGS) -o $@ \
		$(APP_C_OBJS) \
		$(APP_CXX_OBJS) \
		$(NCMPCPP_C_LIB) \
		$(CBASE_LIB) \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)

$(BUILD_DIR)/tests/%: $(OBJ_DIR)/tests/%.c.o $(NCMPCPP_C_LIB) $(NCMPCPP_APP_C_LIB) $(CBASE_LIB)
	@mkdir -p $(@D)
	@printf 'LD  %s\n' '$@'
	@$(CC) $(LDFLAGS) -o $@ \
		$< \
		-Wl,--start-group \
		$(NCMPCPP_APP_C_LIB) \
		$(NCMPCPP_C_LIB) \
		$(CBASE_LIB) \
		-Wl,--end-group \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)


check: $(TEST_BINS)
	@set -e; \
	for test in $(TEST_BINS); do \
		printf 'TEST %s\n' "$$test"; \
		"$$test"; \
	done

install: $(BINARY)
	install -d '$(DESTDIR)$(BINDIR)'
	install -m 755 '$(BINARY)' '$(DESTDIR)$(BINDIR)/ncmpcpp'
	install -d '$(DESTDIR)$(DOCDIR)'
	install -m 644 AUTHORS CHANGELOG.md COPYING '$(DESTDIR)$(DOCDIR)'
	install -m 644 doc/bindings doc/config '$(DESTDIR)$(DOCDIR)'
	install -d '$(DESTDIR)$(MANDIR)/man1'
	install -m 644 doc/ncmpcpp.1 '$(DESTDIR)$(MANDIR)/man1/ncmpcpp.1'

clean:
	rm -rf '$(BUILD_DIR)'

help:
	@printf '%s\n' 'usage: make [all|check|install|clean|help]'
	@printf '%s\n' ''
	@printf '%s\n' 'Common variables:'
	@printf '%s\n' '  CC, CXX, AR        compiler and archive commands'
	@printf '%s\n' '  CFLAGS, CXXFLAGS   extra compiler flags'
	@printf '%s\n' '  CPPFLAGS, LDFLAGS  extra preprocessor and linker flags'
	@printf '%s\n' '  LDLIBS             extra libraries, default: -lm'
	@printf '%s\n' '  BUILD_DIR          build output directory, default: build'
	@printf '%s\n' '  PREFIX             install prefix, default: /usr/local'
	@printf '%s\n' '  BINDIR             binary install directory, default: PREFIX/bin'
	@printf '%s\n' '  DOCDIR             docs install directory, default: PREFIX/share/doc/ncmpcpp'
	@printf '%s\n' '  MANDIR             man install directory, default: PREFIX/share/man'

-include $(DEPS)
