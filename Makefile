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
AUTOTOOLS_PRUNE_LIST := REMOVE_AUTOTOOLS_FILES.txt
BINARY := $(BUILD_DIR)/ncmpcpp
CBASE_LIB := $(BUILD_DIR)/libcbase.a
NCMPCPP_C_LIB := $(BUILD_DIR)/libncmpcpp_c.a
NCMPCPP_APP_C_LIB := $(BUILD_DIR)/libncmpcpp_app_c.a

CBASE_SRCS := cbase/cbase.c
NCMPCPP_C_SRCS := $(shell find src/c -type f -name '*.c' | sort)
APP_C_SRCS := $(shell find src -type f -name '*.c' ! -path 'src/c/*' | sort)
REMOVED_CXX_SRCS := \
	src/actions.cpp \
	src/bindings.cpp \
	src/configuration.cpp \
	src/curl_handle.cpp \
	src/charset.cpp \
	src/curses/formatted_color.cpp \
	src/curses/scrollpad.cpp \
	src/curses/window.cpp \
	src/display.cpp \
	src/format.cpp \
	src/settings.cpp \
	src/title.cpp \
	src/statusbar.cpp \
	src/status.cpp \
	src/lastfm_service.cpp \
	src/lyrics_fetcher.cpp \
	src/enums.cpp \
	src/global.cpp \
	src/helpers.cpp \
	src/macro_utilities.cpp \
	src/mutable_song.cpp \
	src/screens/help.cpp \
	src/screens/help_bridge.cpp \
	src/screens/outputs.cpp \
	src/screens/outputs_bridge.cpp \
	src/screens/server_info.cpp \
	src/screens/server_info_bridge.cpp \
	src/screens/song_info.cpp \
	src/screens/song_info_bridge.cpp \
	src/screens/native_c_screens.cpp \
	src/screens/screen.cpp \
	src/screens/screen_legacy.cpp \
	src/screens/screen_type.cpp \
	src/song.cpp \
	src/song_list.cpp \
	src/tags.cpp \
	src/utility/comparators.cpp \
	src/utility/html.cpp \
	src/utility/option_parser.cpp \
	src/utility/sample_buffer.cpp \
	src/utility/string.cpp \
	src/utility/utf8.cpp
APP_CXX_SRCS := $(filter-out $(REMOVED_CXX_SRCS),\
	$(shell find src -type f -name '*.cpp' | sort))
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

.PHONY: all check install clean prune-autotools cxx-report cxx-count no-cxx help FORCE
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
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)


cxx-report:
	@printf '%s\n' 'C++ sources still built by the root Makefile:'
	@if test -n "$(strip $(APP_CXX_SRCS))"; then \
		for src in $(APP_CXX_SRCS); do \
			printf '%s\n' "$$src"; \
		done; \
	else \
		printf '%s\n' '  none'; \
	fi
	@printf '\n%s\n' 'C++ sources present but not built by the root Makefile:'
	@tmp_all=$$(mktemp); \
	tmp_built=$$(mktemp); \
	find . -type f -name '*.cpp' | sed 's#^./##' | sort >"$$tmp_all"; \
	for src in $(APP_CXX_SRCS); do \
		printf '%s\n' "$$src"; \
	done | sort >"$$tmp_built"; \
	other_cxx=$$(comm -23 "$$tmp_all" "$$tmp_built"); \
	rm -f "$$tmp_all" "$$tmp_built"; \
	if test -n "$$other_cxx"; then \
		printf '%s\n' "$$other_cxx"; \
	else \
		printf '%s\n' '  none'; \
	fi

cxx-count:
	@set -- $(APP_CXX_SRCS); printf 'built_cpp=%s\n' "$$#"
	@find . -type f -name '*.cpp' | wc -l | tr -d ' ' | sed 's/^/total_cpp=/'


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

prune-autotools:
	@test -f '$(AUTOTOOLS_PRUNE_LIST)' || { \
		printf 'missing %s\n' '$(AUTOTOOLS_PRUNE_LIST)' >&2; \
		exit 1; \
	}
	@while IFS= read -r path; do \
		case $$path in ''|'#'*) continue ;; esac; \
		printf 'RM  %s\n' "$$path"; \
		rm -rf -- "$$path"; \
	done < '$(AUTOTOOLS_PRUNE_LIST)'

no-cxx:
	@status=0; \
	source_files=$$(find src extras -type f \
		\( -name '*.c' -o -name '*.h' -o -name '*.cpp' \
		-o -name '*.cc' -o -name '*.cxx' -o -name '*.hpp' \
		-o -name '*.hh' -o -name '*.hxx' \) | sort); \
	cxx_files=$$(find src extras -type f \
		\( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \
		-o -name '*.hpp' -o -name '*.hh' -o -name '*.hxx' \) \
		| sort); \
	if test -n "$$cxx_files"; then \
		printf '%s\n' 'C++ source files remain:' >&2; \
		printf '%s\n' "$$cxx_files" >&2; \
		status=1; \
	fi; \
	if test -n "$$source_files"; then \
		std_include_pattern='^[[:space:]]*#[[:space:]]*include[[:space:]]*<(algorithm|array|atomic|bit|bitset|cassert|cctype|cerrno|cfenv|cfloat|charconv|chrono|cinttypes|climits|clocale|cmath|codecvt|compare|complex|concepts|condition_variable|csetjmp|csignal|cstdarg|cstddef|cstdint|cstdio|cstdlib|cstring|ctime|cwchar|cwctype|deque|exception|execution|filesystem|format|forward_list|fstream|functional|future|initializer_list|iomanip|ios|iosfwd|iostream|istream|iterator|limits|list|locale|map|memory|mutex|new|numbers|numeric|optional|ostream|queue|random|ranges|ratio|regex|scoped_allocator|set|shared_mutex|span|sstream|stack|stdexcept|streambuf|string|string_view|strstream|syncstream|system_error|thread|tuple|type_traits|typeindex|typeinfo|unordered_map|unordered_set|utility|valarray|variant|vector)>'; \
		std_includes=$$(grep -nE "$$std_include_pattern" $$source_files || true); \
		if test -n "$$std_includes"; then \
			printf '%s\n' 'C++ standard library includes remain:' >&2; \
			printf '%s\n' "$$std_includes" >&2; \
			status=1; \
		fi; \
		token_pattern='(^|[^[:alnum:]_])(namespace|class|template|virtual|override|try|catch|throw|new|delete)([^[:alnum:]_]|$$)|std::'; \
		tokens=$$(grep -nE "$$token_pattern" $$source_files || true); \
		if test -n "$$tokens"; then \
			printf '%s\n' 'C++ constructs remain:' >&2; \
			printf '%s\n' "$$tokens" >&2; \
			status=1; \
		fi; \
	fi; \
	if test "$$status" -eq 0; then \
		printf '%s\n' 'No C++ sources or banned C++ constructs remain.'; \
	fi; \
	exit "$$status"

help:
	@printf '%s\n' 'usage: make [all|check|install|clean|prune-autotools|cxx-report|cxx-count|no-cxx|help]'
	@printf '%s\n' ''
	@printf '%s\n' 'Only the project root Makefile is supported during migration.'
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
