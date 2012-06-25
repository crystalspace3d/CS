/*
    Copyright (C) 2011 by Jelle Hellemans

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

#ifndef __IEDITOR_SPACE_H__
#define __IEDITOR_SPACE_H__

#include <csutil/scf.h>
#include <csutil/scf_implementation.h>

class wxWindow;
class wxBitmap;

struct iObjectRegistry;

namespace CS {
namespace EditorApp {

struct iEditor;
struct iHeader;
struct iPanel;
struct iSpaceFactory;

/**
 * A space is a window in the editor.
 * 
 * This space interface allows plugins to implement custom spaces.
 */
struct iSpace : public virtual iBase
{
  SCF_INTERFACE (iSpace, 0, 0, 1);

  virtual bool Initialize (iObjectRegistry* obj_reg, iEditor* editor, iSpaceFactory* factory, wxWindow* parent) = 0;

  virtual iSpaceFactory* GetFactory () const = 0;

  /// Get the underlying wxWindow content area of this space.
  virtual wxWindow* GetwxWindow () = 0;

  virtual void DisableUpdates (bool disabled) = 0;

  virtual void Update () = 0;
};

/**
 *
 */
struct iSpaceFactory : public virtual iBase
{
  SCF_INTERFACE (iSpaceFactory, 0, 0, 1);

  virtual csPtr<iSpace> CreateInstance (wxWindow* parent) = 0;

  virtual const char* GetIdentifier () const = 0;

  virtual const char* GetLabel () const = 0;

  virtual const wxBitmap& GetIcon () const = 0;

  virtual bool GetMultipleAllowed () const = 0;

  virtual size_t GetCount () = 0;
};

/**
 * Manages a set of spaces which make up the visible parts of the editor.
 */
struct iSpaceManager : public virtual iBase
{
  SCF_INTERFACE (iSpaceManager, 0, 1, 0);

  virtual bool RegisterComponent (const char* pluginName) = 0;

  virtual bool RegisterSpace (const char* pluginName) = 0;

  virtual bool RegisterHeader (const char* pluginName) = 0;

  virtual bool RegisterPanel (const char* pluginName) = 0;
};

} // namespace EditorApp
} // namespace CS

#endif
