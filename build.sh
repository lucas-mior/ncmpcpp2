#!/bin/sh

export LC_ALL=C

: "${CC:=clang}"
: "${CXX:=clang++}"
: "${CFLAGS:=-O0 -g3}"
: "${CXXFLAGS:=-O0 -g3}"
export CC CXX CFLAGS CXXFLAGS

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
  CFLAGS="$CFLAGS -Werror" \
  CXXFLAGS="$CXXFLAGS -Werror"

make -j"$(nproc)"

sudo make install
