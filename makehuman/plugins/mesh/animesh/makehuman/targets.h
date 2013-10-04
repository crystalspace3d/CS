/*
    Copyright (C) 2013 by Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __MAKEHUMAN_TARGETS_H__
#define __MAKEHUMAN_TARGETS_H__

#include "makehuman.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

using namespace CS::Mesh;

class MakeHumanMorphTarget
  : public scfImplementation1<MakeHumanMorphTarget, CS::Mesh::iMakeHumanMorphTarget>
{
public:
  MakeHumanMorphTarget ();
  ~MakeHumanMorphTarget () {}

  //-- iMakeHumanMorphTarget
  virtual const char* GetName () const;
  virtual const csArray<csVector3>& GetOffsets () const;
  virtual const csArray<size_t>& GetIndices () const;
  virtual float GetScale () const;
  virtual MakeHumanMorphTargetDirection GetDirection () const;

private:
  csString name;
  csArray<csVector3> offsets;
  csArray<size_t> indices;
  float scale;
  MakeHumanMorphTargetDirection direction;

  friend class MakeHumanCharacter;
};

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)

#endif // __MAKEHUMAN_TARGETS_H__
