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

#include "collisionactor.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"

#include "ivaria/collisions.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{   
  void csBulletCollisionActor::CreateCollisionActor(CS::Collisions::iCollisionActorFactory* props)
  {
    CreateGhostCollisionObject(props);
    
    btPairCachingGhostObject* go = GetPairCachingGhostObject();
    btConvexShape* convShape = (btConvexShape*)(go->getCollisionShape());
    controller = new btKinematicCharacterController(go, convShape, 0.04f, UpAxis);
    
    SetStepHeight(props->GetStepHeight());
    SetWalkSpeed(props->GetWalkSpeed());
    SetJumpSpeed(props->GetJumpSpeed());
    SetAirControlFactor(props->GetAirControlFactor());
  }

  csBulletCollisionActor::csBulletCollisionActor(csBulletSystem* sys) : scfImplementationType (this, sys),
    controller(nullptr), airControlFactor(0)
  {
  }

  csBulletCollisionActor::~csBulletCollisionActor ()
  {
    if (controller)
    {
      delete controller;
    }
  }

  void csBulletCollisionActor::SetAttachedCamera (iCamera* camera)
  {
    this->camera = camera; 
    //SetTransform (camera->GetTransform ());
    //csVector3 upVec = camera->GetTransform ().GetUp ();
    //upVector.setValue (upVec.x, upVec.y, upVec.z);
  }
  

  void csBulletCollisionActor::SetTransform(const csOrthoTransform& trans)
  {
    csBulletCollisionObject::SetTransform(trans);
  }

  bool csBulletCollisionActor::AddBulletObject()
  {
    if (csBulletGhostCollisionObject::AddBulletObject())
    {
      return true;
    }
    return false;
  }

  void csBulletCollisionActor::Walk(csVector3 vel)
  {
    vel.Normalize();
    controller->setVelocityForTimeInterval(CSToBullet(walkSpeed * vel, system->getInternalScale()), csScalar(INT_MAX));
  }

  void csBulletCollisionActor::WalkHorizontal (csVector2 newVel2)
  {
    if (!controller) return;
    newVel2.Normalize();
    newVel2 *= walkSpeed;
    
    //csVector3 vel = GetLinearVelocity(); TODO: Fix this
    csVector3 vel(0);
    if (IsFreeFalling())
    {
      // cannot entirely control movement mid-air
      newVel2 = airControlFactor * newVel2;
      newVel2 += (1.f - airControlFactor) * HORIZONTAL_COMPONENT(vel);
    }
    
    // previous vertical movement is unchanged
    csVector3 newVel = HV_VECTOR3(newVel2, vel[UpAxis]);
    
    controller->setVelocityForTimeInterval(CSToBullet(newVel, system->getInternalScale()), csScalar(INT_MAX));
  }

  void csBulletCollisionActor::StopMoving()
  {
    if (!IsFreeFalling())
    {
      controller->setVelocityForTimeInterval(btVector3(0, 0, 0), csScalar(INT_MAX));
    }
  }

 
  void csBulletCollisionActor::UpdatePreStep (csScalar delta)
  {
    if (!controller) return;

    controller->setGravity(-sector->GetBulletWorld()->getGravity()[UpAxis]);
    controller->updateAction(sector->bulletWorld, delta);
    
    csVector3 pos = BulletToCS(controller->getGhostObject()->getWorldTransform().getOrigin(), this->system->getInverseInternalScale());
    
    // update camera & movable position
    if (camera)
    {
      camera->GetTransform().SetOrigin(pos);
    }
    if (movable)
    {
      movable->SetFullPosition(pos);
    }
  }
  
  void csBulletCollisionActor::UpdatePostStep (csScalar delta)
  {
  }

  void csBulletCollisionActor::Jump ()
  {
    controller->jump();
  }
  
  bool csBulletCollisionActor::IsOnGround () const { return controller->onGround(); }

  void csBulletCollisionActor::SetMaxSlope (float slopeRadians) 
  { 
    controller->setMaxSlope(slopeRadians); 
  }

  float csBulletCollisionActor::GetMaxSlope () const 
  {
    return controller->getMaxSlope();
  }

  float csBulletCollisionActor::GetGravity() const
  {
    return controller->getGravity();
  }

  void csBulletCollisionActor::SetGravity(float gravity)
  {
    controller->setGravity(gravity);
  }

  void csBulletCollisionActor::SetJumpSpeed (float jumpSpeed) 
  {
    controller->setJumpSpeed(jumpSpeed * system->getInternalScale ());
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
