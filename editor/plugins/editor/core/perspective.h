/*
    Copyright (C) 2012 by Christian Van Brussel

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
#ifndef __PERSPECTIVE_H__
#define __PERSPECTIVE_H__

#include "csutil/hash.h"
#include "ieditor/perspective.h"

using namespace CS::EditorApp;

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{

class Editor;
class Perspective;
class Window;

class PerspectiveManager
  : public scfImplementation1<PerspectiveManager, iPerspectiveManager>
{
public:
  PerspectiveManager (iObjectRegistry* object_reg, Editor* editor);
  virtual ~PerspectiveManager ();
  
  //-- iPerspectiveManager
  virtual iPerspective* CreatePerspective
    (const char* name, iPerspective* other = nullptr);
  virtual void DeletePerspective (const char* name);
  virtual void SetCurrentPerspective (const char* name, size_t frameIndex = 0);
  virtual iPerspective* GetCurrentPerspective (size_t frameIndex = 0) const;
  virtual iPerspective* GetPerspective (const char* name) const;
  virtual void RenamePerspective (const char* oldName, const char* newName);

  Window* CreateWindow (iPerspective* perspective);

public:
  iObjectRegistry* object_reg;
  Editor* editor;
  csHash<csRef<Perspective>, csString> perspectives;
  Perspective* currentPerspective;
};

class Perspective
  : public scfImplementation1<Perspective, iPerspective>
{
public:
  Perspective ();
  Perspective (Perspective* other);
  ~Perspective ();

  //-- iPerspective
  virtual void SetSpace (const char* pluginName);
  virtual void SetSplitMode (SplitMode mode);
  virtual void SetSplitPosition (int position);
  virtual iPerspective* GetChild1 () const;
  virtual iPerspective* GetChild2 () const;

  void SetupWindow (Window* window);

public:
  SplitMode mode;
  csString pluginName;
  int position;
  csRef<Perspective> child1;
  csRef<Perspective> child2;
};

}
CS_PLUGIN_NAMESPACE_END (CSEditor)

#endif
