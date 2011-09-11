AC_INIT([usb_moded], [0.47])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_C_CONST
AC_HEADER_STDC

test_gcc_flag() {
    AC_LANG_CONFTEST([int main() {}])
    $CC -c conftest.c $CFLAGS $@ > /dev/null 2> /dev/null
    ret=$?
    rm -f conftest.o
    return $ret
}

# If using GCC specify some additional parameters
if test "x$GCC" = "xyes" ; then
    # We use gnu99 instead of c99 because many have interpreted the standard
    # in a way that int64_t isn't defined on non-64 bit platforms.
    DESIRED_FLAGS="-std=gnu99 -Wall -W -Wextra -pedantic -pipe -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wmissing-noreturn -Wendif-labels -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter"

    for flag in $DESIRED_FLAGS ; do
        AC_MSG_CHECKING([whether $CC accepts $flag])
        if test_gcc_flag $flag ; then 
           CFLAGS="$CFLAGS $flag"
           AC_MSG_RESULT([yes])
        else
           AC_MSG_RESULT([no])
        fi
    done 
fi

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug],[Enable debug @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) debug=true ; CFLAGS="-ggdb -O0 -DDEBUG -DOSSOLOG_STDERR $CFLAGS" ;;
   no)  debug=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
   esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

AC_ARG_ENABLE([nokia], AS_HELP_STRING([--enable-nokia], [Enable Nokia extra's @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) nokia=true ; CFLAGS="-DNOKIA -DN900 $CFLAGS" ;;
   no)  nokia=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-nokia]) ;;
   esac],[nokia=false])
AM_CONDITIONAL([NOKIA], [test x$nokia = xtrue])

AC_ARG_ENABLE([n900], AS_HELP_STRING([--enable-n900], [Enable N900 extra's @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) n900=true ; CFLAGS="-DN900 $CFLAGS" ;;
   no)  n900=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-n900]) ;;
   esac],[n900=false])
AM_CONDITIONAL([N900], [test x$n900 = xtrue])


AC_ARG_ENABLE([bme], AS_HELP_STRING([--enable-bme], [Enable Nokia bme cable connection interface @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) bme=true ; CFLAGS="-DBME $CFLAGS" ;;
   no)  bme=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-bme]) ;;
   esac],[bme=false])
AM_CONDITIONAL([BME], [test x$bme = xtrue])

AC_ARG_ENABLE([gconf], AS_HELP_STRING([--enable-gconf], [Use GConf for configuration storage @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) gconf=true ; CFLAGS="-DGCONF -I/usr/include/gconf/2 -I/usr/include/orbit-2.0 -lgconf-2 $CFLAGS" ;;
   no)  conf=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-gconf]) ;;
   esac],[gconf=false])
AM_CONDITIONAL([GCONF], [test x$gconf = xtrue])

AC_ARG_ENABLE([app_sync], AS_HELP_STRING([--enable-app-sync], [Enable application syncing @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) app_sync=true ; CFLAGS="-DAPP_SYNC $CFLAGS" ;;
   no)  app_sync=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-app_sync]) ;;
   esac],[app_sync=false])
AM_CONDITIONAL([APP_SYNC], [test x$app_sync = xtrue])

AC_ARG_ENABLE([hal], AS_HELP_STRING([--enable-hal], [Enable deprecated hal interface @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) hal=true ; CFLAGS="-DHAL -I/usr/include/hal -lhal $CFLAGS" ;;
   no)  hal=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-hal]) ;;
   esac],[hal=false])
AM_CONDITIONAL([HAL], [test x$hal = xtrue])

AC_ARG_ENABLE([udev], AS_HELP_STRING([--enable-udev], [Enable udev interface @<:@default=true@:>@]),
  [case "${enableval}" in
   yes) udev=true ; CFLAGS="-DUDEV -ludev $CFLAGS" ;;
   no)  udev=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-udev]) ;;
   esac],[udev=false])
AM_CONDITIONAL([UDEV], [test x$udev = xtrue])


#TODO: Figure out how to check for this depending on the gconf flag
# gconf-2.0 >= 2.16.0
PKG_CHECK_MODULES([USB_MODED], [
 glib-2.0 >= 2.24.0
 dbus-1 >= 1.2.1
 dbus-glib-1 >= 0.78
 gobject-2.0 >= 2.16.6
 gio-2.0
])

AC_SUBST(USB_MODED_LIBS)
AC_SUBST(USB_MODED_CFLAGS)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	usb_moded.pc
	docs/usb_moded-doxygen.conf
	docs/Makefile
])

echo "
 == $PACKAGE_NAME $VERSION ==

    Compiler:               ${CC}
    CFLAGS:                 ${CFLAGS}

    Debug enabled:          ${debug}
"
AC_OUTPUT
