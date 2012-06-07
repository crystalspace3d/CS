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

// Macro magic to make csTestModifiable work
CS_IMPLEMENT_FOREIGN_DLL

csTestModifiable :: csTestModifiable(const char* name, const char* job, long itemCount) :
  name(name),
  job(job),
  itemCount(itemCount),
  awesome(false),
  floatThingy(123.55F),
  position(10, 12, 15),
  color(0.5f, 0.2f, 0.2f),
  //*
  // TODO: smart id generation
  id_name(csStringID(1000)),
  id_job(1001),
  id_color(1002),
  id_position(1003),
  id_itemCount(1004),
  id_awesome(1005),
  id_floatThingy(1006),
  id_testModifiable(1010),
  //*/
  scfImplementationType(this)
{
  position = csVector3(10.0f, 10.0f, 10.0f);
  color = csColor(0.8f, 0.1f, 0.1f);
  /*
  iStringSet strings = csQueryRegistryTagInterface<iStringSet>(r, "crystalspace.shared.stringset");
  id_name = strings.Request("test_name");
  id_job = strings->Request("test_job");
  id_color = strings->Request("test_color");
  id_testModifiable = strings->Request("test_dude");
  */
}

csTestModifiable :: ~csTestModifiable() { }

const csStringID csTestModifiable :: GetID() const {
  return id_testModifiable;
}

csPtr<iModifiableDescription> csTestModifiable :: GetDescription () const {

  csBasicModifiableDescription* description = new csBasicModifiableDescription();
  // Currently, the specialized modifiables no longer seem to be needed
  // The old implementation stored a direct pointer to the actual modifiable value (from the class that implemented iModifiable) and performed its sets/gets using that pointer. In this case (also with specialized string/long/float etc. modifiables the need for chained ifs in GetParameter and SetParameter functions would have been eliminated.
  description->Push(new csBasicModifiable("Name", "the dude's name", CSVAR_STRING, id_name));
  description->Push(new csBasicModifiable("Job", "the dude's jawb", CSVAR_STRING, id_job));
  description->Push(new csBasicModifiable("Item count", "How many items this guy has. Coming soon: constraint to prevent negative values!", CSVAR_LONG, id_itemCount));
  description->Push(new csBasicModifiable("Awesome", "Am I awesome, or what?", CSVAR_BOOL, id_awesome));
  description->Push(new csBasicModifiable("FloatThingy", "some float", CSVAR_FLOAT, id_floatThingy));
 // description->Push(new csBasicModifiable("Color", "my color", CSVAR_COLOR, id_color));
 // description->Push(new csBasicModifiable("Position", "spatial position of the unit", CSVAR_VECTOR3, id_position));

  return csPtr<iModifiableDescription>(description);
}

csVariant* csTestModifiable :: GetParameterValue(csStringID id) const {
  // These things could be fixed up through MACROS_MAYBE
  // Especially if we assume that each varName has a corresponding id_varName
  if(id == id_name) {
    return new csVariant(name);
  } else if(id == id_job) {
      return new csVariant(job);
  } else if(id == id_itemCount) {
      return new csVariant(itemCount);
  } else if(id == id_awesome) {
    return new csVariant(awesome);
  } else if(id == id_floatThingy) {
    return new csVariant(floatThingy);
  } else if(id == id_position) {
    return new csVariant(&position);
  } else if(id == id_color) {
    return new csVariant(&color);
  }

  return nullptr;
  // Maybe trigger an assertion here?
  // or change to the old version and return false
  
}

bool csTestModifiable ::SetParameterValue(csStringID id, const csVariant& value) {
  if(id == id_name) {
    name = value.GetString();
    return true;
  } else if(id == id_job) {
    job = value.GetString();
    return true;
  } else if(id == id_itemCount) {
    itemCount = value.GetLong();
    return true;
  } else if(id == id_awesome) {
    awesome = value.GetBool();
    return true;
  } else if(id == id_floatThingy) {
    floatThingy = value.GetFloat();
    return true;
  } else if(id == id_position) {
    position = value.GetVector3();
    return true;
  } else if(id == id_color) {
    color = value.GetColor();
    return true;
  }

  return false;
}

MyGraphNode1 :: MyGraphNode1 (GraphNodeFactory* _factory):GraphNode (_factory)
{
  float r = 0.4f;
  float g = 0.2f;
  float b = 0.7f;	
      
      //factory= _factory;
  /*
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
      */
 }

void PrintVariant (csVariant* variant)
{
	
  switch (variant->GetType ())
    {
    case CSVAR_LONG:
      printf ("[variant type: long value: %li]", variant->GetLong ());
      break;
    case CSVAR_FLOAT:
      printf ("[variant type: float value: %f]", variant->GetFloat ());
      break;
    case CSVAR_BOOL:
      printf ("[variant type: bool value: %i]", variant->GetBool ());
      break;
    case CSVAR_STRING:
      printf ("[variant type: string value: %s]", variant->GetString ());
      break;
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

      // TODO: rest of debug prints (if needed)
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
