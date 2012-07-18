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

#ifndef __CS_BULLET_OBJECTPROPS_H__
#define __CS_BULLET_OBJECTPROPS_H__

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/csstring.h"

#include "ivaria/physics.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

class BulletSector;
class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;


CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  /**
   * Base class for all object properties
   */
  class BulletCollisionObjectProperties : public scfVirtImplementationExt1<
    BulletCollisionObjectProperties, csObject, CS::Collisions::iCollisionObjectProperties> 
  {
  protected:
    csRef<CS::Collisions::iCollider> collider;
    CS::Collisions::CollisionGroup collGroup;

  public:
    BulletCollisionObjectProperties(CS::Collisions::iCollider* collider = nullptr, const csString& name = "CollisionObject")  :
        scfImplementationType (this), collider(collider), collGroup() 
    {
      SetName(name);
    }

    /// Get the type of the object whose data is represented by this properties object
    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const = 0;

    /// Return the underlying object
    virtual iObject *QueryObject (void) { return this; }

    /// Get the collider of all objects that will be constructed with these properties
    virtual CS::Collisions::iCollider* GetCollider() const { return collider; }
    /// Set the collider of all objects that will be constructed with these properties
    virtual void SetCollider(CS::Collisions::iCollider* value) { collider = value; }

    /// Get the collision group of all objects that will be constructed with these properties
    virtual const CS::Collisions::CollisionGroup& GetCollisionGroup() const { return collGroup; }
    /// Set the collision group of all objects that will be constructed with these properties
    virtual void SetCollisionGroup(const CS::Collisions::CollisionGroup& value) { collGroup = value; }
  };

  class BulletGhostCollisionObjectProperties : public scfVirtImplementationExt1<
    BulletGhostCollisionObjectProperties, BulletCollisionObjectProperties, CS::Collisions::iGhostCollisionObjectProperties> 
  {
  public:
    BulletGhostCollisionObjectProperties(CS::Collisions::iCollider* collider = nullptr, const csString& name = "GhostObject") :
        scfImplementationType (this, collider)
    {
    }
    
    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeGhostObject; }
  };

  /**
   * Kinematic Actor
   */
  class BulletCollisionActorProperties : public scfVirtImplementationExt1<
    BulletCollisionActorProperties, BulletGhostCollisionObjectProperties, CS::Collisions::iCollisionActorProperties> 
  {
    float stepHeight;
    float walkSpeed, jumpSpeed;
    float airControlFactor;

  public:
    BulletCollisionActorProperties(CS::Collisions::iCollider* collider = nullptr, const csString& name = "CollisionActor") :
        scfImplementationType (this, collider),
      stepHeight(.5f),
      walkSpeed(10.f),
      jumpSpeed(10.f),
      airControlFactor(0.04f)
    {
    }

    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeCollisionActor; }

    /// Get the max vertical threshold that this actor can step over
    virtual float GetStepHeight () const { return stepHeight; }
    /// Set the max vertical threshold that this actor can step over
    virtual void SetStepHeight (float h) { stepHeight = h; }

    /// Get the walk speed
    virtual float GetWalkSpeed () const { return walkSpeed; }
    /// Set the walk speed
    virtual void SetWalkSpeed (float s) { walkSpeed = s; }

    /// Get the jump speed
    virtual float GetJumpSpeed () const { return jumpSpeed; }
    /// Set the jump speed
    virtual void SetJumpSpeed (float s) { jumpSpeed = s; }

    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual float GetAirControlFactor () const { return airControlFactor; }
    /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
    virtual void SetAirControlFactor (float f) { airControlFactor = f; }
  };



  // ###################################################################################################
  // Physics

  class BulletPhysicalObjectProperties : public scfVirtImplementationExt1<
    BulletPhysicalObjectProperties, BulletCollisionObjectProperties, CS::Physics::iPhysicalObjectProperties>
  {
  protected:
    csScalar density, mass;
    csScalar friction;

  public:
    BulletPhysicalObjectProperties(CS::Collisions::iCollider* collider = nullptr, const csString& name = "") : 
        scfImplementationType (this, collider, name),
        density(0), mass(0),     // static objects
        friction(10)
    {}

    /// Get the density of all objects that will be constructed with these properties
    virtual csScalar GetDensity() const { return density; }
    /// Set the density of all objects that will be constructed with these properties
    virtual void SetDensity(csScalar value) { density = value; mass = 0; }
    
    /// Get the mass of all objects that will be constructed with these properties
    virtual csScalar GetMass() const { return mass; }
    /// Set the mass of all objects that will be constructed with these properties
    virtual void SetMass(csScalar value) { mass = value; density = 0; }

    /// Set the friction of all objects that will be constructed with these properties
    virtual void SetFriction(csScalar value) { friction = value; }

    /// Get the friction of all objects that will be constructed with these properties
    virtual float GetFriction() const { return friction; }
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
  class BulletRigidBodyProperties : public scfVirtImplementationExt1<
    BulletRigidBodyProperties, BulletPhysicalObjectProperties, CS::Physics::iRigidBodyProperties>
  {
  protected:
    float elasticity;
    float linearDamping, angularDamping;

  public:
    BulletRigidBodyProperties(CS::Collisions::iCollider* collider = nullptr, const csString& name = "RigidBody") : 
        scfImplementationType (this, collider, name),
      elasticity(0.5f), linearDamping(0.01f), angularDamping(0.01f)
    {
    }

    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeRigidBody; }
    
    virtual CS::Physics::PhysicalBodyType GetPhysicalBodyType() const { return CS::Physics::BODY_RIGID; }

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
    virtual void SetAngularDamping (float d) { angularDamping = d; }

    /// Get the angular Damping for this rigid body.
    virtual float GetAngularDamping () const { return angularDamping; }
  };

  /**
   * Collection of properties of a soft body
   */
  class BulletSoftBodyProperties : public scfVirtImplementationExt1<
    BulletSoftBodyProperties, BulletPhysicalObjectProperties, CS::Physics::iSoftBodyProperties>
  {
  protected:

  public:
    BulletSoftBodyProperties() : scfImplementationType (this)
    {
      SetFriction(.2);    // between 0 and 1
    }

    virtual CS::Physics::PhysicalBodyType GetPhysicalBodyType() const { return CS::Physics::BODY_SOFT; }
  };

  /**
   * Used to create a one-dimensional softbody
   */
  class BulletSoftRopeProperties : public scfVirtImplementationExt1<
    BulletSoftBodyProperties, BulletSoftBodyProperties, CS::Physics::iSoftRopeProperties>
  {
  protected:
    csVector3 start, end;
    size_t nodeCount;

  public:
    BulletSoftRopeProperties() : scfImplementationType (this),
      start(0), end(0),
      nodeCount(10)
    {
    }
    
    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeSoftRope; }

    /// Start position of the rope
    virtual const csVector3& GetStart() const { return start; }
    virtual void SetStart(const csVector3& v) { start = v; }
    
    /// End position of the rope
    virtual const csVector3& GetEnd() const { return end; }
    virtual void SetEnd(const csVector3& v) { end = v; }
    
    /// Amount of nodes along the rope
    virtual size_t GetNodeCount() const { return nodeCount; }
    virtual void SetNodeCount(size_t c) { nodeCount = c; }
  };

  /**
   * Used to create a two-dimensional softbody
   */
  class BulletSoftClothProperties : public scfVirtImplementationExt1<
    BulletSoftClothProperties, BulletSoftBodyProperties, CS::Physics::iSoftClothProperties> 
  {
  protected:
    csVector3 corners[4];
    size_t counts[2];
    bool withDiagonals;

  public:
    BulletSoftClothProperties() : scfImplementationType (this),
      withDiagonals(false)
    {
      for (size_t i = 0; i < 4; ++i) corners[i] = csVector3(i);
      counts[0] = counts[1] = 10;
    }

    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeSoftCloth; }

    /// Get the four corners of the cloth
    virtual const csVector3* GetCorners() const { return corners; }
    /// Set the four corners of the cloth
    virtual void SetCorners(csVector3 cs[4]) { for (size_t i = 0; i < 4; ++i) corners[i] = cs[i]; }

    /// Get the two segment counts along the two primary axes
    virtual void GetSegmentCounts(size_t& count1, size_t& count2) const { count1 = counts[0]; count2 = counts[1]; }
    /// Set the two segment counts along the two primary axes
    virtual void SetSegmentCounts(size_t count1, size_t count2) { counts[0] = count1; counts[1] = count2; }
    
    /// Whether there must be diagonal segments in the cloth
    virtual bool GetWithDiagonals() const { return withDiagonals; }
    virtual void SetWithDiagonals(bool d) { withDiagonals = d; }
  };
  
  /**
   * Used to create an arbitrary softbody defined by a given mesh
   */
  class BulletSoftMeshProperties : public scfVirtImplementationExt1<
    BulletSoftMeshProperties, BulletSoftBodyProperties, CS::Physics::iSoftMeshProperties> 
  {
  protected:
    iGeneralFactoryState* factory;

  public:
    BulletSoftMeshProperties() : scfImplementationType (this)
    {
    }

    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeSoftMesh; }

    /// Get the factory that contains the mesh to define the softbody
    virtual iGeneralFactoryState* GetGenmeshFactory() const { return factory; }
    /// Set the factory that contains the mesh to define the softbody
    virtual void SetGenmeshFactory(iGeneralFactoryState* s) { factory = s; }
  };


  class BulletDynamicActorProperties : public scfVirtImplementationExt1<
    BulletDynamicActorProperties, BulletRigidBodyProperties, CS::Physics::iDynamicActorProperties>
  {
    float stepHeight;
    float walkSpeed, jumpSpeed;
    float airControlFactor;
    bool kinematicSteps;

  public:
    BulletDynamicActorProperties(CS::Collisions::iCollider* collider, const csString& name = "DynamicActor") : 
        scfImplementationType (this, collider, name),
      stepHeight(.1f),
      walkSpeed(10.f),
      jumpSpeed(10.f),
      airControlFactor(0.04f),
      kinematicSteps(true)
    {
    }

    virtual CS::Collisions::InternalCollisionObjectType GetInternalObjectType() const { return CS::Collisions::InternalCollisionObjectTypeDynamicActor; }

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
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif