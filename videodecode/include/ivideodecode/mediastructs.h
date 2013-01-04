/*
Copyright (C) 2011 by Alin Baciu

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

#ifndef __CS_VPL_COMMON_STRUCTS_H__
#define __CS_VPL_COMMON_STRUCTS_H__

/**\file
 * Video Player: structs
 */

/**
 * \addtogroup videoplay
 * @{ */

#include "csutil/scf.h"

/**\addtogroup vpl
 * @{ */

/**
  * Used to store languages available for a media file.
  */
struct Language
{
  /**
    * The name of the language
    */
  char* name;

  /**
    * The path of the audio file
    */
  char* path;
};

/** @} */

#endif // __CS_VPL_COMMON_STRUCTS_H__
