# This is hacked from glib
# Note that this is NOT a relocatable package
%define ver      0.9.6
%define libver  0.9
%define  RELEASE 1
%define  rel     %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix   /usr
%define name vstr

%define devdoco %{_datadir}/doc/vstr-devel-%{ver}

Summary: String library, safe, quick and easy to use.
Name: %{name}
Version: %ver
Release: %rel
Copyright: LGPL
Group: Development/Libraries
Source: ftp://ftp.and.org/pub/james/%{name}/%{ver}/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/%{name}-%{PACKAGE_VERSION}-root
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

%description devel
 Static libraries and header files for the Vstr string library
Also includes a vstr.pc file for pkg-config.

%changelog
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
  ./autogen.sh
fi

%configure --prefix=%{prefix} \
  --mandir=%{_mandir} --datadir=%{_datadir} \
  --libdir=%{_libdir} --includedir=%{_includedir} --enable-linker-script

make

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
make DESTDIR=$RPM_BUILD_ROOT install
cp TODO BUGS $RPM_BUILD_ROOT/%{devdoco}/

%clean
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog NEWS README TODO

%{_libdir}/lib%{name}-%{libver}.so.*
%{_libdir}/lib%{name}.so

%files devel
%defattr(-, root, root)

%{_libdir}/lib%{name}.a
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/*.h

%doc
%{devdoco}/BUGS
%{devdoco}/TODO
%{devdoco}/functions.txt
%{_mandir}/man3/vstr.3.gz
%{devdoco}/functions.html
%{devdoco}/constants.txt
%{devdoco}/constants.html
%{devdoco}/namespace.txt
%{devdoco}/namespace.html
%{devdoco}/overview.txt
%{devdoco}/overview.html
%{devdoco}/structs.txt
%{devdoco}/structs.html
%{devdoco}/size_cmp.gnumeric
