AC_INIT([ncmpcpp],[0.10.2_dev])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])

AC_PREREQ([2.71])

AC_DEFUN([NCMPCPP_APPEND_SUPPORTED_CFLAG], [
  AC_LANG_PUSH([C])
  AC_MSG_CHECKING([whether C compiler supports $1])
  ncmpcpp_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT([yes])
     ncmpcpp_supported_CFLAGS="$ncmpcpp_supported_CFLAGS $1"],
    [AC_MSG_RESULT([no])])
  CFLAGS="$ncmpcpp_save_CFLAGS"
  AC_LANG_POP([C])
])

AC_DEFUN([NCMPCPP_APPEND_SUPPORTED_CXXFLAG], [
  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([whether C++ compiler supports $1])
  ncmpcpp_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT([yes])
     ncmpcpp_supported_CXXFLAGS="$ncmpcpp_supported_CXXFLAGS $1"],
    [AC_MSG_RESULT([no])])
  CXXFLAGS="$ncmpcpp_save_CXXFLAGS"
  AC_LANG_POP([C++])
])

AC_PROG_CC
AC_PROG_CXX
LT_INIT
AC_LANG([C++])

AC_ARG_WITH(lto, AS_HELP_STRING([--with-lto], [Enable LTO (link time optimization) @<:@default=yes@:>@]), [lto=$withval], [lto=yes])

AC_DEFINE([ENABLE_OUTPUTS], [1], [enables outputs screen])
AC_DEFINE([ENABLE_VISUALIZER], [1], [enables music visualizer screen])

# -std=c23
AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether C compiler supports C23])
old_CFLAGS="$CFLAGS"
std_c23=""
for flag in -std=c23 -std=c2x; do
  CFLAGS="$old_CFLAGS $flag"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
    [std_c23="$flag"; break],
    []
  )
done
if test "x$std_c23" = "x"; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([[Your compiler doesn't seem to support C23, please upgrade]])
fi
AC_MSG_RESULT([$std_c23])
CFLAGS="$old_CFLAGS $std_c23"
AC_LANG_POP([C])

# -flto
if test "$lto" != "no"; then
  AC_MSG_CHECKING([whether compiler supports -flto])
  old_CXXFLAGS="$CXXFLAGS"
  # support specifying the number of threads
  if test "$lto" != "yes"; then
    flto="-flto=$lto"
  else
    flto="-flto"
  fi
  CXXFLAGS="$flto"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[ ]])],
    AC_MSG_RESULT([yes]),
    AC_MSG_RESULT([no])
      flto=""
  )
  CXXFLAGS="$old_CXXFLAGS $flto"
fi

# -ftree-vectorize
AC_MSG_CHECKING([whether compiler supports -ftree-vectorize])
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-ftree-vectorize"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
  AC_MSG_RESULT([yes])
    tree_vectorize="-ftree-vectorize",
  AC_MSG_RESULT([no])
    tree_vectorize=""
)
CXXFLAGS="$old_CXXFLAGS $tree_vectorize"

# -ffast-math
AC_MSG_CHECKING([whether compiler supports -ffast-math])
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-ffast-math"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
  AC_MSG_RESULT([yes])
    fast_math="-ffast-math",
  AC_MSG_RESULT([no])
    fast_math=""
)
CXXFLAGS="$old_CXXFLAGS $fast_math"

# -std=c++20
AC_MSG_CHECKING([whether compiler supports -std=c++20])
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-std=c++20"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
	AC_MSG_RESULT([yes])
		std_cpp14="-std=c++20",
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support C++20, please upgrade]])
)
CXXFLAGS="$old_CXXFLAGS $std_cpp14"

# generic lambda expressions support
AC_MSG_CHECKING([whether compiler supports generic lambda expressions])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[auto f = [](auto n) { return n*n; }; f(7);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support generic lambda expressions, please upgrade (GCC >= 5)]])
)

# warnings
ncmpcpp_supported_CFLAGS=""
ncmpcpp_supported_CXXFLAGS=""

for warning in -Wfatal-errors -Wall -Wextra -Wshadow \
               -Wimplicit-fallthrough; do
  NCMPCPP_APPEND_SUPPORTED_CFLAG([$warning])
  NCMPCPP_APPEND_SUPPORTED_CXXFLAG([$warning])
done

AC_LANG_PUSH([C])
AC_CACHE_CHECK([whether the C compiler is clang],
  [ncmpcpp_cv_c_compiler_clang],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifndef __clang__
#error not clang
#endif
  ]], [[]])],
    [ncmpcpp_cv_c_compiler_clang=yes],
    [ncmpcpp_cv_c_compiler_clang=no])])
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AC_CACHE_CHECK([whether the C++ compiler is clang],
  [ncmpcpp_cv_cxx_compiler_clang],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifndef __clang__
#error not clang
#endif
  ]], [[]])],
    [ncmpcpp_cv_cxx_compiler_clang=yes],
    [ncmpcpp_cv_cxx_compiler_clang=no])])
AC_LANG_POP([C++])

clang_common_warnings="-Weverything"
clang_common_warnings="$clang_common_warnings -Wno-unknown-warning-option"
clang_common_warnings="$clang_common_warnings -Wno-format-pedantic"
clang_common_warnings="$clang_common_warnings -Wno-unused-macros"
clang_common_warnings="$clang_common_warnings -Wno-unused-function"
clang_common_warnings="$clang_common_warnings -Wno-constant-logical-operand"
clang_common_warnings="$clang_common_warnings -Wno-float-equal"
clang_common_warnings="$clang_common_warnings -Wno-undefined-internal"
clang_common_warnings="$clang_common_warnings -Wno-cast-qual"
clang_common_warnings="$clang_common_warnings -Wno-unknown-pragmas"
clang_common_warnings="$clang_common_warnings -Wno-unsafe-buffer-usage"
clang_common_warnings="$clang_common_warnings -Wno-format-nonliteral"
clang_common_warnings="$clang_common_warnings -Wno-disabled-macro-expansion"
clang_common_warnings="$clang_common_warnings -Wno-ignored-attributes"
clang_common_warnings="$clang_common_warnings -Wno-covered-switch-default"
clang_common_warnings="$clang_common_warnings -Wno-used-but-marked-unused"
clang_common_warnings="$clang_common_warnings -Wno-implicit-int-enum-cast"
clang_common_warnings="$clang_common_warnings -Wno-cast-function-type-strict"

clang_cxx_warnings="$clang_common_warnings"
clang_cxx_warnings="$clang_cxx_warnings -Wno-c++98-compat"
clang_cxx_warnings="$clang_cxx_warnings -Wno-c++98-compat-pedantic"
clang_cxx_warnings="$clang_cxx_warnings -Wno-pre-c++20-compat"
clang_cxx_warnings="$clang_cxx_warnings -Wno-pre-c++20-compat-pedantic"

if test "$ncmpcpp_cv_c_compiler_clang" = "yes"; then
  for warning in $clang_common_warnings; do
    NCMPCPP_APPEND_SUPPORTED_CFLAG([$warning])
  done
fi

if test "$ncmpcpp_cv_cxx_compiler_clang" = "yes"; then
  for warning in $clang_cxx_warnings; do
    NCMPCPP_APPEND_SUPPORTED_CXXFLAG([$warning])
  done
fi

CFLAGS="$CFLAGS$ncmpcpp_supported_CFLAGS"
CXXFLAGS="$CXXFLAGS$ncmpcpp_supported_CXXFLAGS"

# various headers
AC_CHECK_HEADERS([netinet/tcp.h netinet/in.h], , AC_MSG_ERROR(vital headers missing))

