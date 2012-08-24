/*
    Copyright (C) 2012 by Dominik Seifert

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

#include "ivaria/collisionscommon.h"

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
