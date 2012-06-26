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

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{   
  void csBulletCollisionActor::CreateCollisionActor(CS::Collisions::CollisionActorProperties* props)
  {
    CreateGhostCollisionObject(props);
  }

  csBulletCollisionActor::csBulletCollisionActor(csBulletSystem* sys) : scfVirtImplementationExt1(this, sys),
    controller(nullptr), pitch(0), flying(false)
  {
  }

  csBulletCollisionActor::~csBulletCollisionActor ()
  {
    if (controller)
    {
      delete controller;
    }
  }
  
  void csBulletCollisionActor::IncreaseYaw(float delta)
  {
    bool reset = !IsFlying() && camera;
    if (reset)
    {
      // align with up-axis before turning horizontally
      camera->GetTransform().RotateThis(CS_VEC_TILT_UP, -pitch);
    }
    csBulletGhostCollisionObject::IncreaseYaw(delta);
    if (reset)
    {
      // re-apply pitch
      camera->GetTransform().RotateThis(CS_VEC_TILT_UP, pitch);
    }
  }
  
  void csBulletCollisionActor::IncreasePitch(float delta)
  {
    pitch += delta;
    if (camera)
    {
      camera->GetTransform().RotateThis(CS_VEC_TILT_UP, delta);
    }
    //csBulletGhostCollisionObject::IncreasePitch(delta);
  }

  void csBulletCollisionActor::SetAttachedCamera (iCamera* camera)
  {
    this->camera = camera; 
    //SetTransform (camera->GetTransform ());
    //csVector3 upVec = camera->GetTransform ().GetUp ();
    //upVector.setValue (upVec.x, upVec.y, upVec.z);
  }

  bool csBulletCollisionActor::AddBulletObject()
  {
    if (csBulletGhostCollisionObject::AddBulletObject())
    {
      if (!controller)
      {
        btPairCachingGhostObject* go = GetPairCachingGhostObject();
        btConvexShape* convShape = (btConvexShape*)(go->getCollisionShape());
        controller = new btKinematicCharacterController(go, convShape, 0.04, 1);
      }
      return true;
    }
    return false;
  }

  void csBulletCollisionActor::SetPlanarVelocity (const csVector2& vel2, float timeInterval)
  {
    if (!controller) return;

    btTransform xform = GetBulletGhostObject()->getWorldTransform();

    btVector3 walkDirection(CSToBullet(csVector3(vel2.y, 0.f, vel2.x), this->system->getInternalScale()));

    btVector3 vel = quatRotate(xform.getRotation(), walkDirection);

    controller->setVelocityForTimeInterval(vel, timeInterval);
  }

  
  void csBulletCollisionActor::UpdateAction(float delta)
  {
    if (!controller) return;

    if (!IsFlying())
    {
      controller->setGravity(-sector->GetGravity()[1]);
    }
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
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
