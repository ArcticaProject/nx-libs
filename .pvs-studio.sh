#!/bin/bash

before_install() {
  if [ "$PVS_ANALYZE" = "yes" ]; then
    sudo wget -q -O - https://files.viva64.com/etc/pubkey.txt | sudo apt-key add -
    sudo wget -O /etc/apt/sources.list.d/viva64.list https://files.viva64.com/etc/viva64.list
    sudo apt-get update -qq
    sudo apt-get install -qq pvs-studio
  else
    echo "not installing PVS-Studio"
  fi
}

build_script() {
  if [ "$PVS_ANALYZE" = "yes" ]; then
    pvs-studio-analyzer credentials "${PVS_USERNAME}" "${PVS_KEY}" -o PVS-Studio.lic
    pvs-studio-analyzer trace -- make -j2
    pvs-studio-analyzer analyze --quiet -j2 -l PVS-Studio.lic -o "PVS-Studio-${CC}.log"
    plog-converter -a "GA:1,2" -t tasklist -o "PVS-Studio-${CC}.tasks" "PVS-Studio-${CC}.log"
    cat "PVS-Studio-${CC}.tasks"
  else
    make -j2
  fi
}

set -e
set -x

$1;
