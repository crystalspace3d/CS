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

#ifndef __CS_BULLET_PHYSICSFACTORIES_H__
#define __CS_BULLET_PHYSICSFACTORIES_H__

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/csstring.h"

#include "ivaria/physics.h"

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

  class csBulletSystem;
  class BulletSector;
  class CollisionGroup;

  /**
   * Base class for all object properties
   */
  class BulletCollisionObjectFactory : public scfVirtImplementationExt1<
    BulletCollisionObjectFactory, csObject, CS::Collisions::iCollisionObjectFactory> 
  {
    friend class csBulletSystem;

  public:
    // TODO: really used?
    csBulletSystem* system;
    csRef<CS::Collisions::iCollider> collider;
    CollisionGroup* group;
    csOrthoTransform transform;

  public:
    BulletCollisionObjectFactory (csBulletSystem* system, CS::Collisions::iCollider* collider);

    virtual iObject *QueryObject (void) { return this; }

    virtual CS::Collisions::iCollisionSystem* GetSystem () const
    { return (CS::Collisions::iCollisionSystem*) system; }

    // TODO: create a real object
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ()
    { return csPtr<CS::Collisions::iCollisionObject> (nullptr); }

    virtual void SetCollider (CS::Collisions::iCollider* value,
			      const csOrthoTransform& transform = csOrthoTransform ())
    {
      collider = value;
      this->transform = transform;
    }
    virtual CS::Collisions::iCollider* GetCollider () const { return collider; }

    virtual void SetColliderTransform (const csOrthoTransform& transform);
    virtual const csOrthoTransform& GetColliderTransform () const;

    virtual void SetCollisionGroup (CS::Collisions::iCollisionGroup* group);
    virtual CS::Collisions::iCollisionGroup* GetCollisionGroup () const;
  };

  class BulletPhysicalObjectFactory : public scfVirtImplementationExt1<
    BulletPhysicalObjectFactory, BulletCollisionObjectFactory, CS::Physics::iPhysicalObjectFactory>
  {
  protected:
    float density, mass;
    float friction;
    bool gravityEnabled;

  public:
    BulletPhysicalObjectFactory (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr) : 
    scfImplementationType (this, system, collider),
        density (1.0f), mass (1.0f),     // static objects // TODO: really?
        friction (10.0f), gravityEnabled (true)
    {}

    virtual float GetDensity () const { return density; }
    virtual void SetDensity (float value) { density = value; mass = 0; }
    
    virtual float GetMass () const { return mass; }
    virtual void SetMass (float value) { mass = value; density = 0; }

    virtual void SetFriction (float value) { friction = value; }
    virtual float GetFriction () const { return friction; }
    
    virtual bool GetGravityEnabled () const { return gravityEnabled; }
    virtual void SetGravityEnabled (bool enabled) { gravityEnabled = enabled; }
  };

  class BulletRigidBodyFactory : public scfVirtImplementationExt1<
    BulletRigidBodyFactory, BulletPhysicalObjectFactory, CS::Physics::iRigidBodyFactory>
  {
    friend class csBulletRigidBody;

  protected:
    CS::Physics::RigidBodyState state;
    float elasticity;
    float linearDamping, angularDamping;

  public:
    BulletRigidBodyFactory (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr) : 
    scfImplementationType (this, system, collider), state (CS::Physics::STATE_DYNAMIC),
      elasticity (0.1f), linearDamping (0.01f), angularDamping (0.01f)
    {
    }

    virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody ();
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();
    
    virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
    { return CS::Physics::PHYSICAL_OBJECT_RIGIDBODY; }

    virtual void SetState (CS::Physics::RigidBodyState state) { this->state = state; }
    virtual CS::Physics::RigidBodyState GetState () const { return state; }

    virtual void SetElasticity (float value) { elasticity = value; }
    virtual float GetElasticity () const { return elasticity; }

    void SetLinearDamping (float d) { linearDamping = d; }
    float GetLinearDamping () const { return linearDamping; }

    virtual void SetAngularDamping (float d) { angularDamping = d; }
    virtual float GetAngularDamping () const { return angularDamping; }
  };

  class BulletSoftBodyFactory : public scfVirtImplementationExt1<
    BulletSoftBodyFactory, BulletPhysicalObjectFactory, CS::Physics::iSoftBodyFactory>
  {
  protected:

  public:
  BulletSoftBodyFactory (csBulletSystem* system) : scfImplementationType (this, system)
    {
      SetFriction (float (.2));    // between 0 and 1
    }

    virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
    { return CS::Physics::PHYSICAL_OBJECT_SOFTYBODY; }

    /// Create a new object
    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody () = 0;
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();
  };

  class BulletSoftRopeFactory : public scfVirtImplementationExt1<
    BulletSoftRopeFactory, BulletSoftBodyFactory, CS::Physics::iSoftRopeFactory>
  {
  protected:
    csVector3 start, end;
    size_t nodeCount;

  public:
  BulletSoftRopeFactory (csBulletSystem* system) : scfImplementationType (this, system),
      start (0), end (0),
      nodeCount (10)
    {
    }

    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    virtual const csVector3& GetStart () const { return start; }
    virtual void SetStart (const csVector3& v) { start = v; }
    
    virtual const csVector3& GetEnd () const { return end; }
    virtual void SetEnd (const csVector3& v) { end = v; }
    
    virtual size_t GetNodeCount () const { return nodeCount; }
    virtual void SetNodeCount (size_t c) { nodeCount = c; }
  };

  /**
   * Used to create a two-dimensional softbody
   */
  class BulletSoftClothFactory : public scfVirtImplementationExt1<
    BulletSoftClothFactory, BulletSoftBodyFactory, CS::Physics::iSoftClothFactory> 
  {
  protected:
    csVector3 corners[4];
    size_t counts[2];
    bool withDiagonals;

  public:
  BulletSoftClothFactory (csBulletSystem* system) : scfImplementationType (this, system),
      withDiagonals (false)
    {
      for (size_t i = 0; i < 4; ++i) corners[i] = csVector3 (i);
      counts[0] = counts[1] = 10;
    }

    /// Create a new object
    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    /// Get the four corners of the cloth
    virtual const csVector3* GetCorners () const { return corners; }
    /// Set the four corners of the cloth
    virtual void SetCorners (csVector3 cs[4]) { for (size_t i = 0; i < 4; ++i) corners[i] = cs[i]; }

    /// Get the two segment counts along the two primary axes
    virtual void GetSegmentCounts (size_t& count1, size_t& count2) const { count1 = counts[0]; count2 = counts[1]; }
    /// Set the two segment counts along the two primary axes
    virtual void SetSegmentCounts (size_t count1, size_t count2) { counts[0] = count1; counts[1] = count2; }
    
    /// Whether there must be diagonal segments in the cloth
    virtual bool GetWithDiagonals () const { return withDiagonals; }
    virtual void SetWithDiagonals (bool d) { withDiagonals = d; }
  };
  
  /**
   * Used to create an arbitrary softbody defined by a given mesh
   */
  class BulletSoftMeshFactory : public scfVirtImplementationExt1<
    BulletSoftMeshFactory, BulletSoftBodyFactory, CS::Physics::iSoftMeshFactory> 
  {
  protected:
    iGeneralFactoryState* factory;

  public:
  BulletSoftMeshFactory (csBulletSystem* system) : scfImplementationType (this, system)
    {
    }

    /// Create a new object
    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    /// Get the factory that contains the mesh to define the softbody
    virtual iGeneralFactoryState* GetGenmeshFactory () const { return factory; }
    /// Set the factory that contains the mesh to define the softbody
    virtual void SetGenmeshFactory (iGeneralFactoryState* s) { factory = s; }
  };

  class BulletDynamicActorFactory : public scfVirtImplementationExt1<
    BulletDynamicActorFactory, BulletRigidBodyFactory, CS::Physics::iDynamicActorFactory>
  {
    friend class BulletDynamicActor;

    float stepHeight;
    float walkSpeed, jumpSpeed;
    float airControlFactor;
    bool kinematicSteps;

  public:
  BulletDynamicActorFactory (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr) : 
    scfImplementationType (this, system, collider),
      stepHeight (.1f),
      walkSpeed (10.f),
      jumpSpeed (10.f),
      airControlFactor (0.04f),
      kinematicSteps (true)
    {
    }

    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();
    virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody ();
    virtual csPtr<CS::Collisions::iActor> CreateActor ();
    virtual csPtr<CS::Physics::iDynamicActor> CreateDynamicActor ();

    float GetStepHeight () const { return stepHeight; }
    void SetStepHeight (float h) { stepHeight = h; }

    float GetWalkSpeed () const { return walkSpeed; }
    void SetWalkSpeed (float s) { walkSpeed = s; }

    float GetJumpSpeed () const { return jumpSpeed; }
    void SetJumpSpeed (float s) { jumpSpeed = s; }

    float GetAirControlFactor () const { return airControlFactor; }
    void SetAirControlFactor (float f) { airControlFactor = f; }
    
    bool GetKinematicStepsEnabled () const { return kinematicSteps; }
    void SetKinematicStepsEnabled (bool u) { kinematicSteps = u; }
  };

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
