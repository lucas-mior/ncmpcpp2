#!/bin/sh -e

# shellcheck disable=SC2086,SC2031

dir=$(dirname "$(readlink -f "$0")")
# shellcheck source=/dev/null
. "$dir/cbase/common.sh"

cd "$dir" || exit
program=$(get_program "$0")
script=$(basename "$0")
target="${1:-debug}"

printf "\n${script} ${RED}${1:-} ${2:-}$RES\n"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

main="src/main.c"
exe="bin/$program"
mkdir -p "$(dirname "$exe")"

CC=$(get_compiler "$target")

CPPFLAGS="$CPPFLAGS -D_DEFAULT_SOURCE"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
# CFLAGS="$CFLAGS -Werror=all -Werror=extra"
# CFLAGS="$CFLAGS -Werror"  # Only uncomment occasionally, keep this line
CFLAGS="$CFLAGS -Wno-cast-qual"
CFLAGS="$CFLAGS -Wno-constant-logical-operand"
CFLAGS="$CFLAGS -Wno-format-pedantic"
CFLAGS="$CFLAGS -Wno-unused-macros"

if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-cast-align"
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
    CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
fi

CFLAGS="$CFLAGS -pthread"

LDFLAGS="$LDFLAGS -lm"

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
    error '%s\n' "$1"
    exit 1
}

require_command() {
    command_string=$1

    # Command variables may contain wrappers.
    # shellcheck disable=SC2086
    set -- $command_string
    if [ "$#" -eq 0 ]; then
        required_command=
    else
        required_command=$1
    fi

    if ! command -v "$required_command" >/dev/null 2>&1; then
        die "missing command: $required_command"
    fi

    return 0
}

run_command() {
    command_string=$1
    shift

    trace_on
    # Commands such as compiler wrappers intentionally require word splitting.
    # shellcheck disable=SC2086
    $command_string "$@"
    trace_off

    return 0
}

pkg_config() {
    run_command pkg-config "$@"
    return 0
}

detect_c_standard() {
    compiler=$1

    if printf 'int main(void) { return 0; }\n' \
        | run_command "$compiler" -std=c11 -c \
            -o /dev/null - >/dev/null 2>&1; then
        return 0
    fi

    return 1
}

load_package_flags() {
    require_command pkg-config

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
        | awk '/[.](cc|c[p]p|cxx)$/ { print }')

    if [ -n "$bad_files" ]; then
        printf '%s\n' 'Non-C source files are not allowed:' >&2
        printf '%s\n' "$bad_files" >&2
        exit 1
    fi

    return 0
}

show_help() {
    cat <<EOF_HELP
usage: ./$script <target>

targets:
  build                    build with CFLAGS=-O2 -flto
  debug                    build with CFLAGS=-g3 -O0 (default)
  fast_feedback            build with clang warning checks
  all                      build with the current CFLAGS
  check                    run the clang static analyzer
  test                     build and run all tests
  check-no-foreign-sources reject C++ source files under src
  install                  build and install the program and documentation
  uninstall                remove installed program and documentation
  clean                    remove bin/
  help                     show this help

common variables:
  PREFIX             install prefix, default: /usr/local
  DESTDIR            destination root, default: /
  CPPFLAGS           extra preprocessor flags
  CFLAGS             extra compiler flags
  LDFLAGS            extra linker flags and libraries
EOF_HELP

    return 0
}

case "$target" in
debug|test)
    CFLAGS="$CFLAGS -g3 -O0"
    ;;
build)
    CFLAGS="$CFLAGS -O2 -flto"
    ;;
fast_feedback)
    CFLAGS="$CFLAGS -g3 -O0"
    ;;
check)
    CFLAGS="$CFLAGS -g3 -O0"
    ;;
all)
    ;;
esac

case "$target" in
debug|build|fast_feedback|all)
    check_no_foreign_sources
    load_package_flags
    require_command "$CC"

    if ! detect_c_standard "$CC"; then
        die 'C compiler does not support C11'
    fi

    temporary_binary=$exe.tmp.$$
    TEMP_FILE=$temporary_binary

    # Flag variables intentionally require shell word splitting.
    # shellcheck disable=SC2086
    run_command "$CC" \
        -I. \
        -Isrc \
        -Icbase \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $CFLAGS \
        "$main" \
        -o "$temporary_binary" \
        $READLINE_LIBS \
        $PKG_LIBS \
        $LDFLAGS

    mv "$temporary_binary" "$exe"
    TEMP_FILE=
    ;;
check)
    check_no_foreign_sources
    load_package_flags
    require_command "$CC"

    if ! detect_c_standard "$CC"; then
        die 'clang analyzer does not support C11'
    fi

    # Flag variables intentionally require shell word splitting.
    # shellcheck disable=SC2086
    run_command "$CC" \
        -I. \
        -Isrc \
        -Icbase \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $CFLAGS \
        --analyze \
        -Xanalyzer -analyzer-output=text \
        -fno-color-diagnostics \
        "$main"
    ;;
test)
    require_command "$CC"

    if ! detect_c_standard "$CC"; then
        die 'C compiler does not support C11'
    fi

    for source in tests/*.c; do
        if [ ! -f "$source" ]; then
            continue
        fi

        test_name=${source#tests/}
        test_name=${test_name%.c}
        test_dir=bin/tests
        binary=$test_dir/$test_name
        temporary_binary=$binary.tmp.$$

        mkdir -p "$test_dir"
        TEMP_FILE=$temporary_binary

        # Flag variables intentionally require shell word splitting.
        # shellcheck disable=SC2086
        run_command "$CC" \
            -Itests \
            -I. \
            -Isrc \
            -Icbase \
            $CPPFLAGS \
            -D_XOPEN_SOURCE=700 \
            $CFLAGS \
            "$source" \
            -o "$temporary_binary" \
            $LDFLAGS

        mv "$temporary_binary" "$binary"
        TEMP_FILE=

        "$binary"
    done
    ;;
check-no-foreign-sources)
    check_no_foreign_sources
    ;;
install)
    if [ ! -f "$exe" ]; then
        "$0" build
    elif find src cbase -type f \
        \( -name '*.c' -o -name '*.h' \) \
        -newer "$exe" -print \
        | grep -q .; then
        "$0" build
    fi

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
    rm -rf bin/
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
