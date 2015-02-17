#!/bin/sh

# Copyright (C) 2015 Mihai Moldovan <ionic@ionic.de>
# Copyright (C) 2015 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

COMPONENT="$1"
VERSION_FILE="VERSION"

case "${COMPONENT}" in
  (1|2|3|4) :;;
  (*) echo "usage: $(basename ${0}) <position-in-version-number>" >&2; exit 1;;
esac

# More than one line is not supported.
VER="$(head -n "1" "${VERSION_FILE}" | cut -d"." -f"${COMPONENT}")"

[ "x${VER}" = "x" ] && VER="0"

printf "${VER}"
