%global _hardened_build 1

# Override values for specific architectures.
%ifarch ppc64le && 0%{?rhel} == 7
# Works around https://bugs.centos.org/view.php?id=13779 / https://bugzilla.redhat.com/show_bug.cgi?id=1489712
# Compilation failure on PPC64LE due to a compiler bug.
# REMEMBER TO REMOVE ONCE DOWNSTREAM FIXES THE ISSUE!
%global __global_cflags %{?__global_cflags} -mno-vsx
%global __global_cxxflags %{?__global_cxxflags} -mno-vsx
%endif

Name:           nx-libs
Version:        3.5.99.27
Release:        0.0build1%{?dist}
Summary:        NX X11 protocol compression libraries

Group:          System Environment/Libraries
%if 0%{?suse_version}
License:        GPL-2.0+
%else
License:        GPLv2+
%endif
URL:            http://github.com/ArcticaProject/nx-libs/
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  autoconf >= 2.13
BuildRequires:  automake
BuildRequires:  gcc-c++
BuildRequires:  libjpeg-devel
BuildRequires:  libtool
BuildRequires:  pkgconfig
%if 0%{?suse_version} && 0%{?suse_version} < 1210
BuildRequires:  xorg-x11-util-devel
%else
BuildRequires:  imake
%endif
# For xkbcomp.pc.
%if 0%{?suse_version}
# Earlier versions don't have xkbcomp.pc at all.
%if 0%{?suse_version} >= 1210
BuildRequires:  xkbcomp-devel
%endif
%else
%if 0%{?fedora} >= 34
BuildRequires:  xkbcomp-devel
%else
BuildRequires:  xorg-x11-xkb-utils-devel
%endif
%endif

# ideally we build with quilt (for mesa-quilt patch appliance script),
# it seems Fedora has it...
%if 0%{?fedora}
BuildRequires:  quilt
%endif

# other distros sometimes do have quilt, sometimes don't, let's
# not differentiate here when it is available and when not. Rather
# rely on stupid patch application fallback mode in mesa-quilt...
%if 0%{?rhel} || 0%{?suse_version}
BuildRequires:  patch
%endif

%if 0%{?suse_version}
BuildRequires:  fdupes

# This is what provides /usr/share/fonts on SUSE systems...
BuildRequires: filesystem

%if 0%{?suse_version} >= 1130
BuildRequires:  pkgconfig(expat)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(pixman-1) >= 0.13.2
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xpm)
#%%if 0%%{?suse_version} >= 42XX
#BuildRequires:  pkgconfig(xfont2)
#%%else
BuildRequires:  pkgconfig(xfont) >= 1.4.2
#%%endif
BuildRequires:  pkgconfig(xdmcp)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(xtst)
BuildRequires:  pkgconfig(xinerama)
%else
BuildRequires:  libexpat-devel
BuildRequires:  libpng-devel
BuildRequires:  libxml2-devel
BuildRequires:  libpixman-1-0-devel
BuildRequires:  xorg-x11-libX11-devel
BuildRequires:  xorg-x11-libXext-devel
BuildRequires:  xorg-x11-libXpm-devel
BuildRequires:  xorg-x11-devel
BuildRequires:  xorg-x11-proto-devel
BuildRequires:  xorg-x11-libXdmcp-devel
BuildRequires:  xorg-x11-libXfixes-devel
%endif
%endif

%if 0%{?fedora} || 0%{?rhel}
BuildRequires:  expat-devel
BuildRequires:  libpng-devel
BuildRequires:  libxml2-devel
BuildRequires:  pixman-devel >= 0.13.2
BuildRequires:  libX11-devel
BuildRequires:  libXext-devel
BuildRequires:  libXpm-devel
%if 0%{?fedora} >= 25 || 0%{?rhel} >= 8
BuildRequires:  libXfont2-devel
%else
BuildRequires:  libXfont-devel >= 1.4.2
%endif
BuildRequires:  libXdmcp-devel
BuildRequires:  libXdamage-devel
BuildRequires:  libXcomposite-devel
BuildRequires:  libXrandr-devel
BuildRequires:  libXfixes-devel
BuildRequires:  libXtst-devel
BuildRequires:  libXinerama-devel
BuildRequires:  xorg-x11-font-utils
%endif

