#!/bin/bash

if [[ "${STATIC_ANALYSIS}" == "yes" ]]; then
    # cppcheck
    if ! [ -x "$(command -v cppcheck)" ]; then
        echo 'Error: cppcheck is not installed.' >&2
        exit 1
    fi
    CPPCHECK_OPTS='--error-exitcode=0 --force --quiet'
    # we exclude some external projects
    CPPCHECK_EXCLUDES='-i ./nx-X11/extras/Mesa* -i ./nx-X11/extras/Mesa_* -i nx-X11/programs/Xserver/GL/mesa*'
    echo "$(cppcheck --version):";
    cppcheck $CPPCHECK_OPTS $CPPCHECK_EXCLUDES .;
fi
