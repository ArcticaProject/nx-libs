%global _hardened_build 1

Name:           nx-libs
Version:        3.5.99.0
Release:        0.0build1%{?dist}
Summary:        NX X11 protocol compression libraries

Group:          System Environment/Libraries
%if 0%{?suse_version}
License:        GPL-2.0+
%else
License:        GPLv2+
%endif
URL:            http://x2go.org/
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  autoconf >= 2.13
BuildRequires:  gcc-c++
BuildRequires:  libjpeg-devel
BuildRequires:  pkgconfig

# suse_version 1315 is SLE-12
%if 0%{?suse_version} != 1315 && 0%{?suse_version} >= 1230
BuildRequires:  gpg-offline
%endif
%if 0%{?suse_version}
BuildRequires:  fdupes
%if 0%{?suse_version} >= 1130
BuildRequires:  pkgconfig(expat)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xpm)
BuildRequires:  pkgconfig(xfont)
BuildRequires:  pkgconfig(xdmcp)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(xrandr)
%else
BuildRequires:  libexpat-devel
BuildRequires:  libpng-devel
BuildRequires:  libxml2-devel
BuildRequires:  xorg-x11-libX11-devel
BuildRequires:  xorg-x11-libXext-devel
BuildRequires:  xorg-x11-libXpm-devel
BuildRequires:  xorg-x11-libXfont-devel
BuildRequires:  xorg-x11-libXdmcp-devel
BuildRequires:  xorg-x11-libXdamage-devel
BuildRequires:  xorg-x11-libXrandr-devel
%endif
BuildRequires:  xorg-x11-util-devel
%endif

%if 0%{?fedora} || 0%{?rhel}
BuildRequires:  expat-devel
BuildRequires:  libpng-devel
BuildRequires:  libxml2-devel
BuildRequires:  libXfont-devel
BuildRequires:  libXdmcp-devel
BuildRequires:  libXdamage-devel
BuildRequires:  libXrandr-devel
%endif

# For imake
BuildRequires:  xorg-x11-proto-devel
BuildRequires:  zlib-devel

%if 0%{?suse_version} >= 1130 || 0%{?fedora}
%define cond_noarch BuildArch: noarch
%else
%define cond_noarch %nil
%endif

Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}

# for Xinerama in NX to work:
%if 0%{?suse_version}
%if 0%{?suse_version} < 1140
Requires:       xorg-x11-libX11%{?_isa}
Requires:       xorg-x11-libXext%{?_isa}
%else
Requires:       libX11-6%{?_isa}
Requires:       libXext6%{?_isa}
%endif
%else
Requires:       libX11%{?_isa}
Requires:       libXext%{?_isa}
%endif

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
Obsoletes:      libNX_X11
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


%package -n libNX_Xau-devel
Group:          Development/Libraries
Summary:        Development files for the NX authorization protocol library
Requires:       libNX_Xau6%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xau-devel
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

libNX_Xau provides mechanisms for individual access to an nx-X11 Window
System display. It uses existing core protocol and library hooks for
specifying authorization data in the connection setup block to restrict
use of the display to only those clients that show that they know a
server-specific key called a "magic cookie".

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xau6
Group:          System Environment/Libraries
Summary:        NX authorization protocol library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xau

%description -n libNX_Xau6
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

libNX_Xau provides mechanisms for individual access to an X Window
System display. It uses existing core protocol and library hooks for
specifying authorization data in the connection setup block to
restrict use of the display to only those clients that show that they
know a server-specific key called a "magic cookie".


%package -n libNX_Xcomposite1
Group:          System Environment/Libraries
Summary:        NX protocol Composite extension client library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xcomposite

%description -n libNX_Xcomposite1
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The Composite extension causes a entire sub-tree of the window
hierarchy to be rendered to an off-screen buffer. Applications can
then take the contents of that buffer and do whatever they like. The
off-screen buffer can be automatically merged into the parent window
or merged by external programs, called compositing managers.

%package -n libNX_Xfixes-devel
Group:          Development/Libraries
Summary:        Development files for the NX Xfixes extension library
Requires:       libNX_Xfixes3%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xfixes-devel
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The nx-X11 Fixes extension provides applications with work-arounds for
various limitations in the core protocol.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xfixes3
Group:          System Environment/Libraries
Summary:        NX miscellaneous "fixes" extension library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xfixes

%description -n libNX_Xfixes3
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The nx_X11 Fixes extension provides applications with work-arounds for
various limitations in the core protocol.


%package -n libNX_Xinerama1
Group:          System Environment/Libraries
Summary:        Xinerama extension to the NX Protocol
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xinerama

%description -n libNX_Xinerama1
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

Xinerama is an extension to the X Window System which enables
multi-headed X applications and window managers to use two or more
physical displays as one large virtual display.


%package -n libNX_Xrender-devel
Group:          Development/Libraries
Summary:        Development files for the NX Render Extension library
Requires:       libNX_Xrender1%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libNX_Xrender-devel
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The Xrender library is designed as a lightweight library interface to
the Render extension.

This package contains all necessary include files and libraries
needed to develop applications that require these.


%package -n libNX_Xrender1
Group:          System Environment/Libraries
Summary:        NX Rendering Extension library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xrender

%description -n libNX_Xrender1
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The Xrender library is designed as a lightweight library interface to
the Render extension.


%package -n libNX_Xtst6
Group:          System Environment/Libraries
Summary:        Xlib-based client API for the XTEST and RECORD extensions on NX
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libNX_Xtst

%description -n libNX_Xtst6
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

The XTEST extension is a minimal set of client and server extensions
required to completely test the X11 server with no user intervention.
This extension is not intended to support general journaling and
playback of user actions.

The RECORD extension supports the recording and reporting of all core
X protocol and arbitrary X extension protocol.


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
Obsoletes:      libXcomp

%description -n libXcomp3
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package contains the NX differential compression library for X11.


%package -n libXcompext-devel
Group:          Development/Libraries
Summary:        Development files for the NX compression extensions library
Requires:       libXcompext3%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}

%description -n libXcompext-devel
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

The NX compression extensions library's development files.


%package -n libXcompext3
Group:          System Environment/Libraries
Summary:        NX protocol compression extensions library
Requires:       %{name}%{?_isa} >= 3.5.0.29
Obsoletes:      libXcompext

%description -n libXcompext3
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package provides the library to support additional features to
the core NX library.


%package -n libXcompshad-devel
Group:          Development/Libraries
Summary:        Development files for the NX session shadowing library
Requires:       libXcompshad3%{?_isa} = %{version}-%{release}
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       libXext-devel%{?_isa} = %{version}-%{release}
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
Obsoletes:      libXcompshad

%description -n libXcompshad3
NX is a software suite from NoMachine which implements very efficient
compression of the X11 protocol. This increases performance when
using X applications over a network, especially a slow one.

This package provides the session shadowing library.


%package devel
Group:          Development/Libraries
Summary:        Include files and libraries for NX development
Requires:       libNX_X11-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xau-devel%{?_isa} = %{version}-%{release}
Requires:       libXext-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xfixes-devel%{?_isa} = %{version}-%{release}
Requires:       libNX_Xrender-devel%{?_isa} = %{version}-%{release}
Requires:       nx-proto-devel%{?_isa} = %{version}-%{release}
Requires:       %{name}%{?_isa} = %{version}-%{release}

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
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}
%if 0%{?suse_version}
Requires:       xorg-x11-fonts-core
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


%package -n nxauth
Group:          Applications/System
Summary:        NX Auth

%description -n nxauth
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

This package provides the NX xauth binary.


%package -n nxproxy
Group:          Applications/System
Summary:        NX Proxy
Obsoletes:      nx < 3.5.0-19
Provides:       nx = %{version}-%{release}
Obsoletes:      nx%{?_isa} < 3.5.0-19
Provides:       nx%{?_isa} = %{version}-%{release}

%description -n nxproxy
NX is a software suite which implements very efficient compression of
the X11 protocol. This increases performance when using X
applications over a network, especially a slow one.

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
# remove build cruft that is in Git (also taken from roll-tarball.sh)
rm -Rf nx*/configure nx*/autom4te.cache*
# Install into /usr
sed -i -e 's,/usr/local,/usr,' nx-X11/config/cf/site.def
# Use rpm optflags
sed -i -e 's#-O3#%{optflags}#' nx-X11/config/cf/host.def
# Use multilib dirs
# We're installing binaries into %%{_libdir}/nx/bin rather than %%{_libexedir}/nx
# because upstream expects libraries and binaries in the same directory
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
# Mesa - Used by the X server

# Xcursor - Other code still references files in it


%build
cat >"my_configure" <<'EOF'
%configure
EOF
chmod a+x my_configure;
# The RPM macro for the linker flags does not exist on EPEL
%{!?__global_ldflags: %global __global_ldflags -Wl,-z,relro}
export SHLIBGLOBALSFLAGS="%{__global_ldflags}"
export LOCAL_LDFLAGS="%{__global_ldflags}"
make %{?_smp_mflags} CONFIGURE="$PWD/my_configure" PREFIX=%{_prefix} USRLIBDIR=%{_libdir} SHLIBDIR=%{_libdir}

%install
make install \
        DESTDIR=%{buildroot} \
        PREFIX=%{_prefix} \
        USRLIBDIR=%{_libdir} SHLIBDIR=%{_libdir} \
        INSTALL_DIR="install -dm0755" \
        INSTALL_FILE="install -pm0644" \
        INSTALL_PROGRAM="install -pm0755"

# Remove static libs (they don't exist on SLES, so using -f here)
rm -f %{buildroot}%{_libdir}/*.a

# Make sure x2goagent is linked relative and on 64-bit
mkdir -p %{buildroot}%{_libdir}/x2go/bin
ln -sf ../../nx/bin/nxagent %{buildroot}%{_libdir}/x2go/bin/x2goagent

# Fix permissions on shared libraries
chmod 755  %{buildroot}%{_libdir}/lib*.so*

#Remove extras, GL, and other unneeded headers
rm -r %{buildroot}%{_includedir}/nx/GL
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/XInput.h
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/XK*.h
rm -r %{buildroot}%{_includedir}/nx/X11/extensions/*Xv*.h
rm -r %{buildroot}%{_includedir}/nx/X11/Xtrans

# Needed for Xinerama support
ln -s -f ../../../../%{_lib}/libX11.so.6 %{buildroot}%{_libdir}/nx/X11/Xinerama/libNX_X11.so.6
ln -s -f ../../../../%{_lib}/libNX_Xinerama.so.1 %{buildroot}%{_libdir}/nx/X11/Xinerama/libXinerama.so.1

%if 0%{?fdupes:1}
%fdupes %buildroot/%_prefix
%endif

%post -p /sbin/ldconfig
%post -n libNX_X11-6 -p /sbin/ldconfig
%post -n libNX_Xau6 -p /sbin/ldconfig
%post -n libNX_Xcomposite1 -p /sbin/ldconfig
%post -n libNX_Xfixes3 -p /sbin/ldconfig
%post -n libNX_Xinerama1 -p /sbin/ldconfig
%post -n libNX_Xrender1 -p /sbin/ldconfig
%post -n libNX_Xtst6 -p /sbin/ldconfig
%post -n libXcomp3 -p /sbin/ldconfig
%post -n libXcompext3 -p /sbin/ldconfig
%post -n libXcompshad3 -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%postun -n libNX_X11-6 -p /sbin/ldconfig
%postun -n libNX_Xau6 -p /sbin/ldconfig
%postun -n libNX_Xcomposite1 -p /sbin/ldconfig
%postun -n libNX_Xfixes3 -p /sbin/ldconfig
%postun -n libNX_Xinerama1 -p /sbin/ldconfig
%postun -n libNX_Xrender1 -p /sbin/ldconfig
%postun -n libNX_Xtst6 -p /sbin/ldconfig
%postun -n libXcomp3 -p /sbin/ldconfig
%postun -n libXcompext3 -p /sbin/ldconfig
%postun -n libXcompshad3 -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING
%doc nx-X11/README
%dir %{_libdir}/nx
%dir %{_libdir}/nx/X11
%dir %{_datadir}/nx
%{_datadir}/nx/SecurityPolicy
%dir %{_libdir}/nx/X11/Xinerama/
%{_libdir}/nx/X11/Xinerama/libNX_X11.so.6
%{_libdir}/nx/X11/Xinerama/libXinerama.so.1*

%files -n libNX_X11-6
%defattr(-,root,root)
%{_libdir}/libNX_X11.so.6*

%files -n libNX_X11-devel
%defattr(-,root,root)
%{_libdir}/libNX_X11.so
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
%defattr(-,root,root)
%{_libdir}/libNX_Xau.so
%{_includedir}/nx/X11/Xauth.h

%files -n libNX_Xau6
%defattr(-,root,root)
%{_libdir}/libNX_Xau.so.6*

%files -n libNX_Xcomposite1
%defattr(-,root,root)
%{_libdir}/libNX_Xcomposite.so.1*

%files -n libNX_Xfixes-devel
%defattr(-,root,root)
%{_libdir}/libNX_Xfixes.so
%{_includedir}/nx/X11/extensions/Xfixes.h

%files -n libNX_Xfixes3
%defattr(-,root,root)
%{_libdir}/libNX_Xfixes.so.3*

%files -n libNX_Xinerama1
%defattr(-,root,root)
%{_libdir}/libNX_Xinerama.so.1*

%files -n libNX_Xrender-devel
%defattr(-,root,root)
%{_libdir}/libNX_Xrender.so
%{_includedir}/nx/X11/extensions/Xrender.h

%files -n libNX_Xrender1
%defattr(-,root,root)
%{_libdir}/libNX_Xrender.so.1*

%files -n libNX_Xtst6
%defattr(-,root,root)
%{_libdir}/libNX_Xtst.so.6*

%files -n libXcomp-devel
%defattr(-,root,root)
%_libdir/libXcomp.so
%{_includedir}/nx/MD5.h
%{_includedir}/nx/NX.h
%{_includedir}/nx/NXalert.h
%{_includedir}/nx/NXmitshm.h
%{_includedir}/nx/NXpack.h
%{_includedir}/nx/NXproto.h
%{_includedir}/nx/NXrender.h
%{_includedir}/nx/NXvars.h

%files -n libXcomp3
%defattr(-,root,root)
%doc COPYING
%doc nxcomp/README
%_libdir/libXcomp.so.3*

%files -n libXcompext-devel
%defattr(-,root,root)
%_libdir/libXcompext.so
%{_includedir}/nx/NXlib.h
%{_includedir}/nx/NXlibint.h

%files -n libXcompext3
%defattr(-,root,root)
%doc COPYING
%doc nxcompext/README
%_libdir/libXcompext.so.3*

%files -n libXcompshad-devel
%defattr(-,root,root)
%_libdir/libXcompshad.so
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

%files -n libXcompshad3
%defattr(-,root,root)
%doc COPYING
%_libdir/libXcompshad.so.3*

%files devel
%defattr(-,root,root)
%{_libdir}/libNX_Xcomposite.so
%{_libdir}/libNX_Xinerama.so
%{_libdir}/libNX_Xtst.so
%{_includedir}/nx/X11/X10.h
%dir %{_includedir}/nx/X11/extensions
%{_includedir}/nx/X11/extensions/XTest.h
%{_includedir}/nx/X11/extensions/Xcomposite.h
%{_includedir}/nx/X11/extensions/Xevie.h
%{_includedir}/nx/X11/extensions/Xinerama.h
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
%{_includedir}/nx/X11/misc.h
%{_includedir}/nx/X11/os.h

%files -n nx-proto-devel
%defattr(-,root,root)
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
%{_includedir}/nx/X11/extensions/XI.h
%{_includedir}/nx/X11/extensions/XIproto.h
%{_includedir}/nx/X11/extensions/XResproto.h
%{_includedir}/nx/X11/extensions/Xeviestr.h
%{_includedir}/nx/X11/extensions/bigreqstr.h
%{_includedir}/nx/X11/extensions/composite.h
%{_includedir}/nx/X11/extensions/compositeproto.h
%{_includedir}/nx/X11/extensions/panoramiXproto.h
%{_includedir}/nx/X11/extensions/recordstr.h
%{_includedir}/nx/X11/extensions/render.h
%{_includedir}/nx/X11/extensions/renderproto.h
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

%files -n nxagent
%defattr(-,root,root)
%dir %{_sysconfdir}/nxagent
%config(noreplace) %{_sysconfdir}/nxagent/keystrokes.cfg
%config(noreplace) %{_sysconfdir}/nxagent/nxagent.keyboard
%config(noreplace) %{_sysconfdir}/nxagent/rgb
%{_bindir}/nxagent
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxagent
%{_datadir}/pixmaps/nxagent.xpm
%{_datadir}/nx/rgb
%{_datadir}/man/man1/nxagent.1*

%files -n nxauth
%defattr(-,root,root)
%{_bindir}/nxauth
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxauth
%{_datadir}/man/man1/nxauth.1*

%files -n nxproxy
%defattr(-,root,root)
%{_bindir}/nxproxy
%{_mandir}/man1/nxproxy.1*
%{_datadir}/man/man1/nxproxy.1*
%dir %{_libdir}/nx/bin
%{_libdir}/nx/bin/nxproxy

%files -n x2goagent
%defattr(-,root,root)
#%%{_sysconfdir}/x2go is owned by x2goserver, which this requires
%dir %{_sysconfdir}/x2go
%dir %{_libdir}/x2go
%dir %{_libdir}/x2go/bin
%config(noreplace) %{_sysconfdir}/x2go/keystrokes.cfg
%config(noreplace) %{_sysconfdir}/x2go/x2goagent.keyboard
%config(noreplace) %{_sysconfdir}/x2go/rgb
%{_bindir}/x2goagent
%{_libdir}/x2go/bin/x2goagent
%{_datadir}/pixmaps/x2go.xpm
%{_datadir}/x2go/
%{_datadir}/man/man1/x2goagent.1*


%changelog
* Thu Jan 29 2015 Mike Gabriel <mike.gabriel@das-netzwerkteam.de> 3.5.99.0
- See upstream ChangeLog and debian/changelog for details.
