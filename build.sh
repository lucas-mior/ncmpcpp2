#!/bin/sh

export LC_ALL=C

CC=clang
CXX=clang++

CFLAGS="-O0 -g3"
CFLAGS="$CFLAGS -Wfatal-errors -Wno-pre-c23-compat -Wno-documentation"

CXXFLAGS="-O0 -g3"
CXXFLAGS="$CXXFLAGS -Wfatal-errors -Wno-pre-c23-compat -Wno-documentation"

export CC CXX CFLAGS CXXFLAGS

make distclean
make clean

# shellcheck disable=SC2046

autoreconf -fiv  # generate the `configure` script.

# ./configure \
#     --enable-outputs \
#     --enable-visualizer \
#     --with-fftw \
#     --with-taglib

./configure \
  --disable-dependency-tracking \
  --with-lto=no \
  --enable-outputs \
  --enable-visualizer \
  --with-fftw \
  --with-taglib \
  CC="$CC" \
  CXX="$CXX" \
  CFLAGS="$CFLAGS" \
  CXXFLAGS="$CXXFLAGS" 

make -j"$(nproc)"

sudo make install
