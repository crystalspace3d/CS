

/*
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

#ifndef __CS_IVARIA_COLLISIONPROPERTIES_H__
#define __CS_IVARIA_COLLISIONPROPERTIES_H__

/**\file
* Collision interfaces
*/

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "iutil/object.h"
#include "colliders.h"
#include "csutil/csstring.h"


namespace CS { namespace Collisions {
  struct csConvexResult;
  struct iCollisionCallback;
  struct iCollisionObject;
  struct iCollisionSector;
  struct iCollider;
  struct CollisionGroup;

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
    CollisionGroupMaskValueTerrain =      0x0008,
    CollisionGroupMaskValuePortalCopy =   0x0010,
    CollisionGroupMaskValueActor =        0x0020
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

  class CollisionObjectProperties
  {
  protected:
    csRef<iCollider> collider;
    csString name;
    CollisionGroup collGroup;

  public:
    CollisionObjectProperties(iCollider* collider) : collider(collider), collGroup() {}

    /// Get the collider of all objects that will be constructed with these properties
    iCollider* GetCollider() const { return collider; }
    /// Set the collider of all objects that will be constructed with these properties
    void SetCollider(iCollider* value) { collider = value; }

    /// Get the name of all objects that will be constructed with these properties
    const csString& GetName() const { return name; }
    /// Set the name of all objects that will be constructed with these properties
    void SetName(const char* newName) { name = newName; }

    /// Get the collision group of all objects that will be constructed with these properties
    const const CollisionGroup& GetCollisionGroup() const { return collGroup; }
    /// Set the collision group of all objects that will be constructed with these properties
    void SetCollisionGroup(const CollisionGroup& value) { collGroup = value; }
  };

  class GhostCollisionObjectProperties : public CollisionObjectProperties
  {
  public:
    GhostCollisionObjectProperties(iCollider* collider) : CollisionObjectProperties(collider) {}

  };

  class CollisionActorProperties : public GhostCollisionObjectProperties
  {
  public:
    CollisionActorProperties(iCollider* collider) : GhostCollisionObjectProperties(collider) 
    {
      SetName("actor");
    }

  };
} }
#endif