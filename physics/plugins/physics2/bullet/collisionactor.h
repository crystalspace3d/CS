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

#ifndef __CS_BULLET_COLLISIONACTOR_H__
#define __CS_BULLET_COLLISIONACTOR_H__

#include "cssysdef.h"
#include "common2.h"
#include "collisionghost.h"
#include "kinematicactorcontroller.h"


CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class BulletCollisionActorFactory : public scfVirtImplementationExt1<
  BulletCollisionActorFactory, BulletGhostCollisionObjectFactory, CS::Collisions::iCollisionActorFactory> 
{
  float stepHeight;
  float walkSpeed, jumpSpeed;
  float airControlFactor;

public:
  BulletCollisionActorFactory
    (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr)
    : scfImplementationType (this, system, collider),
    stepHeight(.5f),
    walkSpeed(10.f),
    jumpSpeed(10.f),
    airControlFactor(0.04f)
    {}

  /// Create a new object
  virtual csPtr<CS::Collisions::iCollisionActor> CreateCollisionActor();
  virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject();

  /// Get the max vertical threshold that this actor can step over
  virtual float GetStepHeight () const { return stepHeight; }
  /// Set the max vertical threshold that this actor can step over
  virtual void SetStepHeight (float h) { stepHeight = h; }

  /// Get the walk speed
  virtual float GetWalkSpeed () const { return walkSpeed; }
  /// Set the walk speed
  virtual void SetWalkSpeed (float s) { walkSpeed = s; }

  /// Get the jump speed
  virtual float GetJumpSpeed () const { return jumpSpeed; }
  /// Set the jump speed
  virtual void SetJumpSpeed (float s) { jumpSpeed = s; }

  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  virtual float GetAirControlFactor () const { return airControlFactor; }
  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  virtual void SetAirControlFactor (float f) { airControlFactor = f; }
};

 /**
  * TODO: IsMovingUp
  */
class csBulletCollisionActor : public scfVirtImplementationExt1<csBulletCollisionActor,
    csBulletGhostCollisionObject, CS::Collisions::iCollisionActor>
{
  csKinematicActorController* controller;
  float airControlFactor;
  float walkSpeed;
  bool gravityEnabled;
  float stepHeight;

public:
  void CreateCollisionActor(CS::Collisions::iCollisionActorFactory* props);

public:
  csBulletCollisionActor (csBulletSystem* sys);
  virtual ~csBulletCollisionActor ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }

  //iCollisionObject
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () { return dynamic_cast<CS::Collisions::iCollisionObject*> (this); }
  virtual CS::Collisions::iActor* QueryActor () { return dynamic_cast<CS::Collisions::iActor*>(this); }

  virtual bool IsPhysicalObject() const { return false; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const {return CS::Collisions::COLLISION_OBJECT_ACTOR;}

  virtual bool AddBulletObject();

  //iCollisionActor
  virtual bool IsOnGround () const;

  virtual void SetAttachedCamera(iCamera* camera);
  
  virtual void UpdatePreStep (float delta);
  virtual void UpdatePostStep (float delta);

  float GetGravity() const;
  void SetGravity(float gravity);

  virtual void SetTransform(const csOrthoTransform& trans);

  virtual void Walk(csVector3 vel);
  
  virtual void WalkHorizontal(csVector2 vel);

  virtual void StopMoving();
  
  /// Determines how much the actor can control movement when free falling
  virtual float GetAirControlFactor () const { return airControlFactor; }
  /// Determines how much the actor can control movement when free falling
  virtual void SetAirControlFactor (float f) { airControlFactor = f; }

  
  /// Get the jump speed. The kinematic controller does not have this method... OUCH!
  virtual float GetJumpSpeed () const { return 0; }
  virtual void SetJumpSpeed (float jumpSpeed);
  
  /// Get the max vertical threshold that this actor can step over
  virtual float GetStepHeight () const { return stepHeight; }
  virtual void SetStepHeight (float stepHeight);

  /// Get the walk speed
  virtual float GetWalkSpeed () const { return walkSpeed; }
  /// Set the walk speed
  virtual void SetWalkSpeed (float s) { walkSpeed = s; }

  virtual void Jump ();

  virtual float GetMaxSlope () const;
  virtual void SetMaxSlope (float slopeRadians);

  /// Whether this object can be affected by gravity
  virtual bool GetGravityEnabled() const { return gravityEnabled; }
  /// Whether this object can be affected by gravity
  virtual void SetGravityEnabled(bool enabled) { gravityEnabled = enabled; }

  bool DoesGravityApply() const { return GetGravityEnabled() && GetGravity() != 0; }
  
  /// Whether the actor is not on ground and gravity applies
  virtual bool IsFreeFalling() const
  { 
    // kinematic character controller does not reveal its linear velocity
    return 
      (!IsOnGround()) &&                // not touching the ground
      DoesGravityApply();               // and gravity applies
  }

  btPairCachingGhostObject* GetBulletGhostObject()
  {
    return static_cast<btPairCachingGhostObject*>(btObject);
  }
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
