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
#include "cstool/initapp.h"
#include "csutil/scf.h"
#include "ieditor/context.h"
#include "ieditor/operator.h"
#include "iengine/engine.h"
#include "igraphic/imageio.h"
#include "imap/loader.h"
#include "imap/saver.h"
#include "iutil/plugin.h"
#include "ivaria/pmeter.h"
//#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
//#include "ivideo/wxwin.h"

#include "maploader.h"
#include "vfsfiledialog.h"

//#include <wx/wx.h>
#include <wx/frame.h>

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{

SCF_IMPLEMENT_FACTORY (MapLoader)

MapLoader::MapLoader (iBase* parent)
  : scfImplementationType (this, parent), loadingResource (nullptr)
{

}

MapLoader::~MapLoader()
{
  delete loadingResource;
}

bool MapLoader::Initialize (iEditor* editor)
{
  this->editor = editor;
  object_reg = editor->GetContext ()->GetObjectRegistry ();

  // Request the main loader plugins of Crystal Space
  if (!csInitializer::RequestPlugins (object_reg,
        CS_REQUEST_IMAGELOADER,
        CS_REQUEST_LEVELLOADER,
        CS_REQUEST_END))
    return ReportError ("Can't initialize standard Crystal Space plugins!");

  // Setup the saveable flag of the engine
  csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);
  if (!engine) return ReportError ("Failed to locate 3D engine!");
  engine->SetSaveableFlag (true);

  // Create the 'loader' collection
  //collection = engine->CreateCollection ("loader");

  // Load the iSaver plugin
  if (!csInitializer::RequestPlugins (object_reg,
				      CS_REQUEST_LEVELSAVER,
				      CS_REQUEST_END))
    return ReportError ("Failed to initialize the iSaver plugin!");

  vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) return ReportError ("Failed to locate Virtual File System!");

  // Setup the loading menus
  fileMenu = editor->GetMenuManager ()->GetSubMenu ("&File");
  openItem = fileMenu->AppendItem ("&Open file", "file.open");
  reloadItem = fileMenu->AppendItem ("&Reload file", "file.reload");
  saveItem = fileMenu->AppendItem ("&Save file", "file.save");
  separator = fileMenu->AppendSeparator ();

  csEventID events[] = { openItem->GetEventID (),
			 reloadItem->GetEventID (),
			 saveItem->GetEventID (),
			 CS_EVENTLIST_END };
  RegisterQueue (editor->GetContext ()->GetEventQueue (), events);

  // TODO: read resource to be loaded from command line
  lastResource.path = "/lev/";
  lastResource.file = "";

  lastResource.path = "/lev/castle/";
  //lastResource.path = "/data/scaletest/";
  lastResource.file = "world";
  //resourceData.Push (lastResource);

  return true;
}

void MapLoader::Update ()
{
  // Check the status of any resource currently loaded
  if (loadingResource && loadingReturn->IsFinished ())
  {
    if (loadingReturn->WasSuccessful ())
    {
      csString text = "File \"";
      text += loadingResource->file + "\" was loaded successfully";
      editor->GetwxFrame ()->SetStatusText (wxString::FromAscii (text.GetData ()));

      csRef<iContextFileLoader> fileLoaderContext =
	scfQueryInterface<iContextFileLoader> (editor->GetContext ());
      fileLoaderContext->SetPath (loadingResource->path);
      fileLoaderContext->SetFilename (loadingResource->file);
      fileLoaderContext->SetCollection (collection);
    }

    else
    {
      csString text = "Failed to load file \"";
      text += loadingResource->file + "\"";
      editor->GetwxFrame ()->SetStatusText (wxString::FromAscii (text.GetData ()));
    }

    //loadingReturn->IncRef ();

    delete loadingResource;
    loadingResource = nullptr;
    loadingReturn = nullptr;
    progressMeter = nullptr;
  }

  // If there are no loading active then push any pending one
  if (!loadingResource && resourceData.GetSize ())
  {
    loadingResource = new ResourceData ();
    *loadingResource = resourceData[0];
    resourceData.DeleteIndex (0);

    // Set the status text
    csString text = "Loading file \"";
    text += loadingResource->file + "\"...";
    editor->GetwxFrame ()->SetStatusText (wxString::FromAscii (text.GetData ()));

    // Create a progress meter
    progressMeter = editor->CreateProgressMeter ();
    csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);
    engine->Prepare (progressMeter);

    // Delete the previous collection from the engine and create a new one
    if (collection)
    {
      csRef<iContextFileLoader> fileLoaderContext =
	scfQueryInterface<iContextFileLoader> (editor->GetContext ());
      fileLoaderContext->SetCollection (nullptr);
      collection->ReleaseAllObjects ();
      engine->RemoveCollection (collection);
    }
    collection = engine->CreateCollection ("loader");

    // Start the file loading
    csRef<iThreadedLoader> loader = csQueryRegistry<iThreadedLoader> (object_reg);
    loadingReturn = loader->LoadFile (loadingResource->path, loadingResource->file, collection);
  }
}

void MapLoader::Save (iDocumentNode* node) const
{

}

bool MapLoader::Load (iDocumentNode* node)
{
  return false;
}

bool MapLoader::HandleEvent (iEvent &event)
{
  printf ("Base::HandleEvent\n");

  if (event.GetName () == openItem->GetEventID ())
  {
    printf ("Open!\n");

    // Ask the user to specify a file from the file dialog
    VFSFileDialog dialog ((wxWindow*) editor->GetwxFrame (), -1, _("Select file to open"),
			  wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE,
			  vfs, lastResource.path + lastResource.file, VFS_OPEN);
    if (!dialog.ShowModal ())
      return false;

    // Save the last file selected & push that file on the loading list
    lastResource.path = dialog.GetPath ();
    lastResource.file = dialog.GetFilename ();
    resourceData.Push (lastResource);

    return true;
  }

  else if (event.GetName () == reloadItem->GetEventID ())
  {
    printf ("Reload!\n");
    if (!lastResource.file.IsEmpty ())
      resourceData.Push (lastResource);

    return true;
  }

  else if (event.GetName () == saveItem->GetEventID ())
  {
    printf ("Save!\n");
    return true;
  }

  return false;
}

}
CS_PLUGIN_NAMESPACE_END(CSEditor)

