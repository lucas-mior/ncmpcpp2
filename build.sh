#!/bin/sh

export LC_ALL=C

CC=clang
CXX=clang++

CFLAGS="-O0 -g3"
CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -Wfatal-errors"

if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wformat-non-literal"
    CFLAGS="$CFLAGS -Wunsafe-buffer-usage"

    CFLAGS="$CFLAGS -Wno-pre-c23-compat"
    CFLAGS="$CFLAGS -Wno-documentation"
fi

CFLAGS="$CXXFLAGS"

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
