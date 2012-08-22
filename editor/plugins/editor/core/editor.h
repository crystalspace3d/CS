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
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "csutil/csstring.h"
#include "ieditor/editor.h"
#include "iutil/comp.h"

#include <wx/frame.h>
#include <wx/timer.h>

struct iEventQueue;
struct iVirtualClock;

using namespace CS::EditorApp;

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{

class ActionManager;
class Editor;
class MenuManager;
class OperatorManager;
class PerspectiveManager;
class SpaceManager;
class StatusBar;

class EditorManager
  : public scfImplementation2<EditorManager, iEditorManager, iComponent>
{
public:
  EditorManager (iBase* parent);
  virtual ~EditorManager ();
  
  //-- Error reporting
  bool ReportError (const char* description, ...) const;
  bool ReportWarning (const char* description, ...) const;

  //-- iComponent
  virtual bool Initialize (iObjectRegistry* reg);

  //-- iEditorManager
  virtual bool StartEngine ();
  virtual bool StartApplication ();

  virtual iEditor* CreateEditor (const char* name, const char* title, iContext* context);
  virtual void RemoveEditor (iEditor* editor);
  virtual iEditor* FindEditor (const char* name);
  virtual iEditor* GetEditor (size_t index);
  virtual size_t GetEditorCount () const;

private:
  void Update ();

public:
  iObjectRegistry* object_reg;
  csArray<Editor*> editors;
  csRef<iVirtualClock> vc;
  csRef<iEventQueue> eventQueue;

private:
  class Pump : public wxTimer
  {
  public:
    Pump (EditorManager* editorManager) : editorManager (editorManager) {}
    
    virtual void Notify ()
    { editorManager->Update (); }

  private:
    EditorManager* editorManager;
  };
  Pump* pump;
};

class Editor
  : public scfImplementation1<Editor, iEditor>, public wxFrame
{
public:
  Editor (EditorManager* manager, const char* name,
	  const char* title, iContext* context);
  ~Editor ();

  //-- iEditor
  virtual iContext* GetContext () const;

  virtual iEditorManager* GetManager () const;
  virtual iActionManager* GetActionManager () const;
  virtual iMenuManager* GetMenuManager () const;
  virtual iOperatorManager* GetOperatorManager () const;
  virtual iPerspectiveManager* GetPerspectiveManager () const;
  virtual iSpaceManager* GetSpaceManager () const;

  virtual csPtr<iProgressMeter> CreateProgressMeter () const;
  virtual void ReportStatus (const char* text);

  virtual wxFrame* GetwxFrame ();

  virtual void Save (iDocumentNode* node) const;
  virtual bool Load (iDocumentNode* node);

public:
  void Init ();
  void Update ();

public:
  csString name;
  EditorManager* manager;
  csRef<iContext> context;
  csRef<ActionManager> actionManager;
  csRef<MenuManager> menuManager;
  csRef<OperatorManager> operatorManager;
  csRef<PerspectiveManager> perspectiveManager;
  csRef<SpaceManager> spaceManager;
  StatusBar* statusBar;
};

}
CS_PLUGIN_NAMESPACE_END (CSEditor)

#endif
