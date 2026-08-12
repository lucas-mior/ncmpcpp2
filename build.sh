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

CPPFLAGS="$CPPFLAGS -I$dir -I$dir/src -I$dir/cbase"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
# CFLAGS="$CFLAGS -Werror=all -Werror=extra"
# CFLAGS="$CFLAGS -Werror"  # Only uncomment occasionally, keep this line

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

die() {
    common_error '%s\n' "$1"
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

pkg_config() {
    pkg-config "$@"
    return 0
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
    require_command "$CC"

    objdir="bin/obj/$mode"
    main_obj="$objdir/src/main.o"
    debug_flags_file="$objdir/flags"
    subdir_sources=
    subdir_objects=

    for source_dir in src/*; do
        if [ ! -d "$source_dir" ]; then
            continue
        fi

        source_subdir=${source_dir#src/}
        wrapper_src=

        for source_file in "$source_dir"/*.c; do
            if [ ! -f "$source_file" ]; then
                continue
            fi

            if awk -v source_subdir="$source_subdir" '
                /^[[:space:]]*#[[:space:]]*include[[:space:]]*"/ {
                    include = $0
                    sub(/^[^"]*"/, "", include)
                    sub(/".*$/, "", include)

                    pattern = "^" source_subdir "/.*\\.c$"
                    if (include ~ pattern) {
                        found = 1
                    }
                }
                END { exit !found }
            ' "$source_file"; then
                if [ -n "$wrapper_src" ]; then
                    die "multiple incremental source files in $source_dir"
                fi

                wrapper_src=$source_file
            fi
        done

        if [ -n "$wrapper_src" ]; then
            subdir_sources="$subdir_sources $wrapper_src"
            subdir_objects="$subdir_objects $objdir/${wrapper_src%.c}.o"
        fi
    done

    if [ -z "$subdir_sources" ]; then
        die "no incremental source files found under src subfolders"
    fi

    mkdir -p "$(dirname "$main_obj")"

    for subdir_object in $subdir_objects; do
        mkdir -p "$(dirname "$subdir_object")"
    done

    debug_flags="CC=$CC
CPPFLAGS=$CPPFLAGS
PKG_CFLAGS=$PKG_CFLAGS
READLINE_CFLAGS=$READLINE_CFLAGS
CFLAGS=$CFLAGS
SUBDIR_SOURCES=$subdir_sources"
    if [ ! -f "$debug_flags_file" ] \
            || ! printf '%s\n' "$debug_flags" \
                | cmp -s - "$debug_flags_file"; then
        rm -f "$main_obj"
        for subdir_object in $subdir_objects; do
            rm -f "$subdir_object"
        done

        printf '%s\n' "$debug_flags" > "$debug_flags_file"
    fi

    compile_pids=

    for subdir_source in $subdir_sources; do
        subdir_object="$objdir/${subdir_source%.c}.o"
        source_dir=$(dirname "$subdir_source")

        if [ ! -f "$subdir_object" ] \
                || find "$source_dir" -maxdepth 1 -type f \
                    \( -name '*.c' -o -name '*.h' \) \
                    -newer "$subdir_object" -print | grep -q . \
                || find src -mindepth 2 -maxdepth 2 -type f \
                    -name '*.h' \
                    -newer "$subdir_object" -print | grep -q . \
                || find src -maxdepth 1 -type f -name '*.h' \
                    -newer "$subdir_object" -print | grep -q . \
                || find cbase -type f -newer "$subdir_object" \
                    -print | grep -q .; then
            (
                trace_on
                $CC \
                    $CPPFLAGS \
                    $PKG_CFLAGS \
                    $READLINE_CFLAGS \
                    $CFLAGS \
                    -c "$subdir_source" \
                    -o "$subdir_object"
                trace_off
            ) &
            compile_pids="$compile_pids $!"
        fi
    done

    if [ ! -f "$main_obj" ] \
            || find src -maxdepth 1 -type f \
                \( -name '*.c' -o -name '*.h' \) \
                -newer "$main_obj" -print | grep -q . \
            || find src -mindepth 2 -maxdepth 2 -type f -name '*.h' \
                -newer "$main_obj" -print | grep -q . \
            || find cbase -type f -newer "$main_obj" -print | grep -q .; then
        (
            trace_on
            $CC \
                $CPPFLAGS \
                $PKG_CFLAGS \
                $READLINE_CFLAGS \
                $CFLAGS \
                -DNCMPCPP_INCREMENTAL_BUILD=1 \
                -c src/main.c \
                -o "$main_obj"
            trace_off
        ) &
        compile_pids="$compile_pids $!"
    fi

    compile_failed=0
    for compile_pid in $compile_pids; do
        if ! wait "$compile_pid"; then
            compile_failed=1
        fi
    done
    if [ "$compile_failed" != 0 ]; then
        exit 1
    fi

    trace_on
    $CC \
        -o "$exe" \
        "$main_obj" \
        $subdir_objects \
        $READLINE_LIBS \
        $PKG_LIBS \
        $LDFLAGS
    trace_off
    ;;
build|fast_feedback)
    load_package_flags
    require_command "$CC"

    trace_on
    $CC \
        $CPPFLAGS \
        $PKG_CFLAGS \
        $READLINE_CFLAGS \
        $CFLAGS \
        -o "$exe" \
        src/main.c \
        $READLINE_LIBS \
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
    require_command "$CC"
    CPPFLAGS="$CPPFLAGS -I$dir/tests"
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
