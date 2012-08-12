/*
    Copyright (C) 2012 by Andrei Bârsan

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
#include "csutil/scf.h"
#include "translation.h"
#include "iutil/vfs.h"
#include "cstool/initapp.h"
#include "ieditor/context.h"
#include "imap/reader.h"
#include "iutil/document.h"
#include "iutil/stringarray.h"

#include <string>
#include <iostream>

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{
  SCF_IMPLEMENT_FACTORY (EditorTranslation)

  using namespace std;

  EditorTranslation::EditorTranslation (iBase* parent)
    : scfImplementationType (this, parent)
  {
  }

  EditorTranslation::~EditorTranslation () 
  {
  }

  bool EditorTranslation::Initialize(iEditor* editor)
  {
    this->editor = editor;
    object_reg = editor->GetContext ()->GetObjectRegistry ();

    // Prepare to load stuff
    csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (object_reg);

    // Load translator and document system plugins
    translator = csLoadPlugin<iTranslator>(pluginManager, "crystalspace.translator.standard");
    if (!translator)
    {
      csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.component.translation",
        "Could not find the translator plugin");
      return false;
    }

    csRef<iDocumentSystem> documentSystem =
      csLoadPlugin<iDocumentSystem>(pluginManager, "crystalspace.documentsystem.xmlread");
    if (!documentSystem)
    {
      csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.component.translation",
        "Could not find the translator XML plugin");
      return false;
    }

    csRef<iVFS> vfs = csQueryRegistry<iVFS>(object_reg);

    string langPath ("/data/editor/lang/");
    string langFile ("de_DE.xml");

    csRef<iDataBuffer> path(vfs->GetRealPath(langPath.data()));
    if (!path.IsValid())
    {
      csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.component.translation",
        csString().Format("Translation file path is not active: %s", langPath.data()));
      return false;
    }

#ifdef CS_DEBUG
    csRef<iStringArray> ls(vfs->FindFiles(langPath.data()));
    cout << "Available language files in /lang/:" << endl;
    for(size_t i = 0; i < ls->GetSize(); i++)
      cout << ls->Get(i) << endl;
#endif

    string fullPath(langPath);
    fullPath += langFile;

    csRef<iFile> file(vfs->Open(fullPath.data() , VFS_FILE_READ));
    if(file.IsValid())
    {
      csRef<iDataBuffer> data(file->GetAllData());
      csRef<iDocument> document(documentSystem->CreateDocument());

      document->Parse(data->GetData());
      csRef<iDocumentNode> root = document->GetRoot();

      csRef<iLoaderPlugin> loader = csLoadPlugin<iLoaderPlugin> (object_reg, "crystalspace.translator.loader.xml");
      csRef<iBase> result = loader->Parse(root, 0, 0, translator);

    } else {
      csReport(object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.component.translation",
        csString().Format("Could not open file: %s", fullPath.data()));
      return false;
    }

    return true;
  }

  void EditorTranslation::Update ()
  {
  }

  void EditorTranslation::Save (iDocumentNode* node) const
  {
  }

  bool EditorTranslation::Load (iDocumentNode* node)
  {
    return false;
  }

}
CS_PLUGIN_NAMESPACE_END(CSEditor)

/**
* 
 */
