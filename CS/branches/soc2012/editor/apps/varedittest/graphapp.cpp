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

#include "graphapp.h"
#include "graphedit.h"
#include "graphnode.h"
//#include "iutil/pluginconfig.h"
#include "pluginconfig.h"


IMPLEMENT_APP(Graph_behaviourApp);

bool Graph_behaviourApp::OnInit()
{
	GraphNodeFactory* factory1 = new GraphNodeFactory ();
	factory1->SetName ("myfactory1"); 

 // Add a parameter description
	csOptionDescription description;
	description.name = "A number";
	description.description = "This is the description of parameter1 (type: long)";
  //description.id ignored?
	description.type = CSVAR_LONG;
	factory1->AddParameter (description);

  // Add a parameter description
	description.name = "A flag";
	description.description = "This is the description of parameter2 (type: bool)";
	description.type = CSVAR_BOOL;
	factory1->AddParameter (description);

  // Add a parameter description
	description.name = "A floating-point number";
	description.description = "This is the description of parameter3 (type: float)";
	description.type = CSVAR_FLOAT;
	factory1->AddParameter (description);

  // Add a parameter description
	description.name = "A string";
	description.description = "This is the description of parameter4 (type: string)";
	description.type = CSVAR_STRING;
	factory1->AddParameter (description);


	// Add a parameter description color
	description.name = "A color";
	description.description = "This is the description of parameter5 (type: csColor)";
	description.type = CSVAR_COLOR;
	factory1->AddParameter (description);

	description.name = "Vector2";
	description.description = "This is the description of parameter6 (type: csVector2)";
	description.type = CSVAR_VECTOR2;
	factory1->AddParameter (description);

	// Add a parameter description 
	description.name = "Vector3";
	description.description = "This is the description of parameter7 (type: csVector3)";
	description.type = CSVAR_VECTOR3;
	factory1->AddParameter (description);
	
	description.name = "Vector4";
	description.description = "This is the description of parameter8 (type: csVector4)";
	description.type = CSVAR_VECTOR4;
	factory1->AddParameter (description);
	

  // Keyval

  // Keyvallist

  // Matrix3

  // Transform
  
  // iBase

  // csArray
	
  MyGraphNode1* node1 = new MyGraphNode1 (factory1);

	Graph_behaviourFrame* frame = new Graph_behaviourFrame ();
	frame->SetGraphNode (node1);
	frame->Show();


	return true;
}

