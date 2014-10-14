SUMMARY = "Usb-moded: usb gadget driver handler and modesetting "
DESCRIPTION = "Usb_moded provides a dbus interface, app autostarting and configuration \
for USB gadgets. This way several different modes can be handled and network gadgets configured.\
It also helps to save memory by only running USB services when plugged etc..." 
HOMEPAGE = "https://github.com/nemomobile/usb-moded"
SECTION = "libs/network"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5f30f0716dfdd0d91eb439ebec522ec2"
DEPENDS = "dbus-glib intltool-native"
#RDEPENDS_${PN} = "ofono"


EXTRA_OECONF  +="\
	--enable-ofono \
	--enable-debug \
	--enable-dbus-appsync \
"
SYSTEMD_SERVICE_${PN} = "usb-moded.service"

do_install_append() {
	install -m 644 -D src/usb_moded-dbus.h ${D}/${includedir}/usb-moded/usb_moded-dbus.h
	install -m 644 -D src/usb_moded-modes.h ${D}/${includedir}/usb-moded/usb_moded-modes.h
	install -m 644 -D src/usb_moded-appsync-dbus.h ${D}/${includedir}/usb-moded/usb_moded-appsync-dbus.h
	install -m 644 -D usb_moded.pc ${D}/${libdir}/pkgconfig/usb_moded.pc
	install -d ${D}/${docdir}/usb-moded
	install -m 644 docs/usb_moded-doc.txt ${D}/${docdir}/usb-moded/
	install -m 644 -D debian/manpage.1 ${D}/${mandir}/man1/usb-moded.1
	install -m 644 -D debian/usb_moded.conf ${D}/${sysconfdir}/dbus-1/system.d/usb_moded.conf
	install -m 644 -D ${S}/debian/usb_moded.conf ${D}/${sysconfdir}/modprobe.d/usb_moded.conf
	install -d ${D}/${sysconfdir}/usb-moded
	install -d ${D}/${sysconfdir}/usb-moded/run
	install -d ${D}/${sysconfdir}/usb-moded/run-diag
	install -d ${D}/${sysconfdir}/usb-moded/dyn-modes
	install -d ${D}/${sysconfdir}/usb-moded/diag
	install -m 644 -D config/dyn-modes/* ${D}/${sysconfdir}/usb-moded/dyn-modes/
	install -m 644 -D config/diag/* ${D}/${sysconfdir}/usb-moded/diag/
	install -m 644 -D config/run/* ${D}/${sysconfdir}/usb-moded/run/
	install -m 644 -D config/run/udhcpd-developer-mode.ini ${D}/${sysconfdir}/usb-moded/run/dhcpd-developer-android-mode.ini
	install -m 644 -D config/run-diag/* ${D}/${sysconfdir}/usb-moded/run-diag/
}

PACKAGES =+ "${PN}-android-gadget-configs ${PN}-kernel-gadget-configs"

FILES_${PN}-android-gadget-configs += "${sysconfdir}/usb-moded/dyn-modes/*android* \
	${sysconfdir}/usb-moded/dyn-modes/connection_sharing.ini \
	${sysconfdir}/usb-moded/dyn-modes/diag_mode.ini \
	${sysconfdir}/usb-moded/dyn-modes/adb_mode.ini \
	${sysconfdir}/usb-moded/run/adb.ini \
	${sysconfdir}/usb-moded/run/adb-diag.ini \
	${sysconfdir}/usb-moded/run/udhcpd-connection-sharing.ini \
	${sysconfdir}/usb-moded/run/udhcpd-developer-mode-android.ini \
	${sysconfdir}/usb-moded/run-diag/* \
	${sysconfdir}/usb-moded/diag/* \
"
FILES_${PN}-kernel-gadget-configs += "${sysconfdir}/usb-moded/dyn-modes/* \
	${sysconfdir}/usb-moded/run/udhcpd-developer-mode.ini \
	${sysconfdir}/usb-moded/run/mtp.ini \
"
	
