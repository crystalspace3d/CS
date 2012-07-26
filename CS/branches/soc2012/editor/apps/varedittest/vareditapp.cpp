/*
  Copyright (C) 2011 Christian Van Brussel, Eutyche Mukuama, Dodzi de Souza
      Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

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
#include "cssysdef.h"
#include "vareditapp.h"
#include "testmodifiable.h"
#include "ivaria/translator.h"

#include "plugins/translator/standard/transldr_xml.h"
#include "iutil/document.h"
#include "plugins/documentsystem/xmlread/xr.h"
#include "plugins/filesys/vfs/vfs.h"
#include "iutil/stringarray.h"

IMPLEMENT_APP(VarEditTestApp);

bool VarEditTestApp::OnInit()
{
#if defined(wxUSE_UNICODE) && wxUSE_UNICODE
  char** csargv;
  csargv = (char**)cs_malloc(sizeof(char*) * argc);
  for(int i = 0; i < argc; i++) 
  {
    csargv[i] = strdup (wxString(argv[i]).mb_str().data());
  }
  object_reg = csInitializer::CreateEnvironment (argc, csargv);
#else
  object_reg = csInitializer::CreateEnvironment (argc, argv);
#endif

  // Load the needed plugins; so far, just the translator is needed
  if( ! csInitializer::RequestPlugins(  object_reg, 

            // Read the XML required for the name translations
            CS_REQUEST_PLUGIN("crystalspace.documentsystem.xmlread",  iDocumentSystem),
            // The translator itself
            CS_REQUEST_PLUGIN("crystalspace.translator.standard",     iTranslator),
            // Translator component for transforming the XML data into usable translations
            CS_REQUEST_PLUGIN("crystalspace.translator.loader.xml",   iLoaderPlugin),
            // We need a file system to read stuff from it
            CS_REQUEST_PLUGIN("crystalspace.kernel.vfs",              iVFS),

            CS_REQUEST_END ) ) 
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.varedittest",
      "Can't initialize plugins!");

    return false;
  }
  
  using namespace CS_PLUGIN_NAMESPACE_NAME(TransStd);

  csRef<iLoaderPlugin>  loaderRef( csQueryRegistry<iLoaderPlugin>(object_reg) );
  csRef<iTranslator>    translator( csQueryRegistry<iTranslator>( object_reg ));
  csRef<iVFS>           vfs( csQueryRegistry<iVFS>( object_reg ) );
  string                langPath("/data/editor/lang/");
  string                langFile("de_DE.xml");
  
  
  
  // Removed since the language files are now in data/editor/lang
  /*
  if(!vfs->Mount("/lang", "$^apps$/varedittest$/lang$/")) {
    cout << "Failed to mount" << endl;
  } else {
    cout << "Mounted data..." << endl;
    cout << "Opening lang file..." << endl;
    */

    csRef<iDataBuffer> path(vfs->GetRealPath(langPath.data()));
    if(path.IsValid()) {
      cout << "Path functioning: " << langPath << endl;
      cout << path->GetData() << endl;
    } else {
      cout << "Path is not active" << endl;
    }

    csRef<iStringArray> ls(vfs->FindFiles(langPath.data()));
    cout << "Available language files in /lang/:" << endl;
    for(size_t i = 0; i < ls->GetSize(); i++)
      cout << ls->Get(i) << endl;

    cout << endl << "Opening translation: " << langFile << endl;

    string fullPath(langPath);
    fullPath += langFile;

    csRef<iFile> file(vfs->Open(fullPath.data() , VFS_FILE_READ));
    if(file.IsValid()) {
      csRef<iDataBuffer> data(file->GetAllData());
      csRef<iDocumentSystem> documentSystem(csQueryRegistry<iDocumentSystem>( object_reg ));
      csRef<iDocument> document(documentSystem->CreateDocument());

      document->Parse(data->GetData());
      csRef<iDocumentNode> root = document->GetRoot();
      csRef<iBase> result = loaderRef->Parse(root, 0, 0, translator);
      
      cout << dynamic_cast<iTranslator*>( &*result )->GetMsg("Hello world") << endl;

    } else {
      cout << "Could not open file..." << fullPath << endl;
    }

  //}

  csRef<iStringArray> mnt(vfs->GetMounts());

  // Create the main frame
  ModifiableTestFrame* frame = new ModifiableTestFrame (object_reg);
  
  // Setup the frame's CS stuff
  frame->Initialize();

  // Add some test objects to the varedittest to check its functionality. 
  // csTestModifiable implements iModifiable
  frame->AddModifiable(new csTestModifiable("Bob", "murderer", 11, object_reg));
  frame->AddModifiable(new csTestModifiable("Jake", "garbage man", 0, object_reg));
  frame->AddModifiable(new csTestModifiable("Frodo", "part-time orc slayer", 2, object_reg));
  frame->Show();

	return true;
}

int VarEditTestApp::OnExit()
{
  csInitializer::DestroyApplication( object_reg );
  return 0;
}
