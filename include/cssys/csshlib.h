/*
    Copyright (C) 1999 by Andrew Zabolotny
    Crystal Space cross-platform shared library management

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

#ifndef __CSSHLIB_H__
#define __CSSHLIB_H__

typedef void *csLibraryHandle;

/**
 * Load a shared library and return:
 * - a library handle (used to unload the library)
 * - a pointer to an array of scfClassInfo structures
 * The installpath points to the CS install dir.
 */
extern csLibraryHandle csLoadLibrary (
  const char *installpath, const char* iName);
/// Return a pointer to a symbol within given shared library
extern void *csGetLibrarySymbol (csLibraryHandle Handle, const char *iName);
/// Unload a shared library given its handle
extern bool csUnloadLibrary (csLibraryHandle Handle);

#endif // __CSSHLIB_H__
