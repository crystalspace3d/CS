/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Barsan

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#define CS_IMPLEMENT_PLATFORM_APPLICATION
/* This is needed due the WX headers using free () inline, but the opposing
 * malloc () is in the WX libs. */
#define CS_NO_MALLOC_OVERRIDE

#include "cssysdef.h"

#include "vareditapp.h"
#include "vareditframe.h"
#include "testmodifiable.h"

#include "cstool/wxappargconvert.h"
#include "imap/reader.h"
#include "iutil/document.h"
#include "iutil/stringarray.h"
#include "iutil/vfs.h"
#include "ivaria/translator.h"

CS_IMPLEMENT_APPLICATION

IMPLEMENT_APP (VarEditTestApp);

using namespace std;

bool VarEditTestApp::OnInit ()
{
  wxInitAllImageHandlers ();

  // Create the Crystal Space environment
  CS::WX::AppArgConvert args (argc, argv);
  object_reg = csInitializer::CreateEnvironment (args.csArgc(), args.csArgv());

  // Load the needed plugins; so far, just the translator is needed
  if (!csInitializer::RequestPlugins (object_reg, 
				      // We need a file system to read stuff from it
				      CS_REQUEST_PLUGIN ("crystalspace.kernel.vfs", iVFS),
				      // Read the XML required for the name translations
				      CS_REQUEST_PLUGIN ("crystalspace.documentsystem.xmlread", iDocumentSystem),
				      // The translator itself
				      CS_REQUEST_PLUGIN ("crystalspace.translator.standard", iTranslator),
				      // Translator component for transforming the XML data into usable translations
				      CS_REQUEST_PLUGIN ("crystalspace.translator.loader.xml", iLoaderPlugin),
				      CS_REQUEST_END ))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
	      "crystalspace.application.varedittest",
	      "Can't initialize plugins!");
    return false;
  }
  
  csRef<iLoaderPlugin> loader = csQueryRegistry<iLoaderPlugin> (object_reg);
  if (!loader) return false; //ReportError ("Could not locate the loader plugin");

  csRef<iTranslator> translator = csQueryRegistry<iTranslator> (object_reg);
  if (!translator) return false; //ReportError ("Could not locate the translator plugin");

  csRef<iVFS> vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) return false; //ReportError ("Could not locate the VFS plugin");

  string langPath ("/data/editor/lang/");
  string langFile ("de_DE.xml");
  
  csRef<iDataBuffer> path (vfs->GetRealPath (langPath.data ()));
  if (path.IsValid ()) {
    cout << "Path functioning: " << langPath << endl;
    cout << path->GetData () << endl;
  } else {
    cout << "Path is not active" << endl;
  }

  csRef<iStringArray> ls (vfs->FindFiles (langPath.data ()));
  cout << "Available language files in /lang/:" << endl;
  for (size_t i = 0; i < ls->GetSize (); i++)
    cout << ls->Get (i) << endl;

  cout << endl << "Opening translation: " << langFile << endl;

  string fullPath (langPath);
  fullPath += langFile;

  csRef<iFile> file (vfs->Open (fullPath.data () , VFS_FILE_READ));
  if (file.IsValid ()) {
    csRef<iDataBuffer> data (file->GetAllData ());
    csRef<iDocumentSystem> documentSystem (csQueryRegistry<iDocumentSystem>(object_reg ));
    csRef<iDocument> document (documentSystem->CreateDocument ());

    document->Parse (data->GetData ());
    csRef<iDocumentNode> root = document->GetRoot ();
    csRef<iBase> result = loader->Parse (root, 0, 0, translator);
      
    cout << dynamic_cast<iTranslator*>(&*result )->GetMsg ("Hello world") << endl;

  } else {
    cout << "Could not open file..." << fullPath << endl;
  }

  csRef<iStringArray> mnt (vfs->GetMounts ());

  // Create the main frame
  ModifiableTestFrame* frame = new ModifiableTestFrame (object_reg);
  
  // Setup the frame's CS stuff
  frame->Initialize ();

  // Add some test modifiable objects to the varedittest in order to check its functionality. 
  csRef<iModifiable> modifiable;

  modifiable.AttachNew (new csTestModifiable ("Bob", "murderer", 11));
  frame->AddModifiable (modifiable);

  modifiable.AttachNew (new csTestModifiable2 ("Jake", "garbage man", 0));
  frame->AddModifiable (modifiable);

  modifiable.AttachNew (new csTestModifiable ("Frodo", "part-time orc slayer", 2));
  frame->AddModifiable (modifiable);

  frame->Show ();

  return true;
}

int VarEditTestApp::OnExit ()
{
  csInitializer::DestroyApplication (object_reg );
  return 0;
}
