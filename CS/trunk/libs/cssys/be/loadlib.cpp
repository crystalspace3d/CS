/*
  BeOS support for Crystal Space 3D library
  Copyright (C) 1998 by Jorrit Tyberghein

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <kernel/image.h>
#include <sys/param.h>
#include "cssysdef.h"
#include "cssys/csshlib.h"
#include "csutil/csstring.h"

csLibraryHandle csFindLoadLibrary (const char *iName)
{
  return csFindLoadLibrary (NULL, iName, ".plugin");
}

csLibraryHandle csLoadLibrary (const char* iName)
{
  return load_add_on (iName);
}

void *csGetLibrarySymbol (csLibraryHandle h, const char* name)
{
  void* sym;
  return (get_image_symbol ((image_id)h, name, B_SYMBOL_TYPE_TEXT, &sym) ==
    B_OK) ? sym : 0;
}

bool csUnloadLibrary (csLibraryHandle h)
{
  return (unload_add_on ((image_id)h) == B_OK);
}
