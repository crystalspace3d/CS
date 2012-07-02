/*
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

/**
TODO: Implement general surface hugging. Kinematic step direction can currently only hug horizontal surfaces.
*/


#include "cssysdef.h"
#include "dynamicactor.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

using namespace CS::Physics;
using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  static const csScalar DefaultGroundAngleCosThresh = csScalar(0.7);             // min cos of angle between ground and up-axis (45 degrees)
  static const btVector3 BTUpVector(CSToBullet(UpVector, 1));
  static const csScalar AddedMargin = csScalar(0.02);


  csBulletActorMotionState::csBulletActorMotionState(csBulletRigidBody* body,
    const btTransform& initialTransform,
    const btTransform& principalAxis) :
  csBulletMotionState(body, initialTransform, principalAxis)
  {

  }

  void csBulletActorMotionState::setWorldTransform (const btTransform& trans)
  {
    csBulletMotionState::setWorldTransform(trans);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // csBulletDynamicActor

  csBulletDynamicActor::csBulletDynamicActor(csBulletSystem* sys) : scfImplementationType(this, sys),
    onGround(false)
  {
  }

  csBulletDynamicActor::~csBulletDynamicActor ()
  {

  }

  void csBulletDynamicActor::CreateDynamicActor(DynamicActorProperties* props)
  {
    if (!props->GetCollisionGroup().name.Length())
    {
      props->SetCollisionGroup(system->FindCollisionGroup("Actor"));
    }
    CreateRigidBodyObject(props);

    SetStepHeight(props->GetStepHeight());
    SetWalkSpeed(props->GetWalkSpeed());
    SetJumpSpeed(props->GetJumpSpeed());
    SetUseKinematicSteps(props->GetUseKinematicSteps());
    SetAirControlFactor(props->GetAirControlFactor());
    
    if (collider->GetColliderType() == COLLIDER_COMPOUND)
    {
      // non-convex actor shape currently not supported with kinematic step correction
      kinematicSteps = false;
    }
    
    btBody->setAngularFactor(btVector3(0, 1, 0));                      // don't turn about anything but the y-axis
  }

  csBulletMotionState* csBulletDynamicActor::CreateMotionState(const btTransform& trans)
  {
    return motionState = new csBulletActorMotionState(this, trans, collider->GetBtPrincipalAxisTransform());
  }

  bool csBulletDynamicActor::AddBulletObject()
  {
    return csBulletRigidBody::AddBulletObject();
  }

  bool csBulletDynamicActor::RemoveBulletObject()
  {
    return csBulletRigidBody::RemoveBulletObject();
  }

  void csBulletDynamicActor::RebuildObject()
  {
    csBulletRigidBody::RebuildObject();
    
    if (collider->GetColliderType() == COLLIDER_COMPOUND)
    {
      // non-convex actor shape currently not supported with kinematic step correction
      kinematicSteps = false;
    }
  }

  void csBulletDynamicActor::SetTransform(const csOrthoTransform& trans)
  {
    csBulletRigidBody::SetTransform(trans);
  }

  void csBulletDynamicActor::Walk(csVector3 newVel)
  {
    newVel.Normalize();
    newVel *= walkSpeed;

    csVector3 vel = GetLinearVelocity();
    if (IsFreeFalling())
    {
      // cannot entirely control movement mid-air
      csScalar realAirFactor = airControlFactor * sector->GetWorldTimeStep();
      newVel = realAirFactor * newVel;
      newVel += (1.f - realAirFactor) * vel;
    }

    SetLinearVelocity(newVel);
  }

  void csBulletDynamicActor::WalkHorizontal(csVector2 newVel2)
  {
    newVel2.Normalize();
    newVel2 *= walkSpeed;

    csVector3 vel = GetLinearVelocity();
    if (IsFreeFalling())
    {
      // cannot entirely control movement mid-air
      csScalar realAirFactor = airControlFactor * sector->GetWorldTimeStep();
      newVel2 = realAirFactor * newVel2;
      newVel2 += (1.f - realAirFactor) * HORIZONTAL_COMPONENT(vel);
    }

    // previous vertical movement is unchanged
    csVector3 newVel = HV_VECTOR3(newVel2, vel[UpAxis]);
    SetLinearVelocity(newVel);
  }

  /// Applies an upward impulse to this actor, and an inverse impulse to objects beneath
  void csBulletDynamicActor::Jump()
  {
    // apply upward impulse to actor
    csVector3 vel = GetLinearVelocity();
    vel[UpAxis] += GetJumpSpeed();
    SetLinearVelocity(vel);

    // Apply inverse of impulse to objects beneath.
    // Of course this is not quite correct, since the jump force on the actor should depend on the restitution of both objects.
    // (imagine jumping from a pillow vs. jumping from concrete)
    // (imagine jumping in professional NIKE's vs jumping in cozy slippers)

    // split inverse of impulse between all objects
   /* csScalar touchedGroundObjectCountInv = 1.f / touchedGroundObjectCount;
    vel[UpAxis] = -GetJumpSpeed() * touchedGroundObjectCountInv;
    vel[HorizontalAxis1] = -vel[HorizontalAxis1] * touchedGroundObjectCountInv;
    vel[HorizontalAxis2] = -vel[HorizontalAxis2] * touchedGroundObjectCountInv;

    btVector3 btVel = CSToBullet(vel, system->getInternalScale());*/

    // iterate over all objects touching the object
    // TODO: Fix me
    //for (int j = 0; j < manifoldArray.size(); ++j)
    //{
    //  btPersistentManifold* manifold = manifoldArray[j];
    //  csScalar directionSign;
    //  btCollisionObject* obj;
    //  if (manifold->getBody0() == ghost)
    //  {
    //    directionSign = csScalar(-1.0);
    //    obj = static_cast<btCollisionObject*> (manifold->getBody1());
    //  }
    //  else
    //  {
    //    directionSign = csScalar(1.0);
    //    obj = static_cast<btCollisionObject*> (manifold->getBody0());
    //  }

    //  btRigidBody* body = btRigidBody::upcast(obj);
    //  if (!body || body == btBody) continue;

    //  // iterate over all contacts with the object
    //  for (int p=0; p < manifold->getNumContacts(); ++p)
    //  {
    //    const btManifoldPoint&pt = manifold->getContactPoint(p);
    //    csScalar dist = pt.getDistance();
    //    if (dist < 0.0)
    //    {
    //      if (pt.m_normalWorldOnB.dot(BTUpVector) > DefaultGroundAngleCosThresh)
    //      {
    //        // add velocity to current velocity
    //        body->setLinearVelocity(pt.m_normalWorldOnB * directionSign * (btVel + body->getLinearVelocity()));
    //        break;
    //      }
    //    } 
    //  }
    //}
  }

  /// Whether actor cannot currently fully control movement
  bool csBulletDynamicActor::IsFreeFalling() const
  { 
    return 
      (!IsOnGround() ||                 // not in contact with ground or
      IsMovingUpward()) &&              // moving upward (away from ground)
      DoesGravityApply();               // and gravity applies
  }

  void csBulletDynamicActor::UpdateAction(csScalar dt)
  {
    onGround = TestOnGround();

    if (!kinematicSteps) return;
    
    if (!IsFreeFalling())
    {
      stepUp(dt);
      stepForwardAndStrafe(dt);
      stepDown(dt);
      SetLinearVelocity(0);              // stop moving
    }
  }
  
  inline btConvexShape* csBulletDynamicActor::GetConvexShape() const { return (btConvexShape*)btBody->getCollisionShape(); }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Code based on btKinematicCharacterController

  void csBulletDynamicActor::stepUp (csScalar dt)
  {
    btVector3& currentPosition = btObject->getWorldTransform().getOrigin();
    m_targetPosition = currentPosition + BTUpVector * stepHeight;
    
    // TODO: This method tests intersections with objects beneath the object when moving upward. Seems highly unnecessary.

    // phase 1: up
    btTransform start;
    start.setIdentity ();
    start.setOrigin (currentPosition + BTUpVector * (GetConvexShape()->getMargin() + AddedMargin));
    
    btTransform end;
    end.setIdentity ();
    end.setOrigin (m_targetPosition);

    csKinematicClosestNotMeConvexResultCallback callback (btObject, -BTUpVector, csScalar(DefaultGroundAngleCosThresh));
    callback.m_collisionFilterGroup = collGroup.value;
    callback.m_collisionFilterMask = collGroup.mask;
    
    sector->GetBulletWorld()->convexSweepTest (GetConvexShape(), start, end, callback);

    if (callback.hasHit())
    {
      // Only modify the position if the hit was a slope and not a wall or ceiling.
      if(callback.m_hitNormalWorld.dot(BTUpVector) > 0.0)
      {
        // we moved up only a fraction of the step height, then hit something underneath us!?
        currentPosition.setInterpolate3 (currentPosition, m_targetPosition, callback.m_closestHitFraction);
      }
      m_currentStepOffset = stepHeight * callback.m_closestHitFraction;
    } 
    else 
    {
      m_currentStepOffset = stepHeight;
      currentPosition = m_targetPosition;
    }
  }

  bool csBulletDynamicActor::adjustHorizontalMovement (const btVector3& hitNormal, btVector3& dist, csScalar normalMag, csScalar tangentMag)
  {
    btVector3& currentPosition = btObject->getWorldTransform().getOrigin();

    csScalar movementLength2 = dist.length2();
    if (movementLength2 > EPSILON)
    {
      csScalar movementLength = sqrt(movementLength2);
      dist /= movementLength;              // normalize

      btVector3 reflectDir = BtVectorComputeReflectionDirection(dist, hitNormal);
      reflectDir.normalize();

      btVector3 tangentialComponent = BtVectorTangentialComponent(reflectDir, hitNormal);

      m_targetPosition = currentPosition;
      //if (normalMag != 0.0)
      //{
      //  btVector3 parallelDir = parallelComponent (reflectDir, hitNormal);
      //  btVector3 parComponent = parallelDir * csScalar (normalMag*movementLength);
      //  //			printf("parComponent=%f,%f,%f\n",parComponent[0],parComponent[1],parComponent[2]);
      //  m_targetPosition +=  parComponent;
      //  dist += ...;
      //}

      //if (tangentMag != 0.0)
      {
        //			printf("perpComponent=%f,%f,%f\n",perpComponent[0],perpComponent[1],perpComponent[2]);
        tangentialComponent *= csScalar (tangentMag*movementLength);
        m_targetPosition = currentPosition + tangentialComponent;
        dist = tangentialComponent;
      }
      return true;
    } 
    return false;
  }

  void csBulletDynamicActor::stepForwardAndStrafe (csScalar dt)
  {
    btVector3 dist = btBody->getLinearVelocity() * dt;
    BulletVectorComponent(dist, UpAxis) = 0;         // only horizontal movement
    
    btVector3& currentPosition = btObject->getWorldTransform().getOrigin();

    // printf("m_normalizedDirection=%f,%f,%f\n",
    // 	m_normalizedDirection[0],m_normalizedDirection[1],m_normalizedDirection[2]);
    // phase 2: forward and strafe
    btTransform start, end;
    m_targetPosition = currentPosition + dist;

    start.setIdentity ();
    end.setIdentity ();

    csScalar fraction = 1.0;

    int iterations = 10;

    do
    {
      start.setOrigin (currentPosition);
      end.setOrigin (m_targetPosition);
      btVector3 sweepDirNegative(currentPosition - m_targetPosition);

      csKinematicClosestNotMeConvexResultCallback callback (btObject, sweepDirNegative, csScalar(0.1));  // ignore anything facing down
      callback.m_collisionFilterGroup = collGroup.value;
      callback.m_collisionFilterMask = collGroup.mask;

      csScalar margin = GetConvexShape()->getMargin();
      GetConvexShape()->setMargin(margin + AddedMargin);

      // Check for collisions along the path
      sector->GetBulletWorld()->convexSweepTest (GetConvexShape(), start, end, callback, sector->GetBulletWorld()->getDispatchInfo().m_allowedCcdPenetration);

      GetConvexShape()->setMargin(margin);

      fraction -= callback.m_closestHitFraction;

      if (callback.hasHit())
      {	
        // we hit something
        onGround = true;

        // currentPosition.setInterpolate3 (currentPosition, m_targetPosition, callback.m_closestHitFraction);

        // move parallel to surface of hit object
        if (!adjustHorizontalMovement (callback.m_hitNormalWorld, dist)) 
          break;    // cannot move further
      }
      else 
      {
        // no obstruction
        currentPosition = m_targetPosition;
        break;
      }
    }
    while (fraction > csScalar(0.01) && --iterations >= 0);
  }

  void csBulletDynamicActor::stepDown (csScalar dt)
  {
    btVector3& currentPosition = btObject->getWorldTransform().getOrigin();

    btTransform start, end;

    // phase 3: down
    
    // we went up by m_currentStepOffset during the stepUp routine, and what goes up must come down!
    BulletVectorComponent(m_targetPosition, UpAxis) -= m_currentStepOffset;

    start.setIdentity ();
    end.setIdentity ();

    start.setOrigin (currentPosition);
    end.setOrigin (m_targetPosition);

    csKinematicClosestNotMeConvexResultCallback callback (btObject, BTUpVector, DefaultGroundAngleCosThresh);
    callback.m_collisionFilterGroup = collGroup.value;
    callback.m_collisionFilterMask = collGroup.mask;

    sector->GetBulletWorld()->convexSweepTest (GetConvexShape(), start, end, callback, sector->GetBulletWorld()->getDispatchInfo().m_allowedCcdPenetration);

    if (callback.hasHit())
    {
      // we dropped a fraction of the height -> hit floor
      currentPosition.setInterpolate3 (currentPosition, m_targetPosition, callback.m_closestHitFraction);
    } 
    else 
    {
      // we dropped the full height without hitting anything
      currentPosition = m_targetPosition;
    }
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)