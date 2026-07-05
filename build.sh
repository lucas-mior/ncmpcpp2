#!/bin/sh

set -eu

export LC_ALL=C

cd -- "$(dirname -- "$0")"
PROJECT_DIR=$(pwd -P)
BUILD_DIR=${BUILD_DIR:-"$PROJECT_DIR/build"}
PREFIX=${PREFIX:-/usr/local}
BINDIR=${BINDIR:-"$PREFIX/bin"}
DOCDIR=${DOCDIR:-"$PREFIX/share/doc/ncmpcpp"}
MANDIR=${MANDIR:-"$PREFIX/share/man"}
PKG_CONFIG=${PKG_CONFIG:-pkg-config}
CC=${CC:-cc}
CXX=${CXX:-c++}
AR=${AR:-ar}

VERSION=0.10.2_dev
PACKAGE=ncmpcpp

fail() {
    printf '%s\n' "$1" >&2
    exit 1
}

usage() {
    cat <<EOF_USAGE
usage: ./build.sh [build|check|install|clean|prune-autotools]

Environment variables:
  CC, CXX, AR       compiler and archive commands
  CFLAGS, CXXFLAGS  extra compiler flags
  CPPFLAGS, LDFLAGS extra preprocessor and linker flags
  BUILD_DIR         build output directory, default: ./build
  PREFIX            install prefix, default: /usr/local
  BINDIR            binary install directory, default: PREFIX/bin
  DOCDIR            docs install directory, default: PREFIX/share/doc/ncmpcpp
  MANDIR            man install directory, default: PREFIX/share/man
EOF_USAGE
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "missing command: $1"
}

require_pkg() {
    "$PKG_CONFIG" --exists "$1" || fail "missing pkg-config package: $1"
}

pkg_flags() {
    require_command "$PKG_CONFIG"
    require_pkg 'libmpdclient >= 2.8'
    require_pkg ncursesw
    require_pkg 'fftw3 >= 3'
    require_pkg libcurl
    require_pkg taglib_c

    PKG_CFLAGS=$(
        "$PKG_CONFIG" --cflags \
            'libmpdclient >= 2.8' \
            ncursesw \
            'fftw3 >= 3' \
            libcurl \
            taglib_c
    )
    PKG_LIBS=$(
        "$PKG_CONFIG" --libs \
            'libmpdclient >= 2.8' \
            ncursesw \
            'fftw3 >= 3' \
            libcurl \
            taglib_c
    )

    if "$PKG_CONFIG" --exists readline; then
        READLINE_CFLAGS=$("$PKG_CONFIG" --cflags readline)
        READLINE_LIBS=$("$PKG_CONFIG" --libs readline)
    else
        READLINE_CFLAGS=
        READLINE_LIBS='-lreadline -lhistory'
    fi
}

choose_c_standard() {
    mkdir -p "$BUILD_DIR"
    printf 'int main(void) { return 0; }\n' >"$BUILD_DIR/conftest.c"

    for flag in -std=c23 -std=c2x; do
        # shellcheck disable=SC2086
        if $CC $flag -c "$BUILD_DIR/conftest.c" \
                -o "$BUILD_DIR/conftest.o" >/dev/null 2>&1; then
            printf '%s\n' "$flag"
            return
        fi
    done

    fail 'C compiler does not support C23 or C2x'
}

choose_cxx_standard() {
    mkdir -p "$BUILD_DIR"
    printf 'int main() { return 0; }\n' >"$BUILD_DIR/conftest.cpp"

    # shellcheck disable=SC2086
    if $CXX -std=c++20 -c "$BUILD_DIR/conftest.cpp" \
            -o "$BUILD_DIR/conftest.o" >/dev/null 2>&1; then
        printf '%s\n' -std=c++20
        return
    fi

    fail 'C++ compiler does not support C++20'
}

write_config_h() {
    mkdir -p "$BUILD_DIR"
    cat >"$BUILD_DIR/config.h" <<EOF_CONFIG
#ifndef NCMPCPP_CONFIG_H
#define NCMPCPP_CONFIG_H

#define ENABLE_OUTPUTS 1
#define ENABLE_VISUALIZER 1
#define HAVE_CURL_CURL_H 1
#define HAVE_CURSES_H 1
#define HAVE_FFTW3_H 1
#define HAVE_LIBNCURSESW 1
#define HAVE_LIBREADLINE 1
#define HAVE_MPD_CLIENT_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_READLINE_HISTORY 1
#define HAVE_READLINE_HISTORY_H 1
#define HAVE_READLINE_READLINE_H 1
#define HAVE_TAGLIB_FILE_NEW 1
#define HAVE_TAGLIB_FILE_SAVE 1
#define HAVE_TAGLIB_H 1
#define HAVE_TAGLIB_PROPERTY_GET 1
#define HAVE_TAGLIB_PROPERTY_SET 1
#define HAVE_TAGLIB_PROPERTY_SET_APPEND 1
#define PACKAGE "$PACKAGE"
#define PACKAGE_NAME "$PACKAGE"
#define PACKAGE_STRING "$PACKAGE $VERSION"
#define PACKAGE_TARNAME "$PACKAGE"
#define PACKAGE_VERSION "$VERSION"
#define VERSION "$VERSION"

#endif
EOF_CONFIG
}

object_path() {
    path=$1
    printf '%s/obj/%s.o\n' "$BUILD_DIR" "${path%.*}"
}

