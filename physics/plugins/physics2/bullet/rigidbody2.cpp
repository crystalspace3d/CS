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
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"

#include "rigidbody2.h"

using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
csBulletRigidBody::csBulletRigidBody (csBulletSystem* phySys, bool isStatic)
: scfImplementationType (this, phySys), btBody (nullptr), 
  physicalState (isStatic ? STATE_STATIC : STATE_DYNAMIC), anchorCount (0)
{
  density  = isStatic ? 0 : btScalar(2);

  // add a temp collider, so we can create the bullet object
  // TODO: Get rid of this crap
  csRef<CS::Collisions::iColliderSphere> firstCollider = phySys->CreateColliderSphere(1);
  tempAddedColliders = true;
  AddCollider(firstCollider, csOrthoTransform (csMatrix3 (), csVector3 (0)));
  tempAddedColliders = false;

  CreateBulletObject();

}

csBulletRigidBody::~csBulletRigidBody ()
{
}

void csBulletRigidBody::AddCollider (CS::Collisions::iCollider* collider, 
                                     const csOrthoTransform& relaTrans)
{
  csRef<csBulletCollider> coll (dynamic_cast<csBulletCollider*>(collider));

  CS::Collisions::ColliderType type = collider->GetType ();
  if (type == CS::Collisions::COLLIDER_CONCAVE_MESH
    ||type == CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED
    ||type == CS::Collisions::COLLIDER_PLANE)
  {
    haveStaticColliders ++;
  }
  else if (type == CS::Collisions::COLLIDER_TERRAIN)
  {
    csFPrintf (stderr, "csBulletRigidBody: Can not add terrain collider to physical body.\n");
    return;
  }

  if (!tempAddedColliders)
  {
    // this is the first "real collider" -> Delete the dummy one
    tempAddedColliders = true;
    colliders.DeleteAll();
    relaTransforms.DeleteAll();
  }

  colliders.Push (coll);
  relaTransforms.Push (relaTrans);
  shapeChanged = true;
}

void csBulletRigidBody::RemoveCollider (CS::Collisions::iCollider* collider)
{
  for (size_t i =0; i < colliders.GetSize(); i++)
  {
    if (colliders[i] == collider)
    {
      RemoveCollider (i);
      return;
    }
  }
}

void csBulletRigidBody::RemoveCollider (size_t index)
{
  if (index >= colliders.GetSize ())
    return;
  CS::Collisions::ColliderType type = colliders[index]->GetType ();
  if (type == CS::Collisions::COLLIDER_CONCAVE_MESH
    ||type == CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED
    ||type == CS::Collisions::COLLIDER_PLANE)
    haveStaticColliders --;
  colliders.DeleteIndex (index);
  relaTransforms.DeleteIndex (index);
}

