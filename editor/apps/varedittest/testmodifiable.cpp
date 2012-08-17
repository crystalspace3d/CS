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
  // We need to generate unique IDs for all the properties needing to be made 
  // modifiable through our system.
  GENERATE_ID_START;
  GENERATE_ID(name);
  GENERATE_ID(job);
  GENERATE_ID(color);
  GENERATE_ID(position);
  GENERATE_ID(itemCount);
  GENERATE_ID(awesome);
  GENERATE_ID(floatThingy);
  GENERATE_ID(floatArray);
  GENERATE_ID(testModifiable);

  floatArray.Push(40.0f);
  floatArray.Push(41.0f);
  floatArray.Push(42.0f);
}

csTestModifiable :: ~csTestModifiable() { }

const csStringID csTestModifiable :: GetID() const {
  return id_testModifiable;
}

csPtr<iModifiableDescription> csTestModifiable :: GetDescription () const {

  csBasicModifiableDescription* description = new csBasicModifiableDescription();
  
  PUSH_PARAM(CSVAR_STRING, name, "Name", "the dude's name");
  PUSH_PARAM(CSVAR_STRING, job, "Job", "the dude's jawb");
  PUSH_PARAM(CSVAR_LONG, itemCount, "Item count", "How many items this guy has (unrelated to the array)."); 
  PUSH_PARAM(CSVAR_BOOL, awesome, "Awesome", "Am I awesome, or what?");
  PUSH_PARAM(CSVAR_FLOAT, floatThingy, "FloatThingy", "some float");
  PUSH_PARAM(CSVAR_ARRAY, floatArray, "Array of floats", "testing arrays being editable in the propgrid");
  PUSH_PARAM(CSVAR_COLOR, color, "Color", "my color");
  PUSH_PARAM(CSVAR_VECTOR2, position, "Position", "spatial position of the unit");

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
  } else if(id == id_floatArray) {
    return new csVariant(MakeVariantArray(floatArray));
  } else if(id == id_position) {    
    return new csVariant(position);
  } else if(id == id_color) {
    return new csVariant(color);
  }

  return nullptr;
}

bool csTestModifiable :: SetParameterValue(csStringID id, const csVariant& value) {

  // Broadcast a varriant set event
  BROADCAST_SET_EVENT();

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
  } else if(id == id_floatArray) {
    floatArray = GetRegularArray<float>(value.GetArray());
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
