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

/**
 * Based on btKinematicActorController.
 */

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"


#include "cssysdef.h"
#include "kinematicactorcontroller.h"

#include "common2.h"


CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  // static helper method
  static btVector3
    getNormalizedVector (const btVector3& v)
  {
    btVector3 n = v.normalized ();
    if (n.length () < SIMD_EPSILON) {
      n.setValue (0, 0, 0);
    }
    return n;
  }

  /*
   * Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
   *
   * from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
   */
  static inline btVector3 BtVectorComputeReflectionDirection (const btVector3& direction, const btVector3& normal)
  {
    return direction - (btScalar (2.0) * direction.dot (normal)) * normal;
  }

  /*
   * Returns the portion of 'direction' that is parallel to 'normal'
   */
  static inline btVector3 BtVectorNormalComponent (const btVector3& direction, const btVector3& normal)
  {
    btScalar magnitude = direction.dot (normal);
    return normal * magnitude;
  }

  /*
   * Returns the portion of 'direction' that is perpendicular to 'normal'
   */
  static inline btVector3 BtVectorTangentialComponent (const btVector3& direction, const btVector3& normal)
  {
    return direction - BtVectorNormalComponent (direction, normal);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Some extra Bullet vector operations

  ///Reflect the vector d around the vector r
  inline btVector3 reflect (const btVector3& d, const btVector3& r)
  {
    return d - (btScalar (2.0) * d.dot (r)) * r;
  }


  csKinematicActorController::csKinematicActorController
    (btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight, int upAxis)
  {
    m_upAxis = upAxis;
    m_addedMargin = 0.02f;
    m_walkDirection.setValue (0,0,0);
    m_useGhostObjectSweepTest = true;
    m_ghostObject = ghostObject;
    m_stepHeight = stepHeight;
    m_turnAngle = btScalar (0.0);
    m_convexShape=convexShape;	
    m_useWalkDirection = true;	// use walk direction by default, legacy behavior
    m_velocityTimeInterval = 0.0;
    m_verticalVelocity = 0.0;
    m_verticalOffset = 0.0;
    m_gravity = 9.8 * 3 ; // 3G acceleration. hu?
    m_jumpSpeed = 10.0; // ?
    m_wasOnGround = false;
    m_wasJumping = false;
    setMaxSlope (btRadians (45.0));
  }

  csKinematicActorController::~csKinematicActorController ()
  {
  }

  btPairCachingGhostObject* csKinematicActorController::getGhostObject ()
  {
    return m_ghostObject;
  }

  bool csKinematicActorController::recoverFromPenetration ( btCollisionWorld* collisionWorld)
  {

    bool penetration = false;

    collisionWorld->getDispatcher ()->dispatchAllCollisionPairs (m_ghostObject->getOverlappingPairCache (), collisionWorld->getDispatchInfo (), collisionWorld->getDispatcher ());

    m_currentPosition = m_ghostObject->getWorldTransform ().getOrigin ();

    btScalar maxPen = btScalar (0.0);
    for (int i = 0; i < m_ghostObject->getOverlappingPairCache ()->getNumOverlappingPairs (); i++)
    {
      m_manifoldArray.resize (0);

      btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache ()->getOverlappingPairArray ()[i];

      if (collisionPair->m_algorithm)
        collisionPair->m_algorithm->getAllContactManifolds (m_manifoldArray);


      for (int j=0;j<m_manifoldArray.size ();j++)
      {
        btPersistentManifold* manifold = m_manifoldArray[j];
        btScalar directionSign = manifold->getBody0 () == m_ghostObject ? btScalar (-1.0) : btScalar (1.0);
        for (int p=0;p<manifold->getNumContacts ();p++)
        {
          const btManifoldPoint&pt = manifold->getContactPoint (p);

          btScalar dist = pt.getDistance ();

          if (dist < 0.0)
          {
            if (dist < maxPen)
            {
              maxPen = dist;
              m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

            }
            m_currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar (0.2);
            penetration = true;
          } else {
            //printf ("touching %f\n", dist);
          }
        }

        //manifold->clearManifold ();
      }
    }
    btTransform newTrans = m_ghostObject->getWorldTransform ();
    newTrans.setOrigin (m_currentPosition);
    m_ghostObject->setWorldTransform (newTrans);
    //	printf ("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
    return penetration;
  }

  void csKinematicActorController::stepUp ( btCollisionWorld* world)
  {
    // phase 1: up
    btTransform start, end;
    m_targetPosition = m_currentPosition + getUpAxisDirections ()[m_upAxis] * (m_stepHeight + (m_verticalOffset > 0.f?m_verticalOffset:0.f));

    start.setIdentity ();
    end.setIdentity ();

    /* FIXME: Handle penetration properly */
    start.setOrigin (m_currentPosition + getUpAxisDirections ()[m_upAxis] * (m_convexShape->getMargin () + m_addedMargin));
    end.setOrigin (m_targetPosition);

    csKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, -getUpAxisDirections ()[m_upAxis], btScalar (0.7071));
    callback.m_collisionFilterGroup = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterMask;

    if (m_useGhostObjectSweepTest)
    {
      m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, world->getDispatchInfo ().m_allowedCcdPenetration);
    }
    else
    {
      world->convexSweepTest (m_convexShape, start, end, callback);
    }

    if (callback.hasHit ())
    {
      // Only modify the position if the hit was a slope and not a wall or ceiling.
      if (callback.m_hitNormalWorld.dot (getUpAxisDirections ()[m_upAxis]) > 0.0)
      {
        // we moved up only a fraction of the step height
        m_currentStepOffset = m_stepHeight * callback.m_closestHitFraction;
        m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
      }
      m_verticalVelocity = 0.0;
      m_verticalOffset = 0.0;
    } else {
      m_currentStepOffset = m_stepHeight;
      m_currentPosition = m_targetPosition;
    }
  }

  void csKinematicActorController::updateTargetPositionBasedOnCollision (const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag)
  {
    btVector3 movementDirection = m_targetPosition - m_currentPosition;
    btScalar movementLength = movementDirection.length ();
    if (movementLength>SIMD_EPSILON)
    {
      movementDirection.normalize ();

      btVector3 reflectDir = BtVectorComputeReflectionDirection (movementDirection, hitNormal);
      reflectDir.normalize ();

      btVector3 parallelDir, perpindicularDir;

      perpindicularDir = BtVectorTangentialComponent (reflectDir, hitNormal);

      m_targetPosition = m_currentPosition;
      if (0)//tangentMag != 0.0)
      {
        parallelDir = BtVectorNormalComponent (reflectDir, hitNormal);
        btVector3 parComponent = parallelDir * btScalar (tangentMag*movementLength);
        //			printf ("parComponent=%f,%f,%f\n",parComponent[0],parComponent[1],parComponent[2]);
        m_targetPosition +=  parComponent;
      }

      if (normalMag != 0.0)
      {
        btVector3 perpComponent = perpindicularDir * btScalar (normalMag*movementLength);
        //			printf ("perpComponent=%f,%f,%f\n",perpComponent[0],perpComponent[1],perpComponent[2]);
        m_targetPosition += perpComponent;
      }
    } else
    {
      //		printf ("movementLength don't normalize a zero vector\n");
    }
  }

  void csKinematicActorController::stepForwardAndStrafe ( btCollisionWorld* collisionWorld, const btVector3& walkMove)
  {
    // printf ("m_normalizedDirection=%f,%f,%f\n",
    // 	m_normalizedDirection[0],m_normalizedDirection[1],m_normalizedDirection[2]);
    // phase 2: forward and strafe
    btTransform start, end;
    m_targetPosition = m_currentPosition + walkMove;

    start.setIdentity ();
    end.setIdentity ();

    btScalar fraction = 1.0;
    btScalar distance2 = (m_currentPosition-m_targetPosition).length2 ();
    //	printf ("distance2=%f\n",distance2);

    if (m_touchingContact)
    {
      if (m_normalizedDirection.dot (m_touchingNormal) > btScalar (0.0))
      {
        updateTargetPositionBasedOnCollision (m_touchingNormal);
      }
    }

    int maxIter = 10;

    while (fraction > btScalar (0.01) && maxIter-- > 0)
    {
      start.setOrigin (m_currentPosition);
      end.setOrigin (m_targetPosition);
      btVector3 sweepDirNegative (m_currentPosition - m_targetPosition);

      csKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, sweepDirNegative, btScalar (0.0));
      callback.m_collisionFilterGroup = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterGroup;
      callback.m_collisionFilterMask = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterMask;


      btScalar margin = m_convexShape->getMargin ();
      m_convexShape->setMargin (margin + m_addedMargin);


      if (m_useGhostObjectSweepTest)
      {
        m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo ().m_allowedCcdPenetration);
      } else
      {
        collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo ().m_allowedCcdPenetration);
      }

      m_convexShape->setMargin (margin);


      fraction -= callback.m_closestHitFraction;

      if (callback.hasHit () && !btGhostObject::upcast (callback.m_hitCollisionObject))   // ignore ghost objects
      {	
        // we moved only a fraction
        //btScalar hitDistance;
        //hitDistance = (callback.m_hitPointWorld - m_currentPosition).length ();

        //			m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);

        updateTargetPositionBasedOnCollision (callback.m_hitNormalWorld);
        btVector3 currentDir = m_targetPosition - m_currentPosition;
        distance2 = currentDir.length2 ();
        if (distance2 > SIMD_EPSILON)
        {
          currentDir.normalize ();
          /* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
          if (currentDir.dot (m_normalizedDirection) <= btScalar (0.0))
          {
            break;
          }
        } 
        else
        {
          //				printf ("currentDir: don't normalize a zero vector\n");
          break;
        }

      } else {
        // we moved whole way
        m_currentPosition = m_targetPosition;
      }

      //	if (callback.m_closestHitFraction < SMALL_EPSILON)
      //		break;

    }
  }

  void csKinematicActorController::stepDown ( btCollisionWorld* collisionWorld, btScalar dt)
  {
    btTransform start, end;

    // phase 3: down
    /*btScalar additionalDownStep = (m_wasOnGround && !onGround ()) ? m_stepHeight : 0.0;
    btVector3 step_drop = getUpAxisDirections ()[m_upAxis] * (m_currentStepOffset + additionalDownStep);
    btScalar downVelocity = (additionalDownStep < SMALL_EPSILON && m_verticalVelocity<0.0?-m_verticalVelocity:0.0) * dt;
    btVector3 gravity_drop = getUpAxisDirections ()[m_upAxis] * downVelocity; 
    m_targetPosition -= (step_drop + gravity_drop);*/

    btScalar downVelocity = (m_verticalVelocity<0.f?-m_verticalVelocity:0.f) * dt;
    if (downVelocity > 0.0 && downVelocity < m_stepHeight
      && (m_wasOnGround || !m_wasJumping))
    {
      downVelocity = m_stepHeight;
    }

    btVector3 step_drop = getUpAxisDirections ()[m_upAxis] * (m_currentStepOffset + downVelocity);
    m_targetPosition -= step_drop;

    start.setIdentity ();
    end.setIdentity ();

    start.setOrigin (m_currentPosition);
    end.setOrigin (m_targetPosition);

    csKinematicClosestNotMeConvexResultCallback callback (m_ghostObject, getUpAxisDirections ()[m_upAxis], m_maxSlopeCosine);
    callback.m_collisionFilterGroup = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = getGhostObject ()->getBroadphaseHandle ()->m_collisionFilterMask;

    if (m_useGhostObjectSweepTest)
    {
      m_ghostObject->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo ().m_allowedCcdPenetration);
    } else
    {
      collisionWorld->convexSweepTest (m_convexShape, start, end, callback, collisionWorld->getDispatchInfo ().m_allowedCcdPenetration);
    }

    if (callback.hasHit ())
    {
      // we dropped a fraction of the height -> hit floor
      m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
      m_verticalVelocity = 0.0;
      m_verticalOffset = 0.0;
      m_wasJumping = false;
    } else {
      // we dropped the full height

      m_currentPosition = m_targetPosition;
    }
  }



  void csKinematicActorController::setWalkDirection
    (
    const btVector3& walkDirection
    )
  {
    m_useWalkDirection = true;
    m_walkDirection = walkDirection;
    m_normalizedDirection = getNormalizedVector (m_walkDirection);
  }



  void csKinematicActorController::setVelocityForTimeInterval
    (
    const btVector3& velocity,
    btScalar timeInterval
    )
  {
    //	printf ("setVelocity!\n");
    //	printf ("  interval: %f\n", timeInterval);
    //	printf ("  velocity: (%f, %f, %f)\n",
    //		 velocity.x (), velocity.y (), velocity.z ());

    m_useWalkDirection = false;
    m_walkDirection = velocity;
    m_normalizedDirection = getNormalizedVector (m_walkDirection);
    m_velocityTimeInterval = timeInterval;
  }

  void csKinematicActorController::preStep (  btCollisionWorld* collisionWorld)
  {

    int numPenetrationLoops = 0;
    m_touchingContact = false;
    while (recoverFromPenetration (collisionWorld))
    {
      numPenetrationLoops++;
      m_touchingContact = true;
      if (numPenetrationLoops > 4)
      {
        //printf ("character could not recover from penetration = %d\n", numPenetrationLoops);
        break;
      }
    }

    m_currentPosition = m_ghostObject->getWorldTransform ().getOrigin ();
    m_targetPosition = m_currentPosition;
    //	printf ("m_targetPosition=%f,%f,%f\n",m_targetPosition[0],m_targetPosition[1],m_targetPosition[2]);


  }

