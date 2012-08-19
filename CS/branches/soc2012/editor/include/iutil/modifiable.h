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
  MODIFIABLE_CONSTRAINT_BITMASK
};


//----------------- iModifiableConstraint ---------------------

/**
 * Useful for validating various iModifiable parameters.
 * It's generally attached to an iModifiableProperty in an iModifiable object's 
 * GetDescription() method.
 *
 * \see iModifiable
 * \see PUSH_PARAM_CONSTRAINT for a helper macro
 */
struct iModifiableConstraint : public virtual iBase
{
  SCF_INTERFACE(iModifiableConstraint, 1, 0, 0);

  virtual iModifiableConstraintType GetType() const = 0;
  
  /**
   * Takes in a const csVariant* that it validates, according to the rules of a specific
   * constraint type. For instance, a long value could be limited by a bounded constraint
   * so that it stays between certain limits.
   *
   * Other types, such as enum constraints, don't actually use this function, since they
   * also have a helper role in creating a GUI that validates itself. For instance, the
   * enum constraint generate combo boxes with the allowed values, so it's certain that
   * any value the user might pick is valid.
   */
  virtual bool Validate(const csVariant* variant) const = 0;
};


//----------------- iModifiableConstraintBounded ---------------------

/**
 * Constraint that forces a variant to either stay under a certain value, over
 * a certain value, or between two values. 
 */
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
  virtual long GetValue (size_t index) const = 0;
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
   * Returns the type of this parameter
   */
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
 * 
 * \see csModifiableDescription for an implementation
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
 * \remark Triggers a crystalspace.modifiable.param.set event when a parameter value
 * gets set
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
   * \remark Each modifiable property should have its own id in
   * order to be properly accessible.
   */
  virtual csVariant* GetParameterValue (csStringID id) const = 0;

  /**
   * Sets a value for the parameter with the unique identifier id. 
   * \remark Each modifiable property should have its own id in 
   * order to be properly accessible. 
   *
   * \return true if the value can be set, false if a property with
   * that id couldn't be found
   */
  virtual bool SetParameterValue (csStringID id, const csVariant& value) = 0;
};

#endif // __MODIFIABLE_H__
