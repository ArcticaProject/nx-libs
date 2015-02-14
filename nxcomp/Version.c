/**************************************************************************/
/*                                                                        */
/* Copyright (C) 2014 Qindel http://qindel.com and QVD http://theqvd.com  */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by   */
/* the Free Software Foundation; either version 3 of the License, or (at  */
/* your option) any later version.                                        */
/*                                                                        */
/* This program is distributed in the hope that it will be useful, but    */
/* WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                   */
/* See the GNU General Public License for more details.                   */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, see <http://www.gnu.org/licenses>.    */
/*                                                                        */
/* Additional permission under GNU GPL version 3 section 7                */
/*                                                                        */
/* If you modify this Program, or any covered work, by linking or         */
/* combining it with [name of library] (or a modified version of that     */
/* library), containing parts covered by the terms of [name of library's  */
/* license], the licensors of this Program grant you additional           */
/* permission to convey the resulting work. {Corresponding Source for a   */
/* non-source form of such a combination shall include the source code    */
/* for the parts of [name of library] used as well as that of the covered */
/* work.}                                                                 */
/*                                                                        */
/*                                                                        */
/**************************************************************************/

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
