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
    if [[ -z "${PVS_USERNAME}" ]]; then
      echo '"PVS_USERNAME" environment variable not set'
      exit 0
    elif [[ -z "${PVS_KEY}" ]]; then
      echo '"PVS_KEY" environment variable not set'
      exit 0
    else
      pvs-studio-analyzer credentials -o "PVS-Studio.lic" "${PVS_USERNAME}" "${PVS_KEY}"
      pvs-studio-analyzer trace -- make -j2
      pvs-studio-analyzer analyze --quiet -j2 --lic-file "PVS-Studio.lic" --output-file "PVS-Studio-${CC}.log"
      plog-converter -a "GA:1,2" -t tasklist -o "PVS-Studio-${CC}.tasks" "PVS-Studio-${CC}.log"
      cat "PVS-Studio-${CC}.tasks"
    fi
  else
    make -j2
  fi
}

set -e
set -x

$1;
