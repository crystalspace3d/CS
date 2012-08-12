#ifndef MODIFIABLE_IMPL_H
#define MODIFIABLE_IMPL_H

#include "iutil/modifiable.h"

/**
 * Implementation of some of the most common iModifiableParameter usage. 
 * Currently part of the particle editor space test code.
 */
class csBasicModifiableParameter : public scfImplementation1<csBasicModifiableParameter, iModifiableParameter> 
{
public:
  csBasicModifiableParameter(const char* name, const char* description, csVariantType type, csStringID id) :
      scfImplementationType (this),
      name(name),
      description(description),
      id(id),
      type(type)
  { }

  ~csBasicModifiableParameter() 
  {
    delete constraint;
    delete[] name;
    delete[] description;
  }

  csStringID GetID() const
  {
    return id;
  }

  const char* GetName() const
  {
    return name;
  }

  const char* GetDescription() const
  {
    return description;
  }

  csVariantType GetType() const 
  {
    return type;
  }

  void SetConstraint(iModifiableConstraint* constraint)
  {
    this->constraint = constraint;
  }

  const iModifiableConstraint* GetConstraint() const 
  {
    return constraint;
  }

private:
  const char* name;
  const char* description;
  csStringID id;
  csVariantType type;
  iModifiableConstraint* constraint;
};


/**
 * Basic implementation of iModifiableDescription, suitable for most uses. 
 * Simply holds a csRefArray of iModifiableParameter and implements 
 * GetParameterCount, GetParameter and GetParameterByIndex.
 */
class csBasicModifiableDescription : public scfImplementation1<csBasicModifiableDescription, iModifiableDescription>
{
public:
  csBasicModifiableDescription() :
      scfImplementationType (this),
      parameters(csArray<iModifiableParameter*>()) {}

  size_t GetParameterCount() const { return parameters.GetSize(); }

  const iModifiableParameter* GetParameter(csStringID id) const 
  {
    for (size_t i = 0; i < parameters.GetSize(); i++)
      if (parameters.Get(i)->GetID() == id)
        return parameters.Get(i);

    return nullptr;
  }

  const iModifiableParameter* GetParameterByIndex(size_t index) const 
  {
    return parameters[index];
  }

  void Push(iModifiableParameter* param) {
    parameters.Push(param);
  }

private:
  csArray<iModifiableParameter*> parameters;
};

#endif