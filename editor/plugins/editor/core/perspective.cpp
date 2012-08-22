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
#include "cssysdef.h"
#include "csutil/stringquote.h"
#include "ivaria/reporter.h"

#include "editor.h"
#include "perspective.h"
#include "window.h"

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{

//------------------------------------  PerspectiveManager  ------------------------------------

PerspectiveManager::PerspectiveManager (iObjectRegistry* object_reg, Editor* editor)
  : scfImplementationType (this), object_reg (object_reg), editor (editor),
  currentPerspective (nullptr)
{
}

PerspectiveManager::~PerspectiveManager ()
{
}

iPerspective* PerspectiveManager::CreatePerspective (const char* name, iPerspective* iother)
{
  csRef<Perspective> perspective;
  Perspective* other = dynamic_cast<Perspective*> (iother);
  perspective.AttachNew (other ? new Perspective (other) : new Perspective ());
  perspectives.Put (name, perspective);

  return perspective;
}

void PerspectiveManager::DeletePerspective (const char* name)
{
  perspectives.DeleteAll (name);
}

void PerspectiveManager::SetCurrentPerspective (const char* name, size_t frameIndex)
{
  currentPerspective = *perspectives.GetElementPointer (name);
}

iPerspective* PerspectiveManager::GetCurrentPerspective (size_t frameIndex) const
{
  if (currentPerspective)
    return currentPerspective;
  if (perspectives.GetSize ())
    return perspectives.GetIterator ().Next ();
  return nullptr;
}

iPerspective* PerspectiveManager::GetPerspective (const char* name) const
{
  return *perspectives.GetElementPointer (name);
}

void PerspectiveManager::RenamePerspective (const char* oldName, const char* newName)
{
  Perspective* perspective = *perspectives.GetElementPointer (oldName);

#ifdef CS_DEBUG
  if (!perspective)
  {
    csReport (editor->manager->object_reg, CS_REPORTER_SEVERITY_ERROR,
	      "crystalspace.editor.core.perspective",
	      "The perspective to rename %s does not exist",
	      CS::Quote::Single (oldName));
    return;
  }
#endif // CS_DEBUG
  
  perspectives.DeleteAll (oldName);
  perspectives.Put (newName, perspective);
}

Window* PerspectiveManager::CreateWindow (iPerspective* perspective)
{
  Window* window = new Window (object_reg, editor, editor);
  ((Perspective*) perspective)->SetupWindow (window);
  return window;
}

//------------------------------------  Perspective  ------------------------------------

Perspective::Perspective ()
  : scfImplementationType (this), mode (SPLIT_NONE), position (0)
{
}

Perspective::Perspective (Perspective* other)
  : scfImplementationType (this), mode (other->mode), pluginName (other->pluginName),
  position (other->position)
{
  if (mode != SPLIT_NONE)
  {
    child1.AttachNew (new Perspective (other->child1));
    child2.AttachNew (new Perspective (other->child2));
  }
}

Perspective::~Perspective ()
{
}

void Perspective::SetSpace (const char* pluginName)
{
  this->pluginName = pluginName;
}

void Perspective::SetSplitMode (SplitMode mode)
{
  this->mode = mode;

  if (mode == SPLIT_NONE)
  {
    child1.Invalidate ();
    child2.Invalidate ();
  }

  else if (!child1)
  {
    child1.AttachNew (new Perspective ());
    child2.AttachNew (new Perspective ());
  }
}

void Perspective::SetSplitPosition (int position)
{
  this->position = position;
}

iPerspective* Perspective::GetChild1 () const
{
  return child1;
}

iPerspective* Perspective::GetChild2 () const
{
  return child2;
}

void Perspective::SetupWindow (Window* window)
{
  if (mode == SPLIT_NONE)
  {
    window->Realize (pluginName);
  }

  else
  {
    window->Split ();
    window->SetSplitMode (mode == SPLIT_HORIZONTAL ? wxSPLIT_HORIZONTAL : wxSPLIT_VERTICAL);
    if (position) window->SetSashPosition (position, true);

    Window* childWindow1 = (Window*) window->GetWindow1 ();
    child1->SetupWindow (childWindow1);
    Window* childWindow2 = (Window*) window->GetWindow2 ();
    child2->SetupWindow (childWindow2);

    window->Realize ();
  }
}

}
CS_PLUGIN_NAMESPACE_END (CSEditor)
