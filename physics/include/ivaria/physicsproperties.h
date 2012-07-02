/*
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

#ifndef __IVARIA_PHYSIPROPERTIESH__
#define __IVARIA_PHYSIPROPERTIESH__

/**\file
* Physics interfaces
*/

#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "iutil/objreg.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "imesh/genmesh.h"
#include "csgeom/tri.h"
#include "cstool/primitives.h"
#include "ivaria/collisions.h"
#include "ivaria/collisionproperties.h"

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
  struct CollisionGroup;
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
  struct iKinematicCallback;
  struct iPhysicalSystem;
  struct iPhysicalSector;

  /**
   * Collection of all properties of a physical object
   */
  class PhysicalObjectProperties : public CS::Collisions::CollisionObjectProperties
  {
  protected:
    float density;
    float friction;

  public:
    PhysicalObjectProperties(CS::Collisions::iCollider* collider) : 
        CollisionObjectProperties(collider), 
        density(0),     // static objects
        friction(10)
    {}

    /// Get the density of all objects that will be constructed with these properties
    float GetDensity() const { return density; }
    /// Set the density of all objects that will be constructed with these properties
    void SetDensity(float value) { density = value; }
    
    /// Get the mass of all objects that will be constructed with these properties
    float GetMass() const { return density * collider->GetVolume(); }
    /// Set the mass of all objects that will be constructed with these properties
    void SetMass(float value) { density = value / collider->GetVolume(); }

    /// Set the friction of all objects that will be constructed with these properties
    void SetFriction(float value) { friction = value; }

    /// Get the friction of all objects that will be constructed with these properties
    float GetFriction() const { return friction; }
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
  class RigidBodyProperties : public PhysicalObjectProperties
  {
  protected:
    float elasticity;
    float linearDamping, angularDamping;

  public:
    RigidBodyProperties(CS::Collisions::iCollider* collider) : PhysicalObjectProperties(collider),
      elasticity(0.5f), linearDamping(0.01f), angularDamping(0.01f)
    {
    }
    
    RigidBodyProperties(CS::Collisions::iCollider* collider, const char* name) : PhysicalObjectProperties(collider),
      elasticity(0.5f), linearDamping(0.01f), angularDamping(0.01f)
    {
      SetName(name);
    }


    /// Set the elasticity of this rigid body.
    void SetElasticity (float value) { elasticity = value; }

    /// Get the elasticity of this rigid body.
    float GetElasticity () const { return elasticity; }
    /**
    * Set the linear Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    * \sa iDynamicSystem::SetLinearDamping()
    */
    void SetLinearDamping (float d) { linearDamping = d; }

    /// Get the linear Damping for this rigid body.
    float GetLinearDamping () const { return linearDamping; }

    /**
    * Set the angular Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    */
    void SetAngularDamping (float d) { angularDamping = d; }

    /// Get the angular Damping for this rigid body.
    float GetAngularDamping () const { return angularDamping; }
  };

  /**
   * Collection of all properties of a soft body
   */
  class SoftBodyProperties : public PhysicalObjectProperties
  {
  protected:

  public:
    SoftBodyProperties(CS::Collisions::iCollider* collider) : 
        PhysicalObjectProperties(collider) 
    {}
  };


  class DynamicActorProperties : public RigidBodyProperties
  {
    float stepHeight;
    float walkSpeed, jumpSpeed;
    float airControlFactor;
    bool kinematicSteps;

  public:
    DynamicActorProperties(CS::Collisions::iCollider* collider) : RigidBodyProperties(collider),
      stepHeight(.1f),
      walkSpeed(10.f),
      jumpSpeed(10.f),
      airControlFactor(0.04f),
      kinematicSteps(true)
    {
      SetName("DynamicActor");
    }

    /// Get the max vertical threshold that this actor can step over
    float GetStepHeight () const { return stepHeight; }
    /// Set the max vertical threshold that this actor can step over
    void SetStepHeight (float h) { stepHeight = h; }

    /// Get the walk speed
    float GetWalkSpeed () const { return walkSpeed; }
    /// Set the walk speed
    void SetWalkSpeed (float s) { walkSpeed = s; }

    /// Get the jump speed
    float GetJumpSpeed () const { return jumpSpeed; }
    /// Set the jump speed
    void SetJumpSpeed (float s) { jumpSpeed = s; }

    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    float GetAirControlFactor () const { return airControlFactor; }
    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    void SetAirControlFactor (float f) { airControlFactor = f; }
    
    /// Get whether to use a kinematic method for smooth steps
    bool GetUseKinematicSteps() const { return kinematicSteps; }
    /// Set whether to use a kinematic method for smooth steps
    void SetUseKinematicSteps(bool u) { kinematicSteps = u; }
  };
}
}
#endif