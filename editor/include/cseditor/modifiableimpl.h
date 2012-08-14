#ifndef MODIFIABLE_IMPL_H
#define MODIFIABLE_IMPL_H

#include "iutil/objreg.h"
#include "iutil/modifiable.h"
#include "iutil/stringarray.h"
#include "iutil/array.h"
#include "csutil/stringarray.h"

/**
 * Sets up the generation of modifiable property IDs. This should be followed 
 * with calls to the GENERATE_ID(varName) macro. It's a bit similar to how the 
 * wx event tables are defined, only these IDs should be generated in the constructor
 * of the iModifiable object (the Initialize method of an iComponent could also work).
 * 
 * Assumes a scoped pointer to the object registry called object_reg. For a custom
 * named one, use GENERATE_ID_START_EXT
 */
#define GENERATE_ID_START GENERATE_ID_START_REG(object_reg)

/**
 * Sets up the generation of modifiable property IDs. Takes in a custom name for the
 * pointer to the object registry.
 * 
 * See GENERATE_ID_START for more info.
 */
#define GENERATE_ID_START_REG(regName)  csRef<iStringSet> strings( csQueryRegistryTagInterface<iStringSet>(regName, "crystalspace.shared.stringset") )

/**
 * Syntactic sugar to make the generation of ids corresponding to object properties
 * a bit nicer. Should follow the GENERATE_ID_START macro.
 */
#define GENERATE_ID(varName) id_##varName = strings->Request(#varName)

/**
 * Used to help build a csBasicModifiableDescription.
 */
#define PUSH_PARAM(type, varName, name, desc)                                         \
  description->Push(new csBasicModifiableParameter(name, desc, type, id_##varName))   \

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
      type(type),
      constraint(nullptr)
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

  /**
   * Gets the parameter that corresponds to the given id. Note that it's not a 
   * direct pointer to the modifiable object's member, but rather a copy. A call
   * to the iModifiable object's SetParameterValue(id, variant) should be made
   * to actually set that property's value.
   */
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

/**
 * Basic implementation of the iModifiableConstraint interface. Its specialized
 * sub-classes are recommended for use.
 */
class csConstraint : public scfImplementation1<csConstraint, iModifiableConstraint>
{
public:
  virtual ~csConstraint()
  {
  }

  iModifiableConstraintType GetType() const
  {
    return type;
  }

protected:
  csConstraint(iModifiableConstraintType type)
    : scfImplementation1(this),
      type(type)
  {
  }

private:
  iModifiableConstraintType type;
};

/**
 * Implements an enum constraint for a CSVAR_LONG iModifiable field. Contains a list of
 * long values that are members of the respective enum, as well as their string labels,
 * for displaying in a combo box.
 */
class csEnumConstraint : public csConstraint
{
public:
  /**
   * Doesn't initialize the list. Requires calls to PushValue(value, label) to
   * populate the valid fields.
   */
  csEnumConstraint()
    : csConstraint(MODIFIABLE_CONSTRAINT_ENUM)
  {
    labels = new csStringArray;
    values = new csArray<long>;
  }  

  /**
   * \remark Takes ownership of the arrays.  
   */
  csEnumConstraint(csStringArray* labels, csArray<long>* values)
    : csConstraint(MODIFIABLE_CONSTRAINT_ENUM)
  {
    CS_ASSERT_MSG ("Number of labels must match number of values.",
      labels->GetSize() == values->GetSize());

    this->labels = labels;
    this->values = values;
  }

  ~csEnumConstraint()
  {
    delete labels;
    delete values;
  }

  void PushValue(long value, const char* label)
  {
    values->Push(value);
    labels->Push(label);
  }

  csStringArray* GetLabels() const
  {
    return labels;
  }

  csArray<long>* GetValues() const
  {
    return values;
  }

private:
  csStringArray* labels;
  csArray<long>* values;
};

#endif