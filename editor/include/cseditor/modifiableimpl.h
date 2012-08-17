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
#define GENERATE_ID_START() GENERATE_ID_START_REG(object_reg)

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
 * Quick helper macro to allow classes who implement iModifiable to broadcas
 * the required set event when SetParameterValue is called
 */
#define BROADCAST_SET_EVENT() \
  csRef<iEventQueue> eq( csQueryRegistry<iEventQueue>( object_reg ) );  \
csRef<iEventNameRegistry> nameReg( csQueryRegistry<iEventNameRegistry>( object_reg ) ); \
csRef<iEvent> event( eq->CreateBroadcastEvent( nameReg->GetID("crystalspace.modifiable.param.set") ) );  \
eq->GetEventOutlet()->Broadcast(event->GetName());  \

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
 * Implements an enum constraint for a CSVAR_LONG iModifiable field. Contains a list of
 * long values that are members of the respective enum, as well as their string labels,
 * for displaying in a combo box.
 */
class csEnumConstraint : public scfImplementation1<csEnumConstraint, iModifiableConstraintEnum>
{
public:
  /**
   * Doesn't initialize the lists. Requires calls to PushValue(value, label) to
   * populate the valid fields.
   */
  csEnumConstraint()
      : scfImplementation1(this)
  {
    labels = new csStringArray;
    values = new csArray<long>;
  }  

  /**
   * \remark Takes ownership of the arrays.  
   */
  csEnumConstraint(csStringArray* labels, csArray<long>* values)
    : scfImplementation1(this)
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


  bool Validate(const csVariant& variant) const
  {
    // No point performing the check - the value was selected from
    // a combo box. And nobody is trying to hack the editor either.
    return true;
  }

  //-- iModifiableConstraintEnum
  size_t GetValueCount () const 
  {
    return labels->GetSize();
  }

  long GetValue (size_t index) const
  {
    return values->Get(index);
  }


  iModifiableConstraintType GetType() const
  {
    return MODIFIABLE_CONSTRAINT_ENUM;
  }

  //-- csEnumConstraint
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

/**
 * Constraint that forces a variant to either stay under a certain value, over
 * a certain value, or between two values.
 */
class csBoundedConstraint : public scfImplementation1<csBoundedConstraint, 
                                                      iModifiableConstraintBounded>
{
public:
  /// Initializes this constraint with both a min and a max value
  csBoundedConstraint(csVariant& min, csVariant& max)
    : scfImplementation1(this),
      min(new csVariant(min)),
      max(new csVariant(max))
  {
    CheckTypes();
  }

  /// Initializes the constraint to have just a maximum value
  csBoundedConstraint(csVariant& max)
    : scfImplementation1(this),
      min(nullptr),
      max(new csVariant(max))
  {
    CheckTypes();
  }

  ~csBoundedConstraint()
  {
    delete min;
    delete max;
  }

  void SetMinimum(csVariant& min)
  {
    delete this->min;
    this->min = new csVariant(min);
    CheckTypes();
  }

  void SetMaximum(csVariant& max)
  {
    delete this->max;
    this->max = new csVariant(max)
    CheckTypes();
  }

  //-- iModifiableConstraint

  iModifiableConstraintType GetType() const 
  {
    return MODIFIABLE_CONSTRAINT_BOUNDED;
  }

  bool Validate(const csVariant& variant) 
  {
    if(min != nullptr)
      CS_ASSERT_MSG("Bounds must be of the same type as the variant", 
        variant.GetType() == min->GetType());

    if(max != nullptr)
      CS_ASSERT_MSG("Bounds must be of the same type as the variant", 
      variant.GetType() == max->GetType());
  }

  //-- iModifiableConstraintBounded

  bool HasMinimum() const 
  {
    return min != nullptr;
  }

  bool HasMaximum() const 
  {
    return max != nullptr;
  }

  csVariant& GetMinimum() const 
  {
    return *min;
  }

  csVariant& GetMaximum() const
  {
    return *max;
  }

private:
  csVariant *min, *max;

  void CheckTypes() {
    if(min != nullptr && max != nullptr)
      CS_ASSERT_MSG("Bounds must be of the same type", min->GetType() == max->GetType());

    csVariantType t;
    if(min != nullptr) {
      t = min->GetType();
    } else if(max != nullptr) {
      t = max->GetType();
    } else {
      CS_ASSERT_MSG("Both constraints can't be null!", false);
      return;
    }

    CS_ASSERT_MSG("Invalid type for comparing...",
      t == CSVAR_FLOAT || t == CSVAR_LONG || t == CSVAR_VECTOR2 
      ||  t == CSVAR_VECTOR3 || t == CSVAR_VECTOR4 || t == CSVAR_FLOAT );
  }
};

#endif