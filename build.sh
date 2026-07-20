#!/bin/sh

set -eu

# if command ctags && command vtags.sed ; then
#     find . -iname "*.[ch]" | xargs ctags --kinds-C=+l+d 2> /dev/null || true
#     vtags.sed tags | sort | uniq > .tags.vim       2> /dev/null || true
# fi

BUILD_DIR=${BUILD_DIR-build}
PREFIX=${PREFIX-/usr/local}
BINDIR=${BINDIR-$PREFIX/bin}
DOCDIR=${DOCDIR-$PREFIX/share/doc/ncmpcpp}
MANDIR=${MANDIR-$PREFIX/share/man}
DESTDIR=${DESTDIR-}

PKG_CONFIG=${PKG_CONFIG-pkg-config}
ORIGINAL_CC=${CC-}
CLANG_ANALYZER=${CLANG_ANALYZER-clang}

CPPFLAGS=${CPPFLAGS-}
if [ "${CFLAGS+x}" = x ]; then
    ORIGINAL_CFLAGS_SET=1
else
    ORIGINAL_CFLAGS_SET=0
fi
ORIGINAL_CFLAGS=${CFLAGS-}
CFLAGS=
LDFLAGS=${LDFLAGS-}
LDLIBS=${LDLIBS--lm}

TEST_CFLAGS=${TEST_CFLAGS--Wno-psabi -Wno-constant-logical-operand}

OS=$(uname -a)
CC=
THREAD_FLAGS=-pthread
NCMPCPP_SOURCE=src/main.c

PKG_CFLAGS=
PKG_LIBS=
READLINE_CFLAGS=
READLINE_LIBS=
CSTD=${CSTD-}
ANALYZER_CSTD=${ANALYZER_CSTD-}
TEMP_FILE=

cleanup() {
    if [ -n "$TEMP_FILE" ]; then
        rm -f "$TEMP_FILE"
    fi

    return 0
}

trap cleanup 0
trap 'cleanup; exit 129' 1
trap 'cleanup; exit 130' 2
trap 'cleanup; exit 143' 15

die() {
    printf '%s\n' "$1" >&2
    exit 1
}

command_name() {
    command_string=$1

    # Command variables may contain wrappers, for example CC='ccache gcc'.
    # shellcheck disable=SC2086
    set -- $command_string
    if [ "$#" -eq 0 ]; then
        printf '%s\n' ''
        return 0
    fi

    printf '%s\n' "$1"
}

require_command() {
    required_command=$(command_name "$1")

    if ! command -v "$required_command" >/dev/null 2>&1; then
        die "missing command: $required_command"
    fi

    return 0
}

configure_compiler_flags() {
    target=$1

    CC=$ORIGINAL_CC
    if [ "$target" = test ] && [ -z "$CC" ] \
        && command -v tcc >/dev/null 2>&1; then
        CC=tcc
    else
        CC=${CC:-cc}
    fi

    case $target in
    debug)
        CFLAGS='-g3 -O0'
        ;;
    build)
        CFLAGS='-O2 -flto'
        ;;
    *)
        if [ "$ORIGINAL_CFLAGS_SET" = 1 ]; then
            CFLAGS=$ORIGINAL_CFLAGS
        else
            CFLAGS='-O0 -g3'
        fi
        ;;
    esac

    CFLAGS="$CFLAGS -Wfatal-errors"
    CFLAGS="$CFLAGS -Wextra -Wall"
    # CFLAGS="$CFLAGS -Werror"
    CFLAGS="$CFLAGS -Wno-format-pedantic"
    CFLAGS="$CFLAGS -Wno-unknown-warning-option"
    CFLAGS="$CFLAGS -Wno-gnu-union-cast"
    CFLAGS="$CFLAGS -Wno-unused-macros"
    CFLAGS="$CFLAGS -Wno-unused-function"
    CFLAGS="$CFLAGS -Wno-constant-logical-operand"
    CFLAGS="$CFLAGS -Wno-float-equal"
    CFLAGS="$CFLAGS -Wno-undefined-internal"
    CFLAGS="$CFLAGS -Wno-cast-qual"
    CFLAGS="$CFLAGS -Wno-unknown-pragmas"

    if [ "$CC" = clang ]; then
        CFLAGS="$CFLAGS -Weverything"
        CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
        CFLAGS="$CFLAGS -Wno-format-nonliteral"
        CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
        CFLAGS="$CFLAGS -Wno-c++-keyword"
        CFLAGS="$CFLAGS -Wno-pre-c11-compat"
        CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
        CFLAGS="$CFLAGS -Wno-ignored-attributes"
        CFLAGS="$CFLAGS -Wno-covered-switch-default"
        CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
        CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
        CFLAGS="$CFLAGS -Wno-assign-enum"
        CFLAGS="$CFLAGS -Wno-cast-function-type-strict"
        CFLAGS="$CFLAGS -Wno-bad-function-cast"
        CFLAGS="$CFLAGS -Wno-pre-c23-compat"
        CFLAGS="$CFLAGS -Wno-padded"
        CFLAGS="$CFLAGS -Wno-nrvo"
        CFLAGS="$CFLAGS -Wno-cast-align"
        CFLAGS="$CFLAGS -Wno-tentative-definition-compat"
    fi

    return 0
}

