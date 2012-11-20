/*
    Copyright (C) 2012 by Dominik Seifert
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

#include "cssysdef.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "iengine/scenenode.h"
#include "ivaria/collisions.h"
#include "rigidbody2.h"

#include "portal.h"

using namespace CS::Physics;
using namespace CS::Collisions;

#define ADD_FORCE applyImpulse
#define ADD_CENTRAL_FORCE applyCentralImpulse

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  // TODO: don't convert to the abstract interface since this method is internal to the plugin
  // or: move this method to the factory instead
  void csBulletRigidBody::CreateRigidBodyObject (CS::Physics::iRigidBodyFactory* props)
  {
    //CreatePhysicalBodyObject (props);
    collider = dynamic_cast<csBulletCollider*>(props->GetCollider ());
    // TODO: really?
    SetName (props->QueryObject ()->GetName ());

    // TODO: use factory's state

    // set mass & density
    btScalar mass;
    if (!collider->IsDynamic ())
    {
      mass = density = 0;
    }
    else if (props->GetDensity ())
    {
      density = props->GetDensity ();
      mass = density * collider->GetVolume ();
    }
    else
    {
      mass = props->GetMass ();
      density = collider->GetVolume () > 0 ? mass / collider->GetVolume () : 0;
    }
    
    // construct bullet object
    btCollisionShape* shape = collider->GetOrCreateBulletShape ();
    btRigidBody::btRigidBodyConstructionInfo infos
      (mass, CreateMotionState (collider->GetPrincipalAxisTransform ()), shape, mass * collider->GetLocalInertia ());

    infos.m_friction = props->GetFriction ();
    infos.m_restitution = props->GetElasticity ();
    infos.m_linearDamping = props->GetLinearDamping ();
    infos.m_angularDamping = props->GetAngularDamping ();

    btObject = btBody = new btRigidBody (infos);
    btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*>(this));
    //btBody->setContactProcessingThreshold (1e18f);

    bool isStatic = density == 0;
    SetState (isStatic ? STATE_STATIC : STATE_DYNAMIC);

    // Set collision group
    if (props->GetCollisionGroup ().name.Length ())
    {
      SetCollisionGroup (props->GetCollisionGroup ());
    }
    else
    {
      SetCollisionGroup (IsDynamic () ? system->FindCollisionGroup ("Default") : system->FindCollisionGroup ("Static"));
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

  csBulletMotionState* csBulletRigidBody::CreateMotionState (const btTransform& trans)
  {
    return motionState = new csBulletMotionState (this, trans, collider->GetPrincipalAxisTransform ());
  }

  bool csBulletRigidBody::RemoveBulletObject ()
  {
    if (insideWorld)
    {
      if (anchorCount > 0)
      {
        // TODO: Fix this
        system->ReportWarning ("Cannot remove anchored body.\n");
        return false;
      }

      for (size_t i = 0; i < joints.GetSize (); i++)
        sector->RemoveJoint (joints[i]);

      sector->bulletWorld->removeRigidBody (btBody);
      insideWorld = false;

      if (portalData && portalData->Portal)
      {
        // Remove from portal
        portalData->Portal->RemoveTraversingObject (this);
      }

      return true;
    }
    return false;
  }

  bool csBulletRigidBody::AddBulletObject ()
  {
    if (insideWorld)
      RemoveBulletObject ();
    
    sector->bulletWorld->addRigidBody (btBody, collGroup.value, collGroup.mask);

    //if (GetName () && strcmp (GetName (), "Actor") == 0)
    //{
    //  btBody->setAngularFactor (0);
    //}

    insideWorld = true;
    return true;
  }

  void csBulletRigidBody::RebuildObject () 
  { 
    // See http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=7988
    /**
     Quote:
     It could be that contact points are not updated because you changed the collision shape. 
     If you modify collision shapes, the best way is to remove the objects first, then make the modification, and re-insert it into the world. 
     Alternatively, you can manually flush the contact points for a given object using an API call, for example:
     world->getBroadphase ()->getOverlappingPairCache ()->cleanProxyFromPairs (object->getBroadphaseHandle (),world->getDispatcher ());
     */

    bool wasInWorld = insideWorld;
    if (insideWorld)
    {
      wasInWorld = true;
      RemoveBulletObject ();
    }
    
    // Create new object
    delete btObject;
    btObject = btBody = static_cast<btRigidBody*>(CreateBulletObject ());

    SetState (physicalState);

    if (wasInWorld)
    {
      AddBulletObject ();
    }
  }

  bool csBulletRigidBody::DoesGravityApply () const { return GetGravityEnabled () && btBody->getGravity ().length2 () != 0; }
  
  bool csBulletRigidBody::IsMovingUpward () const 
  { 
    // TODO: Consider scaling threshold by actor size or similar
    return BulletVectorComponent (btBody->getLinearVelocity (), 1) > 1.f * system->GetInternalScale (); 
  }

  void csBulletRigidBody::SetCollider (CS::Collisions::iCollider* collider)
  {
    csBulletCollisionObject::SetCollider (collider);
  }

  void csBulletRigidBody::SetMass (btScalar mass)
  {
    if (collider->GetVolume ())
    {
      density = mass / collider->GetVolume ();
    }
    else
    {
      mass = 0;
      density = 0;
    }

    // SetMassInternal (mass); SetState calls SetMassInternal

    if (mass == 0)
    {
      SetState (STATE_STATIC);
    }
    else
    {
      SetState (STATE_DYNAMIC);
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
      // we scale pre-computed inertia with actual mass
      btBody->setMassProps (mass, mass * collider->GetLocalInertia ());
    }
    btBody->updateInertiaTensor ();
  }

  void csBulletRigidBody::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT (btObject);

    btTransform btTrans = CSToBullet (trans, system->GetInternalScale ());
    
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
    
    iSceneNode* sceneNode = GetAttachedSceneNode ();
    if (sceneNode)
    {
      // Update movable
      sceneNode->GetMovable ()->SetFullTransform
	(BulletToCS (btTrans * motionState->inversePrincipalAxis, system->GetInverseInternalScale ()));
      sceneNode->GetMovable ()->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS (btTrans * motionState->inversePrincipalAxis, system->GetInverseInternalScale ()));
    }

    btObject->setWorldTransform (btTrans);
  }

  float csBulletRigidBody::GetMass () const
  {
    //btScalar mass = btBody->getInvMass ();
    //return mass > 0 ? 1 / mass : 0;
    return density * collider->GetVolume ();
  }

  void csBulletRigidBody::SetDensity (float newDensity)
  {
    density = newDensity;
    SetMass (density * collider->GetVolume ());
  }

  float csBulletRigidBody::GetVolume () const
  {
    return collider->GetVolume ();
  }


  float csBulletRigidBody::GetElasticity () 
  {
    return btBody->getRestitution ();
  }

  void csBulletRigidBody::SetElasticity (float elasticity)
  {
    btBody->setRestitution (elasticity);
  }

  void csBulletRigidBody::SetState (RigidBodyState state)
  {
    if (!collider->IsDynamic () && state == STATE_DYNAMIC)
    {
      state = STATE_STATIC;
    }
    if (physicalState != state || !insideWorld)
    {
      RigidBodyState previousState = physicalState;
      physicalState = state;

      if (!btBody) return;

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
          btTransform trans;
          motionState->getWorldTransform (trans);
          trans *= motionState->inversePrincipalAxis;

          delete motionState;

          trans *= collider->GetPrincipalAxisTransform ();
          motionState = CreateMotionState (trans);
          btBody->setMotionState (motionState);
        }
      }

      switch (state)
      {
      case STATE_DYNAMIC:
        {
          btBody->setCollisionFlags (btBody->getCollisionFlags () & ~(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_KINEMATIC_OBJECT));

          btBody->forceActivationState (ACTIVE_TAG);

          btBody->setLinearVelocity (linearVelo);
          btBody->setAngularVelocity (angularVelo);

          SetMassInternal (density * collider->GetVolume ());
          break;
        }
      case STATE_KINEMATIC:
        {
          btBody->setCollisionFlags (
            (btBody->getCollisionFlags ()
            | btCollisionObject::CF_KINEMATIC_OBJECT)
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
          motionState = new csBulletKinematicMotionState (this, trans, principalAxis);
          btBody->setMotionState (motionState);

          // set body kinematic
          btBody->setActivationState (DISABLE_DEACTIVATION);
          btBody->setInterpolationWorldTransform (btBody->getWorldTransform ());
          btBody->setInterpolationLinearVelocity (btVector3 (0.0f, 0.0f, 0.0f));
          btBody->setInterpolationAngularVelocity (btVector3 (0.0f, 0.0f, 0.0f));


          SetMassInternal (0);
          break;
        }
      case STATE_STATIC:
        {
          btBody->setCollisionFlags ((btBody->getCollisionFlags ()
            | btCollisionObject::CF_STATIC_OBJECT)
            & ~btCollisionObject::CF_KINEMATIC_OBJECT);
          btBody->setActivationState (ISLAND_SLEEPING);

          SetMassInternal (0);
          break;
        }
      }

      if (insideWorld)
      {
        sector->bulletWorld->addRigidBody (btBody);
      }

      if (state == STATE_DYNAMIC)
      {
        // dynamic now
        SetEnabled (true);
      }
    }
  }

  void csBulletRigidBody::SetLinearVelocity (const csVector3& vel)
  {
    CS_ASSERT (physicalState == STATE_DYNAMIC);
    if (!QueryActor ())
    {
      SetEnabled (true);
    }
    btBody->setLinearVelocity (CSToBullet (vel, system->GetInternalScale ()));
    SetEnabled (true);
  }

  csVector3 csBulletRigidBody::GetLinearVelocity () const
  {
    const btVector3& vel = btBody->getLinearVelocity ();
    return BulletToCS (vel, system->GetInverseInternalScale ());
  }

  void csBulletRigidBody::SetAngularVelocity (const csVector3& vel)
  {
    if (!btBody)
      return; 
    if (physicalState == STATE_DYNAMIC)
    {
      btBody->setAngularVelocity (CSToBullet (vel, 1));
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
      btBody->ADD_CENTRAL_FORCE (CSToBullet (force, system->GetInternalScale ()));
      btBody->activate (true);
    }
  }

  void csBulletRigidBody::AddTorque (const csVector3& torque)
  {
    if (!btBody)
      return; 

    btBody->applyTorque (CSToBullet (torque, system->GetInternalScale () * system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelForce (const csVector3& force)
  {
    if (!btBody)
      return; 

    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    btBody->ADD_CENTRAL_FORCE (CSToBullet (absForce, system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelTorque (const csVector3& torque)
  {
    if (!btBody)
      return; 
    btBody->activate (true);

    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absTorque = trans.This2Other (torque);
    btBody->applyTorque (CSToBullet (absTorque, system->GetInternalScale () * system->GetInternalScale ()));
  }


  void csBulletRigidBody::AddForceAtPos (const csVector3& force, const csVector3& pos)
  {
    if (!btBody)
      return; 
    btBody->activate (true);

    btVector3 btForce (CSToBullet (force, system->GetInternalScale ()));
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 relPos = trans.Other2This (pos);
    
    btBody->ADD_FORCE (btForce, CSToBullet (relPos, system->GetInternalScale ()));
  }

  void csBulletRigidBody::AddForceAtRelPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 
    
    btBody->activate (true);
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 relPos = trans.Other2This (pos);
    btBody->ADD_FORCE (CSToBullet (force, system->GetInternalScale ()), CSToBullet (relPos, system->GetInternalScale ()));
  }

  void csBulletRigidBody::AddRelForceAtPos (const csVector3& force, const csVector3& pos)
  {
    if (!btBody)
      return; 
    
    btBody->activate (true);
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    csVector3 relPos = trans.Other2This (pos);
    btBody->ADD_FORCE (CSToBullet (absForce, system->GetInternalScale ()),
      CSToBullet (relPos, system->GetInternalScale ()));
  }

  void csBulletRigidBody::AddRelForceAtRelPos (const csVector3& force,
    const csVector3& pos)
  {
    if (!btBody)
      return; 
    
    btBody->activate (true);
    csOrthoTransform trans = csBulletCollisionObject::GetTransform ();
    csVector3 absForce = trans.This2Other (force);
    btBody->ADD_FORCE (CSToBullet (absForce, system->GetInternalScale ()),
      CSToBullet (pos, system->GetInternalScale ()));
  }

  csVector3 csBulletRigidBody::GetTotalForce () const
  {
    if (!btBody)
      return csVector3 (0.0f);

    return BulletToCS (btBody->getTotalForce (), system->GetInverseInternalScale ());
  }

  csVector3 csBulletRigidBody::GetTotalTorque () const
  {
    if (!btBody)
      return csVector3 (0.0f);
    
    return BulletToCS (btBody->getTotalTorque (), system->GetInverseInternalScale ());
  }

  float csBulletRigidBody::GetLinearDamping () 
  {
    return btBody->getLinearDamping ();
  }

  void csBulletRigidBody::SetLinearDamping (float d)
  {
    btBody->setDamping (d, GetAngularDamping ());
  }

  float csBulletRigidBody::GetAngularDamping ()
  {
    return btBody->getAngularDamping ();
  }

  void csBulletRigidBody::SetAngularDamping (float d)
  {
    btBody->setDamping (GetLinearDamping (), d);
  }

  csVector3 csBulletRigidBody::GetAngularFactor () const { return BulletToCS (btBody->getAngularFactor (), system->GetInverseInternalScale ()); }
  void csBulletRigidBody::SetAngularFactor (const csVector3& f) { btBody->setAngularFactor (CSToBullet (f, system->GetInternalScale ())); }

  btCollisionObject* csBulletRigidBody::CreateBulletObject ()
  {
    btTransform trans;
    motionState->getWorldTransform (trans);
    trans = trans * motionState->inversePrincipalAxis;
    
    btCollisionShape* shape = collider->GetOrCreateBulletShape ();
    btScalar mass = GetMass ();
    trans *= collider->GetPrincipalAxisTransform ();

    btRigidBody::btRigidBodyConstructionInfo infos (mass, CreateMotionState (trans), shape, mass * collider->GetLocalInertia ());

    infos.m_friction = GetFriction ();
    infos.m_restitution = GetElasticity ();
    infos.m_linearDamping = GetLinearDamping ();
    infos.m_angularDamping = GetAngularDamping ();

    // create new rigid body
    btRigidBody* body = new btRigidBody (infos);

    body->setLinearVelocity (btBody->getLinearVelocity ());
    body->setAngularVelocity (btBody->getAngularVelocity ());
    body->setAngularFactor (btBody->getAngularFactor ());
    //clone->btBody->setLinearFactor (GetLinearFactor ());

    return body;
  }

  csPtr<iCollisionObject> csBulletRigidBody::CloneObject ()
  {
    csBulletRigidBody* clone = new csBulletRigidBody (system);
    clone->collider = collider;
    clone->collGroup = collGroup;

    btTransform trans;
    trans.setIdentity ();
    clone->CreateMotionState (trans);

    clone->btObject = clone->btBody = static_cast<btRigidBody*>(CreateBulletObject ());
    clone->btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*>(this));

    clone->SetState (physicalState);

    // Wake both up
    SetEnabled (true);
    clone->SetEnabled (true);

    return csPtr<iCollisionObject>(clone);
  }

  bool csBulletRigidBody::GetGravityEnabled () const
  {
    return !(btBody->getFlags () & BT_DISABLE_WORLD_GRAVITY);
  }

  void csBulletRigidBody::SetGravityEnabled (bool enabled)
  {
    int flags = btBody->getFlags ();
    if (enabled)
    {
      flags &= ~BT_DISABLE_WORLD_GRAVITY;
    }
    else
    {
      flags |= BT_DISABLE_WORLD_GRAVITY;
    }
    btBody->setFlags (flags);
  }


  // ##########################################################################################
  // csBulletDefaultKinematicCallback implementation

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
    iSceneNode* sceneNode = rigBody->GetAttachedSceneNode ();
    if (sceneNode)
    {
      transform = sceneNode->GetMovable ()->GetFullTransform ();
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
