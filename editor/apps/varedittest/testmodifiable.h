/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Bârsan

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

/// Test entity for GUI generation.
class csTestModifiable : public scfImplementation1<csTestModifiable, iModifiable>
{
public:
  csTestModifiable(const char* name, const char* job, long itemCount, iObjectRegistry* object_reg);
  virtual ~csTestModifiable();
  
  //-- iModifiable
  const csStringID GetID() const;
  csPtr<iModifiableDescription> GetDescription () const;

  csVariant* GetParameterValue (csStringID id) const;
  bool SetParameterValue (csStringID id, const csVariant& value);

private:
  long                itemCount;
  bool                awesome;
  float               floatThingy;
  csVector2           position;
  csColor             color;
  scfString           name;
  scfString           job;
  scfString           vfsFile;
  scfString           vfsDir;
  scfString           vfsPath;
  csArray<float>      floatArray;
  iObjectRegistry*    object_reg;

  //-- iModifiable
  csStringID id_testModifiable;
  csStringID id_name, id_job, id_floatArray, id_position, id_color, id_itemCount, id_awesome, id_floatThingy, id_vfsFile, id_vfsDir, id_vfsPath;

};

#endif// __TEST_MODIFIABLE_H__
