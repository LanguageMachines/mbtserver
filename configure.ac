#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 5764 2010-08-19 14:51:41Z joostvb $
# $URL: https://ilk.uvt.nl/svn/trunk/sources/Mbt3/configure.ac $

AC_PREREQ(2.61)
AC_INIT([MbtServer],[0.1],[timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

# AC_DEFINE_DIR is defined in acinclude.m4
AC_DEFINE_DIR([BIN_PATH], bindir, bindir)
AC_DEFINE_DIR([SYSCONF_PATH], sysconfdir, sysconfdir)
AC_DEFINE_DIR([LOCALSTATE_PATH], localstatedir, localstatedir)

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi

if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

# Checks for programs.
AC_PROG_CXX( [g++] )
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# when running tests, use CXX
AC_LANG([C++])


# Checks for header files.
AC_CHECK_HEADERS([])
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T


PKG_PROG_PKG_CONFIG
# Checks for timbl library.

# inspired by feh-1.3.4/configure.ac.  Tnx Tom Gilbert and feh hackers.

# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
if test "x$PKG_CONFIG_PATH" = x; then
     export PKG_CONFIG_PATH=""
fi
AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>; 
               note that you can install Timbl in a non-default directory with
               ./configure --prefix=<DIR> in the Timbl installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([timbl], [timbl >= 6.2.3] )
CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
LIBS="$LIBS $timbl_LIBS"

AC_ARG_WITH(timblserver,
       [  --with-timblserver=DIR       use timblserver installed in <DIR>; 
               note that you can install Timbl in a non-default directory with
               ./configure --prefix=<DIR> in the Timbl installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([timblserver], [timblserver >= 1.0.0] )
CXXFLAGS="$CXXFLAGS $timblserver_CFLAGS"
LIBS="$LIBS $timblserver_LIBS"

AC_ARG_WITH(mbt,
       [  --with-mbt=DIR       use mbt installed in <DIR>; 
               note that you can install mbt in a non-default directory with
               ./configure --prefix=<DIR> in the mbt installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([mbt], [mbt >= 3.2.2] )
CXXFLAGS="$CXXFLAGS $mbt_CFLAGS"
LIBS="$LIBS $mbt_LIBS"

# Checks for library functions.
AC_CHECK_FUNCS([strdup strerror])

AC_C_CONST
AC_TYPE_SIGNAL

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  include/Makefile
  include/mbtserver/Makefile
  init/Makefile
  init/mbtserver
  docs/Makefile
  example/Makefile
])

AC_OUTPUT
