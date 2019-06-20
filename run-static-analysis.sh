#!/bin/bash

if [[ "${STATIC_ANALYSIS}" == "yes" ]]; then
    # cppcheck
    if ! [ -x "$(command -v cppcheck)" ]; then
        echo 'Error: cppcheck is not installed.' >&2
        exit 1
    fi
    CPPCHECK_OPTS='--error-exitcode=0 --force --quiet --suppressions-list=./static-analysis-suppressions'
    # we exclude some external projects
    CPPCHECK_EXCLUDES='-i ./nx-X11/extras/ -i nx-X11/programs/Xserver/GL/mesa* -i ./.pc -i ./nx-X11/.build-exports -i ./nx-X11/exports -i ./doc'
    echo "$(cppcheck --version):";
    cppcheck $CPPCHECK_OPTS $CPPCHECK_EXCLUDES .;
fi
