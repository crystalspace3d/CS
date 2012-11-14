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

#include "collisionactor.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "ivaria/collisions.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{   
  void csBulletCollisionActor::CreateCollisionActor(CS::Collisions::iCollisionActorFactory* props)
  {
    CreateGhostCollisionObject(props);
    
    btPairCachingGhostObject* go = GetPairCachingGhostObject();
    btConvexShape* convShape = (btConvexShape*)(go->getCollisionShape());
    controller = new csKinematicActorController
      (go, convShape, stepHeight = props->GetStepHeight(), 1);
    
    SetWalkSpeed(props->GetWalkSpeed());
    SetJumpSpeed(props->GetJumpSpeed());
    SetAirControlFactor(props->GetAirControlFactor());
  }

  csBulletCollisionActor::csBulletCollisionActor(csBulletSystem* sys) : scfImplementationType (this, sys),
    controller(nullptr), airControlFactor(0), gravityEnabled(false)
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
  
  void csBulletCollisionActor::SetStepHeight (float newHeight)
  {
    // need to re-create the controller
    btPairCachingGhostObject* go = GetPairCachingGhostObject();
    btConvexShape* convShape = (btConvexShape*)(go->getCollisionShape());
    delete controller;
    controller = new csKinematicActorController(go, convShape, stepHeight = newHeight, 1);
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
      csVector2 horizontalVel (vel.x, vel.z);
      newVel2 += (1.f - airControlFactor) * csVector2 (vel.x, vel.z);
    }
    
    // previous vertical movement is unchanged
    csVector3 newVel (newVel2.x, vel[1], newVel2.y);
    
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

    controller->setGravity(-sector->GetBulletWorld()->getGravity()[1]);

    // Remove ghost objects from list of blocking objects:
    btPairCachingGhostObject* go = GetPairCachingGhostObject();
    for (int i = 0; i < go->getOverlappingPairCache()->getNumOverlappingPairs(); )
    {
      btBroadphasePair* collisionPair = &go->getOverlappingPairCache()->getOverlappingPairArray()[i];
      btBroadphaseProxy* otherProxy;
      if ((btCollisionObject*)collisionPair->m_pProxy0->m_clientObject == (btCollisionObject*)go)
      {
        otherProxy = collisionPair->m_pProxy1;
      }
      else
      {
        otherProxy = collisionPair->m_pProxy0;
      }

      btCollisionObject* otherObj = (btCollisionObject*)otherProxy->m_clientObject;
      if (otherObj->getInternalType() == btCollisionObject::CO_GHOST_OBJECT)
      {
        // Disregard ghost object collisions
        go->getOverlappingPairCache()->removeOverlappingPair(
          collisionPair->m_pProxy0, collisionPair->m_pProxy1,
          sector->GetBulletWorld()->getDispatcher());
      }
      else
      {
        ++i;
      }
    }

    // Resolve collisions and setup step height
    controller->updateAction(sector->bulletWorld, delta);
    
    csVector3 pos = BulletToCS(controller->getGhostObject()->getWorldTransform().getOrigin(), this->system->getInverseInternalScale());
    
    // update camera & movable position
    if (camera)
    {
      camera->GetTransform().SetOrigin(pos);
    }
    iMovable* movable = GetAttachedMovable();
    if (movable)
    {
      movable->SetFullPosition(pos);
      movable->UpdateMove();
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
