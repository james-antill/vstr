# This is hacked from glib
# Note that this is NOT a relocatable package
%define ver      0.9.1
%define libver  0.9
%define  RELEASE 1
%define  rel     %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix   /usr
%define name vstr

Summary: String library, safe, quick and easy to use.
Name: %{name}
Version: %ver
Release: %rel
Copyright: LGPL
Group: Development/Libraries
Source: ftp://ftp.and.org/pub/james/%{name}/%{ver}/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/{name}-%{PACKAGE_VERSION}-root
URL: http://www.and.org/vstr/
Docdir: %{prefix}/doc
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

* Wed Mar 20 2002 James Antill <james@and.org>
- Hack a spec file.

%prep
%setup

%build
# For devel releases
if [ ! -f configure ]; then
  ./autogen.sh
fi

%configure --prefix=%{prefix}

make

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
make DESTDIR=$RPM_BUILD_ROOT install

%clean
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog NEWS README TODO

%{prefix}/lib/lib%{name}-%{libver}.so.*
%{prefix}/lib/lib%{name}.so

%files devel
%defattr(-, root, root)

%doc Documentation/functions.txt Documentation/functions.html Documentation/constants.txt Documentation/constants.html Documentation/namespace.txt Documentation/namespace.html Documentation/overview.txt Documentation/overview.html Documentation/structs.txt Documentation/structs.html Documentation/size_cmp.gnumeric

%{prefix}/lib/lib%{name}.a
%{prefix}/lib/pkgconfig/%{name}.pc
%{prefix}/include/*.h
