# NX development by ArticaProject, X2Go and TheQVD

This source tree started as a re-distribution of those NX packages needed
to setup FreeNX and/or X2Go on a Linux server.

The NX re-distribution (3.5.0.x) has been maintained by the X2Go Project:
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
phases for the upcoming year (06/2015 - 06/2016).

## Release series 3.6.0.x

Scheduled for Q3/2015.

Release goals (phase 1) for nx-libs release series 3.6.0.x:

* CVE security audit (complete)
* remove unused code (+/- complete)
* no bundled non-X11 libraries anymore (done)
* no bundled Mesa library anymore (todo)
* complete imake cleanup (work in progress)
* replace as many libNX_X* libraries by X.org's libX* libraries
  (work in progress)
* support for iOS (nxproxy, complete)
* Unix file socket communication for nxproxy -C <-> nxproxy -S connections
  (todo)
* allow Unix file sockets as channel endpoints (work in progress)
* allow embedding of nxproxy into other windows
 
## Release series 3.7.0.x

Scheduled for Q1/2016.

Release goals (phase 2) for nx-libs release series 3.7.0.x (not branched-off, yet):

* provide support for latest X11 extensions
* event FIFO sockets for attaching external applications
  (todo)
* support for multimedia

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
