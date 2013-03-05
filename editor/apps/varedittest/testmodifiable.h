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
#ifndef __TEST_MODIFIABLE_H__
#define __TEST_MODIFIABLE_H__

#include "cseditor/modifiableimpl.h"
#include "csutil/refarr.h"

// Test modifiable object for the GUI generation.
// This first version is a full implementation
class csTestModifiable
: public scfImplementation1<csTestModifiable, iModifiable>
{
public:
  csTestModifiable (const char* name, const char* job, long itemCount);
  virtual ~csTestModifiable ();
  
  //-- iModifiable
  //virtual const csStringID GetID () const;
  virtual csPtr<iModifiableDescription> GetDescription (iObjectRegistry* object_reg) const;

  virtual void GetParameterValue (size_t parameterIndex, csVariant& value) const;
  virtual void GetParameterValue (size_t parameterIndex, size_t arrayIndex, csVariant& value) const;

  virtual bool SetParameterValue (size_t parameterIndex, const csVariant& value);
  virtual bool SetParameterValue (size_t parameterIndex, size_t arrayIndex, const csVariant& value);
  virtual bool PushParameterValue (size_t parameterIndex, const csVariant& value);
  virtual bool DeleteParameterValue (size_t parameterIndex, size_t arrayIndex, const csVariant& value);

  virtual void AddListener (iModifiableListener* listener);
  virtual void RemoveListener (iModifiableListener* listener);

private:
  csString name;
  csString job;
  long itemCount;
  bool awesome;
  float floatThingy;
  csVector2 position;
  csColor color;
  csString vfsFile;
  csString vfsDir;
  csString vfsPath;

  csRefArray<iModifiableListener> listeners;
};

// This second version of the modifiable object is using the dedicated macro's
class csTestModifiable2
: public scfImplementation1<csTestModifiable2, iModifiable>
{
public:
  csTestModifiable2 (const char* name, const char* job, long itemCount);
  virtual ~csTestModifiable2 () {}
  
  //-- iModifiable
  MODIF_DECLARE ();

  MODIF_GETDESCRIPTION_BEGIN ("Player stats");
  MODIF_GETDESCRIPTION (STRING, "NAME", "Name", "The dude's name");
  MODIF_GETDESCRIPTION (STRING, "JOB", "Job", "The dude's jawb");
  MODIF_GETDESCRIPTION (LONG, "ITEM", "Item count", "How many items this guy has");
  MODIF_GETDESCRIPTION (BOOL, "AWESOME", "Awesome", "Am I awesome, or what?");
  MODIF_GETDESCRIPTION_CHILD_BEGIN ("Other stats");
    MODIF_GETDESCRIPTION_C (FLOAT, "FLOATY", "FloatThingy", "Some float",
			    csConstraintBounded (csVariant (-100.0f), csVariant (500.0f)));
    MODIF_GETDESCRIPTION (VECTOR2, "POSITION", "Position", "Spatial position of the unit");
    MODIF_GETDESCRIPTION (COLOR, "COLOR", "Color", "My color");
    MODIF_GETDESCRIPTION_CHILD_BEGIN ("Sub-other stats");
      MODIF_GETDESCRIPTION_C (STRING, "FILE", "VFS file", "A VFS file name", csConstraintVfsFile);
      MODIF_GETDESCRIPTION_C (STRING, "DIR", "VFS dir", "A VFS dir name", csConstraintVfsDir);
      MODIF_GETDESCRIPTION_C (STRING, "PATH", "VFS path", "A VFS path name", csConstraintVfsPath);
    MODIF_GETDESCRIPTION_CHILD_END ();
  MODIF_GETDESCRIPTION_CHILD_END ();
  MODIF_GETDESCRIPTION_END ();

  MODIF_GETPARAMETERVALUE_BEGIN ();
  MODIF_GETPARAMETERVALUE (0, String, name);
  MODIF_GETPARAMETERVALUE (1, String, job);
  MODIF_GETPARAMETERVALUE (2, Long, itemCount);
  MODIF_GETPARAMETERVALUE (3, Bool, awesome);
  MODIF_GETPARAMETERVALUE (4, Float, floatThingy);
  MODIF_GETPARAMETERVALUE (5, Vector2, position);
  MODIF_GETPARAMETERVALUE (6, Color, color);
  MODIF_GETPARAMETERVALUE (7, String, vfsFile);
  MODIF_GETPARAMETERVALUE (8, String, vfsDir);
  MODIF_GETPARAMETERVALUE (9, String, vfsPath);
  MODIF_GETPARAMETERVALUE_END ();

  MODIF_SETPARAMETERVALUE_BEGIN ();
  MODIF_SETPARAMETERVALUE (0, String, name);
  MODIF_SETPARAMETERVALUE (1, String, job);
  MODIF_SETPARAMETERVALUE (2, Long, itemCount);
  MODIF_SETPARAMETERVALUE (3, Bool, awesome);
  MODIF_SETPARAMETERVALUE (4, Float, floatThingy);
  MODIF_SETPARAMETERVALUE (5, Vector2, position);
  MODIF_SETPARAMETERVALUE (6, Color, color);
  MODIF_SETPARAMETERVALUE (7, String, vfsFile);
  MODIF_SETPARAMETERVALUE (8, String, vfsDir);
  MODIF_SETPARAMETERVALUE (9, String, vfsPath);
  MODIF_SETPARAMETERVALUE_END ();

private:
  csString name;
  csString job;
  long itemCount;
  bool awesome;
  float floatThingy;
  csVector2 position;
  csColor color;
  csString vfsFile;
  csString vfsDir;
  csString vfsPath;
};

#endif // __TEST_MODIFIABLE_H__
