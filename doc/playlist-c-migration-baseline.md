# Playlist C migration baseline

This baseline was recorded before changing playlist implementation or call
sites. The source archive does not contain usable Git metadata, so the
extracted archive is the comparison baseline.

## Repository snapshot

- Total files: 323
- C source files under `src/`, `cbase/`, and `tests/`: 140
- Header files under `src/`, `cbase/`, and `tests/`: 157
- C++ translation units under `src/` and `tests/`: 4

The C++ translation units are:

- `src/actions_legacy.cpp`
- `src/mpdpp.cpp`
- `src/settings_legacy.cpp`
- `tests/settings_legacy_sync_test.cpp`

## Clean build result

The requested baseline commands were run from the repository root.

### `make clean`

Exit status: 0

No output was produced.

### `make`

Exit status: 2

```text
missing pkg-config package(s): 'libmpdclient >= 2.8' ncursesw 'fftw3 >= 3' libcurl taglib_c
make: *** [Makefile:242: build/.tools-ok] Error 1
```

### `make check`

Exit status: 2

```text
missing pkg-config package(s): 'libmpdclient >= 2.8' ncursesw 'fftw3 >= 3' libcurl taglib_c
make: *** [Makefile:242: build/.tools-ok] Error 1
```

## Failure classification

These failures are environment failures, not source or test failures. The
Makefile dependency gate stopped both commands before compilation or test
execution.

The missing pkg-config packages are:

- `libmpdclient >= 2.8`
- `ncursesw`
- `fftw3 >= 3`
- `libcurl`
- `taglib_c`

No pre-existing compiler failures or test failures can be recorded from this
environment. After the dependencies are installed, rerun this exact sequence
before playlist changes:

```sh
make clean
make
make check
```

Any failure after that dependency-only baseline succeeds must be classified
before beginning the playlist cutover.
