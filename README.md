# NX development by ArticaProject, X2Go and TheQVD

This source tree started as a re-distribution of those NX packages needed
to setup FreeNX and/or X2Go on a Linux server.

The NX re-distribution (3.5.0.x) has been maintained by the X2Go Project:
http://wiki.x2go.org

In 2014, TheQVD project run by the Spanish company "The Quindel Group"
joined the group of people being interested in NX maintenance and
improvement.

Since 2015, the Arctica Project has joined in the NX development. The
core devs of X2Go, Arctica and TheQVD have agreed on stopping to
redistribute NX and instead of that: continue the development of NX 3.x
as new upstream and switching the name slightly to *nx-libs*
(starting with nx-libs v3.5.99.0).

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
* replace as many liNX_X* libraries by X.org's libX* libraries
  (work in progress)
* support for iOS (nxproxy)

## Release series 3.6.1.x

Scheduled for Q1/2016.

Release goals (phase 2) for nx-libs release series 3.6.1.x:

* provide support for latest X11 extensions
* socket communication for nxproxy -C <-> nxproxy -S connections
  (todo)
* event FIFO sockets for attaching external applications
  (todo)
* allow embedding of nxproxy into other windows
* support for multimedia

If you have any questions about this NX development or want to file  a
bug, then please contact the Arctica developers, the X2Go developers or
the TheQVD developers the project's Github issue tracker.

thanks+light+love, 20150515
Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
