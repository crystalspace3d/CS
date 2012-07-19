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

#ifndef __CS_BULLET_DYNAMICACTOR_H__
#define __CS_BULLET_DYNAMICACTOR_H__

#include "common2.h"
#include "rigidbody2.h"

#include "kinematicactorcontroller.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  class csBulletActorMotionState : public csBulletMotionState
  {
  private:

  public:
    csBulletActorMotionState (csBulletRigidBody* body,
		         const btTransform& initialTransform,
		         const btTransform& principalAxis);

    virtual void setWorldTransform (const btTransform& trans);
  };

  class csBulletDynamicActor : public scfVirtImplementationExt1<csBulletDynamicActor,
	csBulletRigidBody, 
        CS::Physics::iDynamicActor>
  {
    friend class csBulletSystem;
    friend class csBulletSector;
    friend class BulletDynamicActorFactory;

  private:
    csScalar stepHeight;
    csScalar walkSpeed, jumpSpeed;
    csScalar airControlFactor;
    bool onGround;
    bool kinematicSteps;

  protected:
    void CreateDynamicActor(CS::Physics::iDynamicActorFactory* props);

    virtual csBulletMotionState* CreateMotionState(const btTransform& trans);

  public:
    csBulletDynamicActor(csBulletSystem* sys);
    virtual ~csBulletDynamicActor();

    bool AddBulletObject();
    bool RemoveBulletObject();
    void RebuildObject();

    // ##########################################################################
    // iActor stuff
    virtual CS::Collisions::iCollisionObject* QueryCollisionObject() { return dynamic_cast<CS::Collisions::iCollisionObject*>(this); }
    
    virtual CS::Collisions::iActor* QueryActor () { return dynamic_cast<CS::Collisions::iActor*>(this); }

    /// Update actor
    virtual void UpdatePreStep (csScalar delta);
    virtual void UpdatePostStep (csScalar delta);
    
    /// Start walking in the given direction. Sets linear velocity. Takes air control into consideration.
    virtual void Walk(csVector3 dir);
  
    /// Start walking in the given horizontal direction with walk speed. Sets linear velocity. Takes air control into consideration.
    virtual void WalkHorizontal(csVector2 newVel2);

    /// Applies an upward impulse to this actor, and an inverse impulse to objects beneath
    virtual void Jump();

    virtual void StopMoving()
    {
      if (!IsFreeFalling())
      {
        csVector3 zero(0);

        SetLinearVelocity(zero);
      }
    }

    /// Whether the actor is not on ground and gravity applies
    virtual bool IsFreeFalling() const;

    /// Whether this actor touches the ground
    //virtual bool IsOnGround() const { return touchedGroundObjectCount > 0; }
    virtual bool IsOnGround() const { return onGround; }

    /// Get the max vertical threshold that this actor can step over
    virtual csScalar GetStepHeight () const { return stepHeight; }
    /// Set the max vertical threshold that this actor can step over
    virtual void SetStepHeight (csScalar h) { stepHeight = h; }

    /// Get the walk speed
    virtual csScalar GetWalkSpeed () const { return walkSpeed; }
    /// Set the walk speed
    virtual void SetWalkSpeed (csScalar s) { walkSpeed = s; }

    /// Get the jump speed
    virtual csScalar GetJumpSpeed () const { return jumpSpeed; }
    /// Set the jump speed
    virtual void SetJumpSpeed (csScalar s) { jumpSpeed = s; }

    /// Determines how much the actor can control movement when free falling
    virtual csScalar GetAirControlFactor () const { return airControlFactor; }
    /// Determines how much the actor can control movement when free falling
    virtual void SetAirControlFactor (csScalar f) { airControlFactor = f; }
    
    /// Get whether to use a kinematic method for smooth steps
    virtual bool GetUseKinematicSteps() const { return kinematicSteps; }
    /// Set whether to use a kinematic method for smooth steps
    virtual void SetUseKinematicSteps(bool u) { kinematicSteps = u; }


    /// Override SetTransform
    virtual void SetTransform(const csOrthoTransform& trans);


    // Kinematic stuff
    inline btConvexShape* GetConvexShape() const;

  protected:
    void stepUp(csScalar dt);
    void stepDown(csScalar dt);
  };
}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif