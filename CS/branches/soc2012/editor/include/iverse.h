/*
    Copyright (C) 2006 by Jorrit Tyberghein
    Copyright (C) 2006 by Amir Taaki
    Copyright (C) 2006 by Pablo Martin

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

#ifndef __CS_IVERSE_H__
#define __CS_IVERSE_H__

#include "csutil/scf.h"

/**
 *  This interface is an SCF interface for encapsulating verse protocol.
 */
struct iVerse : public virtual iBase
{
  SCF_INTERFACE (iVerse, 0, 0, 1);

  /// Print the node tree.
  virtual void PrintNodeTree () = 0;
};

#endif // __CS_IVERSE_H__