run_command() {
    command_string=$1
    shift

    # Commands such as CC='ccache gcc' intentionally require word splitting.
    # shellcheck disable=SC2086
    $command_string "$@"
}

pkg_config() {
    run_command "$PKG_CONFIG" "$@"
}

detect_c_standard() {
    compiler=$1

    for standard in -std=c23 -std=c2x; do
        if printf 'int main(void) { return 0; }\n' \
            | run_command "$compiler" "$standard" -x c -c \
                -o /dev/null - >/dev/null 2>&1; then
            printf '%s\n' "$standard"
            return 0
        fi
    done

    return 1
}

load_package_flags() {
    require_command "$PKG_CONFIG"

    if ! pkg_config --exists \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c; then
        missing_packages='libmpdclient >= 2.8, ncursesw, fftw3 >= 3,'
        missing_packages="$missing_packages libcurl, taglib_c"
        die "missing pkg-config packages: $missing_packages"
    fi

    PKG_CFLAGS=$(pkg_config --cflags \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)
    PKG_LIBS=$(pkg_config --libs \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)

    if pkg_config --exists readline 2>/dev/null; then
        READLINE_CFLAGS=$(pkg_config --cflags readline)
        READLINE_LIBS=$(pkg_config --libs readline)
    else
        READLINE_CFLAGS=
        READLINE_LIBS='-lreadline -lhistory'
    fi

    return 0
}

check_no_foreign_sources() {
    bad_files=$(find src -type f \
        |awk '/[.](cc|c[p]p|cxx)$/ { print }')

    if [ -n "$bad_files" ]; then
        printf '%s\n' 'Non-C source files are not allowed:' >&2
        printf '%s\n' "$bad_files" >&2
        exit 1
    fi

    return 0
}

prepare_compiler() {
    load_package_flags
    require_command "$CC"

    if [ -z "$CSTD" ]; then
        if ! CSTD=$(detect_c_standard "$CC"); then
            die 'C compiler does not support C23 or C2x'
        fi
    fi

    return 0
}

compile_main() {
    object_dir=$BUILD_DIR/obj/src
    object=$object_dir/main.c.o
    temporary_object=$object.tmp.$$

    mkdir -p "$object_dir"
    TEMP_FILE=$temporary_object

    # Flag variables intentionally require shell word splitting.
    # shellcheck disable=SC2086
    run_command "$CC" \
        -I"$BUILD_DIR" \
        -I. \
        -Isrc \
        -D_GNU_SOURCE \
        -D_DEFAULT_SOURCE \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $CSTD \
        $CFLAGS \
        $THREAD_FLAGS \
        -c "$NCMPCPP_SOURCE" \
        -o "$temporary_object"

    mv "$temporary_object" "$object"
    TEMP_FILE=

    return 0
}

link_main() {
    object=$BUILD_DIR/obj/src/main.c.o
    binary=$BUILD_DIR/ncmpcpp
    temporary_binary=$binary.tmp.$$

    mkdir -p "$BUILD_DIR"
    TEMP_FILE=$temporary_binary

    # Flag variables intentionally require shell word splitting.
    # shellcheck disable=SC2086
    run_command "$CC" \
        $CFLAGS \
        $LDFLAGS \
        -o "$temporary_binary" \
        "$object" \
        $READLINE_LIBS \
        $PKG_LIBS \
        $LDLIBS \
        $THREAD_FLAGS

    mv "$temporary_binary" "$binary"
    TEMP_FILE=

    return 0
}

build_binary() {
    check_no_foreign_sources
    prepare_compiler
    compile_main
    link_main

    return 0
}

