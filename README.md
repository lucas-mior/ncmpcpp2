# ncmpcpp2
This is a fork of ncmpcpp. The goals are:
- Remove features I don't use:
  + ~~Remove clock~~
  + Remove others?
- Remove dependency on external C++ utility libraries.
- Remove the remaining C++ implementation and build as C-only.
- ~~Remove boost~~
- Convert the project to C23.
- Fix the bugs reported in original repo.

Original README is below.

# NCurses Music Player Client (Plus Plus)

## ncmpcpp – featureful ncurses based MPD client inspired by ncmpc

### Project status

The project is officially in maintenance mode. I (Andrzej Rybczak) still use it
daily, but it's feature complete for me and there is very limited time I have
for tending to the issue tracker and open pull requests.

No new, substantial features should be expected (at least from me). However, if
there are any serious bugs or the project outright stops compiling because of
new, incompatible versions of dependencies, it will be fixed.

### Main features:
* tag editor
* playlist editor
* easy to use search engine
* media library
* music visualizer
* ability to fetch artist info from last.fm
* new display mode
* alternative user interface
* ability to browse and add files from outside of MPD music directory
…and a lot more minor functions.

### Dependencies:
* [ncurses](https://invisible-island.net/ncurses/announce.html)
* [readline](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [curl](https://curl.se), for fetching lyrics and last.fm data
* [fftw](http://www.fftw.org), for frequency spectrum music visualization mode
* [taglib](https://taglib.org/), for tag editing

### Known issues:
* ncmpcpp assumes UTF-8 input, output, and terminal locale.

### Installation:
The simplest way to compile this package is:

  1. `cd` to the directory containing the package's source code.

  2. Install the build dependencies and their pkg-config files:
     * C23-capable C compiler
     * pkg-config
     * ncursesw
     * readline
     * curl
     * fftw3
     * libmpdclient >= 2.8
     * taglib C API

  3. Run `make` to compile the package. The binary is written to
     `build/ncmpcpp`.

  4. Run `make check` to compile and run the test programs.

  5. Run `sudo make install` to install the program, docs, and man page.
     Use `PREFIX=/some/path make install` for a non-default prefix.

  6. Run `make clean` to remove build outputs.

The autotools files are no longer used. If this tree still has generated
artifacts from the old build system or the old shell build script, run
`make prune-autotools` once to remove the paths listed in
`REMOVE_AUTOTOOLS_FILES.txt`.
