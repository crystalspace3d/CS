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
#include "graphnode.h"
//#include "iutil/pluginconfig.h"
#include "pluginconfig.h"
#include <csutil/cscolor.h>

MyGraphNode1 :: MyGraphNode1 (GraphNodeFactory* _factory):GraphNode (_factory)
{
  float r = 0.4f;
  float g = 0.2f;
  float b = 0.7f;
		
      
      //factory= _factory;
      GetParameter (0)->SetLong (1234567);
      GetParameter (1)->SetBool (true);
      GetParameter (2)->SetFloat (0.5f);
      GetParameter (3)->SetString ("testval1");
      GetParameter (4)->SetColor(csColor(r,g,b)); 
      GetParameter (5)->SetVector2(csVector2(1.0f, 0.96f));
      GetParameter (6)->SetVector3(csVector3(1.0f, 0.3f,0.8f));
      GetParameter (7)->SetVector4(csVector4(0.2f,0.3f,0.8f,0.10f));
      // GetParameter (8)->SetVFSPath("home");
      SetName("bouton1");

 }

void PrintVariant (csVariant* variant)
{
	
  switch (variant->GetType ())
    {
      /*
    case CSVAR_LONG:
      printf ("[variant type: long value: %li]", variant->GetLong ());
      break;
      */
    case CSVAR_FLOAT:
      printf ("[variant type: float value: %f]", variant->GetFloat ());
      break;
    case CSVAR_BOOL:
      printf ("[variant type: bool value: %i]", variant->GetBool ());
      break;
      /*
    case CSVAR_CMD:
      printf ("[variant type: command]");
      break;
      */
    case CSVAR_STRING:
      printf ("[variant type: string value: %s]", variant->GetString ());
      break;
      /*
    case CSVAR_VFSPATH:
      printf ("[variant type: vfspath value: %s]", variant->GetVFSPath ());
      break;
      */
    case CSVAR_VECTOR3 :
      printf ("[variant type: vector3 value: %f ; %f ; %f]", variant->GetVector3 ().x,variant->GetVector3 ().y,variant->GetVector3 ().z);
      break;
    case CSVAR_VECTOR2 :
      printf ("[variant type: vector2 value: %f ; %f]", variant->GetVector2 ().x,variant->GetVector2 ().y);
      break;
    case CSVAR_VECTOR4 :
      printf ("[variant type: vector4 value: %f ; %f ; %f ; %f]", variant->GetVector4 ().x,variant->GetVector4 ().y,variant->GetVector4 ().z,variant->GetVector4 ().w);
      break;
    case CSVAR_COLOR :
      csColor col = variant->GetColor();
      printf ("[variant type: color value: %f ; %f ; %f]", col[0], col[1], col[2]);
      break;
    }
}

void MyGraphNode1:: UpdateParameter (size_t index, csVariant* oldValue, csVariant* newValue)
    {
      parameters[index] = *newValue;
      printf ("MyGraphNode1::UpdateParameter %li\n", index);
      printf ("old value: ");
      PrintVariant (oldValue);
      printf ("\n");
      printf ("new value: ");
      PrintVariant (newValue);
      printf ("\n");
    }
  

 MyGraphNode1 ::~MyGraphNode1(){
}
