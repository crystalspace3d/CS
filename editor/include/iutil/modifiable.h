#ifndef __MODIFIABLE_H__
#define __MODIFIABLE_H__

// iBase located here
#include "csutil/scf_interface.h"

// csStringID
#include "iutil/strset.h"

// csVariant and whatnot; long path required so
// the wrong one doesnt get picked up
#include "apps/varedittest/pluginconfig.h"


//----------------- iModifiableConstraintType ---------------------

enum iModifiableConstraintType
{
  MODIFIABLE_CONSTRAINT_BOUNDED = 0,
  MODIFIABLE_CONSTRAINT_ENUM,
  MODIFIABLE_CONSTRAINT_VFS_FILE,
  MODIFIABLE_CONSTRAINT_VFS_DIR,
  MODIFIABLE_CONSTRAINT_VFS_PATH,
  MODIFIABLE_CONSTRAINT_TEXT_ENTRY,
  MODIFIABLE_CONSTRAINT_TEXT_BLOB,
  MODIFIABLE_CONSTRAINT_BITMASK,
};


//----------------- iModifiableConstraint ---------------------

struct iModifiableConstraint : public virtual iBase
{
  iModifiableConstraintType GetType() const;
  // TODO: callbacks
};


//----------------- iModifiableConstraintBounded ---------------------

struct iModifiableConstraintBounded : public virtual iModifiableConstraint
{
  bool HasMinimum () const;
  csVariant& GetMinimum() const;

  bool HasMaximum () const;
  csVariant& GetMaximum () const;
};


//----------------- iModifiableConstraintEnum ---------------------

struct iModifiableConstraintEnum : public virtual iModifiableConstraint
{
  size_t GetValueCount () const;
  csVariant& GetValue (size_t index) const;
};


//----------------- iModifiableParameter ---------------------

struct iModifiableParameter : public virtual iBase
{
  csStringID GetID()  const;

  /**
   *   Returns char* entry for the parameter's name
   *   to be processed by the translator.
   */
  char* GetName () const;

  /**
   *   Returns char* entry for the parameter's description
   *   to be processed by the translator.
   */
  char* GetDescription () const;

  /**
   * Returns the type of this parameter                                                       */
  csVariantType GetType () const;

  /**
   * Returns this parameter's constraint.
   */
  const iModifiableConstraint* GetConstraint() const;
};

//----------------- iModifiableDescription ---------------------

struct iModifiableDescription : public virtual iBase
{
  size_t GetParameterCount () const;

  iModifiableParameter* GetParameter (csStringID id) const;
  iModifiableParameter* GetParameterByIndex (size_t index) const;
};


//----------------- iModifiable ---------------------

struct iModifiable : public virtual iBase 
{
  const csStringID GetID () const;

  csPtr<iModifiableDescription> GetDescription () const;

  void GetParameterValue (csStringID id, const csVariant& value) const;
  bool SetParameterValue (csStringID id, const csVariant& value);
};

#endif // __MODIFIABLE_H__