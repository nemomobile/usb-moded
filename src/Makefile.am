sbin_PROGRAMS = usb_moded

usb_moded_CPPFLAGS = \
        $(USB_MODED_CFLAGS)

usb_moded_LDFLAGS = \
	-Wl,--as-needed

usb_moded_LDADD = \
        $(USB_MODED_LIBS)
		
usb_moded_SOURCES = \
        usb_moded.c \
	usb_moded.h \
	usb_moded-modes.h \
	usb_moded-dbus.c \
	usb_moded-dbus.h \
	usb_moded-dbus-private.h \
	usb_moded-hw-ab.h \
	usb_moded-config-private.h \
	usb_moded-modules.c \
	usb_moded-modules.h \
	usb_moded-log.h \
	usb_moded-log.c \
	usb_moded-config.c \
	usb_moded-config.h \
	usb_moded-modesetting.c \
	usb_moded-modesetting.h 

if GCONF
usb_moded_SOURCES += \
	usb_moded-gconf.c \
	usb_moded-gconf.h 
endif

if HAL
usb_moded_SOURCES += \
	usb_moded-hal.c
endif

if BME
usb_moded_SOURCES += \
	usb_moded-chargerdetection.c 
endif

if UDEV
usb_moded_SOURCES += \
	usb_moded-udev.c \
	usb_moded-trigger.c
endif

if NOKIA	
usb_moded_SOURCES += \
	usb_moded-devicelock.h \
	usb_moded-devicelock.c 
endif

if APP_SYNC
usb_moded_SOURCES += \
	usb_moded-appsync.c \
	usb_moded-appsync.h \
	usb_moded-appsync-dbus.c \
	usb_moded-appsync-dbus.h \
	usb_moded-appsync-dbus-private.h \
	usb_moded-dyn-config.c \
	usb_moded-dyn-config.h
endif
	