bool csBulletRigidBody::RemoveBulletObject ()
{
  if (insideWorld)
  {
    if (anchorCount > 0)
    {
      // TODO: Still want to remove this body.
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

void csBulletRigidBody::CreateBulletObject()
{

  btVector3 localInertia (0.0f, 0.0f, 0.0f);

  btCollisionShape* shape;
  btScalar mass;
  btTransform principalAxis;
  //btVector3 principalInertia(0,0,0);

  // Create btRigidBody
  int shapeCount = colliders.GetSize();
  if (shapeCount > 1)
  {
    CS_ALLOC_STACK_ARRAY(btScalar, masses, shapeCount); 
    mass = 0;
    for (int i = 0; i < shapeCount; i++)
    {
      masses[i] = density * colliders[i]->GetVolume ();
      mass += masses[i];
    }
    if (shapeChanged)
    {
      // apply principal axis
      // creation is faster using a new compound to store the shifted children
      if(compoundShape)
      {
        delete compoundShape;
      }
      compoundShape = new btCompoundShape();

      for (int i = 0; i < shapeCount; i++)
      {
        btTransform relaTrans = CSToBullet (relaTransforms[i], system->getInternalScale ());
        compoundShape->addChildShape (relaTrans, colliders[i]->shape);
      }
      shapeChanged = false;
      //compoundShape->calculatePrincipalAxisTransform(masses, principalAxis, principalInertia);
    }

    principalAxis.setIdentity();
    shape = compoundShape;
  }
  else
  {
    shape = colliders[0]->shape;
    mass = density * colliders[0]->GetVolume();

    principalAxis = CSToBullet (relaTransforms[0], system->getInternalScale ());
  }

  // create new motion state
  btTransform trans;
  motionState->getWorldTransform (trans);
  trans = trans * motionState->inversePrincipalAxis;
  delete motionState;
  motionState = new csBulletMotionState (this, trans * principalAxis, principalAxis);

  // TODO: Uncomment this line
  //shape->calculateLocalInertia (mass, localInertia);

  btRigidBody::btRigidBodyConstructionInfo infos (mass, motionState, shape, localInertia);

  btVector3 linVel, angVel;
  bool existed = btBody != nullptr;
  if (existed)
  {
    infos.m_friction = GetFriction();
    infos.m_restitution = GetElasticity();
    infos.m_linearDamping = GetLinearDampener();
    infos.m_angularDamping = GetRollingDampener();
  }
  else
  {
    infos.m_friction = 5.0f;
    infos.m_restitution = 0.2f;
    infos.m_linearDamping = 0;
    infos.m_angularDamping = 0;
  }
  infos.m_mass = mass;

  // create new rigid body
  btBody = new btRigidBody (infos);
  btObject = btBody;

  // TODO: Set mass

  if (existed)
  {
      btBody->setLinearVelocity(linVel);
      btBody->setAngularVelocity(linVel);
  }

  if (haveStaticColliders > 0)
    physicalState = STATE_STATIC;

  SetState (physicalState);

  btBody->setUserPointer (static_cast<CS::Collisions::iCollisionObject*>(this));
}

bool csBulletRigidBody::AddBulletObject ()
{
  if (insideWorld)
    RemoveBulletObject ();

  sector->bulletWorld->addRigidBody (btBody, collGroup.value, collGroup.mask);
 
  insideWorld = true;
  return true;
}

void csBulletRigidBody::RebuildObject () 
{ 
  bool wasInWorld = insideWorld;
  if (insideWorld)
  {
    wasInWorld = true;
    RemoveBulletObject ();
  }
  
  // TODO: Recreate object only if necessary
  CreateBulletObject();

  if (wasInWorld)
  {
    AddBulletObject ();
  }
}

void csBulletRigidBody::SetMass (float mass)
{
  if (mass == 0)
  {
    btBody->setMassProps (0.0f, btVector3 (0.0f, 0.0f, 0.0f));
    //physicalState = STATE_STATIC;
  }
  else
  {
    btVector3 localInertia;

    if (compoundShape)
      compoundShape->calculateLocalInertia (mass, localInertia);
    else
      colliders[0]->shape->calculateLocalInertia (mass, localInertia);

    btBody->setMassProps(mass, localInertia);
    //physicalState = STATE_DYNAMIC;
  }
  btBody->updateInertiaTensor();
}

float csBulletRigidBody::GetMass () const
{
  if (physicalState != STATE_DYNAMIC)
    return 0.0f;

  btScalar mass = btBody->getInvMass();
  return mass > 0 ? 1 / mass : 0;
}

void csBulletRigidBody::SetDensity (float density)
{
  this->density = density;
}

float csBulletRigidBody::GetVolume () const
{
  float volume = 0;
  for (size_t i = 0; i < colliders.GetSize (); i++)
  {
    float vol = colliders[i]->GetVolume ();

    if (vol < FLT_MAX)
      volume += vol;
    else
      return FLT_MAX;
  }
  return volume;
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
  if (physicalState != state || !insideWorld)
  {
    RigidBodyState previousState = physicalState;
    physicalState = state;

    if (!btBody)
      return false;

    if (haveStaticColliders > 0 && state != STATE_STATIC)
      return false;

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
        btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();
        btTransform trans;
        motionState->getWorldTransform (trans);
        trans = trans * motionState->inversePrincipalAxis;
        delete motionState;
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

        SetMass(GetMass ());    // TODO: Mass is probably 0
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
        
        SetMass(0);
        break;
      }
    case STATE_STATIC:
      {
        btBody->setCollisionFlags (btBody->getCollisionFlags()
          | btCollisionObject::CF_STATIC_OBJECT
          & ~btCollisionObject::CF_KINEMATIC_OBJECT);
        btBody->setActivationState (ISLAND_SLEEPING);
        
        SetMass(0);
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

float csBulletRigidBody::GetLinearDampener () 
{
  return btBody->getLinearDamping();
}

void csBulletRigidBody::SetLinearDampener (float d)
{
  btBody->setDamping (d, GetRollingDampener());
}

float csBulletRigidBody::GetRollingDampener ()
{
  return btBody->getAngularDamping();
}

void csBulletRigidBody::SetRollingDampener (float d)
{
  btBody->setDamping (GetLinearDampener(), d);
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
