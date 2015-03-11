Name:       nemo-qml-plugin-dbus-qt5
Summary:    DBus plugin for Nemo Mobile
Version:    2.0.0
Release:    1
Group:      System/Libraries
License:    LGPLv2.1
URL:        https://github.com/nemomobile/nemo-qml-plugin-dbus
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)

%description
%{summary}.

%package tests
Summary:    DBus plugin tests
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtdeclarative-import-qttest
Requires:   qt5-qtdeclarative-devel-tools

%package testd
Summary:    DBus service used for unit testing
Group:      System/Libraries

%description tests
%{summary}.

%description testd
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make %{?jobs:-j%jobs}
make -C dbustestd %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install
make -C dbustestd install ROOT=%{buildroot} VERS=%{version}

%files
%defattr(-,root,root,-)
%dir %{_libdir}/qt5/qml/org/nemomobile/dbus
%dir %{_libdir}/qt5/qml/org/nemomobile/dbus/qmldir
%{_libdir}/qt5/qml/org/nemomobile/dbus/libnemodbus.so
%{_libdir}/qt5/qml/org/nemomobile/dbus/qmldir

%files tests
%defattr(-,root,root,-)
%dir /opt/tests/nemo-qml-plugins/dbus/auto
/opt/tests/nemo-qml-plugins/dbus/auto/*

%files testd
%defattr(-,root,root,-)
%dir /opt/tests/nemo-qml-plugins/dbus/bin
%dir /opt/tests/nemo-qml-plugins/dbus/manual
%dir /usr/share/dbus-1/services
/opt/tests/nemo-qml-plugins/dbus/bin/dbustestd
/opt/tests/nemo-qml-plugins/dbus/manual/tst_dbus_interface.qml
/usr/share/dbus-1/services/org.nemomobile.dbustestd.service
