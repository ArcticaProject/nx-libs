/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2015 Qindel Formacion y Servicios SL.                    */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License Version 2, as     */
/* published by the Free Software Foundation.                             */
/*                                                                        */
/* This program is distributed in the hope that it will be useful, but    */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-  */
/* BILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General       */
/* Public License for more details.                                       */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, you can request a copy from Qindel    */
/* or write to the Free Software Foundation, Inc., 51 Franklin Street,    */
/* Fifth Floor, Boston, MA 02110-1301 USA.                                */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NX.h"


static int _NXVersionMajor = -1;
static int _NXVersionMinor = -1;
static int _NXVersionPatch = -1;
static int _NXVersionMaintenancePatch = -1;


const char* NXVersion() {
  const char *version = VERSION;
  return version;
}

void _parseNXVersion() {
  char version[32];
  int i;
  strcpy(version, VERSION);

  char *value;
  /* Reset values to 0 if undefined */
  _NXVersionMajor = _NXVersionMinor = _NXVersionPatch = _NXVersionMaintenancePatch = 0;


#define NXVERSIONSEPARATOR "."
  value = strtok(version, NXVERSIONSEPARATOR);

  for (i = 0; value != NULL && i < 4; i++)
  {
    switch (i)
    {
      case 0:
        _NXVersionMajor = atoi(value);
        break;

      case 1:
        _NXVersionMinor = atoi(value);
        break;

      case 2:
        _NXVersionPatch = atoi(value);
        break;

      case 3:
        _NXVersionMaintenancePatch = atoi(value);
        break;
    }

    value = strtok(NULL, NXVERSIONSEPARATOR);
  }
}

int NXMajorVersion() {
  if (_NXVersionMajor == -1)
    _parseNXVersion();
  return _NXVersionMajor;
}
int NXMinorVersion() {
  if (_NXVersionMinor == -1)
    _parseNXVersion();
  return _NXVersionMinor;
}
int NXPatchVersion() {
  if (_NXVersionPatch == -1)
    _parseNXVersion();
  return _NXVersionPatch;
}
int NXMaintenancePatchVersion() {
  if (_NXVersionMaintenancePatch == -1)
    _parseNXVersion();
  return _NXVersionMaintenancePatch;
}
