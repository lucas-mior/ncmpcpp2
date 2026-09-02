#!/bin/sh -e

# shellcheck disable=SC2086,SC2031

dir=$(dirname "$(readlink -f "$0")")
cd "$dir" || exit

# shellcheck source=./cbase/common.sh
. ./cbase/common.sh

program=$(common_get_program "$0")
script=$(basename "$0")
common_build_parse_args "$@"

case "$mode" in
build|check|clean|debug|debug-fast|fast_feedback|install|test|uninstall)
    ;;
*)
    common_build_unknown_mode
    ;;
esac

common_build_tags
common_build_print_invocation "$script"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

exe="bin/${program}"
mkdir -p "$(dirname "$exe")"

CC=$(common_get_compiler "$mode")

CPPFLAGS="$CPPFLAGS -I. -Isrc -Icbase"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"

CFLAGS="$CFLAGS -pthread"
LDFLAGS="$LDFLAGS -lm"

find_curses_pkg() {
    if [ -n "${CURSES_PKG:-}" ]; then
        if pkg-config --exists "$CURSES_PKG"; then
            return 0
        fi

        return 1
    fi

    for package in ncursesw ncurses; do
        if pkg-config --exists "$package"; then
            CURSES_PKG=$package
            return 0
        fi
    done

    CURSES_PKG=ncursesw
    return 1
}

load_package_flags() {
    if ! find_curses_pkg \
            || ! pkg-config --exists \
                'libmpdclient >= 2.8' \
                "$CURSES_PKG" \
                'fftw3 >= 3' \
                libcurl \
                taglib_c; then
        missing_packages='libmpdclient >= 2.8, ncursesw/ncurses, fftw3 >= 3,'
        missing_packages="$missing_packages libcurl, taglib_c"
        error "missing pkg-config packages: $missing_packages"
        exit 1
    fi

    PKG_CFLAGS=$(pkg-config --cflags \
        'libmpdclient >= 2.8' \
        "$CURSES_PKG" \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)
    PKG_LIBS=$(pkg-config --libs \
        'libmpdclient >= 2.8' \
        "$CURSES_PKG" \
        'fftw3 >= 3' \
        libcurl \
        taglib_c)

    if pkg-config --exists readline 2>/dev/null; then
        READLINE_CFLAGS=$(pkg-config --cflags readline)
        READLINE_LIBS=$(pkg-config --libs readline)
    else
        READLINE_CFLAGS=
        case "$(uname -s)" in
        OpenBSD)
            READLINE_LIBS='-lreadline'
            ;;
        *)
            READLINE_LIBS='-lreadline -lhistory'
            ;;
        esac
    fi

    CFLAGS="$CFLAGS $READLINE_CFLAGS"
    LDFLAGS="$LDFLAGS $READLINE_LIBS"

    return 0
}

case "$mode" in
debug|test)
    CFLAGS="$CFLAGS -g3 -Og -DDEBUGGING=1"
    ;;
debug-fast)
    CFLAGS="$CFLAGS -g2 -O2 -flto"
    CPPFLAGS="$CPPFLAGS -DDEBUGGING=1"
    ;;
build)
    CFLAGS="$CFLAGS -O2 -flto"
    ;;
fast_feedback)
    ;;
check)
    ;;
build|check|clean|debug|debug-fast|fast_feedback|install|test|uninstall)
    ;;
*)
    common_build_unknown_mode
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
build|debug-fast|fast_feedback)
    load_package_flags

    trace_on
    $CC $CPPFLAGS $PKG_CFLAGS $CFLAGS -o $exe src/main.c $PKG_LIBS $LDFLAGS
    trace_off
    ;;
check)
    common_build_run_analyzers build
    ;;
test)
    load_package_flags

    CPPFLAGS="$CPPFLAGS -Itests"
    TEST_CPPFLAGS="$TEST_CPPFLAGS $PKG_CFLAGS"
    LDFLAGS="$PKG_LIBS $LDFLAGS"
    TEST_REQUIRE_TESTING_MARKER=0
    common_test "$target" tests
    ;;
install)
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
    echo "Unknown mode $mode"
    exit 1
    ;;
esac
