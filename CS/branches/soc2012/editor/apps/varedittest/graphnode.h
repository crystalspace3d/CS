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
#ifndef MYGRAPHNODE1_H
#define MYGRAPHNODE1_H

#include "graphedit.h"

// Stuff needed for the test iModifiable dude
#include "iutil/modifiable.h"
#include "csutil/refarr.h"

/**
 *  Implementation of some of the most common iModifiableParameter usage. Parent class of most basic parameters
 */
class csBasicModifiable : public scfImplementation1<csBasicModifiable, iModifiableParameter> 
{
public:
  csBasicModifiable(const char* name, const char* description, csVariantType type, csStringID id) :
      name(name),
      description(description),
      type(type),
      scfImplementation1(this),
      id(id)
  { }

  ~csBasicModifiable() {
    // delete value;
  }

  csStringID GetID() const {
    return id;
  }

  const char* GetName() const {
    return name;
  }

  const char* GetDescription() const {
    return description;
  }

  csVariantType GetType() const {
    return type;
  }

  const iModifiableConstraint* GetConstraint() const 
  {
    // Constraints not yet implemented
    return 0;
  }

private:
  const char* name;
  const char* description;
  csStringID id;
  csVariantType type;
};

/**
 * Concrete implementation of a modifiable string property.
 */
// class csModifiableString : public csBasicModifiable
// {
// public:
//   /*
//   I don't think this should really be used. Best let the iModifiable system handle the csVariants
//   iModifiableString(const char* name, const char* description, csVariant* value) :
//       csBasicModifiable(name, description, value)
//   { }
//   */
// 
//   csModifiableString(const char* name, const char* description, /* const scfString& value ,*/ csStringID id) :
//       csBasicModifiable(name, description, /* new csVariant(value),*/ id)
//   { }  
// 
// };

/**
 * Basic implementation of iModifiableDescription, suitable for most uses. Simply holds a csRefArray of iModifiableParameter and implements GetParameterCount, GetParameter and GetParameterByIndex.
 */
class csBasicModifiableDescription : public scfImplementation1<csBasicModifiableDescription, iModifiableDescription>
{
public:
  csBasicModifiableDescription() :
      parameters(csRefArray<iModifiableParameter>()),
      scfImplementation1(this) { }

  size_t GetParameterCount() const { return parameters.GetSize(); }

  csPtr<iModifiableParameter> GetParameter(csStringID id) const 
  {
    for(size_t i = 0; i < parameters.GetSize(); i++)
      if(parameters.Get(i)->GetID() == id)
        return parameters.Get(i);

    return 0;
  }

  csPtr<iModifiableParameter> GetParameterByIndex(size_t index) const 
  {
    return csPtr<iModifiableParameter>(parameters[index]);
  }

  void Push(iModifiableParameter* param) {
    parameters.Push(csRef<iModifiableParameter>(param));
  }

private:
  csRefArray<iModifiableParameter> parameters;
};

// Test dude; It's going to have some properties exposed to the GUI generation 
// by iModifiable.
//*
class csTestModifiable : public scfImplementation1<csTestModifiable, iModifiable>
{
public:
  csTestModifiable(const char* name, const char* job, long itemCount);
  virtual ~csTestModifiable();
  
  const csStringID GetID() const;
  csPtr<iModifiableDescription> GetDescription () const;

  csVariant* GetParameterValue (csStringID id) const;
  bool SetParameterValue (csStringID id, const csVariant& value);

private:
  csStringID id_testModifiable;
  csStringID id_name, id_job, id_position, id_color, id_itemCount, id_awesome, id_floatThingy;

  long itemCount;
  bool awesome;
  float floatThingy;
  csVector3 position;
  csColor color;
  scfString name;
  scfString job;
};
//*/

class MyGraphNode1 : public GraphNode
  {
    public :

    MyGraphNode1(GraphNodeFactory* _factory) ;

    ~MyGraphNode1();

    void UpdateParameter (size_t index, csVariant* oldValue, csVariant* newValue);
};

#endif// MYGRAPHNODE1_H
