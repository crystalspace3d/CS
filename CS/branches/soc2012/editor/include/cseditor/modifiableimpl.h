/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Barsan

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

#ifndef MODIFIABLE_IMPL_H
#define MODIFIABLE_IMPL_H

#include "iutil/modifiable.h"
#include "iutil/objreg.h"
#include "iutil/stringarray.h"
#include "csutil/refarr.h"
#include "csutil/regexp.h"
#include "csutil/stringarray.h"

// TODO: move in csutil

// TODO: using such macros is both tiresome and error-prone.
// Maybe use instead a XML description + a compiler for the code generation?
#define MODIF_DECLARE()\
  csRefArray<iModifiableListener> listeners;\
  virtual void GetParameterValue (size_t parameterIndex, size_t arrayIndex, csVariant& value) const {}\
  virtual bool SetParameterValue (size_t parameterIndex, size_t arrayIndex, const csVariant& value)\
  {return false;}\
  virtual bool PushParameterValue (size_t parameterIndex, const csVariant& value)\
  {return false;}\
  virtual bool DeleteParameterValue (size_t parameterIndex, size_t arrayIndex, const csVariant& value)\
  {return false;}\
  virtual void AddListener (iModifiableListener* listener) {listeners.Push (listener);}\
  virtual void RemoveListener (iModifiableListener* listener) {listeners.Delete (listener);}

