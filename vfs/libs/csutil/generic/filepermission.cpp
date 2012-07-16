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

#include <sys/types.h>
#include <sys/stat.h>

#include "cssysdef.h"
#include "csutil/filepermission.h"

// implementation using POSIX-style stat ()

namespace CS
{
  namespace Platform
  {
    int GetFilePermission (const char *filename, uint32 &oPermission)
    {
      struct stat info;

      int lastErrno = errno; // store last errno
      int result = 0;

      // use stat() to get file information...
      if (stat (filename, &info) != 0)
      {
        // stat() failed...
        result = errno;
      }
      // restore last errno status
      errno = lastErrno;
      // give out permission information
      oPermission = info.st_mode;

      return result;
    }

    int SetFilePermission (const char *filename, uint32 permission)
    {
      // store last errno
      int lastErrno = errno;
      int result = 0;

      // use chmod () to apply desired permission
      if (chmod (filename, permission) != 0)
      {
        // chmod () failed...
        result = errno;
      }
      // restore last errno status
      errno = lastErrno;

      return result;
    }
  } // namespace Platform
} // namespace CS
