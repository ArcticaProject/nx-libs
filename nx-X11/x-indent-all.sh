#!/bin/sh
where=`dirname $0`
git ls-files | grep '\.[chm]$' | xargs sh "$where"/x-indent.sh

