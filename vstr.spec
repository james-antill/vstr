%define ver      0.9.20
%define libver  0.9
%define real_release_num 1
%define RELEASE %{real_release_num}
%define rel     %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix   /usr
%define name    vstr

%define dbg_opt %{?dbg}%{!?dbg:0}

%if %{dbg_opt}
%define debugopt --enable-debug
%define RELEASE %{real_release_num}debug
%else
%define debugopt %nil
%endif

%ifarch i386
%define fmtfloatopt --with-fmt-float=%{?float}%{!?float:glibc}
%else
%define fmtfloatopt --with-fmt-float=%{?float}%{!?float:host}
%endif

%define devdoco %{_datadir}/doc/vstr-devel-%{ver}

Summary: String library, safe, quick and easy to use.
Name: %{name}
Version: %ver
Release: %rel
Copyright: LGPL
Group: Development/Libraries
Source: ftp://ftp.and.org/pub/james/%{name}/%{ver}/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/%{name}-%{PACKAGE_VERSION}-root
BuildRequires: pkgconfig >= 0.8, autoconf, automake, libtool
URL: http://www.and.org/vstr/
Packager: James Antill <james@and.org>

%description
 Vstr is a string library designed for network communication, but applicable 
in a number of other areas. It works on the idea of separate nodes of
information, and works on the length/ptr model and not the termination model a
la "C strings". It can also do automatic referencing for mmap() areas of
memory, and includes a portable version of a printf-like function. 

 Development libs and headers are in vstr-devel.

%package devel
Summary: String library, safe, quick and easy to use. Specilizes in IO.
Group: Development/Libraries
Requires: pkgconfig >= 0.8
Requires: %{name} = %{version}

%description devel
 Static libraries and header files for the Vstr string library
Also includes a vstr.pc file for pkg-config.

%changelog
* Sat Nov 16 2002 James Antill <james@and.org> 0.9.20-%{real_release_num}
- Added wrap memcpy/memset to the default configure options.

* Sat Nov  9 2002 James Antill <james@and.org>
- Add comparison to installed documentation.

* Mon Sep 30 2002 James Antill <james@and.org>
- Remove files for check-files 8.0 rpm check.

* Mon Sep 30 2002 James Antill <james@and.org>
- Changed COPYING to COPYING.LIB

* Tue Sep 17 2002 James Antill <james@and.org>
- Add BuildRequires
- Add Requires to -devel package.
- Use glibc FLOAT by default on i386 arch.

* Mon Sep  2 2002 James Antill <james@and.org>
- Add `float' cmd line define.

* Tue May 21 2002 James Antill <james@and.org>
- Add linker script to configure.

* Wed May  6 2002 James Antill <james@and.org>
- Add man page.

* Wed Mar 20 2002 James Antill <james@and.org>
- Hack a spec file.

%prep
%setup

%build
# For devel releases
if [ ! -f configure ]; then
  ./scripts/autogen.sh
fi

%configure --prefix=%{prefix} \
  --mandir=%{_mandir} --datadir=%{_datadir} \
  --libdir=%{_libdir} --includedir=%{_includedir} --enable-linker-script \
  --enable-wrap-memcpy --enable-wrap-memset \
  %{fmtfloatopt} %{debugopt}

make

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall

cp TODO BUGS $RPM_BUILD_ROOT/%{devdoco}/
rm -f $RPM_BUILD_ROOT/usr/lib/libvstr.la
rm -f $RPM_BUILD_ROOT/usr/share/doc/vstr-devel-%{ver}/functions.3
rm -f $RPM_BUILD_ROOT/usr/share/doc/vstr-devel-%{ver}/constants.3


%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%doc AUTHORS COPYING.LIB ChangeLog NEWS README TODO

%{_libdir}/lib%{name}-%{libver}.so.*

%files devel
%defattr(-, root, root)

%{_libdir}/lib%{name}.so
%{_libdir}/lib%{name}.a
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/*.h

%doc
%{devdoco}/BUGS
%{devdoco}/TODO
%{devdoco}/functions.txt
%{_mandir}/man3/vstr.3.gz
%{_mandir}/man3/vstr_const.3.gz
%{devdoco}/functions.html
%{devdoco}/constants.txt
%{devdoco}/constants.html
%{devdoco}/namespace.txt
%{devdoco}/namespace.html
%{devdoco}/overview.html
%{devdoco}/comparison.html
%{devdoco}/structs.txt
%{devdoco}/structs.html
%{devdoco}/size_cmp.gnumeric
