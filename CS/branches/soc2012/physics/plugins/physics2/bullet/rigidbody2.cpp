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

#include "cssysdef.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "ivaria/collisions.h"
#include "rigidbody2.h"

using namespace CS::Physics;
using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csBulletRigidBody::CreateRigidBodyObject(CS::Physics::RigidBodyProperties* props)
  {
    //CreatePhysicalBodyObject(props);
    SetCollider(props->GetCollider());
    SetName(props->GetName());

    density = props->GetDensity();
    
    btShape = collider->GetOrCreateBulletShape();
    
    // create new motion state
    btTransform principalAxis(collider->GetPrincipalAxisTransform());
    //principalAxis.setIdentity();
    motionState = new csBulletMotionState (this, principalAxis, principalAxis);

    // construct bullet object
    btScalar mass = density * collider->GetVolume();
    btRigidBody::btRigidBodyConstructionInfo infos (mass, motionState, btShape, mass * collider->GetLocalInertia());

    infos.m_friction = props->GetFriction();
    infos.m_restitution = props->GetElasticity();
    infos.m_linearDamping = props->GetLinearDamping();
    infos.m_angularDamping = props->GetAngularDamping();

    btObject = btBody = new btRigidBody (infos);
    btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*>(this));

    bool isStatic = props->GetMass() == 0;
    SetState(isStatic ? STATE_STATIC : STATE_DYNAMIC);

    // Set collision group
    if (props->GetCollisionGroup().name.Length())
    {
      SetCollisionGroup(props->GetCollisionGroup());
    }
    else
    {
      SetCollisionGroup(isStatic ? system->FindCollisionGroup("Static") : system->FindCollisionGroup("Default"));
    }
  }

  csBulletRigidBody::csBulletRigidBody (csBulletSystem* phySys)
    : scfImplementationType (this, phySys), btBody (nullptr), anchorCount (0)
  {
  }

  csBulletRigidBody::~csBulletRigidBody ()
  {
    if (motionState)
    {
      delete motionState;
    }
  }

  bool csBulletRigidBody::RemoveBulletObject ()
  {
    if (insideWorld)
    {
      if (anchorCount > 0)
      {
        // TODO: Fix this
        system->ReportWarning("Cannot remove anchored body.\n");
        return false;
      }

      for (size_t i = 0; i < joints.GetSize (); i++)
        sector->RemoveJoint (joints[i]);

      sector->bulletWorld->removeRigidBody (btBody);
      insideWorld = false;

      csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (objectCopy);
      if (objectCopy)
        rb->sector->RemoveRigidBody (rb);

      rb = dynamic_cast<csBulletRigidBody*> (objectOrigin);
      if (objectOrigin)
        rb->objectCopy = nullptr;

      objectCopy = nullptr;
      objectOrigin = nullptr;
      return true;
    }
    return false;
  }

  bool csBulletRigidBody::AddBulletObject ()
  {
    if (insideWorld)
      RemoveBulletObject ();
    
    sector->bulletWorld->addRigidBody (btBody, collGroup.value, collGroup.mask);

    if (GetName() && strcmp(GetName(), "Actor") == 0)
    {
      // TODO: This is some test code to rotationally lock an actor object - Ignore it
      btTransform frameB;
      frameB.setIdentity();
      btGeneric6DofConstraint* pGen6Dof = new btGeneric6DofConstraint(*btBody, frameB, false );
      sector->bulletWorld->addConstraint(pGen6Dof);
      pGen6Dof->setDbgDrawSize(btScalar(1.f));

      pGen6Dof->setAngularLowerLimit(btVector3(0,0,0));
      pGen6Dof->setAngularUpperLimit(btVector3(0,0,0));
      pGen6Dof->setLinearLowerLimit(btVector3(-INT_MAX, -INT_MAX, -INT_MAX));
      pGen6Dof->setLinearUpperLimit(btVector3(INT_MAX, INT_MAX, INT_MAX));
    }

    insideWorld = true;
    return true;
  }

  void csBulletRigidBody::RebuildObject () 
  { 
    // TODO: This method is utterly useless

    bool wasInWorld = insideWorld;
    if (insideWorld)
    {
      wasInWorld = true;
      RemoveBulletObject ();
    }
    
    btShape = collider->GetOrCreateBulletShape();
    
    // create new motion state
    btTransform trans;
    motionState->getWorldTransform (trans);
    trans = trans * motionState->inversePrincipalAxis;
    delete motionState;

    btTransform principalAxis = collider->GetPrincipalAxisTransform();
    //principalAxis.setIdentity();
    motionState = new csBulletMotionState (this, trans * principalAxis, principalAxis);

    btScalar mass = density * collider->GetVolume();
    btRigidBody::btRigidBodyConstructionInfo infos (mass, motionState, btShape, mass * collider->GetLocalInertia());

    btVector3 linVel, angVel;
    infos.m_friction = GetFriction();
    infos.m_restitution = GetElasticity();
    infos.m_linearDamping = GetLinearDamping();
    infos.m_angularDamping = GetAngularDamping();
    linVel = btBody->getLinearVelocity();
    angVel = btBody->getAngularVelocity();

    // create new rigid body
    btObject = btBody = new btRigidBody (infos);
    btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*>(this));

    btBody->setLinearVelocity(linVel);
    btBody->setAngularVelocity(linVel);

    SetState (physicalState);

    if (wasInWorld)
    {
      AddBulletObject ();
    }
  }

  void csBulletRigidBody::SetCollider (CS::Collisions::iCollider* collider)
  {
    csBulletCollisionObject::SetCollider(collider);
  }

  void csBulletRigidBody::SetMass (btScalar mass)
  {
    if (collider->GetVolume())
    {
      density = mass / collider->GetVolume();
    }
    else
    {
      mass = 0;
      density = 0;
    }

    // SetMassInternal(mass); SetState calls SetMassInternal

    if (mass == 0)
    {
      SetState(STATE_STATIC);
    }
    else
    {
      SetState(STATE_DYNAMIC);
    }
  }
  
  void csBulletRigidBody::SetMassInternal (btScalar mass)
  {
    if (mass == 0)
    {
      btBody->setMassProps (0.0f, btVector3 (0.0f, 0.0f, 0.0f));
    }
    else
    {
      // scale pre-computed inertia with actual mass
      btBody->setMassProps(mass, mass * collider->GetLocalInertia());
    }
    btBody->updateInertiaTensor();
  }

  void csBulletRigidBody::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT(btObject);

    btTransform btTrans = CSToBullet (trans, system->getInternalScale ());
    
    if (insideWorld)
    {
      sector->bulletWorld->removeRigidBody (btRigidBody::upcast (btObject));
    }
    
    btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();
    delete motionState;
    motionState = new csBulletMotionState (this, btTrans * principalAxis, principalAxis);

    btBody->setMotionState (motionState);
    btBody->setCenterOfMassTransform (motionState->m_graphicsWorldTrans);

    if (insideWorld)
    {
      sector->bulletWorld->addRigidBody (btRigidBody::upcast (btObject), collGroup.value, collGroup.mask);
    }

    if (movable)
    {
      movable->SetFullTransform (BulletToCS (btTrans * motionState->inversePrincipalAxis, system->getInverseInternalScale ()));
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS (btTrans * motionState->inversePrincipalAxis, system->getInverseInternalScale ()));
    }

    btObject->setWorldTransform(btTrans);
  }

  float csBulletRigidBody::GetMass () const
  {
    btScalar mass = btBody->getInvMass();
    return mass > 0 ? 1 / mass : 0;
  }

  void csBulletRigidBody::SetDensity (float density)
  {
    this->density = density;
    SetMass(density * collider->GetVolume());
  }

  float csBulletRigidBody::GetVolume () const
  {
    return collider->GetVolume();
  }


  float csBulletRigidBody::GetElasticity () 
  {
    return btBody->getRestitution();
  }

  void csBulletRigidBody::SetElasticity (float elasticity)
  {
    btBody->setRestitution(elasticity);
  }

  bool csBulletRigidBody::SetState (RigidBodyState state)
  {
    if (!collider->IsDynamic() && state == STATE_DYNAMIC)
    {
      state = STATE_STATIC;
    }
    if (physicalState != state || !insideWorld)
    {
      RigidBodyState previousState = physicalState;
      physicalState = state;

      if (!btBody)
      {
        return false;
      }

      //  TODO: Clean this mess up
      if (insideWorld)
      {
        sector->bulletWorld->removeRigidBody (btBody);
      }

      btVector3 linearVelo = btBody->getInterpolationLinearVelocity ();
      btVector3 angularVelo = btBody->getInterpolationAngularVelocity ();

      if (previousState == STATE_KINEMATIC)
      {
        if (insideWorld)
        {
          // create new motion state
          delete motionState;

          btTransform trans;
          motionState->getWorldTransform (trans);
          trans = trans * motionState->inversePrincipalAxis;
          btTransform principalAxis = collider->GetPrincipalAxisTransform();
          motionState = new csBulletMotionState(this, trans * principalAxis, principalAxis);
          btBody->setMotionState (motionState);
        }
      }

      switch (state)
      {
      case STATE_DYNAMIC:
        {
          btBody->setCollisionFlags (btBody->getCollisionFlags() & ~(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_KINEMATIC_OBJECT));

          btBody->forceActivationState (ACTIVE_TAG);

          btBody->setLinearVelocity (linearVelo);
          btBody->setAngularVelocity (angularVelo);

          SetMassInternal(density * collider->GetVolume());
          break;
        }
      case STATE_KINEMATIC:
        {
          btBody->setCollisionFlags (btBody->getCollisionFlags()
            | btCollisionObject::CF_KINEMATIC_OBJECT
            & ~btCollisionObject::CF_STATIC_OBJECT);

          if (!kinematicCb)
          {
            kinematicCb.AttachNew (new csBulletDefaultKinematicCallback ());
          }

          // create new motion state
          btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();
          btTransform trans;
          motionState->getWorldTransform (trans);
          delete motionState;
          motionState = new csBulletKinematicMotionState(this, trans, principalAxis);
          btBody->setMotionState (motionState);

          // set body kinematic
          btBody->setActivationState (DISABLE_DEACTIVATION);
          btBody->setInterpolationWorldTransform (btBody->getWorldTransform ());
          btBody->setInterpolationLinearVelocity (btVector3(0.0f, 0.0f, 0.0f));
          btBody->setInterpolationAngularVelocity (btVector3(0.0f, 0.0f, 0.0f));


          SetMassInternal(0);
          break;
        }
      case STATE_STATIC:
        {
          btBody->setCollisionFlags (btBody->getCollisionFlags()
            | btCollisionObject::CF_STATIC_OBJECT
            & ~btCollisionObject::CF_KINEMATIC_OBJECT);
          btBody->setActivationState (ISLAND_SLEEPING);

          SetMassInternal(0);
          break;
        }
      }

      if (insideWorld)
      {
        sector->bulletWorld->addRigidBody (btBody);
      }

      return true;
    }
    else
    {
      return false;
    }
  }

  void csBulletRigidBody::SetLinearVelocity (const csVector3& vel)
  {
    CS_ASSERT(physicalState == STATE_DYNAMIC);
    btBody->setLinearVelocity (CSToBullet (vel, system->getInternalScale ()));
    Enable();
  }

  csVector3 csBulletRigidBody::GetLinearVelocity (size_t index /* = 0 */) const
  {
    const btVector3& vel = btBody->getLinearVelocity ();
    return BulletToCS (vel, system->getInverseInternalScale ());
  }

  void csBulletRigidBody::SetAngularVelocity (const csVector3& vel)
  {
    if (!btBody)
      return; 
    if (physicalState == STATE_DYNAMIC)
    {
      btBody->setAngularVelocity (btVector3 (vel.x, vel.y, vel.z));
      btBody->activate ();
    }
  }

  csVector3 csBulletRigidBody::GetAngularVelocity () const
  {
    const btVector3& vel = btBody->getAngularVelocity ();
    return csVector3 (vel.getX (), vel.getY (), vel.getZ ());
  }

  void csBulletRigidBody::AddForce (const csVector3& force)
  {
    if (btBody)
    {
      btBody->applyImpulse (btVector3 (force.x * system->getInternalScale (),
        force.y * system->getInternalScale (),
        force.z * system->getInternalScale ()),
        btVector3 (0.0f, 0.0f, 0.0f));
      btBody->setActivationState(ACTIVE_TAG);
    }
  }

  void csBulletRigidBody::AddTorque (const csVector3& torque)
  {
    if (!btBody)
      return; 

    btBody->applyTorque (btVector3 (torque.x * system->getInternalScale () * system->getInternalScale (),
      torque.y * system->getInternalScale () * system->getInternalScale (),
      torque.z * system->getInternalScale () * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }

  void csBulletRigidBody::AddRelForce (const csVector3& force)
  {
    if (!btBody)
      return; 

    csOrthoTransform trans =  csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    btBody->applyImpulse (btVector3 (absForce.x * system->getInternalScale (),
      absForce.y * system->getInternalScale (),
      absForce.z * system->getInternalScale ()),
      btVector3 (0.0f, 0.0f, 0.0f));
    btBody->setActivationState(ACTIVE_TAG);
  }

  void csBulletRigidBody::AddRelTorque (const csVector3& torque)
  {
    if (!btBody)
      return; 
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absTorque = trans.This2Other (torque);
    btBody->applyTorque (btVector3 (absTorque.x * system->getInternalScale () * system->getInternalScale (),
      absTorque.y * system->getInternalScale () * system->getInternalScale (),
      absTorque.z * system->getInternalScale () * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }


  void csBulletRigidBody::AddForceAtPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 

    btVector3 btForce (force.x * system->getInternalScale (),
      force.y * system->getInternalScale (),
      force.z * system->getInternalScale ());
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 relPos = trans.Other2This (pos);

    btBody->applyImpulse (btForce, btVector3 (relPos.x * system->getInternalScale (),
      relPos.y * system->getInternalScale (),
      relPos.z * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }

  void csBulletRigidBody::AddForceAtRelPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 

    btBody->applyImpulse (btVector3 (force.x * system->getInternalScale (),
      force.y * system->getInternalScale (),
      force.z * system->getInternalScale ()),
      btVector3 (pos.x * system->getInternalScale (),
      pos.y * system->getInternalScale (),
      pos.z * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }

  void csBulletRigidBody::AddRelForceAtPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 

    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    csVector3 relPos = trans.Other2This (pos);
    btBody->applyImpulse (btVector3 (absForce.x * system->getInternalScale (),
      absForce.y * system->getInternalScale (),
      absForce.z * system->getInternalScale ()),
      btVector3 (relPos.x * system->getInternalScale (),
      relPos.y * system->getInternalScale (),
      relPos.z * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }

  void csBulletRigidBody::AddRelForceAtRelPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 

    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    btBody->applyImpulse (btVector3 (absForce.x * system->getInternalScale (),
      absForce.y * system->getInternalScale (),
      absForce.z * system->getInternalScale ()),
      btVector3 (pos.x * system->getInternalScale (),
      pos.y * system->getInternalScale (),
      pos.z * system->getInternalScale ()));
    btBody->setActivationState(ACTIVE_TAG);
  }

  csVector3 csBulletRigidBody::GetForce () const
  {
    if (!btBody)
      return csVector3 (0);

    btVector3 force = btBody->getTotalForce ();
    return csVector3 (force.getX () * system->getInverseInternalScale (),
      force.getY () * system->getInverseInternalScale (),
      force.getZ () * system->getInverseInternalScale ());
  }

  csVector3 csBulletRigidBody::GetTorque () const
  {
    if (!btBody)
      return csVector3 (0);

    btVector3 torque = btBody->getTotalTorque ();
    return csVector3
      (torque.getX () * system->getInverseInternalScale () * system->getInverseInternalScale (),
      torque.getY () * system->getInverseInternalScale () * system->getInverseInternalScale (),
      torque.getZ () * system->getInverseInternalScale () * system->getInverseInternalScale ());
  }

  float csBulletRigidBody::GetLinearDamping () 
  {
    return btBody->getLinearDamping();
  }

  void csBulletRigidBody::SetLinearDamping (float d)
  {
    btBody->setDamping (d, GetAngularDamping());
  }

  float csBulletRigidBody::GetAngularDamping ()
  {
    return btBody->getAngularDamping();
  }

  void csBulletRigidBody::SetAngularDamping (float d)
  {
    btBody->setDamping (GetLinearDamping(), d);
  }

  csBulletDefaultKinematicCallback::csBulletDefaultKinematicCallback ()
    : scfImplementationType (this)
  {
  }

  csBulletDefaultKinematicCallback::~csBulletDefaultKinematicCallback ()
  {
  }

  void csBulletDefaultKinematicCallback::GetBodyTransform
    (iRigidBody* body, csOrthoTransform& transform) const
  {

    csBulletRigidBody* rigBody = dynamic_cast<csBulletRigidBody*> (body);
    iMovable* movable = rigBody->GetAttachedMovable ();
    if (movable)
    {
      transform = movable->GetFullTransform ();
      return;
    }
    iCamera* camera = rigBody->GetAttachedCamera ();
    if (camera)
    {
      transform = camera->GetTransform ();
      return;
    }
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
