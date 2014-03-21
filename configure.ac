AC_INIT([usb_moded], [0.79])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_C_CONST
AC_HEADER_STDC

test_gcc_flag() {
#    AC_LANG_CONFTEST([int main() {}])
    $CC -c conftest.c $CFLAGS $@ > /dev/null 2> /dev/null
    ret=$?
    rm -f conftest.o
    return $ret
}

# We use gnu99 instead of c99 because many have interpreted the standard
# in a way that int64_t isn't defined on non-64 bit platforms.
CFLAGS="-Os -std=gnu99 -Wall -W -Wextra -pedantic -pipe -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wendif-labels -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter -finline-small-functions -Wno-unused-result"

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug],[Enable debug @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) debug=true ; CFLAGS="-g -ggdb -O0 -DDEBUG -DOSSOLOG_STDERR $CFLAGS" ;;
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

AC_ARG_ENABLE([meegodevlock], AS_HELP_STRING([--enable-meegodevlock], [Enable Meego devicelock @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) meegodevlock=true ; CFLAGS="-DMEEGOLOCK $CFLAGS" ;;
   no)  meegodevlock=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-meegodevlock]) ;;
   esac],[meegodevlock=false])
AM_CONDITIONAL([MEEGOLOCK], [test x$meegodevlock = xtrue])

AC_ARG_ENABLE([n900], AS_HELP_STRING([--enable-n900], [Enable N900 extra's @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) n900=true ; CFLAGS="-DN900 $CFLAGS" ;;
   no)  n900=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-n900]) ;;
   esac],[n900=false])
AM_CONDITIONAL([N900], [test x$n900 = xtrue])

AC_ARG_ENABLE([gconf], AS_HELP_STRING([--enable-gconf], [Use GConf for configuration storage @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) gconf=true ; CFLAGS="-DGCONF -I/usr/include/gconf/2 -I/usr/include/orbit-2.0 -lgconf-2 $CFLAGS" ;;
   no)  conf=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-gconf]) ;;
   esac],[gconf=false])
AM_CONDITIONAL([GCONF], [test x$gconf = xtrue])

AC_ARG_ENABLE([app_sync], AS_HELP_STRING([--enable-app-sync], [Enable application syncing @<:@default=true@:>@]),
  [case "${enableval}" in
   yes) app_sync=true ; CFLAGS="-DAPP_SYNC $CFLAGS" ;;
   no)  app_sync=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-app-sync]) ;;
   esac],[app_sync=true])
AM_CONDITIONAL([APP_SYNC], [test x$app_sync = xtrue])

AC_ARG_ENABLE([app_sync_dbus], AS_HELP_STRING([--enable-app-sync-dbus], [Enable application syncing with dbus @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) app_sync_dbus=true ; CFLAGS="-DAPP_SYNC_DBUS $CFLAGS" ;;
   no)  app_sync_dbus=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-app-sync-dbus]) ;;
   esac],[app_sync_dbus=false])
AM_CONDITIONAL([APP_SYNC_DBUS], [test x$app_sync_dbus = xtrue])

AC_ARG_ENABLE([udev], AS_HELP_STRING([--enable-udev], [Enable udev interface @<:@default=true@:>@]),
  [case "${enableval}" in
   yes) udev=true ; CFLAGS="-DUDEV -ludev $CFLAGS" ;;
   no)  udev=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-udev]) ;;
   esac],[udev=true CFLAGS="-DUDEV -ludev $CFLAGS" ])
AM_CONDITIONAL([UDEV], [test x$udev = xtrue])

AC_ARG_ENABLE([upstart], AS_HELP_STRING([--enable-upstart], [Enable upstart interface for appsync @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) upstart=true ; CFLAGS="-DUPSTART $CFLAGS" ;;
   no)  upstart=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-upstart]) ;;
   esac],[upstart=false])
AM_CONDITIONAL([UPSTART], [test x$upstart = xtrue])

AC_ARG_ENABLE([connman], AS_HELP_STRING([--enable-connman], [Enable connman support for usb tethering @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) connman=true ; CFLAGS="-DCONNMAN $CFLAGS" ;;
   no)  connman=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-connman]) ;;
   esac],[connman=false])
AM_CONDITIONAL([CONNMAN], [test x$connman = xtrue])

AC_ARG_ENABLE([systemd], AS_HELP_STRING([--enable-systemd], [Enable systemd notify interface @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) systemd=true ; CFLAGS="-DSYSTEMD -lsystemd-daemon $CFLAGS" ;;
   no)  systemd=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-systemd]) ;;
   esac],[systemd=false])
AM_CONDITIONAL([SYSTEMD], [test x$systemd = xtrue])

AC_ARG_ENABLE([ofono], AS_HELP_STRING([--enable-ofono], [Enable ofono DBUS interface for usb tethering roaming detection  @<:@default=false@:>@]),
  [case "${enableval}" in
   yes) ofono=true ; CFLAGS="-DOFONO $CFLAGS" ;;
   no)  ofono=false ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-ofono]) ;;
   esac],[ofono=false])
AM_CONDITIONAL([SYSTEMD], [test x$ofono = xtrue])

#TODO: Figure out how to check for this depending on the gconf flag
# gconf-2.0 >= 2.16.0
PKG_CHECK_MODULES([USB_MODED], [
 glib-2.0 >= 2.24.0
 dbus-1 >= 1.2.1
 dbus-glib-1 >= 0.78
 gobject-2.0 >= 2.16.6
 gio-2.0
 libkmod
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
    LDFLAGS:		    ${LDFLAGS}

    Debug enabled:          ${debug}
"
AC_OUTPUT
