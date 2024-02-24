%define name binio
%define version 1.4
%define release 1mdk
%define libname %mklibname %name 1

Summary: Binary I/O stream class library
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.bz2
URL:http://adplug.github.io/libbinio
License: LGPL
Group: System/Libraries
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

%package -n %libname
Summary: Shared library for lib%name
Group: System/Libraries

%description -n %libname
The binary I/O stream class library presents a platform-independent
way to access binary data streams in C++.

The library is hardware independent in the form that it transparently
converts between the different forms of machine-internal binary data
representation.

It further employs no special I/O protocol and can be used on
arbitrary binary data sources.

This package contains the shared library needed to run applications
based on %name.

%package -n %libname-devel
Summary: Development files for lib%name
Group: Development/C++
Provides: %name-devel = %version-%release
Provides: lib%name-devel = %version-%release
Requires: %libname = %version-%release

%description -n %libname-devel
The binary I/O stream class library presents a platform-independent
way to access binary data streams in C++.

The library is hardware independent in the form that it transparently
converts between the different forms of machine-internal binary data
representation.

It further employs no special I/O protocol and can be used on
arbitrary binary data sources.

This package contains C++ header files, the shared library symlink and
the developer documentation for %name.

%package -n %libname-static-devel
Summary: Static library for lib%name
Group: Development/C++
Requires: %libname-devel = %version-%release

%description -n %libname-static-devel
The binary I/O stream class library presents a platform-independent
way to access binary data streams in C++.

The library is hardware independent in the form that it transparently
converts between the different forms of machine-internal binary data
representation.

It further employs no special I/O protocol and can be used on
arbitrary binary data sources.

This package contains the static library of %name.

%prep
%setup -q

%build
%configure2_5x
%make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall_std

%clean
rm -rf $RPM_BUILD_ROOT

%post -n %libname -p /sbin/ldconfig
%postun -n %libname -p /sbin/ldconfig

%post -n %libname-devel
%_install_info libbinio.info

%postun -n %libname-devel
%_remove_install_info libbinio.info

%files -n %libname
%defattr(-,root,root)
%doc README AUTHORS ChangeLog NEWS
%_libdir/*.so.*

%files -n %libname-devel
%defattr(-,root,root)
%_includedir/*.h
%_libdir/*.so
%_libdir/*.la
%_infodir/*.info*

%files -n %libname-static-devel
%defattr(-,root,root)
%_libdir/*.a


%changelog
* Mon Mar  3 2003 Götz Waschk <waschk@linux-mandrake.com> 1.2-1mdk
- initial package