compile_test() {
    source=$1
    test_name=${source#tests/}
    test_name=${test_name%.c}
    test_dir=$BUILD_DIR/tests
    binary=$test_dir/$test_name
    temporary_binary=$binary.tmp.$$

    mkdir -p "$test_dir"
    TEMP_FILE=$temporary_binary

    if [ "${TEST_CPPFLAGS+x}" = x ]; then
        # Flag variables intentionally require shell word splitting.
        # shellcheck disable=SC2086
        run_command "$CC" \
            $TEST_CPPFLAGS \
            $CSTD \
            $CFLAGS \
            $TEST_CFLAGS \
            $THREAD_FLAGS \
            "$source" \
            -o "$temporary_binary" \
            ${TEST_LDLIBS-$LDLIBS $THREAD_FLAGS}
    else
        # Flag variables intentionally require shell word splitting.
        # shellcheck disable=SC2086
        run_command "$CC" \
            -I"$BUILD_DIR" \
            -Itests \
            -I. \
            -Isrc \
            -D_GNU_SOURCE \
            -D_DEFAULT_SOURCE \
            $CPPFLAGS \
            $CSTD \
            $CFLAGS \
            $TEST_CFLAGS \
            $THREAD_FLAGS \
            "$source" \
            -o "$temporary_binary" \
            ${TEST_LDLIBS-$LDLIBS $THREAD_FLAGS}
    fi

    mv "$temporary_binary" "$binary"
    TEMP_FILE=

    "$binary"

    return 0
}

run_tests() {
    require_command "$CC"
    if [ -z "$CSTD" ]; then
        if ! CSTD=$(detect_c_standard "$CC"); then
            die 'C compiler does not support C23 or C2x'
        fi
    fi

    for source in tests/*.c; do
        if [ ! -f "$source" ]; then
            continue
        fi

        compile_test "$source"
    done

    return 0
}

run_analyzer() {
    check_no_foreign_sources
    load_package_flags
    require_command "$CLANG_ANALYZER"

    if [ -z "$ANALYZER_CSTD" ]; then
        if ! ANALYZER_CSTD=$(detect_c_standard "$CLANG_ANALYZER"); then
            die 'clang analyzer does not support C23 or C2x'
        fi
    fi

    # Flag variables intentionally require shell word splitting.
    # shellcheck disable=SC2086
    run_command "$CLANG_ANALYZER" \
        -I"$BUILD_DIR" \
        -I. \
        -Isrc \
        -D_GNU_SOURCE \
        -D_DEFAULT_SOURCE \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $ANALYZER_CSTD \
        $CFLAGS \
        $THREAD_FLAGS \
        --analyze \
        -Xanalyzer -analyzer-output=text \
        -fno-color-diagnostics \
        "$NCMPCPP_SOURCE"

    return 0
}

binary_needs_build() {
    binary=$BUILD_DIR/ncmpcpp

    if [ ! -f "$binary" ]; then
        return 0
    fi

    if find src cbase -type f \
        \( -name '*.c' -o -name '*.h' \) \
        -newer "$binary" -print \
        | grep -q .; then
        return 0
    fi

    return 1
}

install_program() {
    if binary_needs_build; then
        build_binary
    fi

    install -d "$DESTDIR$BINDIR"
    install -m 755 "$BUILD_DIR/ncmpcpp" "$DESTDIR$BINDIR/ncmpcpp"
    install -d "$DESTDIR$DOCDIR"
    install -m 644 AUTHORS COPYING "$DESTDIR$DOCDIR"
    install -m 644 doc/bindings doc/config "$DESTDIR$DOCDIR"
    install -d "$DESTDIR$MANDIR/man1"
    install -m 644 doc/ncmpcpp.1 "$DESTDIR$MANDIR/man1/ncmpcpp.1"

    return 0
}

clean_build() {
    case $BUILD_DIR in
    ''|/)
        die "refusing to remove unsafe BUILD_DIR: $BUILD_DIR"
        ;;
    esac

    rm -rf "$BUILD_DIR"

    return 0
}

show_help() {
    cat <<EOF_HELP
usage: ./build.sh [target ...]

targets:
  debug                    build with CFLAGS=-g3 -O0
  build                    build with CFLAGS=-O2 -flto
  all                      build with the current CFLAGS (default)
  check                    run the clang static analyzer
  test                     build and run all tests
  check-no-foreign-sources reject C++ source files under src
  install                  build and install the program and documentation
  clean                    remove BUILD_DIR
  help                     show this help

common variables:
  CC                 compiler command
  CLANG_ANALYZER     clang command for the check target
  CFLAGS             compiler flags used by all, check, test, and install
  CPPFLAGS, LDFLAGS  extra preprocessor and linker flags
  LDLIBS             extra libraries, default: -lm
  TEST_CPPFLAGS      replacement preprocessor flags for tests
  TEST_CFLAGS        extra compiler flags for tests
  TEST_LDLIBS        libraries for tests
  BUILD_DIR          build output directory, default: build
  PREFIX             install prefix, default: /usr/local
  BINDIR             binary install directory, default: PREFIX/bin
  DOCDIR             docs install directory, default: PREFIX/share/doc/ncmpcpp
  MANDIR             man install directory, default: PREFIX/share/man
EOF_HELP

    return 0
}

run_target() {
    target=$1

    configure_compiler_flags "$target"

    case $target in
    debug|build)
        build_binary
        ;;
    all)
        build_binary
        ;;
    check)
        run_analyzer
        ;;
    test)
        run_tests
        ;;
    check-no-foreign-sources)
        check_no_foreign_sources
        ;;
    install)
        install_program
        ;;
    clean)
        clean_build
        ;;
    help|-h|--help)
        show_help
        ;;
    *)
        printf 'unknown target: %s\n\n' "$target" >&2
        show_help >&2
        exit 1
        ;;
    esac

    return 0
}

if [ "$#" -eq 0 ]; then
    set -- all
fi

for target in "$@"; do
    run_target "$target"
done
