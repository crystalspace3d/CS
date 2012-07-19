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

#ifndef __IVARIA_PHYSICSFACTORIESH__
#define __IVARIA_PHYSICSFACTORIESH__

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
#include "ivaria/collisionfactories.h"

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
  struct iDynamicActor;
  struct iKinematicCallback;
  struct iPhysicalSystem;
  struct iPhysicalSector;

  /**
  * The type of a physical body.
  */
  enum PhysicalBodyType
  {
    BODY_RIGID = 0,
    BODY_SOFT
  };

  /**
   * Collection of all properties of a physical object
   */
  struct iPhysicalObjectFactory : public virtual CS::Collisions::iCollisionObjectFactory
  {
    /// Get the PhysicalBodyType of the object whose data is stored in this properties object
    virtual PhysicalBodyType GetPhysicalBodyType() const = 0;

    /// Get the density of all objects that will be constructed with these properties
    virtual float GetDensity() const = 0;
    /// Set the density of all objects that will be constructed with these properties
    virtual void SetDensity(float value) = 0;
    
    /// Get the mass of all objects that will be constructed with these properties
    virtual float GetMass() const = 0;
    /// Set the mass of all objects that will be constructed with these properties
    virtual void SetMass(float value) = 0;

    /// Set the friction of all objects that will be constructed with these properties
    virtual void SetFriction(float value) = 0;

    /// Get the friction of all objects that will be constructed with these properties
    virtual float GetFriction() const = 0;
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
    virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody() = 0;

    /// Set the elasticity of this rigid body.
    virtual void SetElasticity (float value) = 0;

    /// Get the elasticity of this rigid body.
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
    virtual csPtr<iSoftBody> CreateSoftBody() = 0;
  };

  /**
   * Used to create a one-dimensional softbody
   */
  struct iSoftRopeFactory : public virtual iSoftBodyFactory
  {
    /// Start position of the rope
    virtual const csVector3& GetStart() const = 0;
    virtual void SetStart(const csVector3& v) = 0;
    
    /// End position of the rope
    virtual const csVector3& GetEnd() const = 0;
    virtual void SetEnd(const csVector3& v) = 0;
    
    /// Amount of nodes along the rope
    virtual size_t GetNodeCount() const = 0;
    virtual void SetNodeCount(size_t c) = 0;
  };

  /**
   * Used to create a two-dimensional softbody
   */
  struct iSoftClothFactory : public virtual iSoftBodyFactory
  {
    /// Get the four corners of the cloth
    virtual const csVector3* GetCorners() const = 0;
    /// Set the four corners of the cloth
    virtual void SetCorners(csVector3 corners[4]) = 0;

    /// Get the two segment counts along the two primary axes
    virtual void GetSegmentCounts(size_t& count1, size_t& count2) const = 0;
    /// Set the two segment counts along the two primary axes
    virtual void SetSegmentCounts(size_t count1, size_t count2) = 0;
    
    /// Whether there must be diagonal segments in the cloth
    virtual bool GetWithDiagonals() const = 0;
    virtual void SetWithDiagonals(bool d) = 0;
  };
  
  /**
   * Used to create an arbitrary softbody defined by a given mesh
   */
  struct iSoftMeshFactory : public virtual iSoftBodyFactory
  {
    /// Get the factory that contains the mesh to define the softbody
    virtual iGeneralFactoryState* GetGenmeshFactory() const = 0;
    /// Set the factory that contains the mesh to define the softbody
    virtual void SetGenmeshFactory(iGeneralFactoryState* s) = 0;

  };


  struct iDynamicActorFactory : public virtual iRigidBodyFactory
  {
    virtual csPtr<iDynamicActor> CreateDynamicActor() = 0;

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
    virtual bool GetUseKinematicSteps() const = 0;
    /// Set whether to use a kinematic method for smooth steps
    virtual void SetUseKinematicSteps(bool u) = 0;
  };
}
}
#endif