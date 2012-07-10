/*
    Copyright (C) 2012 by Christian Van Brussel
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
#include <cssysdef.h>
#include "csutil/scf.h"

#include <iengine/campos.h>
#include <iengine/collection.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <iengine/texture.h>
#include "iutil/event.h"
#include "iutil/eventh.h"
#include <iutil/object.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <ivideo/shader/shader.h>

#include <wx/wx.h>
#include <wx/treectrl.h>

#include "scenetree.h"

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{

#include "data/editor/images/sceneIcon.xpm"
  
SCF_IMPLEMENT_FACTORY (SceneTree)

BEGIN_EVENT_TABLE (SceneTree, wxPanel)
  EVT_SIZE (SceneTree::OnSize)
END_EVENT_TABLE ()

BEGIN_EVENT_TABLE (SceneTreeCtrl, wxTreeCtrl)
  EVT_TREE_ITEM_ACTIVATED (SceneTree_Ctrl, SceneTreeCtrl::OnItemActivated)
  EVT_TREE_SEL_CHANGED (SceneTree_Ctrl, SceneTreeCtrl::OnSelChanged)
END_EVENT_TABLE ()

// ----------------------------------------------------------------------------

SceneTree::SceneTree (iBase* parent)
 : scfImplementationType (this, parent)
{
  treectrl = 0;
}

SceneTree::~SceneTree ()
{
}

bool SceneTree::Initialize (iObjectRegistry* obj_reg, iEditor* editor,
			    iSpaceFactory* fact, wxWindow* parent)
{
  object_reg = obj_reg;
  
  // Create this panel
  Create (parent, -1, wxPoint (0, 0), wxSize (250, 250));
  
  // Create the tree control
  treectrl = new SceneTreeCtrl (object_reg, editor, this, SceneTree_Ctrl,
				wxPoint (0, 0), wxSize (100, 100),
				wxTR_MULTIPLE | wxTR_FULL_ROW_HIGHLIGHT
				| wxTR_EDIT_LABELS | wxTR_HAS_BUTTONS);

  return true;
}

wxWindow* SceneTree::GetwxWindow ()
{
  return this;
}

void SceneTree::Update ()
{
}

void SceneTree::OnSize (wxSizeEvent& event)
{
  // Resize the tree control
  if (treectrl)
    treectrl->SetSize (event.GetSize ());
  event.Skip ();
}

// ----------------------------------------------------------------------------

SceneTreeCtrl::SceneTreeCtrl (iObjectRegistry* object_reg, iEditor* editor,
			      wxWindow *parent, const wxWindowID id,
			      const wxPoint& pos, const wxSize& size,
			      long style)
  : wxTreeCtrl (parent, id, pos, size, style), editor (editor)
{
  imageList = new wxImageList (16, 16);
  AssignImageList (imageList);

  wxBitmap sceneBmp (sceneIcon_xpm);
  rootIconIdx = imageList->Add (sceneBmp);

  // Hide the root item
  SetWindowStyle (GetWindowStyle () | wxTR_HIDE_ROOT);

  // Register the event handler
  iEventNameRegistry* registry =
    csEventNameRegistry::GetRegistry (object_reg);
  csEventID eventSetCollection =
    registry->GetID ("crystalspace.editor.context.setcollection");
  RegisterQueue (editor->GetContext ()->GetEventQueue (),
		 eventSetCollection);

  categories.Put (CAMERA_POSITION, "Camera positions");
  categories.Put (LIGHT, "Lights");
  categories.Put (LIGHT_FACTORY, "Light factories");
  categories.Put (MATERIAL, "Materials");
  categories.Put (MESH, "Meshes");
  categories.Put (MESH_FACTORY, "Mesh factories");
  categories.Put (SECTOR, "Sectors");
  categories.Put (SHADER, "Shaders");
  categories.Put (TEXTURE, "Textures");
  categories.Put (UNKNOWN, "Unknown type");

  UpdateTree ();
}

bool SceneTreeCtrl::HandleEvent (iEvent &event)
{
  UpdateTree ();
  return false;
}

void SceneTreeCtrl::UpdateTree ()
{
  // Clear the tree and rebuild it
  DeleteAllItems ();
  AddRoot (wxString ("", *wxConvCurrent));

  // Re-initialize the object categories
  rootIDs.Truncate (0);
  rootIDs.SetSize (TYPE_COUNT, wxTreeItemId ());

  // Add all objects in the context's collection to the tree
  csRef<iContextFileLoader> fileLoaderContext =
    scfQueryInterface<iContextFileLoader> (editor->GetContext ());
  iCollection* collection = fileLoaderContext->GetCollection ();
  if (!collection) return;

  iObject* collisionObject = collection->QueryObject ();
  for (csRef<iObjectIterator> it = collisionObject->GetIterator (); it->HasNext (); )
  {
    iObject* object = it->Next ();
   
    // Search the type of the object
    csRef<iMeshWrapper> mesh =
      scfQueryInterface<iMeshWrapper> (object);

    if (mesh)
    {
      AppendObject (object, MESH);
      continue;
    }
    
    csRef<iMeshFactoryWrapper> meshFactory =
      scfQueryInterface<iMeshFactoryWrapper> (object);

    if (meshFactory)
    {
      AppendObject (object, MESH_FACTORY);
      continue;
    }
    
    csRef<iTextureWrapper> texture =
      scfQueryInterface<iTextureWrapper> (object);

    if (texture)
    {
      AppendObject (object, TEXTURE);
      continue;
    }
    
    csRef<iMaterialWrapper> material =
      scfQueryInterface<iMaterialWrapper> (object);

    if (material)
    {
      AppendObject (object, MATERIAL);
      continue;
    }
    
    csRef<iShader> shader =
      scfQueryInterface<iShader> (object);

    if (shader)
    {
      AppendObject (object, SHADER);
      continue;
    }
    
    csRef<iLightFactory> lightFactory =
      scfQueryInterface<iLightFactory> (object);

    if (lightFactory)
    {
      AppendObject (object, LIGHT_FACTORY);
      continue;
    }
    
    csRef<iLight> light =
      scfQueryInterface<iLight> (object);

    if (light)
    {
      AppendObject (object, LIGHT);
      continue;
    }
    
    csRef<iSector> sector =
      scfQueryInterface<iSector> (object);

    if (sector)
    {
      AppendObject (object, SECTOR);
      continue;
    }
    
    csRef<iCameraPosition> cameraPosition =
      scfQueryInterface<iCameraPosition> (object);

    if (cameraPosition)
    {
      AppendObject (object, CAMERA_POSITION);
      continue;
    }
    
    AppendObject (object, UNKNOWN);
  }

  // Rename the categories to make appear the count of children objects
  for (int i = 0; i < TYPE_COUNT; i++)
    if (rootIDs[i].IsOk ())
    {
      unsigned int count = GetChildrenCount (rootIDs[i], false);
      csString text;
      text.Format ("%s (%u)", categories[i]->GetData (), count);
      SetItemText (rootIDs[i], wxString (text.GetData (), *wxConvCurrent));
    }

  //ExpandAll ();
}

void SceneTreeCtrl::AppendObject (iObject* object, ObjectType type)
{
  // Create the object's category if not yet made
  if (!rootIDs[type].IsOk ())
  {
    // Search the index where the category must be placed
    wxTreeItemId previous;
    for (int i = type - 1; i >= 0; i--)
      if (rootIDs[i].IsOk ())
      {
	previous = rootIDs[i];
	break;
      }

    CS_ASSERT (categories[type]);

    if (previous.IsOk ())
      rootIDs[type] = InsertItem (GetRootItem (), previous,
				  wxString (categories[type]->GetData (), *wxConvCurrent),
				  rootIconIdx, -1, nullptr);
    else
      rootIDs[type] = InsertItem (GetRootItem (), 0,
				  wxString (categories[type]->GetData (), *wxConvCurrent),
				  rootIconIdx, -1, nullptr); 
  }

  // Add the object entry in the tree
  csString name = object->GetName ();
  if (name.Trim ().IsEmpty ())
    name = "Unnamed object";

  wxTreeItemId id =
    AppendItem (rootIDs[type], wxString (name, *wxConvCurrent),
		/*imageIdx*/ -1, -1, new SceneTreeItemData (object));
}

