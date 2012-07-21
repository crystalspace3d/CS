

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

#ifndef __CS_IVARIA_COLLISIONFACTORIES_H__
#define __CS_IVARIA_COLLISIONFACTORIES_H__

/**\file
* Collision interfaces
*/

#include "csutil/scf.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "iutil/object.h"
#include "colliders.h"
#include "csutil/csstring.h"


namespace CS 
{ 
namespace Collisions 
{
  struct csConvexResult;
  struct iCollisionCallback;
  struct iCollisionObject;
  struct iCollisionSector;
  struct iCollisionSystem;
  struct iCollisionObject;
  struct iGhostCollisionObject;
  struct iCollisionActor;

  struct iCollider;

  struct CollisionGroup;

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

  struct iCollisionObjectFactory : public virtual iBase
  {
    /// Return the underlying object
    virtual iObject *QueryObject (void) = 0;

    /// Get the system of this factory
    virtual iCollisionSystem* GetSystem() const = 0;

    /// Create a baby
    virtual csPtr<iCollisionObject> CreateCollisionObject() = 0;

    /// Get the collider of all objects that will be constructed with these properties
    virtual iCollider* GetCollider() const = 0;
    /// Set the collider of all objects that will be constructed with these properties
    virtual void SetCollider(iCollider* value)  = 0;

    /// Get the collision group of all objects that will be constructed with these properties
    virtual const CollisionGroup& GetCollisionGroup() const = 0;
    /// Set the collision group of all objects that will be constructed with these properties
    virtual void SetCollisionGroup(const CollisionGroup& value)  = 0;
  };

  struct iGhostCollisionObjectFactory : public virtual iCollisionObjectFactory
  {
    /// Create a baby
    virtual csPtr<iGhostCollisionObject> CreateGhostCollisionObject() = 0;
  };

  struct iCollisionActorFactory : public virtual iGhostCollisionObjectFactory
  {
    /// Create a baby
    virtual csPtr<iCollisionActor> CreateCollisionActor() = 0;

    /// Get the max vertical threshold that this actor can step over
    virtual float GetStepHeight () const = 0;
    /// Set the max vertical threshold that this actor can step over
    virtual void SetStepHeight (float h) = 0;

    /// Get the walk speed
    virtual float GetWalkSpeed () const = 0;
    /// Set the walk speed
    virtual void SetWalkSpeed (float s) = 0;

    /// Get the jump speed
    virtual float GetJumpSpeed () const = 0;
    /// Set the jump speed
    virtual void SetJumpSpeed (float s)  = 0;

    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual float GetAirControlFactor () const = 0;
    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual void SetAirControlFactor (float f) = 0;
  };
} 
}
#endif
