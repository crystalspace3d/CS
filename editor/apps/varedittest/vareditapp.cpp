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
#include "testModifiable.h"

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