# For imake
BuildRequires:  xorg-x11-proto-devel
BuildRequires:  zlib-devel

# RPC headers. Fedora 28+ and OpenSuSE Tumbleweed phased them out of glibc, like upstream did.
%if 0%{?fedora} > 27 || ( 0%{?suse_version} && ( 0%{?suse_version} > 1500 || 0%{?sle_version} > 150200 ) )
BuildRequires:  libtirpc-devel
%endif

%if 0%{?fedora} > 26 || 0%{?rhel} > 6
BuildRequires:  /usr/bin/pathfix.py
%endif

Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Provides:       nx%{?_isa} = %{version}-%{release}
Obsoletes:      libNX_Xau6 < 3.5.99.1
Obsoletes:      libNX_Xcomposite1 < 3.5.99.1
Obsoletes:      libNX_Xdamage1 < 3.5.99.1
Obsoletes:      libNX_Xdmcp6 < 3.5.99.1
Obsoletes:      libNX_Xext6 < 3.5.99.1
Obsoletes:      libNX_Xfixes3 < 3.5.99.1
Obsoletes:      libNX_Xinerama1 < 3.5.99.1
Obsoletes:      libNX_Xpm4 < 3.5.99.1
Obsoletes:      libNX_Xrandr2 < 3.5.99.1
Obsoletes:      libNX_Xrender1 < 3.5.99.1
Obsoletes:      libNX_Xtst6 < 3.5.99.1
Obsoletes:      libXcompext < 3.5.99.3

%if 0%{?el5}
# For compatibility with EPEL5
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
%else
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%endif

%description
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.


%package -n libNX_X11-6
Group:          System Environment/Libraries
Summary:        Core NX protocol client library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_X11 < 3.5.0.30
Provides:       libNX_X11 = %{version}-%{release}
Provides:       libNX_X11%{?_isa} = %{version}-%{release}
%if 0%{?suse_version}
Requires:       xorg-x11-fonts-core
%endif

%description -n libNX_X11-6
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The X Window System is a network-transparent window system that was
designed at MIT. X display servers run on computers with either
monochrome or color bitmap display hardware. The server distributes
user input to and accepts output requests from various client
programs located either on the same machine or elsewhere in the
network. Xlib is a C subroutine library that application programs
(clients) use to interface with the window system by means of a
stream connection.


%package -n libNX_X11-devel
Group:          Development/Libraries
Summary:        Development files for the Core NX protocol library
Requires:       libNX_X11-6%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_X11-devel
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The X Window System is a network-transparent window system that was
designed at MIT. X display servers run on computers with either
monochrome or color bitmap display hardware. The server distributes
user input to and accepts output requests from various client
programs located either on the same machine or elsewhere in the
network. Xlib is a C subroutine library that application programs
(clients) use to interface with the window system by means of a
stream connection.

This package contains all necessary include files and libraries
needed to develop applications that require these.

%package -n libXcomp-devel
Group:          Development/Libraries
Summary:        Development files for the NX differential compression library
Requires:       libXcomp3%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel = %{version}-%{release}

%description -n libXcomp-devel
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The NX differential compression library's development files.


%package -n libXcomp3
Group:          System Environment/Libraries
Summary:        NX differential compression library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libXcomp <= 3.5.1
Provides:       libXcomp = %{version}-%{release}
Provides:       libXcomp%{?_isa} = %{version}-%{release}

%description -n libXcomp3
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package contains the NX differential compression library for X11.


%package -n libXcompshad-devel
Group:          Development/Libraries
Summary:        Development files for the NX session shadowing library
Requires:       libXcompshad3%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}
Requires:       %{name}-devel%{?_isa} = %{version}-%{release}

%description -n libXcompshad-devel
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

The NX session shadowing library's development files.


