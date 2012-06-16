/*
  Copyright (C) 2011 by Liu Lu

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

#ifndef __CS_BULLET_COLLISIONACTOR_H__
#define __CS_BULLET_COLLISIONACTOR_H__

#include "cssysdef.h"
#include "common2.h"
#include "collisionghost.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletCollisionActor : public scfVirtImplementationExt1<csBulletCollisionActor,  csBulletCollisionGhostObject,  CS::Collisions::iCollisionActor>
{
  btKinematicCharacterController* controller;
  float pitch;
  bool flying;

public:
  csBulletCollisionActor (csBulletSystem* sys, csBulletCollider* collider);
  virtual ~csBulletCollisionActor ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }
  //iCollisionObject
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () {return dynamic_cast<csBulletCollisionObject*> (this);}
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return nullptr;}

  virtual bool IsPhysicalObject() const { return false; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const {return CS::Collisions::COLLISION_OBJECT_ACTOR;}

  virtual bool AddBulletObject();
  
  virtual void IncreaseYaw(float delta);
  virtual void IncreasePitch(float delta);

  //iCollisionActor
  virtual bool IsFlying () const { return flying; }

  virtual void SetFlying (bool value) 
  { 
    flying = value; 
    controller->setGravity(0);
  }

  virtual bool IsOnGround () const { return controller->onGround(); }

  /// Gets the pitch (up/down angle) to determine head pose and camera angle
  virtual float GetPitch () const { return pitch; }

  virtual void SetAttachedCamera(iCamera* camera);

  virtual void UpdateAction (float delta);

  float GetGravity()
  {
    return controller->getGravity();
  }

  void SetGravity(float gravity)
  {
    controller->setGravity(gravity);
  }

  virtual void SetVelocity (const csVector3& vel, float timeInterval)
  {
    controller->setVelocityForTimeInterval(CSToBullet(vel, system->getInternalScale()), timeInterval);
  }
  
  virtual void SetPlanarVelocity (const csVector2& vel, float timeInterval = float(INT_MAX));

  virtual void SetFallSpeed (float fallSpeed) 
  {
    controller->setFallSpeed(fallSpeed * system->getInternalScale ());
  }

  virtual void SetJumpSpeed (float jumpSpeed) 
  {
    controller->setJumpSpeed(jumpSpeed * system->getInternalScale ());
  }

  virtual void SetMaxJumpHeight (float maxJumpHeight)
  {
    controller->setMaxJumpHeight(maxJumpHeight * system->getInternalScale ());
  }

  virtual void StepHeight (float stepHeight)
  {
    //this->stepHeight = stepHeight * system->getInternalScale ();
  }

  virtual void Jump ()
  {
    controller->jump();
  }

  virtual void SetMaxSlope (float slopeRadians) 
  { 
    controller->setMaxSlope(slopeRadians); 
  }

  virtual float GetMaxSlope () const 
  {
    return controller->getMaxSlope();
  }

  btPairCachingGhostObject* GetBulletGhostObject()
  {
    return static_cast<btPairCachingGhostObject*>(btObject);
  }
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif