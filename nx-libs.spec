%global _hardened_build 1

Name:           nx-libs
Version:        3.5.0.22
Release:        0.0x2go1%{?dist}
Summary:        NX X11 protocol compression libraries

Group:          System Environment/Libraries
License:        GPLv2+
URL:            http://x2go.org/
#Source0:        http://code.x2go.org/releases/source/%{name}/%{name}-%{version}-full.tar.gz
Source0:        %{name}-%{version}.tar.gz
# git clone git://code.x2go.org/nx-libs
# cd nx-libs
# debian/roll-tarballs.sh HEAD server
# mv _releases_/source/nx-libs/nx-libs-HEAD-full.tar.gz .
#Source0:       ns-libs-HEAD-full.tar.gz
# Remove bundled libraries
#Patch0:         nx-libs-bundled.patch

BuildRequires:  autoconf
BuildRequires:  expat-devel
BuildRequires:  fontconfig-devel
BuildRequires:  freetype-devel
BuildRequires:  libfontenc-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libpng-devel
BuildRequires:  libxml2-devel
BuildRequires:  zlib-devel

Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}

# For compatibility with EPEL5
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

This package provides the core nx-X11 libraries customized for
nxagent/x2goagent.


%package -n libNX_X11
Group:          System Environment/Libraries
Summary:        Core NX protocol client library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_X11
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
Requires:       libNX_X11%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_X11-devel
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


%package -n libNX_Xau-devel
Group:          Development/Libraries
Summary:        Development files for the NX authorization protocol library
Requires:       libNX_Xau%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xau-devel
libXau provides mechanisms for individual access to an X Window
System display. It uses existing core protocol and library hooks for
specifying authorization data in the connection setup block to
restrict use of the display to only those clients that show that they
know a server-specific key called a "magic cookie".

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xau
Group:          System Environment/Libraries
Summary:        NX authorization protocol library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xau
libXau provides mechanisms for individual access to an X Window
System display. It uses existing core protocol and library hooks for
specifying authorization data in the connection setup block to
restrict use of the display to only those clients that show that they
know a server-specific key called a "magic cookie".


%package -n libNX_Xcomposite
Group:          System Environment/Libraries
Summary:        NX protocol Composite extension client library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xcomposite
The Composite extension causes a entire sub-tree of the window
hierarchy to be rendered to an off-screen buffer. Applications can
then take the contents of that buffer and do whatever they like. The
off-screen buffer can be automatically merged into the parent window
or merged by external programs, called compositing managers.


%package -n libNX_Xdamage
Group:          System Environment/Libraries
Summary:        NX Damage Extension library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xdamage
The X Damage Extension allows applications to track modified regions
of drawables.


%package -n libNX_Xdmcp-devel
Group:          Development/Libraries
Summary:        Development files for the NXDM Control Protocol library
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       libNX_Xdmcp%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xdmcp-devel
The X Display Manager Control Protocol (XDMCP) provides a uniform
mechanism for an autonomous display to request login service from a
remote host. By autonomous, we mean the display consists of hardware
and processes that are independent of any particular host where login
service is desired. An X terminal (screen, keyboard, mouse,
processor, network interface) is a prime example of an autonomous
display.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xdmcp
Group:          System Environment/Libraries
Summary:        NX Display Manager Control Protocol library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xdmcp
The X Display Manager Control Protocol (XDMCP) provides a uniform
mechanism for an autonomous display to request login service from a
remote host. By autonomous, we mean the display consists of hardware
and processes that are independent of any particular host where login
service is desired. An X terminal (screen, keyboard, mouse,
processor, network interface) is a prime example of an autonomous
display.


%package -n libNX_Xext-devel
Group:          Development/Libraries
Summary:        Development files for the NX Common Extensions library
Requires:       libNX_Xext%{?_isa} = %{version}-%{release}
Requires:       libNX_Xau-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xext-devel
The Xext library contains a handful of X11 extensions:
- Double Buffer extension (DBE/Xdbe)
- Display Power Management Signaling (DPMS) extension
- X11 Nonrectangular Window Shape extension (Xshape)
- The MIT Shared Memory extension (MIT-SHM/Xshm)
- TOG-CUP (colormap) protocol extension (Xcup)
- X Extended Visual Information extension (XEvi)
- X11 Double-Buffering, Multi-Buffering, and Stereo extension (Xmbuf)

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xext
Group:          System Environment/Libraries
Summary:        Common extensions to the NX protocol
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xext
The Xext library contains a handful of X11 extensions:
- Double Buffer extension (DBE/Xdbe)
- Display Power Management Signaling (DPMS) extension
- X11 Nonrectangular Window Shape extension (Xshape)
- The MIT Shared Memory extension (MIT-SHM/Xshm)
- TOG-CUP (colormap) protocol extension (Xcup)
- X Extended Visual Information extension (XEvi)
- X11 Double-Buffering, Multi-Buffering, and Stereo extension (Xmbuf)


%package -n libNX_Xfixes-devel
Group:          Development/Libraries
Summary:        Development files for the NX Xfixes extension library
Requires:       libNX_Xfixes%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xfixes-devel
The X Fixes extension provides applications with work-arounds for
various limitations in the core protocol.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xfixes
Group:          System Environment/Libraries
Summary:        NX miscellaneous "fixes" extension library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xfixes
The X Fixes extension provides applications with work-arounds for
various limitations in the core protocol.


%package -n libNX_Xinerama
Group:          System Environment/Libraries
Summary:        Xinerama extension to the NX Protocol
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       libX11
Requires:       libXext

%description -n libNX_Xinerama
Xinerama is an extension to the X Window System which enables
multi-headed X applications and window managers to use two or more
physical displays as one large virtual display.


%package -n libNX_Xpm-devel
Group:          Development/Libraries
Summary:        Development files for the NX Pixmap image file format library
Requires:       libNX_Xpm%{?_isa} = %{version}-%{release}

%description -n libNX_Xpm-devel
libXpm facilitates working with XPM (X PixMap), a format for
storing/retrieving X pixmaps to/from files.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xpm
Group:          System Environment/Libraries
Summary:        NX Pixmap image file format library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xpm
libXpm facilitates working with XPM (X PixMap), a format for
storing/retrieving X pixmaps to/from files.


%package -n libNX_Xrandr
Group:          System Environment/Libraries
Summary:        NX Resize, Rotate and Reflection extension library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xrandr
The X Resize, Rotate and Reflect Extension (RandR) allows clients to
dynamically change X screens, so as to resize, to change the
orientation and layout of the root window of a screen.


%package -n libNX_Xrender-devel
Group:          Development/Libraries
Summary:        Development files for the NX Render Extension library
Requires:       libNX_Xrender%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xrender-devel
The Xrender library is designed as a lightweight library interface to
the Render extension.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xrender
Group:          System Environment/Libraries
Summary:        NX Rendering Extension library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xrender
The Xrender library is designed as a lightweight library interface to
the Render extension.


%package -n libNX_Xtst
Group:          System Environment/Libraries
Summary:        Xlib-based client API for the XTEST and RECORD extensions on NX
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libNX_Xtst
The XTEST extension is a minimal set of client and server extensions
required to completely test the X11 server with no user intervention.
This extension is not intended to support general journaling and
playback of user actions.

The RECORD extension supports the recording and reporting of all core
X protocol and arbitrary X extension protocol.


%package -n libXcomp-devel
Group:          Development/Libraries
Summary:        Development files for the NX differential compression library
Requires:       libXcomp%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel = %{version}-%{release}

%description -n libXcomp-devel
The NX differential compression library's development files.


%package -n libXcomp
Group:          System Environment/Libraries
Summary:        NX differential compression library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libXcomp
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package contains the NX differential compression library for X11.


%package -n libXcompext-devel
Group:          Development/Libraries
Summary:        Development files for the NX compression extensions library
Requires:       libXcompext%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libXcompext-devel
The NX compression extensions library's development files.


%package -n libXcompext
Group:          System Environment/Libraries
Summary:        NX protocol compression extensions library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libXcompext
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package provides the library to support additional features to
the core NX library.


%package -n libXcompshad-devel
Group:          Development/Libraries
Summary:        Development files for the NX session shadowing library
Requires:       libXcompshad%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xext-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}
Requires:       %{name}-devel%{?_isa} = %{version}-%{release}

%description -n libXcompshad-devel
The NX session shadowing library's development files.


%package -n libXcompshad
Group:          System Environment/Libraries
Summary:        NX session shadowing Library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n libXcompshad
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package provides the session shadowing library.


%package devel
Group:          Development/Libraries
Summary:        Include files and libraries for NX development
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xau-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xdmcp-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xext-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xfixes-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xpm-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xrender-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains all necessary include files and libraries
needed to develop X11 applications that require these.


%package -n nx-proto-devel
Group:          Development/Libraries
Summary:        Include files for NX development

%description -n nx-proto-devel
This package contains all necessary include files and libraries
needed to develop X11 applications that require these.


%package -n nxagent
Group:          Applications/System
Summary:        NX Agent
Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}

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


%package -n nxauth
Group:          Applications/System
Summary:        NX Auth

%description -n nxauth
This package provides the NX xauth binary.


%package -n nxproxy
Group:          Applications/System
Summary:        NX Proxy
Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}

%description -n nxproxy
This package provides the NX proxy (client) binary.


%package -n x2goagent
Group:          Applications/System
Summary:        X2Go Agent
Requires:       nxagent

%description -n x2goagent
X2Go Agent functionality has been completely incorporated into
nxagent's code base. If the nxagent binary is executed under the name
of "x2goagent", the X2Go functionalities get activated.

The x2goagent package is a wrapper that activates X2Go branding in
nxagent. Please refer to the nxagent package's description for more
information on NX.


%prep
%setup -q
# copy files from the debian/ folder to designated places in the source tree,
# taken from roll-tarball.sh:
mkdir bin/
cp -v debian/wrappers/* bin/
mkdir etc/
cp -v debian/keystrokes.cfg etc/keystrokes.cfg
cp -v debian/Makefile.nx-libs Makefile
cp -v debian/Makefile.replace.sh replace.sh
cp -v debian/rgb rgb
cp -v debian/VERSION.x2goagent VERSION.x2goagent
# remove bundled libraries (also taken from roll-tarball.sh)
rm -Rf nx-X11/extras/{drm,expat,fontconfig,freetype2,fonts,ogl-sample,regex,rman,ttf2pt1,x86emu,zlib}
rm -Rf nx-X11/lib/{expat,fontconfig,fontenc,font/FreeType,font/include/fontenc.h,freetype2,regex,zlib}
rm -Rf nx-X11/lib/{FS,ICE,SM,Xaw,Xft,Xt,Xmu,Xmuu}
# remove build cruft that is in Git (also taken from roll-tarball.sh)
rm -Rf nx*/configure nx*/autom4te.cache*
# Install into /usr
sed -i -e 's,/usr/local,/usr,' nx-X11/config/cf/site.def
# Use rpm optflags
sed -i -e 's#-O3#%{optflags}#' nx-X11/config/cf/host.def
# Use multilib dirs
# We're installing binaries into %%{_libdir}/nx/bin rather than %%{_libexedir}/nx
# becuase upstream expects libraries and binaries in the same directory
sed -i -e 's,/lib/nx,/%{_lib}/nx,' Makefile nx-X11/config/cf/X11.tmpl
sed -i -e 's,/lib/x2go,/%{_lib}/x2go,' Makefile
sed -i -e 's,/usr/lib/,/usr/%{_lib}/,' bin/*
# Fix FSF address
find -name LICENSE | xargs sed -i \
  -e 's/59 Temple Place/51 Franklin Street/' -e 's/Suite 330/Fifth Floor/' \
  -e 's/MA  02111-1307/MA  02110-1301/'
# Fix source permissions
find -type f -name '*.[hc]' | xargs chmod -x

# Bundled nx-X11/extras
# Xpm - Is needed and needs to get linked to libXcomp
# Mesa - Used by the X server

# Xcursor - Other code still references files in it
# Xfont - Statically linked to nxarget, others?
# Xpm


%build
cat >"my_configure" <<'EOF'
%configure
EOF
chmod a+x my_configure;
# The RPM macro for the linker flags does not exist on EPEL
%{!?__global_ldflags: %global __global_ldflags -Wl,-z,relro}
export SHLIBGLOBALSFLAGS="%{__global_ldflags}"
make %{?_smp_mflags} CONFIGURE="$PWD/my_configure" USRLIBDIR=%{_libdir}/nx SHLIBDIR=%{_libdir}/nx

%install
make install \
        DESTDIR=%{buildroot} \
        PREFIX=%{_prefix} \
        USRLIBDIR=%{_libdir}/nx SHLIBDIR=%{_libdir}/nx \
        INSTALL_DIR="install -dm0755" \
        INSTALL_FILE="install -pm0644" \
        INSTALL_PROGRAM="install -pm0755"

# Remove static libs
rm %{buildroot}%{_libdir}/nx/*.a

# Make sure x2goagent is linked relative and on 64-bit
mkdir -p %{buildroot}%{_libdir}/x2go/bin
ln -sf ../../nx/bin/nxagent %{buildroot}%{_libdir}/x2go/bin/x2goagent

# Fix permissions on shared libraries
chmod 755  %{buildroot}%{_libdir}/nx/{,X11/}lib*.so*

# Linker
mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d/
echo %{_libdir}/nx > %{buildroot}%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf
echo %{_libdir}/nx/X11 >> %{buildroot}%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf

#Remove extras, GL, and other unneeded headers
rm -r %{buildroot}%{_includedir}/nx/{extras,GL}
rm -r %{buildroot}%{_includedir}/nx/X11/bitmaps
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/XInput.h
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/XK*.h
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/*Xv*.h
rm -r %{buildroot}%{_includedir}/nx/X11/Xtrans


%post -p /sbin/ldconfig
%post -n libNX_X11 -p /sbin/ldconfig
%post -n libNX_Xau -p /sbin/ldconfig
%post -n libNX_Xcomposite -p /sbin/ldconfig
%post -n libNX_Xdamage -p /sbin/ldconfig
%post -n libNX_Xdmcp -p /sbin/ldconfig
%post -n libNX_Xext -p /sbin/ldconfig
%post -n libNX_Xfixes -p /sbin/ldconfig
%post -n libNX_Xinerama
ln -s -f %{_libdir}/libX11.so.6 %{_libdir}/nx/X11/Xinerama/libNX_X11.so.6
ln -s -f %{_libdir}/libXext.so.6 %{_libdir}/nx/X11/Xinerama/libNX_Xext.so.6
/sbin/ldconfig

%post -n libNX_Xpm -p /sbin/ldconfig
%post -n libNX_Xrandr -p /sbin/ldconfig
%post -n libNX_Xrender -p /sbin/ldconfig
%post -n libNX_Xtst -p /sbin/ldconfig
%post -n libXcomp -p /sbin/ldconfig
%post -n libXcompext -p /sbin/ldconfig
%post -n libXcompshad -p /sbin/ldconfig

%preun -n libNX_Xinerama
rm -f %{_libdir}/nx/X11/Xinerama/libNX_X11.so.6
rm -f %{_libdir}/nx/X11/Xinerama/libNX_Xext.so.6

%postun -p /sbin/ldconfig
%postun -n libNX_X11 -p /sbin/ldconfig
%postun -n libNX_Xau -p /sbin/ldconfig
%postun -n libNX_Xcomposite -p /sbin/ldconfig
%postun -n libNX_Xdamage -p /sbin/ldconfig
%postun -n libNX_Xdmcp -p /sbin/ldconfig
%postun -n libNX_Xext -p /sbin/ldconfig
%postun -n libNX_Xfixes -p /sbin/ldconfig
%postun -n libNX_Xinerama -p /sbin/ldconfig
%postun -n libNX_Xpm -p /sbin/ldconfig
%postun -n libNX_Xrandr -p /sbin/ldconfig
%postun -n libNX_Xrender -p /sbin/ldconfig
%postun -n libNX_Xtst -p /sbin/ldconfig
%postun -n libXcomp -p /sbin/ldconfig
%postun -n libXcompext -p /sbin/ldconfig
%postun -n libXcompshad -p /sbin/ldconfig

%files
%doc nx-X11/{COPYING,LICENSE,README}
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf
%dir %{_libdir}/nx
%{_datadir}/nx/SecurityPolicy

%files -n libNX_X11
%{_libdir}/nx/X11/libNX_X11.so.6*

%files -n libNX_X11-devel
%{_libdir}/nx/X11/libNX_X11.so
%dir %{_includedir}/nx
%dir %{_includedir}/nx/X11
%{_includedir}/nx/X11/ImUtil.h
%{_includedir}/nx/X11/XKBlib.h
%{_includedir}/nx/X11/Xcms.h
%{_includedir}/nx/X11/Xlib.h
%{_includedir}/nx/X11/XlibConf.h
%{_includedir}/nx/X11/Xlibint.h
%{_includedir}/nx/X11/Xlocale.h
%{_includedir}/nx/X11/Xregion.h
%{_includedir}/nx/X11/Xresource.h
%{_includedir}/nx/X11/Xutil.h
%{_includedir}/nx/X11/cursorfont.h

%files -n libNX_Xau-devel
%{_libdir}/nx/X11/libNX_Xau.so
%{_includedir}/nx/X11/Xauth.h

%files -n libNX_Xau
%{_libdir}/nx/X11/libNX_Xau.so.6*

%files -n libNX_Xcomposite
%{_libdir}/nx/X11/libNX_Xcomposite.so.1*

%files -n libNX_Xdamage
%{_libdir}/nx/X11/libNX_Xdamage.so.1*

%files -n libNX_Xdmcp-devel
%{_libdir}/nx/X11/libNX_Xdmcp.so
%{_includedir}/nx/X11/Xdmcp.h

%files -n libNX_Xdmcp
%{_libdir}/nx/X11/libNX_Xdmcp.so.6*

%files -n libNX_Xext-devel
%{_libdir}/nx/X11/libNX_Xext.so
%dir %{_includedir}/nx/X11/extensions
%{_includedir}/nx/X11/extensions/MITMisc.h
%{_includedir}/nx/X11/extensions/XEVI.h
%{_includedir}/nx/X11/extensions/XEVIstr.h
%{_includedir}/nx/X11/extensions/XLbx.h
%{_includedir}/nx/X11/extensions/XShm.h
%{_includedir}/nx/X11/extensions/Xag.h
%{_includedir}/nx/X11/extensions/Xagsrv.h
%{_includedir}/nx/X11/extensions/Xagstr.h
%{_includedir}/nx/X11/extensions/Xcup.h
%{_includedir}/nx/X11/extensions/Xcupstr.h
%{_includedir}/nx/X11/extensions/Xdbe.h
%{_includedir}/nx/X11/extensions/Xdbeproto.h
%{_includedir}/nx/X11/extensions/Xext.h
%{_includedir}/nx/X11/extensions/dpms.h
%{_includedir}/nx/X11/extensions/dpmsstr.h
%{_includedir}/nx/X11/extensions/extutil.h
%{_includedir}/nx/X11/extensions/lbxstr.h
%{_includedir}/nx/X11/extensions/mitmiscstr.h
%{_includedir}/nx/X11/extensions/multibuf.h
%{_includedir}/nx/X11/extensions/multibufst.h
%{_includedir}/nx/X11/extensions/security.h
%{_includedir}/nx/X11/extensions/securstr.h
%{_includedir}/nx/X11/extensions/shape.h
%{_includedir}/nx/X11/extensions/sync.h
%{_includedir}/nx/X11/extensions/xtestext1.h
%{_includedir}/nx/X11/extensions/xteststr.h

%files -n libNX_Xext
%{_libdir}/nx/X11/libNX_Xext.so.6*

%files -n libNX_Xfixes-devel
%{_libdir}/nx/X11/libNX_Xfixes.so
%{_includedir}/nx/X11/extensions/Xfixes.h

%files -n libNX_Xfixes
%{_libdir}/nx/X11/libNX_Xfixes.so.3*

%files -n libNX_Xinerama
%{_libdir}/nx/X11/libNX_Xinerama.so.1*
%{_libdir}/nx/X11/Xinerama/

%files -n libNX_Xpm-devel
%{_libdir}/nx/X11/libNX_Xpm.so
%{_includedir}/nx/X11/xpm.h

%files -n libNX_Xpm
%{_libdir}/nx/X11/libNX_Xpm.so.4*

%files -n libNX_Xrandr
%{_libdir}/nx/X11/libNX_Xrandr.so.2*

%files -n libNX_Xrender-devel
%{_libdir}/nx/X11/libNX_Xrender.so
%{_includedir}/nx/X11/extensions/Xrender.h

%files -n libNX_Xrender
%{_libdir}/nx/X11/libNX_Xrender.so.1*

%files -n libNX_Xtst
%{_libdir}/nx/X11/libNX_Xtst.so.6*

%files -n libXcomp-devel
%_libdir/nx/libXcomp.so
%{_includedir}/nx/MD5.h
%{_includedir}/nx/NX.h
%{_includedir}/nx/NXalert.h
%{_includedir}/nx/NXmitshm.h
%{_includedir}/nx/NXpack.h
%{_includedir}/nx/NXproto.h
%{_includedir}/nx/NXrender.h
%{_includedir}/nx/NXvars.h

%files -n libXcomp
%doc nxcomp/{COPYING,LICENSE,README}
%_libdir/nx/libXcomp.so.3*

%files -n libXcompext-devel
%_libdir/nx/libXcompext.so
%{_includedir}/nx/NXlib.h
%{_includedir}/nx/NXlibint.h

%files -n libXcompext
%doc nxcompext/{COPYING,LICENSE,README}
%_libdir/nx/libXcompext.so.3*

%files -n libXcompshad-devel
%_libdir/nx/libXcompshad.so
%{_includedir}/nx/Core.h
%{_includedir}/nx/Input.h
%{_includedir}/nx/Logger.h
%{_includedir}/nx/Manager.h
%{_includedir}/nx/Misc.h
%{_includedir}/nx/Poller.h
%{_includedir}/nx/Regions.h
%{_includedir}/nx/Shadow.h
%{_includedir}/nx/Updater.h
%{_includedir}/nx/Win.h
%{_includedir}/nx/X11.h

%files -n libXcompshad
%doc nxcompshad/{CHANGELOG,COPYING,LICENSE}
%_libdir/nx/libXcompshad.so.3*

%files devel
%{_libdir}/nx/X11/libNX_Xcomposite.so
%{_libdir}/nx/X11/libNX_Xdamage.so
%{_libdir}/nx/X11/libNX_Xinerama.so
%{_libdir}/nx/X11/libNX_Xrandr.so
%{_libdir}/nx/X11/libNX_Xtst.so
%{_includedir}/nx/X11/X10.h
%dir %{_includedir}/nx/X11/extensions
%{_includedir}/nx/X11/extensions/XRes.h
%{_includedir}/nx/X11/extensions/XTest.h
%{_includedir}/nx/X11/extensions/Xcomposite.h
%{_includedir}/nx/X11/extensions/Xdamage.h
%{_includedir}/nx/X11/extensions/Xevie.h
%{_includedir}/nx/X11/extensions/Xinerama.h
%{_includedir}/nx/X11/extensions/Xrandr.h
%{_includedir}/nx/X11/extensions/dmxext.h
%{_includedir}/nx/X11/extensions/lbxbuf.h
%{_includedir}/nx/X11/extensions/lbxbufstr.h
%{_includedir}/nx/X11/extensions/lbxdeltastr.h
%{_includedir}/nx/X11/extensions/lbximage.h
%{_includedir}/nx/X11/extensions/lbxopts.h
%{_includedir}/nx/X11/extensions/lbxzlib.h
%{_includedir}/nx/X11/extensions/panoramiXext.h
%{_includedir}/nx/X11/extensions/record.h
%{_includedir}/nx/X11/extensions/xf86dga1.h
%{_includedir}/nx/X11/extensions/xf86vmode.h
%dir %{_includedir}/nx/X11/fonts
%{_includedir}/nx/X11/fonts/bdfint.h
%{_includedir}/nx/X11/fonts/bitmap.h
%{_includedir}/nx/X11/fonts/bufio.h
%{_includedir}/nx/X11/fonts/fntfil.h
%{_includedir}/nx/X11/fonts/fntfilio.h
%{_includedir}/nx/X11/fonts/fntfilst.h
%{_includedir}/nx/X11/fonts/fontencc.h
%{_includedir}/nx/X11/fonts/fontmisc.h
%{_includedir}/nx/X11/fonts/fontmod.h
%{_includedir}/nx/X11/fonts/fontshow.h
%{_includedir}/nx/X11/fonts/fontutil.h
%{_includedir}/nx/X11/fonts/fontxlfd.h
%{_includedir}/nx/X11/fonts/pcf.h
%{_includedir}/nx/X11/misc.h
%{_includedir}/nx/X11/os.h

%files -n nx-proto-devel
%dir %{_includedir}/nx/X11
%{_includedir}/nx/X11/DECkeysym.h
%{_includedir}/nx/X11/HPkeysym.h
%{_includedir}/nx/X11/Sunkeysym.h
%{_includedir}/nx/X11/X.h
%{_includedir}/nx/X11/XF86keysym.h
%{_includedir}/nx/X11/XWDFile.h
%{_includedir}/nx/X11/Xalloca.h
%{_includedir}/nx/X11/Xarch.h
%{_includedir}/nx/X11/Xatom.h
%{_includedir}/nx/X11/Xdefs.h
%{_includedir}/nx/X11/Xfuncproto.h
%{_includedir}/nx/X11/Xfuncs.h
%{_includedir}/nx/X11/Xmd.h
%{_includedir}/nx/X11/Xos.h
%{_includedir}/nx/X11/Xos_r.h
%{_includedir}/nx/X11/Xosdefs.h
%{_includedir}/nx/X11/Xpoll.h
%{_includedir}/nx/X11/Xproto.h
%{_includedir}/nx/X11/Xprotostr.h
%{_includedir}/nx/X11/Xthreads.h
%{_includedir}/nx/X11/ap_keysym.h
%{_includedir}/nx/X11/keysym.h
%{_includedir}/nx/X11/keysymdef.h
%{_includedir}/nx/X11/extensions/Print.h
%{_includedir}/nx/X11/extensions/Printstr.h
%{_includedir}/nx/X11/extensions/XI.h
%{_includedir}/nx/X11/extensions/XIproto.h
%{_includedir}/nx/X11/extensions/XResproto.h
%{_includedir}/nx/X11/extensions/Xeviestr.h
%{_includedir}/nx/X11/extensions/bigreqstr.h
%{_includedir}/nx/X11/extensions/composite.h
%{_includedir}/nx/X11/extensions/compositeproto.h
%{_includedir}/nx/X11/extensions/damageproto.h
%{_includedir}/nx/X11/extensions/damagewire.h
%{_includedir}/nx/X11/extensions/dmxproto.h
%{_includedir}/nx/X11/extensions/panoramiXproto.h
%{_includedir}/nx/X11/extensions/randr.h
%{_includedir}/nx/X11/extensions/randrproto.h
%{_includedir}/nx/X11/extensions/recordstr.h
%{_includedir}/nx/X11/extensions/render.h
%{_includedir}/nx/X11/extensions/renderproto.h
%{_includedir}/nx/X11/extensions/shapestr.h
%{_includedir}/nx/X11/extensions/shmstr.h
%{_includedir}/nx/X11/extensions/syncstr.h
%{_includedir}/nx/X11/extensions/xcmiscstr.h
%{_includedir}/nx/X11/extensions/xf86bigfont.h
%{_includedir}/nx/X11/extensions/xf86bigfstr.h
%{_includedir}/nx/X11/extensions/xf86dga.h
%{_includedir}/nx/X11/extensions/xf86dga1str.h
%{_includedir}/nx/X11/extensions/xf86dgastr.h
%{_includedir}/nx/X11/extensions/xf86misc.h
%{_includedir}/nx/X11/extensions/xf86mscstr.h
%{_includedir}/nx/X11/extensions/xf86vmstr.h
%{_includedir}/nx/X11/extensions/xfixesproto.h
%{_includedir}/nx/X11/extensions/xfixeswire.h
%{_includedir}/nx/X11/extensions/xtrapbits.h
%{_includedir}/nx/X11/extensions/xtrapddmi.h
%{_includedir}/nx/X11/extensions/xtrapdi.h
%{_includedir}/nx/X11/extensions/xtrapemacros.h
%{_includedir}/nx/X11/extensions/xtraplib.h
%{_includedir}/nx/X11/extensions/xtraplibp.h
%{_includedir}/nx/X11/extensions/xtrapproto.h
%dir %{_includedir}/nx/X11/fonts
%{_includedir}/nx/X11/fonts/FS.h
%{_includedir}/nx/X11/fonts/FSproto.h
%{_includedir}/nx/X11/fonts/font.h
%{_includedir}/nx/X11/fonts/fontstruct.h
%{_includedir}/nx/X11/fonts/fsmasks.h

%files -n nxagent
%dir %{_sysconfdir}/nxagent
%config(noreplace) %{_sysconfdir}/nxagent/keystrokes.cfg
%{_bindir}/nxagent
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxagent

%files -n nxauth
%{_bindir}/nxauth
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxauth

%files -n nxproxy
%{_bindir}/nxproxy
%{_mandir}/man1/nxproxy.1*

%files -n x2goagent
#%%{_sysconfdir}/x2go is owned by x2goserver, which this requires
%config(noreplace) %{_sysconfdir}/x2go/keystrokes.cfg
%{_bindir}/x2goagent
%{_libdir}/x2go/bin/x2goagent
%{_datadir}/pixmaps/x2go.xpm
%{_datadir}/x2go/


%changelog