#define MODIF_GETDESCRIPTION_BEGIN(name)		\
csPtr<iModifiableDescription> GetDescription (iObjectRegistry* object_reg) const\
{\
  csBasicModifiableDescription* description = new csBasicModifiableDescription (name);\
  csRef<csBasicModifiableParameter> parameter;\
  csRef<iModifiableConstraint> constraint;\
  csRef<iStringSet> strings =\
    csQueryRegistryTagInterface<iStringSet> (object_reg, "crystalspace.shared.stringset");\
  csStringID id;

#define MODIF_GETDESCRIPTION(type, id, name, desc)\
  parameter.AttachNew (new csBasicModifiableParameter (CSVAR_##type, strings->Request (id), name, desc));\
  description->Push (parameter);

#define MODIF_GETDESCRIPTION_C(type, id, name, desc, constr)\
  constraint.AttachNew (new constr);\
  parameter.AttachNew (new csBasicModifiableParameter (CSVAR_##type, strings->Request (id), name, desc, constraint));\
  description->Push (parameter);

#define MODIF_GETDESCRIPTION_CENUM_DECLARE()\
  {csRef<csConstraintEnum> constraint;\
  constraint.AttachNew (new csConstraintEnum);

#define MODIF_GETDESCRIPTION_CENUM_PUSH(value, desc)\
  constraint->PushValue (value, desc);

#define MODIF_GETDESCRIPTION_CENUM(type, id, name, desc)\
  parameter.AttachNew (new csBasicModifiableParameter (CSVAR_##type, strings->Request (id), name, desc, constraint));\
  description->Push (parameter);}

#define MODIF_GETDESCRIPTION_END()\
  return description;\
}

#define MODIF_GETDESCRIPTION_CHILD_BEGIN(name)\
  {\
  csRef<csBasicModifiableDescription> child;\
  child.AttachNew (new csBasicModifiableDescription (name));\
  description->Push (child);\
  {\
    csBasicModifiableDescription* description = child;

#define MODIF_GETDESCRIPTION_CHILD_END()\
  }}

#define MODIF_GETPARAMETERVALUE_BEGIN()\
void GetParameterValue (size_t index, csVariant& value) const\
{\
  switch (index)\
  {

#define MODIF_GETPARAMETERVALUE(id, type, val)	\
  case id:\
    value.Set##type (val);\
    break;

#define MODIF_GETPARAMETERVALUE_END()\
  default:\
    break;\
  }\
}

#define MODIF_SETPARAMETERVALUE_BEGIN()\
bool SetParameterValue (size_t index, const csVariant& value)\
{\
  switch (index)\
  {

#define MODIF_SETPARAMETERVALUE(id, type, val)\
  case id:\
    val = value.Get##type ();\
    break;

#define MODIF_SETPARAMETERVALUE_F(id, type, func)\
  case id:\
    func (value.Get##type ());\
    break;

#define MODIF_SETPARAMETERVALUE_ENUM(id, type, val, enumt)\
  case id:\
  val = (enumt) value.Get##type ();\
    break;

#define MODIF_SETPARAMETERVALUE_END()\
  default:\
    return false;\
  }\
  for (size_t i = 0; i < listeners.GetSize (); i++)\
    listeners[i]->ValueChanged (this, index);\
  return true;\
}

/**
 * Implementation of some of the most common iModifiableParameter usage. 
 * Stores the parameter's name, description, type, ID and an optional constraint.
 */
class csBasicModifiableParameter
: public scfImplementation1<csBasicModifiableParameter, iModifiableParameter> 
{
public:
  csBasicModifiableParameter (csVariantType type, csStringID id,
			      const char* name, const char* description,
			      iModifiableConstraint* constraint = nullptr)
    : scfImplementationType (this),
    id (id),
    name (name),
    description (description),
    type (type),
    constraint (constraint)
    {}

  virtual csStringID GetID () const
  { return id; }

  virtual const char* GetName () const
  { return name; }

  virtual const char* GetDescription () const
  { return description; }

  virtual csVariantType GetType () const 
  { return type; }

  virtual void SetConstraint (iModifiableConstraint* constraint)
  { this->constraint = constraint; }

  virtual const iModifiableConstraint* GetConstraint () const 
  { return constraint; }

private:
  csStringID id;
  csString name;
  csString description;
  csVariantType type;
  csRef<iModifiableConstraint> constraint;
};

/**
 * Basic implementation of iModifiableDescription, suitable for most uses. 
 * Simply holds a csRefArray of iModifiableParameter and implements 
 * GetParameterCount() and GetParameter().
 */
class csBasicModifiableDescription
: public scfImplementation1<csBasicModifiableDescription, iModifiableDescription>
{
public:
  csBasicModifiableDescription (const char* name) :
  scfImplementationType (this), name (name) {}

  /// Default implementation for iModifiableDescription::GetName()
  virtual const char* GetName () const
  { return name; }

  /// Default implementation for iModifiableDescription::GetParameterCount()
  virtual size_t GetParameterCount () const
  { return parameters.GetSize (); }

  /// Default implementation for iModifiableDescription::GetParameter(csStringID)
  virtual const iModifiableParameter* GetParameter (csStringID id) const 
  {
    for (size_t i = 0; i < parameters.GetSize (); i++)
      if (parameters.Get (i)->GetID () == id)
        return parameters.Get (i);

    for (size_t i = 0; i < children.GetSize (); i++)
    {
      const iModifiableParameter* parameter = children[i]->GetParameter (id);
      if (parameter) return parameter;
    }

    return nullptr;
  }

  /// Default implementation for iModifiableDescription::GetParameter(size_t)
  virtual const iModifiableParameter* GetParameter (size_t index) const
  {
    return GetParameterInternal (index);
  }

  /// Default implementation for iModifiableDescription::FindParameter()
  virtual size_t FindParameter (csStringID id) const
  {
    for (size_t i = 0; i < parameters.GetSize (); i++)
      if (parameters.Get (i)->GetID () == id)
        return i;

    for (size_t i = 0; i < children.GetSize (); i++)
    {
      size_t index = children[i]->FindParameter (id);
      if (index != (size_t) ~0) return index;
    }

    return (size_t) ~0;
  }

  // TODO: remove?
  inline void Push (iModifiableParameter* param)
  {
    parameters.Push (param);
  }

  /// Default implementation for iModifiableDescription::GetChildrenCount()
  virtual size_t GetChildrenCount () const
  {
    return children.GetSize ();
  }

  /// Default implementation for iModifiableDescription::GetChild()
  virtual const iModifiableDescription* GetChild (size_t index) const
  {
    return children[index];
  }

  // TODO: remove?
  inline void Push (iModifiableDescription* child)
  {
    children.Push (child);
  }

private:
  virtual const iModifiableParameter* GetParameterInternal (size_t& index) const
  {
    if (index < parameters.GetSize ())
      return parameters[index];

    index -= parameters.GetSize ();
    for (size_t i = 0; i < children.GetSize (); i++)
    {
      const iModifiableParameter* parameter = children[i]->GetParameter (index);
      if (parameter) return parameter;
    }

    return nullptr;
  }

private:
  csString name;
  csRefArray<iModifiableParameter> parameters;
  csRefArray<iModifiableDescription> children;
};


/**
 * Implements an enum constraint for a CSVAR_LONG iModifiable field. Contains a list of
 * long values that are members of the respective enum, as well as their string labels,
 * for displaying in a combo box.
 */
class csConstraintEnum : public scfImplementation1<csConstraintEnum, iModifiableConstraintEnum>
{
public:
  /**
   * Constructor
   */
  csConstraintEnum ()
    : scfImplementationType (this)
  {
  }  

  virtual bool Validate (const csVariant* variant) const
  {
    // No point performing the check - the value was selected from
    // a combo box. And nobody is trying to hack the editor either.
    return true;
  }

  //-- iModifiableConstraintEnum
  virtual size_t GetValueCount () const 
  {
    return labels.GetSize ();
  }

  virtual long GetValue (size_t index) const
  {
    return values.Get (index);
  }

  virtual iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_ENUM;
  }

  //-- csConstraintEnum
  virtual void PushValue (long value, const char* label)
  {
    values.Push (value);
    labels.Push (label);
  }

  virtual const char* GetLabel (size_t index) const
  {
    return labels[index];
  }

private:
  csStringArray labels;
  csArray<long> values;
};

/**
 * Implementation of iModifiableConstraintBounded. See its documentation for a bit more help.
 * Currently works for the following value types:
 * CSVAR_FLOAT, CSVAR_LONG, CSVAR_VECTOR2, CSVAR_VECTOR3, CSVAR_VECTOR4
 */
class csConstraintBounded : public scfImplementation1<csConstraintBounded, 
  iModifiableConstraintBounded>
{
public:
  /// Initializes this constraint with both a min and a max value
  csConstraintBounded (const csVariant& min, const csVariant& max)
    : scfImplementationType (this),
    min (new csVariant (min)),
    max (new csVariant (max))
    {
      CheckTypes ();
    }

  /// Initializes the constraint to have just a maximum value
  csConstraintBounded (const csVariant& max)
    : scfImplementationType (this),
    min (nullptr),
    max (new csVariant (max))
    {
      CheckTypes ();
    }

  ~csConstraintBounded ()
  {
    delete min;
    delete max;
  }

  virtual void SetMinimum (csVariant* min)
  {
    delete this->min;
    this->min = min;
    CheckTypes ();
  }

  virtual void SetMaximum (csVariant* max)
  {
    delete this->max;
    this->max = max;
    CheckTypes ();
  }

  //-- iModifiableConstraint

  virtual iModifiableConstraintType GetType () const 
  {
    return MODIFIABLE_CONSTRAINT_BOUNDED;
  }

  bool Validate (const csVariant* variant) const 
  {
    if (min != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type as the variant", 
		     variant->GetType () == min->GetType ());

    if (max != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type as the variant", 
		     variant->GetType () == max->GetType ());

    switch (variant->GetType ())
    {
    case CSVAR_FLOAT:
      if (max != nullptr) {
        if (variant->GetFloat () > max->GetFloat ()) return false;
      }
      if (min != nullptr) {
        if (variant->GetFloat () < min->GetFloat ()) return false;
      }
      break;

    case CSVAR_LONG:
      if (max != nullptr) {
        if (variant->GetLong () > max->GetLong ()) return false;
      }
      if (min != nullptr) {
        if (variant->GetLong () < min->GetLong ()) return false;
      }
      break;

    case CSVAR_VECTOR2:
      if (max != nullptr) {
        if (variant->GetVector2 ().x > max->GetVector2 ().x
            || variant->GetVector2 ().y > max->GetVector2 ().y
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector2 ().x < min->GetVector2 ().x
            || variant->GetVector2 ().y < min->GetVector2 ().y
	  )
          return false;
      }
      break;

    case CSVAR_VECTOR3:
      if (max != nullptr) {
        if (variant->GetVector3 ().x > max->GetVector3 ().x
	    || variant->GetVector3 ().y > max->GetVector3 ().y
	    || variant->GetVector3 ().z > max->GetVector3 ().z
	    || variant->GetVector3 ().z > max->GetVector3 ().z
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector3 ().x < min->GetVector3 ().x
	    || variant->GetVector3 ().y < min->GetVector3 ().y
	    || variant->GetVector3 ().z < min->GetVector3 ().z
	  )
          return false;
      }
      break;

    case CSVAR_VECTOR4:
      if (max != nullptr) {
        if (variant->GetVector4 ().x > max->GetVector4 ().x
	    || variant->GetVector4 ().y > max->GetVector4 ().y
	    || variant->GetVector4 ().z > max->GetVector4 ().z
	    || variant->GetVector4 ().w > max->GetVector4 ().w
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector4 ().x < min->GetVector4 ().x
	    || variant->GetVector4 ().y < min->GetVector4 ().y
	    || variant->GetVector4 ().z < min->GetVector4 ().z
	    || variant->GetVector4 ().w > max->GetVector4 ().w
          )
          return false;
      }

    default:
      // TODO
      break;
    }

    return true;
  }

  //-- iModifiableConstraintBounded

  bool HasMinimum () const 
  {
    return min != nullptr;
  }

  bool HasMaximum () const 
  {
    return max != nullptr;
  }

  csVariant& GetMinimum () const 
  {
    return *min;
  }

  csVariant& GetMaximum () const
  {
    return *max;
  }

private:
  csVariant *min, *max;

  /// Some helpful assertions to make sure no funny stuff is going on
  void CheckTypes () {
    if (min != nullptr && max != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type", min->GetType () == max->GetType ());

    csVariantType t;
    if (min != nullptr) {
      t = min->GetType ();
    } else if (max != nullptr) {
      t = max->GetType ();
    } else {
      CS_ASSERT_MSG ("Both constraints can't be null!", false);
      return;
    }

    CS_ASSERT_MSG ("Invalid type for comparing...",
		   t == CSVAR_FLOAT || t == CSVAR_LONG || t == CSVAR_VECTOR2 
		   ||  t == CSVAR_VECTOR3 || t == CSVAR_VECTOR4);
  }
};

/**
 * Attached to an iModifiable parameters, verifies that the value entered within
 * is always a VFS file, not a path or a directory.
 */
class csConstraintVfsFile : public scfImplementation1<csConstraintVfsFile, iModifiableConstraint>
{
public:
  csConstraintVfsFile () 
    : scfImplementationType (this)
  {
    // Should match anything that's got a special delimiter in it
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$-]");
  }

  virtual ~csConstraintVfsFile () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_FILE;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    csRegExpMatchError result = matcher->Match (variant->GetString ());
    // It's ok as long as not special delimiters are found
    return result == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};

/**
 * Attached to an iModifiable parameters, verifies that the value entered within
 * is always a VFS directory, relative or absolute.
 */
class csConstraintVfsDir : public scfImplementation1<csConstraintVfsDir, iModifiableConstraint>
{
public:
  csConstraintVfsDir ()
    : scfImplementationType (this)
  {
    // Just like the file matcher, only allows colons and forward slashes
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$/-]");
  }

  virtual ~csConstraintVfsDir () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_DIR;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    return matcher->Match (variant->GetString ()) == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};


/**
 * Attached to an iModifiable parameters, verifies that the value entered within
 * is always a full VFS path - a directory and a file, relative or absolute.
 */
class csConstraintVfsPath : public scfImplementation1<csConstraintVfsPath, iModifiableConstraint>
{
public:
  csConstraintVfsPath ()
    : scfImplementationType (this)
  {
    // Just like the dir regex
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$/-]");
  }

  virtual ~csConstraintVfsPath () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_PATH;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    return matcher->Match (variant->GetString ()) == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};

/**
 * Can validate a text entry, using minimum/ maximum length and/or a regular expression.
 */
class csConstraintTextEntry : public scfImplementation1<csConstraintTextEntry, iModifiableConstraint>
{
public:
  csConstraintTextEntry (long maxLength = -1, long minLength = -1, const char* regex = 0)
    : scfImplementationType (this),
    minLength (minLength),
    maxLength (maxLength)
    {
      if (regex)
	matcher = new csRegExpMatcher (regex);
      else
	matcher = nullptr;
    }

  virtual ~csConstraintTextEntry () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_TEXT_ENTRY;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    csString val = variant->GetString ();
/*
  if (minLength >= 0 && (val.Length () < minLength || val.Length () > maxLength))
  return false;

  if (maxLength >= 0 && (matcher != nullptr && matcher->Match (val) == csrxNoMatch))
  return false;
*/
    return true;
  }

private:
  csRegExpMatcher* matcher;
  long minLength, maxLength;
};

/**
 * Validates a CSVAR_LONG value, checking that its bits satisfy a given mask. When a bit
 * that's not part of the mask is set, the validation fails.
 */
class csConstraintBitMask : public scfImplementation1<csConstraintBitMask, iModifiableConstraint>
{
  csConstraintBitMask (long mask)
    : scfImplementationType (this),
    mask (mask)
    {
    }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_BITMASK;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT_MSG ("Can only apply bitmasks on integer-type values.", variant->GetType () == CSVAR_LONG);
    return (variant->GetLong () & (~mask)) == 0;
  }

private:
  long mask;
};

#endif
