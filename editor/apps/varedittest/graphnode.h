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
 * Concrete implementation of a modifiable string property.
 */
class iModifiableString : public scfImplementation1<iModifiableString, iModifiableParameter>
{
public:
  iModifiableString() : scfImplementation1(this) {
    this->name = new char();
    this->description = new char();
    this->value = new csVariant("");
  }

  iModifiableString(const char* name, const char* description, csVariant* value) : scfImplementation1(this) 
  {
    this->name = new char(*name);
    this->description = new char(*description);
    this->value = value;
  }

  csStringID GetID() const {
    return id;
  }

  char* GetName() const {
    return name;
  }

  char* GetDescription() const {
    return description;
  }

  csVariantType GetType() const {
    return CSVAR_STRING;
  }

  csVariant* GetParameterValue() const {
    return value;
  }

  bool SetParameterValue(const csVariant& value)
  {
    this->value->SetString(value.GetString());
    return true;
  }

  const iModifiableConstraint* GetConstraint() const 
  {
    return 0;
  }

private:
  char* name;
  char* description;
  csStringID id;
  csVariant* value;
};


//*
class testDudeDescription : public scfImplementation1<testDudeDescription, iModifiableDescription>
{
public:
  testDudeDescription(shared_ptr< name)
    : scfImplementation1(this)
  {
    this->name = name;
    parameters = csRefArray<iModifiableParameter>();
    csVariant *nameVariant = new csVariant(name);
    parameters.Push(csRef<iModifiableParameter>( new iModifiableString("name", "the dude's name", nameVariant)));
    
  }

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
    return parameters[index];
  }

private:
  csRefArray<iModifiableParameter> parameters;
  char* name;
};
//*/

// Test dude; It's going to have some properties exposed to the GUI generation 
// by iModifiable.
//*
class csTestModifiable : public scfImplementation1<csTestModifiable, iModifiable>
{
public:
  csTestModifiable();
  virtual ~csTestModifiable();

  const csStringID GetID() const;
  csPtr<iModifiableDescription> GetDescription () const;

  void GetParameterValue (csStringID id, const csVariant& value) const;
  bool SetParameterValue (csStringID id, const csVariant& value);

private:
  csVector3 position;
  csColor color;
  csString name;

  csStringID testID;
  csRef<iModifiableDescription> description;
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
