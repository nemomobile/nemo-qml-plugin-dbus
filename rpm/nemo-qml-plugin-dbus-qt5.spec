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

%description
%{summary}.

%package tests
Summary:    DBus plugin tests
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtdeclarative-import-qttest
Requires:   qt5-qtdeclarative-devel-tools

%description tests
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install

%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/nemomobile/dbus/libnemodbus.so
%{_libdir}/qt5/qml/org/nemomobile/dbus/qmldir

%files tests
%defattr(-,root,root,-)
/opt/tests/nemo-qml-plugins/dbus/*
