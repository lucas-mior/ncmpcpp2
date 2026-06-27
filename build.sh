#!/bin/sh

# make clean

# shellcheck disable=SC2046

# autoreconf -fiv  # generate the `configure` script.

# ./configure \
#     --enable-outputs \
#     --enable-visualizer \
#     --with-fftw \
#     --with-taglib

# ./configure \
#   --disable-dependency-tracking \
#   --with-lto=no \
#   --enable-outputs \
#   --enable-visualizer \
#   --with-fftw \
#   --with-taglib \
#   CXXFLAGS="-O0 -g3"

make -j"$(nproc)"

sudo make install
