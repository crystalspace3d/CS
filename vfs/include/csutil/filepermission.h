/*
    Copyright (C) 2012 by Eunsoo Roh <nes1209@hotmail.com>

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

#ifndef __CSUTIL_FILE_PERMISSION_H__
#define __CSUTIL_FILE_PERMISSION_H__

/**\file
 * Functions to read/write file permissions across platform.
 */

#include "cssysdef.h"
#include "csextern.h"

namespace CS
{
  namespace Platform
  {
    
    /**
     * Open a file from a native path, encoded in UTF-8.
     * The function takes care of translating the file name to the
     * platform-specific file name encoding.
     * \param filename Native file name, encoded in UTF-8.
     * \param oPermission variable to store \c Unix-style permission
     *     represented in octal format.
     * \return 0 if operation succeeded; otherwise errno-equivalent
     *    error code is returned.
     */
    CS_CRYSTALSPACE_EXPORT int GetFilePermission (const char* filename,
                                                  uint32 &oPermission);
    /**
     * Set permission of file or directory at given path.
     * \param filename Native file name, encoded in UTF-8
     * \param oPermission \c Unix-style permission to set, represented in
     *     octal format
     * \return 0 if operation succeeded; otherwise errno-equivalent
     *    error code is returned.
     */
    CS_CRYSTALSPACE_EXPORT int SetFilePermission (const char *filename,
                                                  uint32 permission);
  } // namespace Platform
} // namespace CS

#endif // __CSUTIL_FILE_PERMISSION_H__
