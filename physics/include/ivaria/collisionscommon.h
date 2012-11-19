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
  /**
  * The type of a collision object.
  */
  enum CollisionObjectType
  {
    COLLISION_OBJECT_SIMPLE = 0,     /*!< The collision object is a simple static collision object.
				       It can never be upcast to a iPhysicalObject.*/
    COLLISION_OBJECT_PHYSICAL,       /*!< The collision object is a physical object and can be
				       upcast to a iPhysicalObject. */
    COLLISION_OBJECT_GHOST,          /*!< The collision object is a ghost. */
    COLLISION_OBJECT_ACTOR          /*!< The collision object is an actor. */
  };

  typedef short CollisionGroupMask;

  /// \todo remove this
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

  /// \todo remove this
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
  * \todo Real group management, with transparent allocation of the masks, inherit iBase
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