%package -n libXcompshad3
Group:          System Environment/Libraries
Summary:        NX session shadowing Library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libXcompshad <= 3.5.1
Provides:       libXcompshad = %{version}-%{release}
Provides:       libXcompshad%{?_isa} = %{version}-%{release}

%description -n libXcompshad3
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package provides the session shadowing library.


%package devel
Group:          Development/Libraries
Summary:        Include files and libraries for NX development
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}
Requires:       %{name}%{?_isa} = %{version}-%{release}
Obsoletes:      libNX_Xau-devel < 3.5.99.1
Obsoletes:      libNX_Xdmcp-devel < 3.5.0.32-2
Obsoletes:      libNX_Xext-devel < 3.5.99.1
Obsoletes:      libNX_Xfixes-devel < 3.5.99.1
Obsoletes:      libNX_Xpm-devel < 3.5.0.32-2
Obsoletes:      libNX_Xrender-devel < 3.5.99.1

%description devel
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package contains all necessary include files and libraries
needed to develop nx-X11 applications that require these.


%package -n nx-proto-devel
Group:          Development/Libraries
Summary:        Include files for NX development

%description -n nx-proto-devel
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package contains all necessary include files and libraries
for the nx_X11 wire protocol.


%package -n nxagent
Group:          Applications/System
Summary:        NX Agent
Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Provides:       nx%{?_isa} = %{version}-%{release}
Obsoletes:      nxauth < 3.5.99.1
%if 0%{?fedora} || 0%{?rhel}
# For /usr/share/X11/fonts
Requires:       xorg-x11-font-utils
%endif

# Should be a weak dependency, because this package
# works without the dependency.
# *SUSE supports weak dependencies since version 10,
# so when it comes to our "supported" platforms any
# *SUSE version supports that feature.
# Fedora supports it since version 21. RHEL has not
# yet had any release based on anything newer than
# FC19, so no support on RHEL.
%if 0%{?fedora} >= 21 || 0%{?suse_version}
Recommends:     xkeyboard-config
%else
Requires:       xkeyboard-config
%endif

# For /usr/bin/xkbcomp
%if 0%{?fedora} || 0%{?rhel}
Requires:       xorg-x11-xkb-utils
%else
%if 0%{?suse_version}
%if 0%{?suse_version} >= 1310
Requires:       xkbcomp
%else
# Older *SUSE versions bundle xkbcomp in xorg-x11. Ugly, but nothing we could change.
Requires:       xorg-x11
%endif
%endif
%endif

%description -n nxagent
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

nxagent is an agent providing NX transport of X sessions. The
application is based on the well-known Xnest server. nxagent, like
Xnest, is an X server for its own clients, and at the same time, an X
client for a system's local X server.

The main scope of nxagent is to eliminate X round-trips or transform
them into asynchronous replies. nxagent works together with nxproxy.
nxproxy itself does not make any effort to minimize round-trips by
itself, this is demanded of nxagent.

Being an X server, nxagent is able to resolve all the property/atoms
related requests locally, ensuring that the most common source of
round-trips are nearly reduced to zero.


%package -n nxproxy
Group:          Applications/System
Summary:        NX Proxy
Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Provides:       nx%{?_isa} = %{version}-%{release}

%description -n nxproxy
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

This package provides the NX proxy (client) binary.


%package -n nxdialog
Group:          Applications/System
Summary:        NX Dialog
Provides:       nxdialog = %{version}-%{release}
Provides:       nxdialog%{?_isa} = %{version}-%{release}

%description -n nxdialog
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

This package provides the nxdialog helper script.


%prep
%setup -q
# remove build cruft that is in Git (also taken from roll-tarball.sh)
rm -Rf nx*/configure nx*/autom4te.cache*
# Install into /usr
sed -i -e 's,/usr/local,/usr,' nx-X11/config/cf/site.def
# Fix FSF address
find -name LICENSE | xargs sed -i \
  -e 's/59 Temple Place/51 Franklin Street/' -e 's/Suite 330/Fifth Floor/' \
  -e 's/MA  02111-1307/MA  02110-1301/'
# Fix source permissions
find -type f -name '*.[hc]' | xargs chmod -x
# Some systems do not know -Wpedantic
%if ( 0%{?rhel} && 0%{?rhel} < 7 ) || ( 0%{?suse_version} && 0%{?suse_version} < 1310 )
sed -i -e 's/Wpedantic/pedantic/g' nx-X11/config/cf/{{host,xorgsite}.def,xorg.cf}
%endif

# Bundled nx-X11/extras
# Mesa - Used by the X server

# Xcursor - Other code still references files in it


%build
cat >"my_configure" <<'EOF'
%configure \
  --disable-silent-rules \
  "${@}"
EOF
chmod a+x my_configure;
# The RPM macro for the linker flags does not exist on EPEL
%{!?__global_ldflags: %global __global_ldflags -Wl,-z,relro}
SHLIBGLOBALSFLAGS="%{__global_ldflags}"
LOCAL_LDFLAGS="%{__global_ldflags}"
CDEBUGFLAGS="%{?__global_cppflags} %{?__global_cflags} %{?optflags}"
IMAKE_DEFINES=''
FORCE_TIRPC='NO'
%if 0%{?fedora} > 27 || ( 0%{?suse_version} && ( 0%{?suse_version} > 1500 || 0%{?sle_version} > 150200 ) )
FORCE_TIRPC='YES'
%endif
IMAKE_DEFINES="-DUseTIRPC=${FORCE_TIRPC}"
make VERBOSE=1 CONFIGURE="$PWD/my_configure" PREFIX=%{_prefix} LIBDIR=%{_libdir} CDEBUGFLAGS="${CDEBUGFLAGS}" LOCAL_LDFLAGS="${LOCAL_LDFLAGS}" SHLIBGLOBALSFLAGS="${SHLIBGLOBALSFLAGS}" IMAKE_DEFINES="${IMAKE_DEFINES}"

%install
make install \
        DESTDIR=%{buildroot} \
        PREFIX=%{_prefix} \
        LIBDIR=%{_libdir} SHLIBDIR=%{_libdir} \
        INSTALL_DIR="install -dm0755" \
        INSTALL_FILE="install -pm0644" \
        INSTALL_PROGRAM="install -pm0755"

# this needs to be adapted distribution-wise...
%if 0%{?suse_version}
ln -s ../fonts %{buildroot}%{_datadir}/nx/fonts
%else
%if 0%{?fedora} || 0%{?rhel}
ln -s ../X11/fonts %{buildroot}%{_datadir}/nx/fonts
%endif
%endif

# Remove static libs (they don't exist on SLES, so using -f here)
rm -f %{buildroot}%{_libdir}/*.a

# Fix permissions on shared libraries
chmod 755  %{buildroot}%{_libdir}/lib*.so*

#Remove extras, GL, and other unneeded headers
rm -r %{buildroot}%{_includedir}/GL
rm -r %{buildroot}%{_includedir}/nx-X11/extensions/XK*.h
rm -r %{buildroot}%{_includedir}/nx-X11/extensions/*Xv*.h
rm -r %{buildroot}%{_includedir}/nx-X11/extensions/XRes*.h
rm -r %{buildroot}%{_includedir}/nx-X11/extensions/XIproto.h
rm -r %{buildroot}%{_includedir}/nx-X11/extensions/XI.h
rm -r %{buildroot}%{_includedir}/nx-X11/Xtrans

#Remove our shared libraries' .la files before wrapping up the packages
rm %{buildroot}%{_libdir}/*.la

# Fix python scripts
%if 0%{?suse_version}
%if 0%{?suse_version} >= 1500
sed -i -e '1c\
#!%{_bindir}/python3' '%{buildroot}%{_bindir}/nxdialog'
%else
sed -i -e '1c\
#!%{_bindir}/python2' '%{buildroot}%{_bindir}/nxdialog'
%endif
%else
# Unfortunately, pathfix.py is not available everywhere, so where it is not,
# we replace it in a less sophisticated way.
%if 0%{?fedora} > 22 || 0%{?rhel} > 7
%if 0%{?fedora} > 26 || 0%{?rhel} > 7
pathfix.py -pni "%{__python3} %{py3_shbang_opts}" '%{buildroot}%{_bindir}/nxdialog'
%else
sed -i -e '1c\
#!%{__python3}' '%{buildroot}%{_bindir}/nxdialog'
%endif
%else
%if 0%{?rhel} > 6
pathfix.py -pni "%{__python2} %{py2_shbang_opts}" '%{buildroot}%{_bindir}/nxdialog'
%else
sed -i -e '1c\
#!/usr/bin/python2' '%{buildroot}%{_bindir}/nxdialog'
%endif
%endif
%endif

%if 0%{?fdupes:1}
%fdupes %{buildroot}%{_prefix}
%endif

%post -p /sbin/ldconfig
%post -n libNX_X11-6 -p /sbin/ldconfig
%post -n libXcomp3 -p /sbin/ldconfig
%post -n libXcompshad3 -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%postun -n libNX_X11-6 -p /sbin/ldconfig
%postun -n libXcomp3 -p /sbin/ldconfig
%postun -n libXcompshad3 -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING
%dir %{_libdir}/nx
%dir %{_datadir}/nx
%dir %{_datadir}/nx/X11
%{_datadir}/nx/SecurityPolicy
%{_datadir}/nx/X11/XErrorDB
%{_datadir}/nx/X11/Xcms.txt

%files -n libNX_X11-6
%defattr(-,root,root)
%{_libdir}/libNX_X11.so.6*

%files -n libNX_X11-devel
%defattr(-,root,root)
%{_libdir}/libNX_X11.so
%{_libdir}/pkgconfig/nx-x11.pc
%dir %{_includedir}/nx
%dir %{_includedir}/nx-X11
%{_includedir}/nx-X11/ImUtil.h
%{_includedir}/nx-X11/Xauth.h
%{_includedir}/nx-X11/XKBlib.h
%{_includedir}/nx-X11/Xcms.h
%{_includedir}/nx-X11/Xlib.h
%{_includedir}/nx-X11/XlibConf.h
%{_includedir}/nx-X11/Xlibint.h
%{_includedir}/nx-X11/Xlocale.h
%{_includedir}/nx-X11/Xregion.h
%{_includedir}/nx-X11/Xresource.h
%{_includedir}/nx-X11/Xutil.h
%{_includedir}/nx-X11/cursorfont.h

%files -n libXcomp-devel
%defattr(-,root,root)
%{_libdir}/libXcomp.so
%{_includedir}/nx/MD5.h
%{_includedir}/nx/NX.h
%{_includedir}/nx/NXalert.h
%{_includedir}/nx/NXpack.h
%{_includedir}/nx/NXproto.h
%{_includedir}/nx/NXvars.h
%{_libdir}/pkgconfig/nxcomp.pc

%files -n libXcomp3
%defattr(-,root,root)
%doc COPYING
%doc doc/nxcomp/README.on-retroactive-DXPC-license
%doc doc/nxcomp/nxcomp-3.6-drops-compat-code-3.4.x-testing.pdf
%{_libdir}/libXcomp.so.3*

%files -n libXcompshad-devel
%defattr(-,root,root)
%{_libdir}/libXcompshad.so
%{_includedir}/nx/Shadow.h
%{_libdir}/pkgconfig/nxcompshad.pc

%files -n libXcompshad3
%defattr(-,root,root)
%doc COPYING
%{_libdir}/libXcompshad.so.3*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/nx-X11/extensions
%{_includedir}/nx-X11/extensions/panoramiXext.h
%{_includedir}/nx-X11/misc.h
%{_includedir}/nx-X11/os.h

%files -n nx-proto-devel
%defattr(-,root,root)
%dir %{_includedir}/nx-X11
%{_includedir}/nx-X11/DECkeysym.h
%{_includedir}/nx-X11/HPkeysym.h
%{_includedir}/nx-X11/Sunkeysym.h
%{_includedir}/nx-X11/X.h
%{_includedir}/nx-X11/XF86keysym.h
%{_includedir}/nx-X11/Xarch.h
%{_includedir}/nx-X11/Xatom.h
%{_includedir}/nx-X11/Xdefs.h
%{_includedir}/nx-X11/Xfuncproto.h
%{_includedir}/nx-X11/Xfuncs.h
%{_includedir}/nx-X11/Xmd.h
%{_includedir}/nx-X11/Xos.h
%{_includedir}/nx-X11/Xos_r.h
%{_includedir}/nx-X11/Xosdefs.h
%{_includedir}/nx-X11/Xpoll.h
%{_includedir}/nx-X11/Xproto.h
%{_includedir}/nx-X11/Xprotostr.h
%{_includedir}/nx-X11/Xthreads.h
%{_includedir}/nx-X11/keysym.h
%{_includedir}/nx-X11/keysymdef.h
%{_includedir}/nx-X11/extensions/Xdbeproto.h
%{_includedir}/nx-X11/extensions/bigreqstr.h
%{_includedir}/nx-X11/extensions/composite.h
%{_includedir}/nx-X11/extensions/compositeproto.h
%{_includedir}/nx-X11/extensions/damagewire.h
%{_includedir}/nx-X11/extensions/damageproto.h
%{_includedir}/nx-X11/extensions/dpms.h
%{_includedir}/nx-X11/extensions/dpmsstr.h
%{_includedir}/nx-X11/extensions/panoramiXproto.h
%{_includedir}/nx-X11/extensions/randr.h
%{_includedir}/nx-X11/extensions/randrproto.h
%{_includedir}/nx-X11/extensions/record*.h
%{_includedir}/nx-X11/extensions/render.h
%{_includedir}/nx-X11/extensions/renderproto.h
%{_includedir}/nx-X11/extensions/saver.h
%{_includedir}/nx-X11/extensions/saverproto.h
%{_includedir}/nx-X11/extensions/security.h
%{_includedir}/nx-X11/extensions/securstr.h
%{_includedir}/nx-X11/extensions/shapeconst.h
%{_includedir}/nx-X11/extensions/sync.h
%{_includedir}/nx-X11/extensions/syncstr.h
%{_includedir}/nx-X11/extensions/xcmiscstr.h
%{_includedir}/nx-X11/extensions/xf86bigfont.h
%{_includedir}/nx-X11/extensions/xf86bigfproto.h
%{_includedir}/nx-X11/extensions/xfixesproto.h
%{_includedir}/nx-X11/extensions/xfixeswire.h
%{_includedir}/nx-X11/extensions/xtestconst.h
%{_includedir}/nx-X11/extensions/xteststr.h

%files -n nxagent
%defattr(-,root,root)
%dir %{_sysconfdir}/nxagent
%config(noreplace) %{_sysconfdir}/nxagent/keystrokes.cfg
%doc doc/nxagent/README.keystrokes
%{_bindir}/nxagent
# FIXME: compatibility symlink to BINDIR/nxagent, remove for 3.6.0
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxagent
%dir %{_libdir}/nx/X11
%{_libdir}/nx/X11/libX11.so*
%dir %{_datadir}/nx
%{_datadir}/nx/VERSION.nxagent
%{_datadir}/man/man1/nxagent.1*
%{_datadir}/nx/fonts

%files -n nxproxy
%defattr(-,root,root)
%doc doc/nxproxy/README-VALGRIND
%{_bindir}/nxproxy
%{_datadir}/man/man1/nxproxy.1*
%dir %{_datadir}/nx
%{_datadir}/nx/VERSION.nxproxy

%files -n nxdialog
%defattr(-,root,root)
%doc nxdialog/README.md
%{_bindir}/nxdialog
%{_datadir}/man/man1/nxdialog.1*

%changelog
* Sun May 31 2020 Mike Gabriel <mike.gabriel@das-netzwerkteam.de> 3.5.99.27
- See upstream ChangeLog and debian/changelog for details.
