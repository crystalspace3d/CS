/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Bârsan

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
#include "testmodifiable.h"
#include "csutil/event.h"
#include "cstool/initapp.h"
#include <csutil/ref.h>
#include <iutil/eventq.h>
#include <csutil/cscolor.h>

// Macro magic to make csTestModifiable work
//CS_IMPLEMENT_FOREIGN_DLL

csTestModifiable :: csTestModifiable(const char* name, const char* job, long itemCount, iObjectRegistry* object_reg) :
  name(name),
  job(job),
  object_reg(object_reg),
  itemCount(itemCount),
  awesome(false),
  floatThingy(123.55F),
  position(10, 12),
  color(0.5f, 0.2f, 0.2f),
  scfImplementationType(this)
{
  csRef<iStringSet> strings( csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset") );

  // We need to generate unique IDs for all the properties needing to be made 
  // modifiable through our system.
  id_name = strings->Request("test_name");
  id_job = strings->Request("test_job");
  id_color = strings->Request("test_color");
  id_position = strings->Request("test_position");
  id_itemCount = strings->Request("test_itemcount");
  id_awesome = strings->Request("test_awesome");
  id_floatThingy = strings->Request("test_float");
  id_testModifiable = strings->Request("test_dude");
}

csTestModifiable :: ~csTestModifiable() { }

const csStringID csTestModifiable :: GetID() const {
  return id_testModifiable;
}

csPtr<iModifiableDescription> csTestModifiable :: GetDescription () const {

  csBasicModifiableDescription* description = new csBasicModifiableDescription();
  
  // The old implementation stored a direct pointer to the actual modifiable value (from the class that implemented iModifiable) and performed its sets/gets using that pointer. In this case (also with specialized string/long/float etc. modifiables the need for chained ifs in GetParameter and SetParameter functions would have been eliminated.
  description->Push(new csBasicModifiableParameter("Name", "the dude's name", CSVAR_STRING, id_name));
  description->Push(new csBasicModifiableParameter("Job", "the dude's jawb", CSVAR_STRING, id_job));
  description->Push(new csBasicModifiableParameter("Item count", "How many items this guy has. Coming soon: constraint to prevent negative values!", CSVAR_LONG, id_itemCount));
  description->Push(new csBasicModifiableParameter("Awesome", "Am I awesome, or what?", CSVAR_BOOL, id_awesome));
  description->Push(new csBasicModifiableParameter("FloatThingy", "some float", CSVAR_FLOAT, id_floatThingy));
  description->Push(new csBasicModifiableParameter("Color", "my color", CSVAR_COLOR, id_color));
  description->Push(new csBasicModifiableParameter("Position", "spatial position of the unit", CSVAR_VECTOR2, id_position));

  return csPtr<iModifiableDescription>(description);
}

csVariant* csTestModifiable :: GetParameterValue(csStringID id) const {
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
    return new csVariant(position);
  } else if(id == id_color) {
    return new csVariant(color);
  }

  return nullptr;
  // Maybe trigger an assertion here?
  // or change to the old version and return false
}

bool csTestModifiable :: SetParameterValue(csStringID id, const csVariant& value) {

  // Broadcast a set event
  // TODO: maybe only do it if the assignment succeeds?
  csRef<iEventQueue> eq( csQueryRegistry<iEventQueue>( object_reg ) );
  csRef<iEventNameRegistry> nameReg( csQueryRegistry<iEventNameRegistry>( object_reg ) );
  csRef<iEvent> event( eq->CreateBroadcastEvent( nameReg->GetID("crystalspace.modifiable.param.set") ) );  
  eq->GetEventOutlet()->Broadcast(event->GetName());

  if(id == id_name) {
    // should we implement constraints here?
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
    position = value.GetVector2();
    return true;
  } else if(id == id_color) {
    color = value.GetColor();
    return true;
  }

  return false;
}

// Ancient debug function; might still be useful
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
