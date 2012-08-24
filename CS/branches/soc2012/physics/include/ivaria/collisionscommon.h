/*
    Copyright (C) 2012 by Dominik Seifert
    Copyright (C) 2011 by Liu Lu

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

#ifndef __CS_IVARIA_COLLISIONCOMMON_H__
#define __CS_IVARIA_COLLISIONCOMMON_H__

/**\file
 * Collision interfaces
 */

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "iutil/object.h"
#include "colliders.h"

/// The scalar type to be used for physics (float is default, but can easily be changed to double)
typedef float csScalar;

// Some temporary stuff that has to be moved to a utility class or replaced by something entirely different:
#define	SQRT2				1.41421356237f	

/// 3D vector defined by Horizontal (2D) and Vertical (1D) components
#define HV_VECTOR3(horizontal2, vertical1) csVector3((horizontal2).x, vertical1, (horizontal2).y)

/// 2D horizontal components of the given 3D vector
#define HORIZONTAL_COMPONENT(vec3) csVector2((vec3).x, (vec3).z);

struct iTerrainSystem;
struct iSector;
struct iMeshWrapper;
struct iMovable;
struct iPortal;
struct iCamera;
struct iView;

namespace CS
{
namespace Collisions
{
static const int UpAxis = 1;
static const int HorizontalAxis1 = 0;
static const int HorizontalAxis2 = 2;
static const csVector3 UpVector(0, 1, 0);

  /**
  * The type of a collision object.
  */
  enum CollisionObjectType
  {
    COLLISION_OBJECT_PHYSICAL = 0,
    COLLISION_OBJECT_GHOST,
    COLLISION_OBJECT_ACTOR,
    COLLISION_OBJECT_END
  };

  typedef short CollisionGroupMask;

  enum CollisionGroupType
  {
    CollisionGroupTypeDefault =     0,
    CollisionGroupTypeStatic =      1,
    CollisionGroupTypeKinematic =   2,
    CollisionGroupTypePortal =      3,
    CollisionGroupTypeTerrain =     3,
    CollisionGroupTypePortalCopy =  4,
    CollisionGroupTypeActor =       5
  };

  enum CollisionGroupMaskValue
  {
    CollisionGroupMaskValueDefault =      0x0001,
    CollisionGroupMaskValueStatic =       0x0002,
    CollisionGroupMaskValueKinematic =    0x0004,
    CollisionGroupMaskValuePortal =       0x0008,
    CollisionGroupMaskValuePortalCopy =   0x0010,
    CollisionGroupMaskValueActor =        0x0020,
    CollisionGroupMaskValueNone =         0x0040
  };

  /**
  * A structure of collision group. 
  * The objects in the group will not collide with each other.
  */
  struct CollisionGroup
  {
    /// The name of the group.
    csString name;

    /// The value of the group.
    CollisionGroupMask value;

    /// The mask of the group.
    CollisionGroupMask mask;

    CollisionGroup () {}

    CollisionGroup (const char* name)
      : name (name)
    {}
  };

}
}

#endif