#include <stdio.h>

  void csKinematicActorController::playerStep (  btCollisionWorld* collisionWorld, btScalar dt)
  {
    //	printf ("playerStep (): ");
    //	printf ("  dt = %f", dt);

    // quick check...
    if (!m_useWalkDirection && m_velocityTimeInterval <= 0.0) {
      //		printf ("\n");
      return;		// no motion
    }

    m_wasOnGround = onGround ();

    // Update fall velocity.
    m_verticalVelocity -= m_gravity * dt;
    m_verticalOffset = m_verticalVelocity * dt;


    btTransform xform;
    xform = m_ghostObject->getWorldTransform ();

    //	printf ("walkDirection (%f,%f,%f)\n",walkDirection[0],walkDirection[1],walkDirection[2]);
    //	printf ("walkSpeed=%f\n",walkSpeed);

    stepUp (collisionWorld);
    if (m_useWalkDirection) {
      stepForwardAndStrafe (collisionWorld, m_walkDirection);
    } else {
      //printf ("  time: %f", m_velocityTimeInterval);
      // still have some time left for moving!
      btScalar dtMoving =
        (dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
      m_velocityTimeInterval -= dt;

      // how far will we move while we are moving?
      btVector3 move = m_walkDirection * dtMoving;

      //printf ("  dtMoving: %f", dtMoving);

      // okay, step
      stepForwardAndStrafe (collisionWorld, move);
    }
    stepDown (collisionWorld, dt);

    // printf ("\n");

    xform.setOrigin (m_currentPosition);
    m_ghostObject->setWorldTransform (xform);
  }

  void csKinematicActorController::jump ()
  {
    m_verticalVelocity = m_jumpSpeed;
    m_wasJumping = true;

#if 0
    currently no jumping.
      btTransform xform;
    m_rigidBody->getMotionState ()->getWorldTransform (xform);
    btVector3 up = xform.getBasis ()[1];
    up.normalize ();
    btScalar magnitude = (btScalar (1.0)/m_rigidBody->getInvMass ()) * btScalar (8.0);
    m_rigidBody->applyCentralImpulse (up * magnitude);
#endif
  }

  void csKinematicActorController::setJumpSpeed (btScalar jumpSpeed)
  {
    m_jumpSpeed = jumpSpeed;
  }

  btScalar csKinematicActorController::getJumpSpeed () const
  {
    return m_jumpSpeed;
  }

  void csKinematicActorController::setGravity (btScalar gravity)
  {
    m_gravity = gravity;
  }

  btScalar csKinematicActorController::getGravity () const
  {
    return m_gravity;
  }

  void csKinematicActorController::setMaxSlope (btScalar slopeRadians)
  {
    m_maxSlopeRadians = slopeRadians;
    m_maxSlopeCosine = btCos (slopeRadians);
  }

  btScalar csKinematicActorController::getMaxSlope () const
  {
    return m_maxSlopeRadians;
  }

  bool csKinematicActorController::onGround () const
  {
    return m_verticalVelocity < SMALL_EPSILON && m_verticalOffset < SMALL_EPSILON;
  }


  btVector3* csKinematicActorController::getUpAxisDirections ()
  {
    static btVector3 sUpAxisDirection[3] = { btVector3 (1.0f, 0.0f, 0.0f), btVector3 (0.0f, 1.0f, 0.0f), btVector3 (0.0f, 0.0f, 1.0f) };

    return sUpAxisDirection;
  }

  void csKinematicActorController::debugDraw (btIDebugDraw* debugDrawer)
  {
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
