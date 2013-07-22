Name:     usb-moded
Version:  0.64
Release:  0 
Summary:  USB mode controller
Group:    System/System Control
License:  LGPLv2
URL:      https://github.com/nemomobile/usb-moded
Source0:  %{name}-%{version}.tar.bz2
Source1:  %{name}.service
Source2:  usb_moded.conf

BuildRequires: pkgconfig(dbus-1)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(gconf-2.0)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(udev)
BuildRequires: pkgconfig(libkmod)
BuildRequires: doxygen
BuildRequires: GConf2

Requires: dbus-x11
Requires: lsof
Requires: usb-moded-configs
Requires(post): GConf2
Requires(pre): GConf2
Requires(preun): GConf2
Requires(post): systemd
Requires(postun): systemd

%description
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

%package devel
Summary:  USB mode controller - development files
Group:    Development/Libraries

%description devel
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the files needed to program for usb_moded.

%package doc
Summary:  USB mode controller - documentation
Group:    Documentation

%description doc
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the documentation.

%package developer-mode
Summary:  USB mode controller - developer mode config
Group:  Config

%description developer-mode
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the developer mode config, which enables
usb networking.

%package mtp-mode
Summary:  USB mode controller - mtp mode config
Group:  Config
Requires: buteo-mtp

%description mtp-mode
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the mtp mode config.

%package mass-storage-mode
Summary:  USB mode controller - mass-storage mode config
Group:  Config

%description mass-storage-mode
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the mass-storage mode config.

%package adb-mode
Summary:  USB mode controller - android adb mode config
Group:  Config

%description adb-mode
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the adb config for use with the android
gadget driver.

%package developer-mode-android
Summary:  USB mode controller - android developer mode config
Group:  Config

%description developer-mode-android
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the developer mode config for use with
the android gadget. This will provide usb networking.

%package mtp-mode-android
Summary:  USB mode controller - android mtp mode config
Group:  Config

%description mtp-mode-android
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the mtp mode config for android.

%package diag-mode-android
Summary:  USB mode controller - android diag mode config
Group:  Config

%description diag-mode-android
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the diag mode config for use with the
android gadget.

%package acm-mode-android
Summary:  USB mode controller - android acm mode config
Group:  Config

%description acm-mode-android
Usb_moded is a daemon to control the USB states. For this
it loads unloads the relevant usb gadget modules, keeps track
of the filesystem(s) and notifies about changes on the DBUS
system bus.

This package contains the acm mode config for use with the android
gadget.

%package usb-moded-defaults
Summary: USB mode controller - default configuration
Group: Config
Provides: usb-moded-configs
Requires: usb-moded-developer-mode

%description usb-moded-defaults
This package provides the default configuration for usb-moded, so
basic functionality is provided (i.e. usb networking, ask and charging
modes)

%package usb-moded-defaults-android
Summary: USB mode controller - default configuration
Group: Config
Provides: usb-moded-configs
Requires: usb-moded-developer-mode-android

%description usb-moded-defaults-android
This package provides the default configuration for usb-moded, so
basic functionality is provided (i.e. usb networking, ask and charging
modes with the android gadget driver)

%prep
%setup -q

%build
%autogen
%configure --enable-app-sync --enable-udev --enable-n900
make all doc %{?_smp_mflags}

