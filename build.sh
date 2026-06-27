#!/bin/sh

# make clean

# shellcheck disable=SC2046

# autoreconf -fiv  # generate the `configure` script.

./configure \
    --enable-outputs \
    --enable-visualizer \
    --with-fftw \
    --with-taglib

make -j$(nproc)