# libmpdclient2
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= 2.8], [
	AC_SUBST(libmpdclient_CFLAGS)
	AC_SUBST(libmpdclient_LIBS)
	CPPFLAGS="$CPPFLAGS $libmpdclient_CFLAGS"
	AC_CHECK_HEADERS([mpd/client.h],
		LIBS="$LIBS $libmpdclient_LIBS"
	,
		AC_MSG_ERROR([missing mpd/client.h header])
	)
],
	AC_MSG_ERROR([libmpdclient >= 2.8 is required!])
)

# readline
AX_LIB_READLINE
if test "$ax_cv_lib_readline" = "no"; then
  AC_MSG_ERROR([no readline compatible library found])
fi
if test "$ax_cv_lib_readline_history" = "no"; then
  AC_MSG_WARN([readline library has no history functionality])
fi

# pthread
AC_CHECK_LIB(pthread, pthread_create, LIBS="$LIBS -lpthread",
	AC_MSG_ERROR([pthread library is required])
)

# ncursesw
PKG_CHECK_MODULES([ncursesw], [ncursesw], [
	AC_SUBST(ncursesw_CFLAGS)
	AC_SUBST(ncursesw_LIBS)
	CPPFLAGS="$CPPFLAGS $ncursesw_CFLAGS"
	AC_CHECK_HEADERS([curses.h],
		LIBS="$LIBS $ncursesw_LIBS"
	,
		AC_MSG_ERROR([missing curses.h header])
	)
  AC_CHECK_LIB(ncursesw, initscr, , AC_MSG_ERROR([ncursesw doesn't provide initscr]))
],
	AC_MSG_ERROR([ncursesw is required!])
)

# fftw3
PKG_CHECK_MODULES([fftw3], [fftw3 >= 3], [
	AC_SUBST(fftw3_LIBS)
	AC_SUBST(fftw3_CFLAGS)
	CPPFLAGS="$CPPFLAGS $fftw3_CFLAGS"
	AC_CHECK_HEADERS([fftw3.h],
		LIBS="$LIBS $fftw3_LIBS"
	,
		AC_MSG_ERROR([missing fftw3.h header])
	)
],
	AC_MSG_ERROR([fftw3 library is required!])
)

# libcurl
PKG_CHECK_MODULES([libcurl], [libcurl], [
	AC_SUBST(libcurl_CFLAGS)
	AC_SUBST(libcurl_LIBS)
	CPPFLAGS="$CPPFLAGS $libcurl_CFLAGS"
	AC_CHECK_HEADERS([curl/curl.h],
		LIBS="$LIBS $libcurl_LIBS"
	,
		AC_MSG_ERROR([missing curl/curl.h header])
	)
],
	AC_MSG_ERROR([libcurl is required!])
)

# taglib C API
PKG_CHECK_MODULES([taglib], [taglib_c], [
	AC_SUBST(taglib_CFLAGS)
	AC_SUBST(taglib_LIBS)
], [
	AC_CHECK_LIB([tag_c], [taglib_file_new], [
		taglib_LIBS="-ltag_c"
		AC_SUBST(taglib_CFLAGS)
		AC_SUBST(taglib_LIBS)
	], [
		AC_MSG_ERROR([could not find taglib_c.pc or libtag_c])
	])
])

taglib_c_ok=yes
CPPFLAGS="$CPPFLAGS $taglib_CFLAGS"
LIBS="$LIBS $taglib_LIBS"

AC_CHECK_HEADER([tag_c.h], [], [taglib_c_ok=no])
AC_CHECK_FUNCS([taglib_file_new taglib_file_save \
	taglib_property_get taglib_property_set \
	taglib_property_set_append], [], [taglib_c_ok=no])

if test "$taglib_c_ok" = "yes" ; then
	AC_DEFINE([HAVE_TAGLIB_H], [1],
	          [Define if TagLib C API is available])
else
	AC_MSG_ERROR([missing required TagLib C API])
fi

AC_CONFIG_FILES([Makefile cbase/Makefile src/Makefile tests/Makefile doc/Makefile])
AC_OUTPUT
