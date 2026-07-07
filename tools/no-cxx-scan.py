#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path

CXX_EXTENSIONS = {
    ".cc",
    ".cpp",
    ".cxx",
    ".hh",
    ".hpp",
    ".hxx",
}

SOURCE_EXTENSIONS = CXX_EXTENSIONS | {
    ".c",
    ".h",
}

CXX_STDLIB_HEADERS = {
    "algorithm",
    "array",
    "atomic",
    "bit",
    "bitset",
    "cassert",
    "cctype",
    "cerrno",
    "cfenv",
    "cfloat",
    "charconv",
    "chrono",
    "cinttypes",
    "climits",
    "clocale",
    "cmath",
    "codecvt",
    "compare",
    "complex",
    "concepts",
    "condition_variable",
    "csetjmp",
    "csignal",
    "cstdarg",
    "cstddef",
    "cstdint",
    "cstdio",
    "cstdlib",
    "cstring",
    "ctime",
    "cwchar",
    "cwctype",
    "deque",
    "exception",
    "execution",
    "filesystem",
    "format",
    "forward_list",
    "fstream",
    "functional",
    "future",
    "initializer_list",
    "iomanip",
    "ios",
    "iosfwd",
    "iostream",
    "istream",
    "iterator",
    "limits",
    "list",
    "locale",
    "map",
    "memory",
    "mutex",
    "new",
    "numbers",
    "numeric",
    "optional",
    "ostream",
    "queue",
    "random",
    "ranges",
    "ratio",
    "regex",
    "scoped_allocator",
    "set",
    "shared_mutex",
    "span",
    "sstream",
    "stack",
    "stdexcept",
    "streambuf",
    "string",
    "string_view",
    "strstream",
    "syncstream",
    "system_error",
    "thread",
    "tuple",
    "type_traits",
    "typeindex",
    "typeinfo",
    "unordered_map",
    "unordered_set",
    "utility",
    "valarray",
    "variant",
    "vector",
}

CXX_TOKENS = {
    "catch",
    "class",
    "delete",
    "namespace",
    "new",
    "override",
    "template",
    "throw",
    "try",
    "virtual",
}

INCLUDE_RE = re.compile(r"^\s*#\s*include\s*<([^>]+)>")
TOKEN_RE = re.compile(
    r"\b("
    r"catch|class|delete|namespace|new|override|template|throw|try|virtual"
    r")\b|\bstd\s*::"
)


def iter_source_files(paths):
    for raw_path in paths:
        path = Path(raw_path)
        if not path.exists():
            continue
        if path.is_file():
            if path.suffix in SOURCE_EXTENSIONS:
                yield path
            continue
        for child in sorted(path.rglob("*")):
            if child.is_file() and child.suffix in SOURCE_EXTENSIONS:
                yield child


def erase_span(chars, start, end):
    for i in range(start, end):
        if chars[i] != "\n":
            chars[i] = " "


def erase_raw_string(chars, start):
    n = len(chars)
    i = start + 2
    while i < n and chars[i] != "(":
        if chars[i] in " \t\r\n\\":
            return start + 1
        i += 1
    if i >= n:
        return start + 1

    delimiter = "".join(chars[start + 2:i])
    closing = ")" + delimiter + '"'
    j = i + 1
    haystack = "".join(chars)
    end = haystack.find(closing, j)
    if end < 0:
        erase_span(chars, start, n)
        return n

    end += len(closing)
    erase_span(chars, start, end)
    return end


def erase_comments_and_literals(text):
    chars = list(text)
    n = len(chars)
    i = 0

    while i < n:
        ch = chars[i]
        nxt = chars[i + 1] if i + 1 < n else ""

        if ch == "/" and nxt == "/":
            start = i
            i += 2
            while i < n and chars[i] != "\n":
                i += 1
            erase_span(chars, start, i)
            continue

        if ch == "/" and nxt == "*":
            start = i
            i += 2
            while i + 1 < n and not (chars[i] == "*" and chars[i + 1] == "/"):
                i += 1
            if i + 1 < n:
                i += 2
            erase_span(chars, start, i)
            continue

        if ch == "R" and nxt == '"':
            i = erase_raw_string(chars, i)
            continue

        if ch == '"':
            start = i
            i += 1
            while i < n:
                if chars[i] == "\\":
                    i += 2
                    continue
                if chars[i] == '"':
                    i += 1
                    break
                i += 1
            erase_span(chars, start, i)
            continue

        if ch == "'":
            start = i
            i += 1
            while i < n:
                if chars[i] == "\\":
                    i += 2
                    continue
                if chars[i] == "'":
                    i += 1
                    break
                i += 1
            erase_span(chars, start, i)
            continue

        i += 1

    return "".join(chars)


def line_col(line_starts, index):
    lo = 0
    hi = len(line_starts)

    while lo + 1 < hi:
        mid = (lo + hi)//2
        if line_starts[mid] <= index:
            lo = mid
        else:
            hi = mid

    return lo + 1, index - line_starts[lo] + 1


def compute_line_starts(text):
    starts = [0]
    for match in re.finditer("\n", text):
        starts.append(match.end())
    return starts


def find_includes(path, cleaned):
    for line_no, line in enumerate(cleaned.splitlines(), start=1):
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        header = match.group(1)
        if header in CXX_STDLIB_HEADERS:
            col = match.start(1) + 1
            yield f"{path}:{line_no}:{col}: #include <{header}>"


def find_tokens(path, cleaned):
    line_starts = compute_line_starts(cleaned)
    for match in TOKEN_RE.finditer(cleaned):
        token = match.group(1) if match.group(1) else "std::"
        line_no, col = line_col(line_starts, match.start())
        yield f"{path}:{line_no}:{col}: {token}"


def read_file(path):
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return path.read_text(encoding="utf-8", errors="replace")


def print_group(title, lines):
    if not lines:
        return
    print(title, file=sys.stderr)
    for line in lines:
        print(line, file=sys.stderr)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", help="files or directories to scan")
    args = parser.parse_args()

    files = sorted(set(iter_source_files(args.paths)))
    cxx_files = [str(path) for path in files if path.suffix in CXX_EXTENSIONS]
    std_includes = []
    tokens = []

    for path in files:
        text = read_file(path)
        cleaned = erase_comments_and_literals(text)
        std_includes.extend(find_includes(path, cleaned))
        tokens.extend(find_tokens(path, cleaned))

    print_group("C++ source files remain:", cxx_files)
    print_group("C++ standard library includes remain:", std_includes)
    print_group("C++ constructs remain:", tokens)

    if cxx_files or std_includes or tokens:
        return 1

    print("No C++ sources or banned C++ constructs remain.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