%install
%make_install
install -m 644 -D src/usb_moded-dbus.h %{buildroot}/%{_includedir}/%{name}/usb_moded-dbus.h
install -m 644 -D src/usb_moded-modes.h %{buildroot}/%{_includedir}/%{name}/usb_moded-modes.h
install -m 644 -D src/usb_moded-appsync-dbus.h %{buildroot}/%{_includedir}/%{name}/usb_moded-appsync-dbus.h
install -m 644 -D usb_moded.pc %{buildroot}/%{_libdir}/pkgconfig/usb_moded.pc
install -d %{buildroot}/%{_docdir}/%{name}/html/
install -m 644 docs/html/* %{buildroot}/%{_docdir}/%{name}/html/
install -m 644 -D debian/%{name}.schemas %{buildroot}/%{_sysconfdir}/gconf/schemas/%{name}.schemas
install -m 644 -D debian/manpage.1 %{buildroot}/%{_mandir}/man1/usb-moded.1
install -m 644 -D debian/usb_moded.conf %{buildroot}/%{_sysconfdir}/dbus-1/system.d/usb_moded.conf
install -m 644 -D %{SOURCE2} %{buildroot}/%{_sysconfdir}/modprobe.d/usb_moded.conf
install -m 644 -D %{SOURCE1} %{buildroot}/lib/systemd/system/%{name}.service
install -d %{buildroot}/%{_sysconfdir}/usb-moded
install -d %{buildroot}/%{_sysconfdir}/usb-moded/run
install -d %{buildroot}/%{_sysconfdir}/usb-moded/dyn-modes
install -m 644 -D config/dyn-modes/* %{buildroot}/%{_sysconfdir}/usb-moded/dyn-modes/
install -m 644 -D config/run/* %{buildroot}/%{_sysconfdir}/usb-moded/run/
install -d $RPM_BUILD_ROOT/lib/systemd/system/multi-user.target.wants/
ln -s ../%{name}.service $RPM_BUILD_ROOT/lib/systemd/system/multi-user.target.wants/%{name}.service
# Sync mode not packaged for now.
rm %{buildroot}/etc/usb-moded/dyn-modes/sync_mode.ini

%pre
#if [ "$1" -gt 1 ]; then
#  export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
#  gconftool-2 --makefile-uninstall-rule \
#    %{_sysconfdir}/gconf/schemas/usb-moded.schemas \
#    > /dev/null || :
#fi

%preun
#if [ "$1" -eq 0 ]; then
#  export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
#  gconftool-2 --makefile-uninstall-rule \
#    %{_sysconfdir}/gconf/schemas/usb-moded.schemas \
#    > /dev/null || :
#fi
systemctl daemon-reload

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule \
    %{_sysconfdir}/gconf/schemas/usb-moded.schemas  > /dev/null || :
systemctl daemon-reload

%files
%defattr(-,root,root,-)
%doc debian/copyright
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/usb_moded.conf
%config(noreplace) %{_sysconfdir}/gconf/schemas/%{name}.schemas
%config(noreplace) %{_sysconfdir}/modprobe.d/usb_moded.conf
%{_sbindir}/usb_moded
%{_sbindir}/usb_moded_util
%{_mandir}/man1/usb-moded.1.gz
/lib/systemd/system/%{name}.service
/lib/systemd/system/multi-user.target.wants/%{name}.service

%files devel
%defattr(-,root,root,-)
%doc debian/copyright
%{_includedir}/%{name}/*
%{_libdir}/pkgconfig/usb_moded.pc

%files doc
%defattr(-,root,root,-)
%doc debian/changelog debian/copyright LICENSE
%{_docdir}/%{name}/html/*

%files developer-mode
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/developer_mode.ini
%{_sysconfdir}/usb-moded/dyn-modes/sync_mode.ini

%files mtp-mode
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/mtp_mode.ini
%{_sysconfdir}/usb-moded/run/mtp.ini

%files mass-storage-mode
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/mass-storage.ini

%files developer-mode-android
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/developer_mode-android.ini

%files adb-mode
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/adb_mode.ini
%{_sysconfdir}/usb-moded/run/adb.ini

%files mtp-mode-android
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/mtp_mode-android.ini

%files diag-mode-android
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/diag_mode.ini
%{_sysconfdir}/usb-moded/run/adb-diag.ini

%files acm-mode-android
%defattr(-,root,root,-)
%{_sysconfdir}/usb-moded/dyn-modes/android_acm.ini

%files usb-moded-defaults
%defattr(-,root,root,-)

%files usb-moded-defaults-android
%defattr(-,root,root,-)