compile_c() {
    src=$1
    obj=$(object_path "$src")
    mkdir -p "$(dirname -- "$obj")"
    printf 'CC  %s\n' "$src"

    # shellcheck disable=SC2086
    $CC \
        -I"$BUILD_DIR" \
        -I"$PROJECT_DIR" \
        -I"$PROJECT_DIR/src" \
        -D_GNU_SOURCE \
        -D_DEFAULT_SOURCE \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $C_STD \
        -Wall \
        -Wextra \
        -Wfatal-errors \
        $CFLAGS \
        -pthread \
        -MMD \
        -MP \
        -c "$PROJECT_DIR/$src" \
        -o "$obj"
}

compile_cxx() {
    src=$1
    obj=$(object_path "$src")
    mkdir -p "$(dirname -- "$obj")"
    printf 'CXX %s\n' "$src"

    # shellcheck disable=SC2086
    $CXX \
        -I"$BUILD_DIR" \
        -I"$PROJECT_DIR" \
        -I"$PROJECT_DIR/src" \
        -D_GNU_SOURCE \
        -D_DEFAULT_SOURCE \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $CXX_STD \
        -Wall \
        -Wextra \
        -Wfatal-errors \
        $CXXFLAGS \
        -pthread \
        -MMD \
        -MP \
        -c "$PROJECT_DIR/$src" \
        -o "$obj"
}

archive() {
    lib=$1
    shift
    rm -f "$lib"
    # shellcheck disable=SC2068
    $AR rcs "$lib" $@
}

prepare() {
    CPPFLAGS=${CPPFLAGS:-}
    CFLAGS=${CFLAGS:-'-O2 -g'}
    CXXFLAGS=${CXXFLAGS:-'-O2 -g'}
    LDFLAGS=${LDFLAGS:-}
    DESTDIR=${DESTDIR:-}

    pkg_flags
    C_STD=$(choose_c_standard)
    CXX_STD=$(choose_cxx_standard)
    write_config_h
}

build_libs() {
    CBASE_OBJECTS=
    for src in cbase/cbase.c; do
        compile_c "$src"
        CBASE_OBJECTS="$CBASE_OBJECTS $(object_path "$src")"
    done
    archive "$BUILD_DIR/libcbase.a" $CBASE_OBJECTS

    NCMPCPP_C_OBJECTS=
    for src in $(find src/c -type f -name '*.c' | sort); do
        compile_c "$src"
        NCMPCPP_C_OBJECTS="$NCMPCPP_C_OBJECTS $(object_path "$src")"
    done
    archive "$BUILD_DIR/libncmpcpp_c.a" $NCMPCPP_C_OBJECTS
}

build_program() {
    APP_OBJECTS=
    for src in $(find src -type f -name '*.c' ! -path 'src/c/*' | sort); do
        compile_c "$src"
        APP_OBJECTS="$APP_OBJECTS $(object_path "$src")"
    done
    for src in $(find src -type f -name '*.cpp' | sort); do
        compile_cxx "$src"
        APP_OBJECTS="$APP_OBJECTS $(object_path "$src")"
    done

    printf 'LD  %s\n' "$BUILD_DIR/ncmpcpp"
    # shellcheck disable=SC2086
    $CXX $LDFLAGS -o "$BUILD_DIR/ncmpcpp" \
        $APP_OBJECTS \
        "$BUILD_DIR/libncmpcpp_c.a" \
        "$BUILD_DIR/libcbase.a" \
        $READLINE_LIBS \
        $PKG_LIBS \
        -pthread
}

build_tests() {
    for src in tests/*_test.c; do
        test -f "$src" || continue
        compile_c "$src"
        test_name=$(basename -- "$src" .c)
        printf 'LD  %s\n' "$BUILD_DIR/tests/$test_name"
        mkdir -p "$BUILD_DIR/tests"
        # shellcheck disable=SC2086
        $CC $LDFLAGS -o "$BUILD_DIR/tests/$test_name" \
            "$(object_path "$src")" \
            "$BUILD_DIR/libncmpcpp_c.a" \
            "$BUILD_DIR/libcbase.a" \
            $PKG_LIBS \
            -pthread
    done
}

run_tests() {
    for test in "$BUILD_DIR"/tests/*_test; do
        test -x "$test" || continue
        printf 'TEST %s\n' "$test"
        "$test"
    done
}

build() {
    prepare
    build_libs
    build_program
    printf 'built %s\n' "$BUILD_DIR/ncmpcpp"
}

check() {
    prepare
    build_libs
    build_tests
    run_tests
}

install_files() {
    build

    install -d "$DESTDIR$BINDIR"
    install -m 755 "$BUILD_DIR/ncmpcpp" "$DESTDIR$BINDIR/ncmpcpp"

    install -d "$DESTDIR$DOCDIR"
    install -m 644 AUTHORS CHANGELOG.md COPYING "$DESTDIR$DOCDIR"
    install -m 644 doc/bindings doc/config "$DESTDIR$DOCDIR"

    install -d "$DESTDIR$MANDIR/man1"
    install -m 644 doc/ncmpcpp.1 "$DESTDIR$MANDIR/man1/ncmpcpp.1"
}

clean() {
    rm -rf "$BUILD_DIR"
}

prune_autotools() {
    while IFS= read -r path; do
        case $path in
            ''|'#'*) continue ;;
        esac
        rm -rf -- $path
    done < REMOVE_AUTOTOOLS_FILES.txt
}

case ${1:-build} in
    build)
        build
        ;;
    check)
        check
        ;;
    install)
        install_files
        ;;
    clean)
        clean
        ;;
    prune-autotools)
        prune_autotools
        ;;
    -h|--help|help)
        usage
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac
