# NX development by ArticaProject, X2Go and TheQVD

This source tree started as a re-distribution of those NX packages needed
to setup FreeNX and/or X2Go on a Linux server.

In the past, the NX re-distribution (3.5.0.x) had been maintained by the X2Go Project:
http://wiki.x2go.org

In 2014, [the QVD project](http://theqvd.com) run by the company Qindel
joined the group of people being interested in NX maintenance and
improvement.

Since 2015, the Arctica Project has joined in the NX development. The core devs
of X2Go, Arctica and TheQVD have agreed on stopping to redistribute NX and to
continue the development of NX 3.x as the new upstream instead. The package
will see a slight name change to **nx-libs** starting with version 3.5.99.0.

Our intentions for nx-libs are:

* provide _one_ tarball that builds NX projects via a common Makefile
* provide _one_ tarball for distribution packagers
* provide support for security issues
* provide support for latest X11 extensions
* improve NX where possible

This source tree is maintained on Github:

  https://github.com/ArcticaProject/nx-libs (3.6.x branch)

For the the post-NoMachine era of nx-libs, we will focus on two release
phases for the upcoming two years (06/2015 - 06/2017).

## Release series 3.6.0.x

Scheduled for end of Q2/2016.

Release goals (phase 1) for nx-libs release series 3.6.0.x:

* CVE security audit (complete)
* remove unused code (+/- complete)
* no bundled non-X11 libraries anymore (complete)
* complete imake cleanup (+/- complete)
* replace as many libNX_X* libraries by X.org's libX* libraries
  (complete, only remaining library: libNX_X11)
* support for iOS (nxproxy, complete)
* Unix file socket communication for nxproxy -C <-> nxproxy -S connections
  (complete)
* allow Unix file sockets as channel endpoints (complete)
* allow embedding of nxproxy into other windows (work pending)
* new RandR based Xinerama extension (+/- complete, more QA needed)
* Fix Xcomposite extension in Xserver (work pending)
* nxcomp protocol clean-up (complete)
* nxcomp logging clean-up (work pending)
* optimizing documentation: how to tune NX connections (work pending)


## Release series 3.7.0.x

Scheduled for end of Q2/2017.

Release goals (phase 2) for nx-libs release series 3.7.0.x (not branched-off, yet):

* rebase Xserver code against latest X.Org server (work in progress)
* event FIFO sockets for attaching external applications
  (todo, to be discussed)
* enable/support XV extension (todo)
* software cursor for shadow sessions (todo)
* no bundled Mesa library anymore (todo, to be discussed)
* use recent MesaGL (todo, to-be-discussed)


If you have any questions about this NX development or want to file a bug
report, please contact the Arctica developers, the X2Go developers or the
TheQVD developers via the project's Github issue tracker.

*Sincerely,*

*The nx-libs developers (a combined effort of ArcticaProject / TheQVD / X2Go)*

## Building Under Fedora or EPEL using Mock

Assuming:

1. The branch you are building is 3.6.x
2. The current version is 3.5.99.0 (specified in the .spec file)
3. The current release is 0.0build1 (specified in the .spec file)
4. You wish for the RPM files and the mock build logs to be under ~/result

Prerequisites:

1. Install package "mock"
2. Add your user account to the "mock" group (recommended)
3. cd to the nx-libs directory that you cloned using git

```
mkdir -p ~/result
git archive -o ../nx-libs-3.5.99.0.tar.gz --prefix=nx-libs-3.5.99.0/ 3.6.x
cp --preserve=time nx-libs.spec ../
cd ..
mock --buildsrpm --spec ./nx-libs.spec --sources ./nx-libs-3.5.99.0.tar.gz --resultdir ~/result
mock --rebuild ~/result/nx-libs-3.5.99.0-0.0build1.fc23.src.rpm --resultdir ~/result
```

The end result is RPMs under ~/result that you can install (or upgrade to) using yum or dnf, which will resolve their dependencies.

## Building for openSUSE using OBS Build

Assuming:

1. The branch you are building is 3.6.x
2. The current version is 3.5.99.0 (specified in the .spec file)
3. The current release is 0.0build1 (specified in the .spec file)
4. You wish for the RPM files and the obs-build logs to be under ~/rpmbuild

Prerequisites:

1. Install package "obs-build"
2. Make sure your user account can become root via sudo
3. cd to the nx-libs directory that you cloned using git

```
mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/RPMS ~/rpmbuild/SRPMS ~/rpmbuild/OTHER ~/rpmbuild/BUILD
git archive -o $HOME/rpmbuild/SOURCES/nx-libs-3.5.99.0.tar.gz --prefix=nx-libs-3.5.99.0/ 3.6.x
cp --preserve=time nx-libs.spec ~/rpmbuild/SOURCES
cd ..
sudo obs-build --clean --nosignature --repo http://download.opensuse.org/distribution/<OPENSUSE-VERSION>/repo/oss/suse/ --root /var/lib/obs-build/ ~/rpmbuild/SOURCES/nx-libs.spec
cp -ar /var/lib/obs-build/home/abuild/rpmbuild/RPMS/* ~/rpmbuild/RPMS/
cp -ar /var/lib/obs-build/home/abuild/rpmbuild/SRPMS/* ~/rpmbuild/SRPMS/
cp -ar /var/lib/obs-build/home/abuild/rpmbuild/OTHER/* ~/rpmbuild/OTHER/
cp -ar /var/lib/obs-build/.build.log ~/rpmbuild/BUILD/
```

The end result is RPMs under ~/result that you can install (or upgrade to) using yum or dnf, which will resolve their dependencies.

## Building Under Debian or Ubuntu using debuild

Assuming:

1. The current version is 3.5.99\* (specified in debian/changelog)

Prerequisites:

1. Install package "devscripts"
2. Install the build dependencies. `dpkg-checkbuilddeps` can help you identify them.

```
git clone <url> nx-libs
cd nx-libs
debuild -uc -us
cd ..
sudo dpkg -i *3.5.99*.deb
```

## Building on Windows

The only components that can be built on Windows at the time of writing are `nxcomp` and `nxproxy` (with the latter utilizing the former).

**The next section is only relevant for git-based source code builds. Released tarballs do not require special handling.**

Since this project makes use of UNIX-style symlinks, it is imperative to clone the repository using Cygwin's git binary. MSYS(2) git is not able to handle UNIX-style symlinks.
Make sure that all build utilities are Cygwin-based. Non-Cygwin binaries will bail out with errors during the build process or insert garbage.
