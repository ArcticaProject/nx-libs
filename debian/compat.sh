#!/bin/bash

# This script employs compatibility measures, mainly for older Debian
# or Ubuntu versions.
typeset -a compat_shims_all compat_shims_active
compat_shims_all=("Wpedantic")
compat_shims_active=()

typeset debian_release_ver=''
typeset -i ubuntu_release_ver_major='0' ubuntu_release_ver_minor='0'

# Check distro version and enable compat shims if required.
if dpkg-vendor --is "Debian" || dpkg-vendor --is "Raspbian"; then
  debian_release_ver="$(lsb_release -r | sed -e 's/[	 ]*//g' | cut -d ':' -f '2' | cut -d '.' -f '1')"

  [[ "${debian_release_ver}" = 'testing' ]] && debian_release_ver='999'
  [[ "${debian_release_ver}" = 'unstable' ]] && debian_release_ver='9999'

  if [[ "${debian_release_ver}" -le '7' ]]; then
    compat_shims_active+=("Wpedantic")
  fi
elif dpkg-vendor --is "Ubuntu"; then
  ubuntu_release_ver_major="$(lsb_release -r | sed -e 's/[	 ]*//g' | cut -d ':' -f '2' | cut -d '.' -f '1')"
  ubuntu_release_ver_minor="$(lsb_release -r | sed -e 's/[	 ]*//g' | cut -d ':' -f '2' | cut -d '.' -f '2')"

  if [[ "${ubuntu_release_ver_major}" -le '13' ]]; then
    if [[ "${ubuntu_release_ver_major}" -lt '13' ]] || [[ "${ubuntu_release_ver_minor}" -le '4' ]]; then
      compat_shims_active+=("Wpedantic")
    fi
  fi
fi


# Apply enabled compat shims.
# Ignore unknown values.
typeset cur_compat_shim='' cur_enabled_compat_shim=''
for cur_compat_shim in "${compat_shims_all[@]}"; do
  for cur_enabled_compat_shim in "${compat_shims_active[@]}"; do
    if [[ "${cur_compat_shim}" = "${cur_enabled_compat_shim}" ]]; then
      if [[ "${cur_compat_shim}" = 'Wpedantic' ]]; then
        sed -i -e 's/Wpedantic/pedantic/g' nx-X11/config/cf/{{host,xorgsite}.def,xorg.cf}
        continue
      fi
    fi
  done
done
