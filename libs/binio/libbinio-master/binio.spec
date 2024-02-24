%define name binio
%define version 1.5
%define release 1

Summary: Binary I/O stream class library
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.bz2
URL:http://adplug.github.io/libbinio
License: LGPL
Group: System Environment/Libraries
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}

%description
The binary I/O stream class library presents a platform-independent
way to access binary data streams in C++.

The library is hardware independent in the form that it transparently
converts between the different forms of machine-internal binary data
representation.

It further employs no special I/O protocol and can be used on
arbitrary binary data sources.


%package devel
Summary: Development files for lib%name
Group: Development/Libraries
Requires: %name = %version-%release

%description devel
The binary I/O stream class library presents a platform-independent
way to access binary data streams in C++.

The library is hardware independent in the form that it transparently
converts between the different forms of machine-internal binary data
representation.

It further employs no special I/O protocol and can be used on
arbitrary binary data sources.

This package contains C++ header files, the shared library symlink and
the developer documentation for %name.

%prep
%setup -q

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post devel
if [[ -f /usr/share/info/libbinio.info.gz ]];then /sbin/install-info /usr/share/info/libbinio.info.gz --dir=/usr/share/info/dir;fi

%postun devel
if [ "$1" = "0" ]; then if [[ -f /usr/share/info/libbinio.info.gz ]];then /sbin/install-info /usr/share/info/libbinio.info.gz --dir=/usr/share/info/dir --remove ;fi; fi

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog NEWS
%_libdir/*.so.*

%files devel
%defattr(-,root,root)
%_includedir/*.h
%_libdir/*.so
%_libdir/*a
%_infodir/*.info*

%changelog
* Mon Mar  3 2003 Götz Waschk <waschk@linux-mandrake.com> 1.2-1
- initial package
