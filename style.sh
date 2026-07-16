#!/bin/sh

# shellcheck disable=SC2035 

clang-format -i "$@"
    --style=file:/home/lucas/.config/clangd/clang-format.yaml

for f in src/*.c src/*.h; do 
    sed -E -i -f style.sed "$f"
done

for f in src/*.c src/*.h; do
    sed -E -i 's/(\S+) \* (\S+)/\1*\2/g;' "$f"
done
