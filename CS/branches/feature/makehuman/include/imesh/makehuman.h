/*
    Copyright (C) 2012-2013 by Anthony Legrand
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
#ifndef __CS_IMESH_MAKEHUMAN_H__
#define __CS_IMESH_MAKEHUMAN_H__

/**\file
 * MakeHuman character mesh generation.
 */ 

#include "csutil/scf.h"

/**\addtogroup meshplugins
 * @{ */

namespace CS {
namespace Mesh {

struct iAnimatedMeshFactory;

enum MakeHumanMorphTargetDirection
{
  MH_DIRECTION_BOTH = 0,
  MH_DIRECTION_UP,
  MH_DIRECTION_DOWN
};

enum MakeHumanUpdateMode
{
  MH_UPDATE_FULL = 0,
  MH_UPDATE_FAST
};

struct iMakeHumanMorphTarget : public virtual iBase
{
  SCF_INTERFACE (iMakeHumanMorphTarget, 1, 0, 0);

  virtual const char* GetName () const = 0;
  virtual const csArray<csVector3>& GetOffsets () const = 0;
  virtual const csArray<size_t>& GetIndices () const = 0;
  virtual float GetScale () const = 0;
  virtual MakeHumanMorphTargetDirection GetDirection () const = 0;
};

// TODO: iMakeHumanParameters: MH model parameters -> struct MakeHumanModel
// TODO: MH generation parameters

struct iMakeHumanCharacter : public virtual iBase
{
  SCF_INTERFACE (iMakeHumanCharacter, 1, 0, 0);

  virtual void SetExpressionGeneration (bool generate) = 0;
  virtual bool GetExpressionGeneration () const = 0;

  virtual void SetUpdateMode (MakeHumanUpdateMode mode) = 0;
  virtual MakeHumanUpdateMode GetUpdateMode () const = 0;

  virtual iAnimatedMeshFactory* GetMeshFactory () const = 0;
  virtual bool UpdateMeshFactory () = 0;

  virtual void Clear () = 0;
  virtual void SetNeutral () = 0;

  virtual bool Parse (const char* filename) = 0;

  virtual void SetProxy (const char* proxy) = 0;
  virtual void SetRig (const char* rig) = 0;

  virtual void SetProperty (const char* property, float value) = 0;
  virtual float GetProperty (const char* property) const = 0;
  virtual void SetMeasure (const char* measure, float value) = 0;
  virtual float GetMeasure (const char* measure) const = 0;
  //void SetMeasureDistance (const char* measure, float distance);

  virtual bool GetPropertyTargets
    (const char* property, csRefArray<iMakeHumanMorphTarget>& targets) = 0;
  virtual bool GetMeasureTargets
    (const char* measure, csRefArray<iMakeHumanMorphTarget>& targets) = 0;

  virtual void ClearClothes () = 0;
  virtual size_t GetClothCount () const = 0;

  // TODO: remove
  virtual iAnimatedMeshFactory* GetClothMesh (size_t index) const = 0;
};

/**
 * TODO
 */
struct iMakeHumanManager : public virtual iBase
{
  SCF_INTERFACE (iMakeHumanManager, 1, 0, 0);

  virtual csPtr<iMakeHumanCharacter> CreateCharacter () = 0;

  virtual csPtr<iStringArray> GetProxies () const = 0;
  virtual csPtr<iStringArray> GetRigs () const = 0;
  virtual csPtr<iStringArray> GetProperties () const = 0;
  virtual csPtr<iStringArray> GetMeasures () const = 0;
};

/** @} */

} // namespace Mesh
} // namespace CS

#endif // __CS_IMESH_MAKEHUMAN_H__
