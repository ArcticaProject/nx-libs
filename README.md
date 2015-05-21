NX development by ArticaProject, X2Go and TheQVD
------------------------------------------------

This source tree started as a re-distribution of those NX packages needed
to setup FreeNX and/or X2Go on a Linux server.

The NX re-distribution (3.5.0.x) has been maintained by the X2Go Project:
http://wiki.x2go.org

In 2014, http://theqvd.com run by the Company Qindel
joined the group of people being interested in NX maintenance and
improvement.

Since 2015, the Arctica Project has joined in the NX development. The
core devs of X2Go and Arctica have agreed on stopping to redistribute
NX and instead of that: continue the development of NX 3.x as new
upstream (starting with NX 3.5.99).

Our goal is: 

* provide _one_ tarball that builds NX projects via a common Makefile
* provide _one_ tarball for distribution packagers
* provide support for security issues
* provide support for latest X11 extensions
* improve NX where possible

This source tree is maintained on Github:

  https://github.com/ArcticaProject/nx-libs (3.6.x branch)

Release goals for the post-NoMachine nx-libs release series 3.6.x:

* CVE security audit (complete)
* remove unused code (+/- complete)
* no bundled libraries anymore (work in progress)
* complete imake cleanup (work in progress)
* replace as many liNX_X* libraries by X.org's libX* libraries
  (work in progress)
* socket communication for nxproxy -C <-> nxproxy -S connections
  (todo)
* event FIFO sockets for attaching external applications
  (todo)

If you have any questions about this NX development or want to file  a
bug, then please contact the Arctica or the X2Go developers via the
Github issue tracker.

thanks+light+love, 20150313
Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
