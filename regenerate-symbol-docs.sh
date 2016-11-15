#!/bin/bash


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
