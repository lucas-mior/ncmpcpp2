#!/bin/sh -e

# Lists bool-returning declarations/definitions outside cbase/ and classifies
# them according to the error-returning rules in cbase/c-guidelines.md.
# This script is intentionally not wired into build.sh yet.

usage() {
    cat <<'EOF_USAGE'
usage: tools/audit_bool_returns.sh [--all|--suspicious-only|--summary] [files...]

Categories:
  allowed-predicate  bool-returning names that look like predicates
  allowed-callback   bool-returning filter/match/predicate callback APIs
  suspicious         bool returns that should be reviewed for status migration

When no files are given, the script scans src/ and tests/ and ignores cbase/.
EOF_USAGE
    return
}

print_files() {
    if [ "$#" -gt 0 ]; then
        printf '%s\n' "$@"
        return
    fi

    find src tests \
        \( -name '*.c' -o -name '*.h' \) \
        -type f \
        | sort
    return
}

mode=all
while [ "$#" -gt 0 ]; do
    case "$1" in
    --all)
        mode=all
        shift
        ;;
    --suspicious-only)
        mode=suspicious
        shift
        ;;
    --summary)
        mode=summary
        shift
        ;;
    --help|-h)
        usage
        exit 0
        ;;
    --)
        shift
        break
        ;;
    -*)
        printf 'unknown option: %s\n' "$1" >&2
        usage >&2
        exit 1
        ;;
    *)
        break
        ;;
    esac
done

records=$(mktemp "${TMPDIR:-/tmp}/bool_returns.XXXXXX")
trap 'rm -f "$records"' EXIT HUP INT TERM

