/*
    Copyright (C) 2006 by Jorrit Tyberghein
    Copyright (C) 2006 by Amir Taaki
    Copyright (C) 2006 by Pablo Martin

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

#ifndef __CLOUDS_INTERFACE_H__
#define __CLOUDS_INTERFACE_H__

#include <csutil/scf.h>
#include <csutil/scf_implementation.h>
#include <csutil/csstring.h>
#include <csutil/refarr.h>
#include <csutil/refcount.h>
#include <csgeom/vector2.h>
#include <csgeom/vector3.h>

struct iMaterialWrapper;
struct iParticleBuiltinEffectorLinColor;
struct iParticleBuiltinEffectorForce;
struct iSector;

#define MAX_PUFF            64
#define MAX_CLOUD           128
#define MAX_PUFF_PER_CLOUD  8

// Cloud puff
struct CloudPuff : public csRefCount
{
public:
  iMaterialWrapper* material;
  float num, speed, ttl;
  csVector3 velocity;
  csVector2 size;
  csRef<iParticleBuiltinEffectorLinColor> lincol;
  csRef<iParticleBuiltinEffectorForce> force;
};

// Cloud itself
class Cloud : public csRefCount
{
public:
  // cloud name
  csString name;
  // bbox shape where puffs lives in
  csVector3 shape;
  // puff size is a cloud attribute
  csVector3 puffminsize;
  csVector3 puffmaxsize;
  // min/max nbr of puffs
  csVector2 npuffs;
  // puff types
  csRefArray<CloudPuff> puffs;
};

/**
 * This is the API for the volumetric clouds plugin
 */
struct iClouds : public virtual iBase
{
  SCF_INTERFACE (iClouds, 1, 0, 0);

  // Push a new Puff
  virtual CloudPuff* PushPuff (const char* matname, int num, float speed,
                               float ttl, csVector3 velocity, csVector2 size,
                               csRef<iParticleBuiltinEffectorLinColor>* lincol,
                               csRef<iParticleBuiltinEffectorForce>* force) = 0;

  // Push a new Cloud
  virtual Cloud* PushCloud (const char* name, csVector3 shape,
                            csVector3 puffminsize, csVector3 puffmaxsize,
                            csVector2 npuffs) = 0;

  // Add a puff type to a cloud
  virtual void CloudAddPuff (Cloud* cloud, CloudPuff *puff) = 0;

  // Generate a cloud
  virtual void CloudGen (iSector* sector, Cloud* cloud, csVector3 pos) = 0;

  // Flush everything
  virtual void Flush () = 0;
};

#endif // __CLOUDS_INTERFACE_H__
