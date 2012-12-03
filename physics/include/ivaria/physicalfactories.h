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

#ifndef __IVARIA_PHYSICSFACTORIESH__
#define __IVARIA_PHYSICSFACTORIESH__

/**\file
* Physics factories interfaces
*/

#include "csgeom/tri.h"
#include "cstool/primitives.h"
#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "iutil/objreg.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "imesh/genmesh.h"

#include "ivaria/collisionfactories.h"
#include "ivaria/physics.h"

namespace CS 
{ 
namespace Mesh
{
  struct iAnimatedMesh;
}
}

namespace CS 
{ 
namespace Collisions 
{
  struct iCollisionCallback;
  struct iCollisionObject;
  struct iCollisionGroup;
  struct iCollisionObject;
}
}

namespace CS 
{
namespace Physics 
{
  struct iJoint;
  struct iObject;
  struct iRigidBody;
  struct iSoftBody;
  struct iDynamicActor;
  struct iKinematicCallback;
  struct iPhysicalSystem;
  struct iPhysicalSector;

  /**
   * The state of a rigid body.
   */
  enum RigidBodyState
  {
    STATE_STATIC = 0,    /*!< The body is in the static state. */
    STATE_DYNAMIC,       /*!< The body is in the dynamic state. */
    STATE_KINEMATIC      /*!< The body is in the kinematic state. */
  };

/**
 * The type of a physical body.
 */
  enum PhysicalObjectType
  {
    PHYSICAL_OBJECT_RIGIDBODY = 0,
    PHYSICAL_OBJECT_SOFTYBODY,
    PHYSICAL_OBJECT_DYNAMICACTOR
  };

  /**
   * Collection of all properties of a physical object
   */
  struct iPhysicalObjectFactory : public virtual CS::Collisions::iCollisionObjectFactory
  {
    SCF_INTERFACE (CS::Physics::iPhysicalObjectFactory, 1, 0, 0);

    /// Get the PhysicalObjectType of the object whose data is stored in this properties object
    virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const = 0;

    /// Get the density of all objects that will be constructed by this factory
    virtual float GetDensity () const = 0;
    /// Set the density of all objects that will be constructed by this factory
    virtual void SetDensity (float value) = 0;
    
    /// Get the mass of all objects that will be constructed by this factory
    virtual float GetMass () const = 0;
    /// Set the mass of all objects that will be constructed by this factory
    virtual void SetMass (float value) = 0;

    /// Set the friction of all objects that will be constructed by this factory
    virtual void SetFriction (float value) = 0;
    /// Get the friction of all objects that will be constructed by this factory
    virtual float GetFriction () const = 0;
    
    /// Whether this object is affected by gravity
    virtual bool GetGravityEnabled () const = 0;
    /// Whether this object is affected by gravity
    virtual void SetGravityEnabled (bool enabled) = 0;
  };

  // TODO: There are a lot more configurable parameters - See btRigidBodyConstructionInfo:
  /*
		btVector3			m_localInertia;

		///best simulation results using zero restitution.
		btScalar			m_restitution;

		btScalar			m_linearSleepingThreshold;
		btScalar			m_angularSleepingThreshold;

		//Additional damping can help avoiding lowpass jitter motion, help stability for ragdolls etc.
		//Such damping is undesirable, so once the overall simulation quality of the rigid body dynamics system has improved, this should become obsolete
		bool				m_additionalDamping;
		btScalar			m_additionalDampingFactor;
		btScalar			m_additionalLinearDampingThresholdSqr;
		btScalar			m_additionalAngularDampingThresholdSqr;
		btScalar			m_additionalAngularDampingFactor;
  */

  /**
   * Collection of all properties of a rigid body
   */
  struct iRigidBodyFactory : public virtual iPhysicalObjectFactory
  {
    SCF_INTERFACE (CS::Physics::iRigidBodyFactory, 1, 0, 0);

    /// Create a rigid body
    virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody () = 0;

    /// Set the dynamic state of this rigid body factory.
    virtual void SetState (CS::Physics::RigidBodyState state) = 0;
    virtual CS::Physics::RigidBodyState GetState () const = 0;

    /// Set the elasticity of this rigid body factory.
    virtual void SetElasticity (float value) = 0;

    /// Get the elasticity of this rigid body factory.
    virtual float GetElasticity () const = 0;
    /**
    * Set the linear Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    * \sa iDynamicSystem::SetLinearDamping()
    */
    virtual void SetLinearDamping (float d) = 0;

    /// Get the linear Damping for this rigid body.
    virtual float GetLinearDamping () const = 0;

    /**
    * Set the angular Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    */
    virtual void SetAngularDamping (float d) = 0;

    /// Get the angular Damping for this rigid body.
    virtual float GetAngularDamping () const = 0;
  };

  /**
   * Collection of all properties of a soft body
   */
  struct iSoftBodyFactory : public virtual iPhysicalObjectFactory
  {
    SCF_INTERFACE (CS::Physics::iSoftBodyFactory, 1, 0, 0);

