#!/bin/sh

set -e

# 1. Get which files have changed.
changed_files=$(git diff --name-only)

only_whitespace=true

if [ -n "$changed_files" ]; then
    # Read line by line to handle any filenames with spaces correctly
    while IFS= read -r file; do
        [ -z "$file" ] && continue
        
        # 2. Use tr -d to remove all whitespace on the original and modified file, then run diff.
        git show :"$file" 2>/dev/null | tr -d '[:space:]' > .tmp_orig
        cat "$file" 2>/dev/null | tr -d '[:space:]' > .tmp_mod
        
        if ! diff -q .tmp_orig .tmp_mod >/dev/null 2>&1; then
            only_whitespace=false
            break
        fi
    done <<EOF
$changed_files
EOF
    rm -f .tmp_orig .tmp_mod
fi

# Check if the non-whitespace content is identical
if [ "$only_whitespace" = true ]; then
    if [ -n "$changed_files" ]; then
        git commit -n -a -m "style (whitespace)"
    else
        printf "No changes detected.\n"
    fi
else
    printf "Non-whitespace changes were made. Check diff below\n\n"
    # This will use your 'diffr' configuration from gitconfig
    git diff
fi
