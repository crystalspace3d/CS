/*
    Copyright (C) 2000 by Andrew Zabolotny
    Shared library path support

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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cssysdef.h"
#include "cssys/csshlib.h"
#include "cssys/system.h"
#include "csutil/csstrvec.h"
#include "csutil/util.h"

static bool verbose_errors = true;

void csSetLoadLibraryVerbose(bool b) { verbose_errors = b; }
bool csGetLoadLibraryVerbose() { return verbose_errors; }