    /// Create a soft body
    virtual csPtr<iSoftBody> CreateSoftBody () = 0;
  };

  /**
   * Used to create a one-dimensional softbody
   * \todo Remove that class
   */
  struct iSoftRopeFactory : public virtual iSoftBodyFactory
  {
    SCF_INTERFACE (CS::Physics::iSoftRopeFactory, 1, 0, 0);

    /// Get the start position of the rope
    virtual const csVector3& GetStart () const = 0;
    /// Set the start position of the rope
    virtual void SetStart (const csVector3& v) = 0;
    
    /// Get the end position of the rope
    virtual const csVector3& GetEnd () const = 0;
    /// Set the end position of the rope
    virtual void SetEnd (const csVector3& v) = 0;
    
    /// Get the amount of nodes along the rope
    virtual size_t GetNodeCount () const = 0;
    /// Set the amount of nodes along the rope
    virtual void SetNodeCount (size_t c) = 0;
  };

  /**
   * Used to create a two-dimensional softbody
   * \todo Remove that class
   */
  struct iSoftClothFactory : public virtual iSoftBodyFactory
  {
    SCF_INTERFACE (CS::Physics::iSoftClothFactory, 1, 0, 0);

    /// Get the four corners of the cloth
    virtual const csVector3* GetCorners () const = 0;
    /// Set the four corners of the cloth
    virtual void SetCorners (csVector3 corners[4]) = 0;

    /// Get the two segment counts along the two primary axes
    virtual void GetSegmentCounts (size_t& count1, size_t& count2) const = 0;
    /// Set the two segment counts along the two primary axes
    virtual void SetSegmentCounts (size_t count1, size_t count2) = 0;
    
    /// Get whether there must be diagonal segments in the cloth
    virtual bool GetWithDiagonals () const = 0;
    /// Set whether there must be diagonal segments in the cloth
    virtual void SetWithDiagonals (bool d) = 0;
  };
  
  /**
   * Used to create an arbitrary softbody defined by a given mesh
   * \todo Remove that class
   */
  struct iSoftMeshFactory : public virtual iSoftBodyFactory
  {
    SCF_INTERFACE (CS::Physics::iSoftMeshFactory, 1, 0, 0);

    /// Get the factory that contains the mesh to define the softbody
    virtual iGeneralFactoryState* GetGenmeshFactory () const = 0;
    /// Set the factory that contains the mesh to define the softbody
    virtual void SetGenmeshFactory (iGeneralFactoryState* s) = 0;

  };

  /**
   * Used to create a dynamic actor
   */
  struct iDynamicActorFactory : public virtual iRigidBodyFactory
  {
    SCF_INTERFACE (CS::Physics::iDynamicActorFactory, 1, 0, 0);

    /// Create a dynamic actor
    virtual csPtr<iDynamicActor> CreateDynamicActor () = 0;

    /// Get the max vertical threshold that this actor can step over
    virtual float GetStepHeight () const = 0;
    /// Set the max vertical threshold that this actor can step over
    virtual void SetStepHeight (float h) = 0;

    /// Get the walk speed
    virtual float GetWalkSpeed () const = 0;
    /// Set the walk speed
    virtual void SetWalkSpeed (float s) = 0;

    /// Get the jump speed
    virtual float GetJumpSpeed () const = 0;
    /// Set the jump speed
    virtual void SetJumpSpeed (float s) = 0;

    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual float GetAirControlFactor () const = 0;
    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual void SetAirControlFactor (float f) = 0;
    
    /// Get whether to use a kinematic method for smooth steps
    virtual bool GetUseKinematicSteps () const = 0;
    /// Set whether to use a kinematic method for smooth steps
    virtual void SetUseKinematicSteps (bool u) = 0;
  };

  /**
   * A joint that can constrain the relative motion between two iPhysicalBody.
   * For instance if all motion in along the local X axis is constrained
   * then the bodies will stay motionless relative to each other
   * along an x axis rotated and positioned by the joint's transform.
   *
   * Main creators of instances implementing this interface:
   * - iPhysicalSystem::CreateJoint()
   * 
   * Main users of this interface:
   * - iPhysicalSector
   * \todo Joint factories
   */
  struct iJointFactory : public virtual iBase
  {
    SCF_INTERFACE (CS::Physics::iJointFactory, 1, 0, 0);

    /// Create a joint instance
    virtual csPtr<CS::Physics::iJoint> CreateJoint () = 0;

    /**
     * Set the translation constraints on the 3 axes. If true is
     * passed for an axis then the Joint will constrain all motion along
     * that axis (ie no motion will be allowed). If false is passed in then all motion along that
     * axis is free, but bounded by the minimum and maximum distance
     * if set. Set force_update to true if you want to apply the changes 
     * right away.
     */
    virtual void SetTransConstraints (bool X, bool Y, bool Z) = 0;

    /// True if this axis' translation is constrained.
    virtual bool IsXTransConstrained () const = 0;

    /// True if this axis' translation is constrained.
    virtual bool IsYTransConstrained () const = 0;

    /// True if this axis' translation is constrained.
    virtual bool IsZTransConstrained () const = 0;

    /**
     * Set the minimum allowed distance between the two bodies. Set force_update to true if 
     * you want to apply the changes right away.
     */
    virtual void SetMinimumDistance (const csVector3& dist) = 0;

    /// Get the minimum allowed distance between the two bodies.
    virtual const csVector3& GetMinimumDistance () const = 0;

    /**
     * Set the maximum allowed distance between the two bodies. Set force_update to true if 
     * you want to apply the changes right away.
     */
    virtual void SetMaximumDistance (const csVector3& dist) = 0;

    /// Get the maximum allowed distance between the two bodies.
    virtual const csVector3& GetMaximumDistance () const = 0;

    /**
     * Set the rotational constraints on the 3 axes. If true is
     * passed for an axis then the Joint will constrain all rotation around
     * that axis (ie no motion will be allowed). If false is passed in then all rotation around that
     * axis is free, but bounded by the minimum and maximum angle
     * if set. Set force_update to true if you want to apply the changes 
     * right away.
     */
    virtual void SetRotConstraints (bool X, bool Y, bool Z) = 0;

    /// True if this axis' rotation is constrained.
    virtual bool IsXRotConstrained () const = 0;

    /// True if this axis' rotation is constrained.
    virtual bool IsYRotConstrained () const = 0;

    /// True if this axis' rotation is constrained.
    virtual bool IsZRotConstrained () const = 0;

    /**
     * Set the minimum allowed angle between the two bodies, in radian. Set force_update to true if 
     * you want to apply the changes right away.
     */
    virtual void SetMinimumAngle (const csVector3& angle) = 0;

    /// Get the minimum allowed angle between the two bodies (in radian).
    virtual const csVector3& GetMinimumAngle () const = 0;

    /**
     * Set the maximum allowed angle between the two bodies (in radian). Set force_update to true if 
     * you want to apply the changes right away.
     */
    virtual void SetMaximumAngle (const csVector3& angle) = 0;

    /// Get the maximum allowed angle between the two bodies (in radian).
    virtual const csVector3& GetMaximumAngle () const = 0;

    /** 
     * Set the restitution of the joint's stop point (this is the 
     * elasticity of the joint when say throwing open a door how 
     * much it will bounce the door back closed when it hits).
     */
    virtual void SetBounce (const csVector3& bounce) = 0;

    /// Get the joint restitution.
    virtual const csVector3& GetBounce () const = 0;

    /**
     * Apply a motor velocity to joint (for instance on wheels). Set force_update to true if 
     * you want to apply the changes right away.
     */
    virtual void SetDesiredVelocity (const csVector3& velo) = 0;

    /// Get the desired velocity of the joint motor.
    virtual const csVector3& GetDesiredVelocity () const = 0;

    /**
     * Set the maximum force that can be applied by the joint motor to reach the desired velocity.
     * Set force_update to true if  you want to apply the changes right away.
     */
    virtual void SetMaxForce (const csVector3& force) = 0;

    /// Get the maximum force that can be applied by the joint motor to reach the desired velocity.
    virtual const csVector3& GetMaxForce () const = 0;

    /// Set this joint to a spring joint.
    virtual void SetSpring (bool isSpring) = 0;

    /// Set the linear stiffness of the spring.
    virtual void SetLinearStiffness (const csVector3& stiff) = 0;

    /// Get the linear stiffness of the spring.
    virtual const csVector3& GetLinearStiffness () const = 0;

    /// Set the angular stiffness of the spring.
    virtual void SetAngularStiffness (const csVector3& stiff) = 0;

    /// Get the angular stiffness of the spring.
    virtual const csVector3& GetAngularStiffness () const = 0;

    /// Set the linear damping of the spring.
    virtual void SetLinearDamping (const csVector3& damp) = 0;

    /// Get the linear damping of the spring.
    virtual const csVector3& GetLinearDamping () const = 0;

    /// Set the angular damping of the spring.
    virtual void SetAngularDamping (const csVector3& damp) = 0;

    /// Get the angular damping of the spring.
    virtual const csVector3& GetAngularDamping () const = 0;
  
    /// Set the value to an equilibrium point for translation.
    virtual void SetLinearEquilibriumPoint (const csVector3& point) = 0;

    /// Set the value to an equilibrium point for rotation.
    virtual void SetAngularEquilibriumPoint (const csVector3& point) = 0;

    /// Set the threshold of a breaking impulse.
    virtual void SetBreakingImpulseThreshold (float threshold) = 0;

    /// Get the threshold of a breaking impulse.
    virtual float GetBreakingImpulseThreshold () const = 0;
  };

}
}
#endif
