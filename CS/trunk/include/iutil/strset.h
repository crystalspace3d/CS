/*
    Copyright (C) 2003 by Anders Stenberg

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

#ifndef __CS_IUTIL_STRINGSET_H__
#define __CS_IUTIL_STRINGSET_H__

/**\file
 * Stringset interface
 */
/**\addtogroup util
 * @{ */
#include <stdarg.h>
#include "csutil/scf.h"
#include "csutil/strset.h"

SCF_VERSION (iStringSet, 0, 1, 0);

/// This is a SCF-compatible interface for csStringSet.
struct iStringSet : public iBase
{
  /**
  * Request the ID for the given string. Create a new ID
  * if the string was never requested before.
  */
  virtual csStringID Request (const char *s) = 0;

  /**
  * Request the string for a given ID. Return NULL if the string
  * has not been requested (yet).
  */
  virtual const char* Request (csStringID id) = 0;

  /**
  * Delete all stored strings. When new strings are registered again, new
  * ID values will be used, not the old ones reused.
  */
  virtual void Clear () = 0;
};

/** @} */

#endif // __CS_IUTIL_STRINGSET_H__
