#!/bin/sh

# make clean

# autoreconf -fiv  # generate the `configure` script.

./configure --enable-visualizer

make -j$(nproc)