void SceneTreeCtrl::OnItemActivated (wxTreeEvent& event)
{
  wxTreeItemData* itemData = GetItemData (event.GetItem ());
  if (!itemData)
  {
    Toggle (event.GetItem ());
    return;
  }

  SceneTreeItemData* data =
    static_cast<SceneTreeItemData*> (itemData);

  csRef<iContextObjectSelection> objectSelectionContext =
    scfQueryInterface<iContextObjectSelection> (editor->GetContext ());
  objectSelectionContext->SetActiveObject (data->GetObject ());
}

void SceneTreeCtrl::OnSelChanged (wxTreeEvent& event)
{
  csRef<iContextObjectSelection> objectSelectionContext =
    scfQueryInterface<iContextObjectSelection> (editor->GetContext ());

  wxArrayTreeItemIds selectionIds;
  unsigned int selSize = GetSelections (selectionIds);
  
  objectSelectionContext->ClearSelectedObjects ();
  
  for (unsigned int i = 0; i < selSize; i++)
  {
    wxTreeItemId itemId = selectionIds[i];
    
    SceneTreeItemData* data =
      static_cast<SceneTreeItemData*> (GetItemData (itemId));
    if (!data) continue;

    iObject* object = data->GetObject ();
    if (!object) continue;

    objectSelectionContext->AddSelectedObject (object);
  }
}

}
CS_PLUGIN_NAMESPACE_END (CSEditor)