print_files "$@" | while read -r file; do
    case "$file" in
    cbase/*|./cbase/*)
        continue
        ;;
    esac

    if [ ! -f "$file" ]; then
        printf 'missing file: %s\n' "$file" >&2
        exit 1
    fi

    awk '
        function trim(s) {
            sub(/^[[:space:]]+/, "", s);
            sub(/[[:space:]]+$/, "", s);
            return s;
        }

        function remove_comments(line,    output, start, stop) {
            output = "";
            while (line != "") {
                if (in_comment) {
                    stop = index(line, "*/");
                    if (stop == 0) {
                        return output;
                    }
                    line = substr(line, stop + 2);
                    in_comment = 0;
                }

                start = index(line, "/*");
                if (start == 0) {
                    output = output line;
                    line = "";
                } else {
                    output = output substr(line, 1, start - 1);
                    line = substr(line, start + 2);
                    in_comment = 1;
                }
            }
            return output;
        }

        function bool_name_category(name, kind) {
            if (name ~ /(^|_)is($|_)/) {
                return "allowed-predicate";
            }
            if (name ~ /(^|_)has($|_)/) {
                return "allowed-predicate";
            }
            if (name ~ /(^|_)can($|_)/) {
                return "allowed-predicate";
            }
            if (name ~ /(^|_)contains($|_)/) {
                return "allowed-predicate";
            }
            if (name ~ /(^|_)matches?($|_)/) {
                return "allowed-predicate";
            }
            if (name ~ /_position_is_/) {
                return "allowed-predicate";
            }

            if ((kind == "typedef") && (name ~ /(Is|Has|Can|Contains|Matches)/)) {
                return "allowed-callback";
            }
            if ((kind == "function-pointer") && (name ~ /(^|_)filter($|_)/)) {
                return "allowed-callback";
            }
            if ((kind == "function-pointer") && (name ~ /(^|_)matcher($|_)/)) {
                return "allowed-callback";
            }
            if ((kind == "function-pointer") && (name ~ /(^|_)predicate($|_)/)) {
                return "allowed-callback";
            }
            if ((kind == "function-pointer") && (name ~ /(^|_)matches?($|_)/)) {
                return "allowed-callback";
            }
            if (name ~ /(^|_)filter(_callback)?$/) {
                return "allowed-callback";
            }
            if (name ~ /(^|_)matcher(_callback)?$/) {
                return "allowed-callback";
            }
            if (name ~ /(^|_)predicate(_callback)?$/) {
                return "allowed-callback";
            }
            if (name ~ /(^|_)matches?(_callback)?$/) {
                return "allowed-callback";
            }

            return "suspicious";
        }

        function emit(kind, line_no, name, category) {
            if (name == "") {
                return;
            }
            category = bool_name_category(name, kind);
            printf "%s\t%s\t%d\t%s\t%s\n", category, FILENAME, line_no, kind, name;
            return;
        }

        function maybe_emit_single_line(line, line_no,    name) {
            if (match(line, /^[[:space:]]*(static[[:space:]]+)?bool[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(/)) {
                name = line;
                sub(/^[[:space:]]*(static[[:space:]]+)?bool[[:space:]]+/, "", name);
                sub(/[[:space:]]*\(.*/, "", name);
                emit("function", line_no, name);
                return 1;
            }
            return 0;
        }

        function maybe_emit_typedef(line, line_no,    name) {
            if (match(line, /^[[:space:]]*typedef[[:space:]]+bool[[:space:]]*\(\*[A-Za-z_][A-Za-z0-9_]*\)[[:space:]]*\(/)) {
                name = line;
                sub(/^[[:space:]]*typedef[[:space:]]+bool[[:space:]]*\(\*/, "", name);
                sub(/\)[[:space:]]*\(.*/, "", name);
                emit("typedef", line_no, name);
                return 1;
            }
            if (match(line, /^[[:space:]]*typedef[[:space:]]+bool[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(/)) {
                name = line;
                sub(/^[[:space:]]*typedef[[:space:]]+bool[[:space:]]+/, "", name);
                sub(/[[:space:]]*\(.*/, "", name);
                emit("typedef", line_no, name);
                return 1;
            }
            return 0;
        }

        function maybe_emit_function_pointer(line, line_no,    name) {
            if ((line ~ /;$/) && match(line, /^[[:space:]]*bool[[:space:]]*\(\*[A-Za-z_][A-Za-z0-9_]*\)[[:space:]]*\(/)) {
                name = line;
                sub(/^[[:space:]]*bool[[:space:]]*\(\*/, "", name);
                sub(/\)[[:space:]]*\(.*/, "", name);
                emit("function-pointer", line_no, name);
                return 1;
            }
            return 0;
        }

        {
            line = remove_comments($0);
            sub(/\/\/.*$/, "", line);
            line = trim(line);

            if (pending_bool) {
                if (line == "") {
                    next;
                }
                if (match(line, /^[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(/)) {
                    name = line;
                    sub(/[[:space:]]*\(.*/, "", name);
                    emit("function", pending_line, name);
                }
                pending_bool = 0;
            }

            if (line == "") {
                next;
            }

            if (maybe_emit_typedef(line, FNR)) {
                next;
            }
            if (maybe_emit_function_pointer(line, FNR)) {
                next;
            }
            if (maybe_emit_single_line(line, FNR)) {
                next;
            }
            if (line ~ /^(static[[:space:]]+)?bool$/) {
                pending_bool = 1;
                pending_line = FNR;
                next;
            }
        }
    ' "$file"
done > "$records"

case "$mode" in
summary)
    awk -F '\t' '
        {
            counts[$1] += 1;
            total += 1;
        }
        END {
            printf "total\t%d\n", total;
            printf "allowed-predicate\t%d\n", counts["allowed-predicate"];
            printf "allowed-callback\t%d\n", counts["allowed-callback"];
            printf "suspicious\t%d\n", counts["suspicious"];
        }
    ' "$records"
    ;;
suspicious)
    awk -F '\t' '$1 == "suspicious" { print }' "$records"
    ;;
all)
    awk -F '\t' '
        function print_category(category, title,    i) {
            printf "\n%s\n", title;
            for (i = 1; i <= n; i += 1) {
                if (categories[i] == category) {
                    printf "%s:%s: %s %s\n",
                           files[i], lines[i], kinds[i], names[i];
                }
            }
            return;
        }

        {
            n += 1;
            categories[n] = $1;
            files[n] = $2;
            lines[n] = $3;
            kinds[n] = $4;
            names[n] = $5;
            counts[$1] += 1;
        }
        END {
            printf "total: %d\n", n;
            printf "allowed-predicate: %d\n", counts["allowed-predicate"];
            printf "allowed-callback: %d\n", counts["allowed-callback"];
            printf "suspicious: %d\n", counts["suspicious"];
            print_category("suspicious", "suspicious");
            print_category("allowed-predicate", "allowed-predicate");
            print_category("allowed-callback", "allowed-callback");
        }
    ' "$records"
    ;;
*)
    printf 'invalid mode: %s\n' "$mode" >&2
    exit 1
    ;;
esac
