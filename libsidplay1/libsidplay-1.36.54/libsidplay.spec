Name: libsidplay
Summary: A Commodore 64 music player and SID chip emulator library.
Version: 1.36.54
Release: 1
Source: libsidplay-%{version}.tgz
Icon: sidplay.xpm
Group: System Environment/Libraries
URL: http://www.geocities.com/SiliconValley/Lakes/5147/
License: GPL
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}

%description
This library provides the Sound Interface Device (SID) chip emulator
engine that is used by music player programs like SIDPLAY. With it
you can play musics from Commodore 64 (or compatible) programs.

%package -n libsidplay-devel
Summary: Header files for compiling apps that use libsidplay.
Group: System Environment/Libraries

%description -n libsidplay-devel
This package contains the header files for compiling applications
that use libsidplay.

%prep
rm -rf %{buildroot}

%setup -q

%build
%ifarch i386 i486 i586 i686 athlon
CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --enable-optfixpoint --enable-optendian
%endif
%ifnarch i386 i486 i586 i686 athlon
CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix}
%endif
make

%install
mkdir -p %{buildroot}
make DESTDIR=%{buildroot} install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{prefix}/lib/libsidplay*

%files -n libsidplay-devel
%defattr(-,root,root)
%doc AUTHORS COPYING DEVELOPER
%doc src/fastforward.txt src/format.txt src/mixing.txt src/mpu.txt src/panning.txt
%{_includedir}/sidplay

%changelog
* Thu Mar 28 2002 Michael Schwendt <sidplay@geocities.com>
- Updated to 1.36.53.

* Wed Feb 27 2002 Michael Schwendt <sidplay@geocities.com>
- Generated Makefile templates with automake -i. (Christian Weisgerber)
- Added two library optimization "configure" script options.
  This got rid of patch1.
- Rudimentary PSID v2NG support.

* Tue Feb 19 2002 Michael Schwendt <sidplay@geocities.com>
- Remerged support for backward compatible non-standard <strstream.h>.

* Fri Dec 28 2001 Michael Schwendt <sidplay@geocities.com>
- Merged MSVC compatibility changes.

* Mon Dec 17 2001 Michael Schwendt <sidplay@geocities.com>
- Fixed fast forward time count.

* Mon Nov 19 2001 Michael Schwendt <sidplay@geocities.com>
- Made compatible with libstdc++3 and GCC3.
- Rebuilt on Red Hat Linux 7.2.

* Thu May 03 2001 Michael Schwendt <sidplay@geocities.com>
- Rebuilt on Red Hat Linux 7.1.

* Sat Apr 15 2000 Michael Schwendt <sidplay@geocities.com>
- Initial RPM build of automake/libtool converted libsidplay.

