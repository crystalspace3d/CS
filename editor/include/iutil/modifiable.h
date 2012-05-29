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
  SCF_INTERFACE(iModifiableConstraint, 1, 0, 0);

  virtual iModifiableConstraintType GetType() const = 0;
  // TODO: callbacks
};


//----------------- iModifiableConstraintBounded ---------------------

struct iModifiableConstraintBounded : public virtual iModifiableConstraint
{
  virtual bool HasMinimum () const = 0;
  virtual csVariant& GetMinimum() const = 0;

  virtual bool HasMaximum () const = 0;
  virtual csVariant& GetMaximum () const = 0;
};


//----------------- iModifiableConstraintEnum ---------------------

struct iModifiableConstraintEnum : public virtual iModifiableConstraint
{
  virtual size_t GetValueCount () const = 0;
  virtual csVariant& GetValue (size_t index) const = 0;
};


//----------------- iModifiableParameter ---------------------

struct iModifiableParameter : public virtual iBase
{
  SCF_INTERFACE(iModifiableParameter, 1, 0, 0);

  virtual csStringID GetID() const = 0;

  /**
   *   Returns char* entry for the parameter's name
   *   to be processed by the translator.
   */
  virtual char* GetName () const = 0;

  /**
   *   Returns char* entry for the parameter's textual description
   *   to be processed by the translator.
   */
  virtual char* GetDescription () const = 0;

  /**
   * Returns the type of this parameter                                                           */
  virtual csVariantType GetType () const = 0;

  /**
   * Gets this parameter's value.
   */
  virtual csVariant* GetParameterValue () const = 0;

  /**
   * Sets this parameter's value.
   */
  virtual bool SetParameterValue (const csVariant& value) = 0;

  /**
   * Returns this parameter's constraint.
   */
  virtual const iModifiableConstraint* GetConstraint() const = 0;
};

//----------------- iModifiableDescription ---------------------

struct iModifiableDescription : public virtual iBase
{
  SCF_INTERFACE(iModifiableDescription, 1, 0, 0);

  virtual size_t GetParameterCount () const = 0;

  virtual csPtr<iModifiableParameter> GetParameter (csStringID id) const = 0;
  virtual csPtr<iModifiableParameter> GetParameterByIndex (size_t index) const = 0;
};


//----------------- iModifiable ---------------------

struct iModifiable : public virtual iBase 
{
  SCF_INTERFACE(iModifiable, 1, 0 ,0);

  virtual const csStringID GetID () const = 0;
  virtual csPtr<iModifiableDescription> GetDescription () const = 0;

  virtual void GetParameterValue (csStringID id, const csVariant& value) const = 0;
  virtual bool SetParameterValue (csStringID id, const csVariant& value) = 0;
};

#endif // __MODIFIABLE_H__