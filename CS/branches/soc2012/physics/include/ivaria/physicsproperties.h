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
    virtual void SetElasticity (float value) { elasticity = value; }

    /// Get the elasticity of this rigid body.
    virtual float GetElasticity () const { return elasticity; }
    /**
    * Set the linear Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    * \sa iDynamicSystem::SetLinearDamping()
    */
    virtual void SetLinearDamping (float d) { linearDamping = d; }

    /// Get the linear Damping for this rigid body.
    virtual float GetLinearDamping () const { return linearDamping; }

    /**
    * Set the angular Damping for this rigid body. The damping correspond to
    * how much the movements of the objects will be reduced. It is a value
    * between 0 and 1, giving the ratio of speed that will be reduced
    * in one second. 0 means that the movement will not be reduced, while
    * 1 means that the object will not move.
    * The default value is 0.
    */
    virtual void SetAngularDamping (float d) { angularDamping = d; }

    /// Get the angular Damping for this rigid body.
    virtual float GetAngularDamping () const { return angularDamping; }
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

}
}
#endif