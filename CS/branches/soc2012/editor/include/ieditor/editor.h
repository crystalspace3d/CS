/*
    Copyright (C) 2011-2012 by Jorrit Tyberghein, Jelle Hellemans, Christian Van Brussel
    Copyright (C) 2007 by Seth Yastrov

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
#ifndef __IEDITOR_EDITOR_H__
#define __IEDITOR_EDITOR_H__

//#include "csutil/array.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"

//#include <wx/string.h>

class wxBitmap;
class wxFrame;
class wxWindow;

struct iDocumentNode;
struct iProgressMeter;

namespace CS {
namespace EditorApp {

struct iActionManager;
struct iContext;
struct iEditor;
struct iMenuManager;
struct iOperatorManager;
struct iSpaceManager;

// TODO namespace CS::Editor::Core

struct iEditorManager : public virtual iBase
{
  SCF_INTERFACE (iEditorManager, 1, 0, 0);

  virtual bool StartEngine () = 0;
  virtual bool StartApplication () = 0;

  virtual iEditor* CreateEditor (const char* name, const char* title,
				 iContext* context) = 0;
  virtual void RemoveEditor (iEditor* editor) = 0;
  virtual iEditor* FindEditor (const char* name) = 0;
  virtual iEditor* GetEditor (size_t index) = 0;
  virtual size_t GetEditorCount () const = 0;
};

struct iEditor : public virtual iBase
{
  SCF_INTERFACE (iEditor, 1, 0, 0);

  virtual iEditorManager* GetManager () const = 0;

  virtual iContext* GetContext () const = 0;

  virtual wxFrame* GetwxFrame () = 0;

  virtual iActionManager* GetActionManager () const = 0;
  virtual iMenuManager* GetMenuManager () const = 0;
  virtual iOperatorManager* GetOperatorManager () const = 0;
  virtual iSpaceManager* GetSpaceManager () const = 0;
  // TODO: icon/image manager

  virtual csPtr<iProgressMeter> CreateProgressMeter () const = 0;

  virtual void Save (iDocumentNode* node) const = 0;
  virtual bool Load (iDocumentNode* node) = 0;
};

struct iEditorComponent : public virtual iBase
{
  SCF_INTERFACE (iEditorComponent, 1, 0, 0);

  virtual bool Initialize (iEditor* editor) = 0;
  virtual void Update () = 0;

  virtual void Save (iDocumentNode* node) const = 0;
  virtual bool Load (iDocumentNode* node) = 0;
};

} // namespace EditorApp
} // namespace CS

#endif
