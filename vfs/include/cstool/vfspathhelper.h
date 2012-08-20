/*
    Copyright (C) 2012 by Eunsoo Roh (nes1209@hotmail.com)

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

/**\file
 * Helper class for VFS path manipulation.
 */

#ifndef __CS_VFS_PATH_HELPER_H__
#define __CS_VFS_PATH_HELPER_H__

#include "iutil/vfs.h"
#include "csutil/csstring.h"

/**
 * Helper class to change the current VFS directory and restore the previous
 * directory when an instance goes out of scope.
 */
struct csVfsPathHelper
{
  /// Compose two VFS path, adding slash in the middle if necessary
  static csString ComposePath (const char *base, const char *suffix)
  {
    csString path (base); // start from base path
    // append suffix and return
    return AppendPath (path, suffix);
  }

  /// Append suffix to existing VFS base path.
  static csString &AppendPath (csString &base, const char *suffix)
  {
    const size_t len = base.Length (); // length of path

    // if the base path already doesn't end with VFS_PATH_SEPARATOR,
    // and suffix doesn't start with VFS_PATH_SEPARATOR,
    // add one.
    if (len > 0 && base[len - 1] != VFS_PATH_SEPARATOR
      && (suffix && *suffix != VFS_PATH_SEPARATOR))
      base << VFS_PATH_SEPARATOR;

    // add the suffix part
    base << suffix;
    // done.
    return base;
  }
};

#endif
