dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Netrek installation
dnl	to configure the system for the local environment.
AC_PREREQ([2.71])
AC_INIT([netrek-client-cow],[3.3.2],[quozl@netrek.org])
AC_CONFIG_SRCDIR([socket.c])
AC_CANONICAL_TARGET
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

AC_USE_SYSTEM_EXTENSIONS

AC_C_INLINE

#--------------------------------------------------------------------
#	Include sys/select.h if it exists and if it supplies things
#	that appear to be useful.  This appears to be true only on
#	the RS/6000 under AIX.  Some systems like OSF/1 have a
#	sys/select.h that's of no use, and other systems like SCO
#	UNIX have a sys/select.h that's pernicious.  If there isn't
#	a sys/select.h, then make sure that "fd_set" is defined in
#	sys/types.h.
#--------------------------------------------------------------------

AC_MSG_CHECKING(if fd_set requires sys/select.h)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>]], [[fd_set readMask, writeMask;]])],[AC_MSG_RESULT(no) ],[AC_EGREP_HEADER(fd_set, sys/select.h,
      AC_DEFINE(NEED_SYS_SELECT_H) AC_MSG_RESULT(yes),
      AC_DEFINE(NO_FD_SET) AC_MSG_RESULT(fd_set missing))])

#--------------------------------------------------------------------
#	Check for various typedefs and provide substitutes if
#	they don't exist.
#--------------------------------------------------------------------

AC_CHECK_HEADERS(unistd.h memory.h math.h stdlib.h)
AC_CHECK_HEADERS(sys/timeb.h sys/ptyio.h sys/fcntl.h fcntl.h)
AC_CHECK_HEADERS(ctype.h machine/endian.h sys/resource.h)
AC_CHECK_HEADERS(sys/wait.h netinet/in.h netinet/tcp.h sys/filio.h)

AC_TYPE_PID_T
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_FUNC_FORK
AC_STRUCT_TM

AC_MSG_CHECKING(for itimer in time.h)
AC_EGREP_HEADER(itimerval, time.h, AC_MSG_RESULT(yes) , AC_DEFINE(NEED_SYS_TIME_H) AC_MSG_RESULT(no))

AC_CHECK_SIZEOF(long)

AC_MSG_CHECKING(for u_int in sys/types.h)
AC_EGREP_HEADER(u_int, sys/types.h, AC_MSG_RESULT(yes) , AC_DEFINE(NO_U_INT) AC_MSG_RESULT(no))

AC_MSG_CHECKING(for PATH_MAX in limits.h)
AC_EGREP_CPP(PATH_MAX, [
#include <limits.h>
PATH_MAX
], AC_DEFINE(NO_PATH_MAX) AC_MSG_RESULT(no), AC_MSG_RESULT(yes))

AC_CHECK_FUNCS(strcmpi strncmpi)

#--------------------------------------------------------------------
#       Backend selection: --with-backend=sdl2|x11|auto
#       Default: auto (prefer SDL2 when all deps present, else X11)
#--------------------------------------------------------------------

AC_ARG_WITH([backend],
  [AS_HELP_STRING([--with-backend=sdl2|x11|auto],
    [select rendering backend (default: auto, prefer sdl2)])],
  [backend=$withval], [backend=auto])

NOSDL2="#"
NOWIN32="#"

#--------------------------------------------------------------------
#       Check for SDL2 backend dependencies via pkg-config
#--------------------------------------------------------------------

PKG_PROG_PKG_CONFIG

have_sdl2_all=no
if test "x$backend" = xsdl2 || test "x$backend" = xauto; then
  PKG_CHECK_MODULES([SDL2],       [sdl2 >= 2.0.0],
    [have_sdl2=yes],       [have_sdl2=no])
  PKG_CHECK_MODULES([SDL2_IMAGE], [SDL2_image >= 2.0.0],
    [have_sdl2_image=yes], [have_sdl2_image=no])
  PKG_CHECK_MODULES([SDL2_TTF],   [SDL2_ttf >= 2.0.0],
    [have_sdl2_ttf=yes],   [have_sdl2_ttf=no])
  PKG_CHECK_MODULES([SDL2_MIXER], [SDL2_mixer >= 2.0.0],
    [have_sdl2_mixer=yes], [have_sdl2_mixer=no])
  if test "x$have_sdl2$have_sdl2_image$have_sdl2_ttf$have_sdl2_mixer" = xyesyesyesyes; then
    have_sdl2_all=yes
  fi
fi

if test "x$backend" = xsdl2 && test "x$have_sdl2_all" = xno; then
  AC_MSG_ERROR([--with-backend=sdl2 requires sdl2 >= 2.0, SDL2_image, SDL2_ttf, and SDL2_mixer])
fi

if test "x$have_sdl2_all" = xyes; then
  AC_DEFINE([HAVE_SDL2], [1], [SDL2 rendering backend])
  NOSDL2=""
  NOX11="#"
  XXF86VM_LIBS=""
  AC_MSG_NOTICE([SDL2 rendering backend selected])
else
  XXF86VM_LIBS="-lXxf86vm"
  AC_MSG_NOTICE([X11 rendering backend selected])
fi

AC_SUBST(NOSDL2)
AC_SUBST(SDL2_CFLAGS)
AC_SUBST(SDL2_LIBS)
AC_SUBST(SDL2_IMAGE_CFLAGS)
AC_SUBST(SDL2_IMAGE_LIBS)
AC_SUBST(SDL2_TTF_CFLAGS)
AC_SUBST(SDL2_TTF_LIBS)
AC_SUBST(SDL2_MIXER_CFLAGS)
AC_SUBST(SDL2_MIXER_LIBS)
AC_SUBST(XXF86VM_LIBS)

#--------------------------------------------------------------------
#	Locate the X11 header files and the X11 library archive.
#--------------------------------------------------------------------

AC_PATH_X

if test "x$NOSDL2" = "x#"; then
  AC_DEFINE(HAVE_X11)
  AC_DEFINE(HAVE_X11_XPM_H)
fi

if test -z "$NOX11" ; then

if test -z "$x_libraries" ; then
echo checking for X11 header files
XINCLUDES=""
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <X11/Intrinsic.h>]])],[],[XINCLUDES="nope"])
if test "$XINCLUDES" = nope; then
    dirs="/usr/unsupported/include /usr/local/include /usr/X386/include \
      /usr/include/X11R4 /usr/X11R5/include /usr/include/X11R5 \
      /usr/openwin/include /usr/X11/include /pub/X11R5/include \
      /usr/local/X11R5/include /usr/X11R6/include /usr/include/X11R6 \
      /pub/X11R6/include /usr/local/X11R6/include"

    for i in $dirs ; do
	if test -r $i/X11/Intrinsic.h; then
	    XINCLUDES=" -I$i"
	fi
    done
fi
if test "$XINCLUDES" = nope; then
  echo "Warning:  couldn't find any X11 include files."
  XINCLUDES=""
fi

echo "checking for X11 library archive"
AC_CHECK_LIB(X11, main, XLIBSWLIB="-lX11", XLIBSWLIB=nope)
if test "$XLIBSWLIB" = nope; then
    dirs="/usr/unsupported/lib /usr/local/lib /usr/X386/lib \
      /usr/lib/X11R4 /usr/X11R5/lib /usr/lib/X11R5 /usr/openwin/lib \
      /usr/X11/lib /pub/X11R5/lib /usr/local/X11R5/lib \
      /usr/X11R6/lib /usr/lib/X11R6 /pub/X11R6/lib /usr/local/X11R6/lib"

    for i in $dirs ; do
	if test -r $i/libX11.a; then
	    XLIBSWDIR="-L$i"
	    XLIBSWLIB="-lX11"
	fi
    done
fi
if test "$XLIBSWLIB" = nope ; then
    AC_CHECK_LIB(Xwindow, main, XLIBSWLIB=-lXwindow)
fi
if test "$XLIBSWLIB" = nope ; then
    echo "Warning:  couldn't find the X11 library archive.  Using -lX11."
    XLIBSWLIB=-lX11
fi
else
XINCLUDES="-I$x_includes"
XLIBSWDIR="-L$x_libraries"
XLIBSWLIB="-lX11"
fi

fi

AC_SUBST(NOX11)
AC_SUBST(NOWIN32)
INCS=""

if test "x$NOSDL2" = "x#"; then
  SAVELIBS="$LIBS"
  LIBS="$LIBS $XLIBSWDIR $XLIBSWLIB"
  SAVEINCS="$INCS"
  INCS="$INCS $XINCLUDES"
  AC_CHECK_LIB(Xpm, main, [AC_DEFINE(HAVE_XPM) XPMLIBS="-lXpm"])
  AC_CHECK_HEADERS(X11/xpm.h)
  LIBS="$SAVELIBS"
  INCS="$SAVEINCS"

  # Fix link order for Xpm
  XLIBSW="$XLIBSWDIR $XPMLIBS $XLIBSWLIB"

  INCS="$INCS $XINCLUDES"
  LIBS="$LIBS $XLIBSW"
fi
AC_SUBST(INCS)

#--------------------------------------------------------------------
#	Check for the existence of various libraries.  The order here
#	is important, so that then end up in the right order in the
#	command line generated by Make.
#--------------------------------------------------------------------

AC_CHECK_LIB(Xbsd, main, [LIBS="$LIBS -lXbsd"])
AC_CHECK_LIB(socket, main, [LIBS="$LIBS -lsocket"])
AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])
AC_CHECK_LIB(seq, main, [LIBS="$LIBS -lseq"])
AC_CHECK_LIB(sun, main, [LIBS="$LIBS -lsun"])
AC_CHECK_FUNCS(usleep random setstate strftime ftime)
AC_CHECK_LIB(m, main, [LIBS="$LIBS -lm"])
AC_CHECK_FUNCS(nint)
AC_EGREP_HEADER(rint, math.h, , AC_DEFINE(NEED_RINT_DEC))
AC_REPLACE_FUNCS(usleep setstate strdup rint)

#--------------------------------------------------------------------
#       Check for system dependent programs
#--------------------------------------------------------------------

#--------------------------------------------------------------------
#       Check for Imlib2 (X11 backend only)
#--------------------------------------------------------------------
if test "x$NOSDL2" = "x#"; then
  AC_SEARCH_LIBS([imlib_load_image], [Imlib2], [],
                 [AC_MSG_ERROR([Could not find Imlib2 (required for X11 backend)])])
fi

#--------------------------------------------------------------------
#       Check for SDL 1.2 + SDL_mixer (X11 backend sound only)
#       SDL2 backend uses SDL2_mixer detected above via pkg-config.
#--------------------------------------------------------------------

SDL_CONFIG=""
SDL_CFLAGS=""
SDL_LIBS=""
SDL_MIXER_LIBS=""

if test "x$NOSDL2" = "x#"; then
  PKG_CHECK_MODULES([SDL], [sdl >= 1.2.4],
    [AC_DEFINE(HAVE_SDL, 1, [SDL 1.2 available]) no_sdl=no],
    [no_sdl=yes])
  if test "x$no_sdl" = xno; then
    SDL_CFLAGS="$SDL_CFLAGS"
    SDL_LIBS="$SDL_LIBS"
    AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
      [SDL_MIXER_LIBS="-lSDL_mixer"],
      [AC_MSG_ERROR([SDL_mixer not found; required for X11 backend sound])])
  fi
fi

AC_SUBST(SDL_CONFIG)
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)
AC_SUBST(SDL_MIXER_LIBS)

#--------------------------------------------------------------------
#       Emit output
#--------------------------------------------------------------------
AC_CONFIG_FILES([system.mk])
AC_OUTPUT
