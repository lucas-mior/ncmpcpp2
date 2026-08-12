#!/bin/sh -e

# shellcheck disable=SC2086,SC2031

dir=$(dirname "$(readlink -f "$0")")
cd "$dir" || exit

# shellcheck source=./cbase/common.sh
. ./cbase/common.sh

program=$(common_get_program "$0")
script=$(basename "$0")
common_build_parse_args "$@"

common_build_print_invocation "$script"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

exe="bin/${program}"
mkdir -p "$(dirname "$exe")"

CC=$(common_get_compiler "$mode")

CPPFLAGS="$CPPFLAGS -I. -Isrc -Icbase"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Werror=all -Werror=extra"
CFLAGS="$CFLAGS -Werror"  # Only uncomment occasionally, keep this line

if [ "$CC" = "clang" ] || [ "$CC" = "zig cc" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-cast-align"
    CFLAGS="$CFLAGS -Wno-cast-qual"
    CFLAGS="$CFLAGS -Wno-constant-logical-operand"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
    CFLAGS="$CFLAGS -Wno-float-equal"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-nrvo"
    CFLAGS="$CFLAGS -Wno-padded"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-tentative-definition-compat"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-unused-macros"
    CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
fi

CFLAGS="$CFLAGS -pthread"

LDFLAGS="$LDFLAGS -lm"

load_package_flags() {
    if ! pkg-config --exists \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c; then
        missing_packages='libmpdclient >= 2.8, ncursesw, fftw3 >= 3,'
        missing_packages="$missing_packages libcurl, taglib_c"
        error "missing pkg-config packages: $missing_packages"
        exit 1
    fi

    PKG_CFLAGS=$(pkg-config --cflags \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)
    PKG_LIBS=$(pkg-config --libs \
        'libmpdclient >= 2.8' \
        ncursesw \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)

    if pkg-config --exists readline 2>/dev/null; then
        READLINE_CFLAGS=$(pkg-config --cflags readline)
        READLINE_LIBS=$(pkg-config --libs readline)
    else
        READLINE_CFLAGS=
        READLINE_LIBS='-lreadline -lhistory'
    fi

    CFLAGS="$CFLAGS $READLINE_CFLAGS"
    LDFLAGS="$LDFLAGS $READLINE_LIBS"

    return 0
}

case "$mode" in
debug|test)
    CFLAGS="$CFLAGS -g3 -Og -DDEBUGGING=1"
    ;;
build)
    CFLAGS="$CFLAGS -O2 -flto"
    ;;
fast_feedback)
    CFLAGS="$CFLAGS -Werror"
    ;;
check)
    ;;
esac

case "$mode" in
debug)
    load_package_flags

    common_build_incremental_binary \
        $exe \
        src \
        src/main.c \
        "$CPPFLAGS $PKG_CFLAGS $CFLAGS" \
        "$PKG_LIBS $LDFLAGS"
    ;;
build|fast_feedback)
    load_package_flags

    trace_on
    $CC \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $CFLAGS \
        -o $exe \
        src/main.c \
        $PKG_LIBS \
        $LDFLAGS
    trace_off
    ;;
check)
    set +e
    CC=gcc CFLAGS="-fanalyzer -fdiagnostics-color=never" "$0" debug

    CFLAGS="--analyze -Xanalyzer -analyzer-output=text"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-werror"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-opt-analyze-headers"
    CFLAGS="$CFLAGS -Wno-unused-command-line-argument"
    CFLAGS="$CFLAGS -fno-color-diagnostics"
    CC=clang CFLAGS="$CFLAGS" "$0" debug
    exit
    ;;
test)
    CPPFLAGS="$CPPFLAGS -Itests"
    TEST_REQUIRE_TESTING_MARKER=0
    common_test "$target" tests
    ;;
install)
    "$0" build

    install -d "${DESTDIR}${PREFIX}/bin"
    install -m 755 "$exe" "${DESTDIR}${PREFIX}/bin/$program"
    install -d "${DESTDIR}${PREFIX}/share/doc/$program"
    install -m 644 AUTHORS LICENSE "${DESTDIR}${PREFIX}/share/doc/$program"
    install -m 644 doc/bindings doc/config \
        "${DESTDIR}${PREFIX}/share/doc/$program"
    install -d "${DESTDIR}${PREFIX}/share/man/man1"
    install -m 644 "doc/$program.1" \
        "${DESTDIR}${PREFIX}/share/man/man1/$program.1"
    ;;
uninstall)
    rm -f "${DESTDIR}${PREFIX}/bin/$program"
    rm -f "${DESTDIR}${PREFIX}/share/man/man1/$program.1"
    rm -rf "${DESTDIR}${PREFIX}/share/doc/$program"
    ;;
clean)
    rm -rf bin/obj/
    rm -rf bin/
    ;;
*)
    printf 'unknown mode: %s\n\n' "$mode" >&2
    exit 1
    ;;
esac
