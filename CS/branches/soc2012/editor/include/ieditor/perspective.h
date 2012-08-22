/*
    Copyright (C) 2012 by Christian Van Brussel, Jelle Hellemans

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

#ifndef __IEDITOR_PERSPECTIVE_H__
#define __IEDITOR_PERSPECTIVE_H__

#include <csutil/scf.h>
#include <csutil/scf_implementation.h>

namespace CS {
namespace EditorApp {

enum SplitMode
{
  SPLIT_NONE = 0,
  SPLIT_HORIZONTAL,
  SPLIT_VERTICAL
};

/**
 *
 */
struct iPerspective : public virtual iBase
{
  SCF_INTERFACE (iPerspective, 1, 0, 0);

  virtual void SetSpace (const char* pluginName) = 0;
  virtual void SetSplitMode (SplitMode mode) = 0;
  virtual void SetSplitPosition (int position) = 0;
  virtual iPerspective* GetChild1 () const = 0;
  virtual iPerspective* GetChild2 () const = 0;
};

/**
 * 
 */
struct iPerspectiveManager : public virtual iBase
{
  SCF_INTERFACE (iPerspectiveManager, 1, 0, 0);

  virtual iPerspective* CreatePerspective
    (const char* name, iPerspective* other = nullptr) = 0;
  virtual void DeletePerspective (const char* name) = 0;
  virtual void SetCurrentPerspective (const char* name, size_t frameIndex = 0) = 0;
  virtual iPerspective* GetCurrentPerspective (size_t frameIndex = 0) const = 0;
  virtual iPerspective* GetPerspective (const char* name) const = 0;
  virtual void RenamePerspective (const char* oldName, const char* newName) = 0;
};

} // namespace EditorApp
} // namespace CS

#endif // __IEDITOR_PERSPECTIVE_H__
