#ifndef __MODIFIABLE_H__
#define __MODIFIABLE_H__

// iBase located here
#include "csutil/scf_interface.h"

// csStringID
#include "iutil/strset.h"

// csVariant and whatnot; long path required so the wrong one doesn't get picked up
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
   * Returns char* entry for the parameter's name
   * to be processed by the translator.
   */
  virtual const char* GetName () const = 0;

  /**
   * Returns char* entry for the parameter's textual description
   * to be processed by the translator.
   */
  virtual const char* GetDescription () const = 0;

  /**
   * Returns the type of this parameter                                                      */
  virtual csVariantType GetType () const = 0;

  /**
   * Returns this parameter's constraint.
   */
  virtual const iModifiableConstraint* GetConstraint() const = 0;
};

//----------------- iModifiableDescription ---------------------

/**
 * The descriptor of an iModifiable object. Contains ways to expose and access its
 * properties.
 */
struct iModifiableDescription : public virtual iBase
{
  SCF_INTERFACE(iModifiableDescription, 1, 0, 0);

  /**
   * Returns the number of editable parameters of the current object.
   */
  virtual size_t GetParameterCount () const = 0;

  /**
   * Returns a parameter based on its csStringID. \see csStringID
   */
  virtual const iModifiableParameter* GetParameter (csStringID id) const = 0;

  /**
   * Returns a parameter based on its index in the iModifiableDescription's list of
   * parameters. Fields should be added in the same order to the GUI, so their position
   * in the GUI corresponds with their position in the list.
   */
  virtual const iModifiableParameter* GetParameterByIndex (size_t index) const = 0;
  
  /**
   * Adds another iModifiableParameter to the current description. This parameter 
   * will therefore be exposed to external editors.
   */
  virtual void Push(iModifiableParameter* param) = 0;
};


//----------------- iModifiable ---------------------

/**
 * Core interface of the cseditor framework. This interface should be implemented
 * by any object that the developer would like to edit in an outside editor. An 
 * iModifiableDescription object should also be provided to allow a listing of said
 * properties, helping programs such as cseditor generate GUIs to allow the visual
 * editing of those attributes.
 *
 * \see iModifiableDescription
 * \see csBasicModifiableDescription 
 * \see iModifiableParameter
 * \see csBasicModifiableParameter
 */
struct iModifiable : public virtual iBase 
{
  SCF_INTERFACE(iModifiable, 1, 0 ,0);

  /**
   * Returns this object's unique ID. 
   */
  virtual const csStringID GetID () const = 0;

  /**
   * Returns this object's description.
   */
  virtual csPtr<iModifiableDescription> GetDescription () const = 0;

  /**
   * Returns the value of one of this object's parameters. 
   * \ remark Each modifiable property should have its own id in
   * order to be properly accessible.
   */
  virtual csVariant* GetParameterValue (csStringID id) const = 0;

  /**
   * Sets a value for the parameter with the unique identifier id. 
   * \ remark Each modifiable property should have its own id in 
   * order to be properly accessible. 
   *
   * \return true if the value can be set, false if a property with
   * that id couldn't be found
   */
  virtual bool SetParameterValue (csStringID id, const csVariant& value) = 0;
};

#endif // __MODIFIABLE_H__
