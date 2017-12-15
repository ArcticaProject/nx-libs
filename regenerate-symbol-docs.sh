#!/bin/bash

# Copyright (C) 2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
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

# libNX_X11

SYMBOLS_FILE="doc/libNX_X11/symbols/libNX_X11::symbols.txt"

DOC_FILE="doc/libNX_X11/symbols/libNX_X11::symbol-usage_internally.txt"
echo "Scanning for libNX_X11 symbols: in libNX_X11 internally: $DOC_FILE"
cd nx-X11/lib/X11/
cat "../../../$SYMBOLS_FILE"  | grep -v -E "^#" | while read symbol; do

	echo
	echo "#### $symbol ####"
	grep -n $symbol *.{c,h} 2>/dev/null

done > "../../../$DOC_FILE"
cd - 1>/dev/null

DOC_FILE="doc/libNX_X11/symbols/libNX_X11::symbol-usage_nxagent.txt"
echo "Scanning for libNX_X11 symbols: in hw/nxagent: $DOC_FILE"
cd nx-X11/programs/Xserver/hw/nxagent/
cat "../../../../../$SYMBOLS_FILE"  | grep -v -E "^#" | while read symbol; do

	echo
	echo "#### $symbol ####"
	grep -n $symbol *.{c,h} 2>/dev/null

done > "../../../../../$DOC_FILE"
cd - 1>/dev/null

DOC_FILE="doc/libNX_X11/symbols/libNX_X11::symbol-usage_nxcompext.txt"
echo "Scanning for libNX_X11 symbols: in hw/nxagent/compext: $DOC_FILE"
cd nx-X11/programs/Xserver/hw/nxagent/compext/
cat "../../../../../../$SYMBOLS_FILE" | grep -v -E "^#" | while read symbol; do

	echo
	echo "#### $symbol ####"
	grep -n $symbol *.{c,h} 2>/dev/null

done > "../../../../../../$DOC_FILE"
cd - 1>/dev/null


# nxcompext

SYMBOLS_FILE="doc/nxcompext/symbols/nxcompext::symbols.txt"

DOC_FILE="doc/nxcompext/symbols/nxcompext::symbol-usage_internally.txt"
echo "Scanning for nxcompext symbols: in hw/nxagent/compext internally: $DOC_FILE"
cd nx-X11/programs/Xserver/hw/nxagent/compext/
cat "../../../../../../$SYMBOLS_FILE" | grep -v -E "^#" | while read symbol; do

	echo
	echo "#### $symbol ####"
	grep -n $symbol *.{c,h} 2>/dev/null

done > "../../../../../../$DOC_FILE"
cd - 1>/dev/null

DOC_FILE="doc/nxcompext/symbols/nxcompext::symbol-usage_nxagent.txt"
echo "Scanning for nxcompext symbols: in hw/nxagent: $DOC_FILE"
cd nx-X11/programs/Xserver/hw/nxagent/
cat ../../../../../$SYMBOLS_FILE  | grep -v -E "^#" | while read symbol; do

	echo
	echo "#### $symbol ####"
	grep -n $symbol *.{c,h} 2>/dev/null

done > "../../../../../$DOC_FILE"
cd - 1>/dev/null
