#!/bin/sh

# create copies of upstream changelogs so that names apply to Debian policy...
cp -a nx-X11/CHANGELOG nx-X11/changelog
cp -a nxcomp/CHANGELOG nxcomp/changelog
cp -a nxcompext/CHANGELOG nxcompext/changelog
cp -a nxcompshad/CHANGELOG nxcompshad/changelog
cp -a nx-X11/programs/Xserver/hw/nxagent/CHANGELOG nx-X11/programs/Xserver/hw/nxagent/changelog
cp -a nx-X11/programs/nxauth/CHANGELOG nx-X11/programs/nxauth/changelog
cp -a nxproxy/CHANGELOG nxproxy/changelog

exit 0
