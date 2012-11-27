/*
    Copyright (C) 2011-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html
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

#ifndef __CS_BULLET_PHYSICS_H__
#define __CS_BULLET_PHYSICS_H__

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/nobjvec.h"
#include "csutil/scf_implementation.h"
#include "csutil/hash.h"
#include "ivaria/reporter.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/view.h"
#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "csutil/csobject.h"

#include "physicsfactories.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

struct iSector;
class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
struct btSoftBodyWorldInfo;
class btBroadphaseInterface;
class btTriangleMesh;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  
#define WORLD_AABB_DIMENSIONS 10000.0f

class csBulletSector;
class csBulletSystem;
class csBulletDebugDraw;
class csBulletGhostCollisionObject;
class csBulletRigidBody;
class csBulletSoftBody;
class csBulletDynamicActor;
class csBulletCollisionObject;
class csBulletCollisionActor;
class csBulletCollider;
class csBulletJoint;

class CollisionGroup : public scfImplementation1<CollisionGroup,
  CS::Collisions::iCollisionGroup>
{
private:
  csString name;
  char index;

public:
  /// The value of the group.
  int value;

  /// The mask of the group.
  int mask;

  CollisionGroup (const char* name, char index);

   virtual const char* GetName () const
   { return name.GetData (); }

   virtual void SetCollisionEnabled (iCollisionGroup* other, bool enabled);
   virtual bool GetCollisionEnabled (iCollisionGroup* other);
};

class csBulletSystem : public scfImplementationExt2<
  csBulletSystem, csObject,
  CS::Physics::iPhysicalSystem, 
  iComponent>
{
  friend class csBulletColliderConvexMesh;
  friend class csBulletColliderConcaveMesh;
  friend class csBulletSector;
  friend class CollisionPortal;
  friend class csBulletRigidBody;
  friend class csBulletSoftBody;

private:
  iObjectRegistry* object_reg;
  btSoftBodyWorldInfo* defaultInfo;

  float internalScale;
  float inverseInternalScale;

  csRefArrayObject<csBulletSector> collSectors;
  csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*> vehicleMap;

  CollisionGroup* defaultGroup;
  csHash< csRef<CollisionGroup>, const char*> collisionGroups;

  float simulationSpeed;
  float worldTimeStep;
  int worldMaxSteps;
  int stepIterations;

  bool isSoftWorld;

  float linearDampening;
  float angularDampening;

  float linearDisableThreshold;
  float angularDisableThreshold;
  float timeDisableThreshold;

  csBulletDebugDraw* debugDraw;

public:
  csBulletSystem (iBase* iParent);
  virtual ~csBulletSystem ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  //-- iCollisionSystem
  virtual csPtr<CS::Collisions::iCollider> CreateCollider ();
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (iTriangleMesh* mesh, bool simplify = false);
  virtual csPtr<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh (iTriangleMesh* mesh);
  virtual csPtr<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
      (CS::Collisions::iColliderConcaveMesh* collider, const csVector3& scale);
  virtual csPtr<CS::Collisions::iColliderCylinder> CreateColliderCylinder (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
  virtual csPtr<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
  virtual csPtr<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
  virtual csPtr<CS::Collisions::iCollisionTerrain> CreateCollisionTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0);

  
  virtual CS::Collisions::iCollisionSector* CreateCollisionSector (iSector* sector = nullptr);
  virtual size_t GetCollisionSectorCount () const { return collSectors.GetSize (); }
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (size_t index) 
  {
    return csRef<CS::Collisions::iCollisionSector>
      (scfQueryInterface<CS::Collisions::iCollisionSector>(collSectors.Get (index)));
  }
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const iSector* sceneSector);

  virtual CS::Collisions::iCollisionGroup* CreateCollisionGroup (const char* name);
  virtual CS::Collisions::iCollisionGroup* FindCollisionGroup (const char* name) const;
  virtual size_t GetCollisionGroupCount () const;
  virtual CS::Collisions::iCollisionGroup* GetCollisionGroup (size_t index) const;

  //-- iPhysicalSystem
  virtual void SetSimulationSpeed (float speed);
  virtual void SetStepParameters (float timeStep, size_t maxSteps, size_t iterations);
  virtual void Step (csTicks duration);

  virtual void SetSoftBodyEnabled (bool enabled);
  virtual bool GetSoftBodyEnabled () { return isSoftWorld; }

  virtual void SetInternalScale (float scale);
  virtual float GetInternalScale () const { return internalScale; }

  virtual void SetLinearDamping (float d);
  virtual float GetLinearDamping () const { return linearDampening; }

  virtual void SetAngularDamping (float d);
  virtual float GetAngularDamping () const { return angularDampening; }

  virtual void SetAutoDisableParams (float linear,
    float angular, float time);

  // Factories
  virtual csPtr<CS::Collisions::iCollisionObjectFactory> CreateCollisionObjectFactory
    (CS::Collisions::iCollider *collider, const char* name = "CollisionObject");

  virtual csPtr<CS::Collisions::iGhostCollisionObjectFactory> CreateGhostCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr, const char* name = "GhostObject");

  virtual csPtr<CS::Collisions::iCollisionActorFactory> CreateCollisionActorFactory
    (CS::Collisions::iCollider* collider = nullptr, const char* name = "CollisionActor");

  virtual csPtr<CS::Physics::iRigidBodyFactory> CreateRigidBodyFactory
    (CS::Collisions::iCollider* collider = nullptr, const char* name = "RigidBody");

  virtual csPtr<CS::Physics::iDynamicActorFactory> CreateDynamicActorFactory
    (CS::Collisions::iCollider* collider = nullptr, const char* name = "DynamicActor");

  virtual csPtr<CS::Physics::iSoftRopeFactory> CreateSoftRopeFactory ();
  virtual csPtr<CS::Physics::iSoftClothFactory> CreateSoftClothFactory () ;
  virtual csPtr<CS::Physics::iSoftMeshFactory> CreateSoftMeshFactory ();

  // Joints & Constraints

  virtual csPtr<CS::Physics::iJoint> CreateJoint ();
  virtual csPtr<CS::Physics::iJoint> CreateRigidP2PJoint (const csVector3 position);
  virtual csPtr<CS::Physics::iJoint> CreateRigidSlideJoint (const csOrthoTransform trans,
    float minDist, float maxDist, float minAngle, float maxAngle, int axis);
  virtual csPtr<CS::Physics::iJoint> CreateRigidHingeJoint (const csVector3 position,
    float minAngle, float maxAngle, int axis);
  virtual csPtr<CS::Physics::iJoint> CreateRigidConeTwistJoint (const csOrthoTransform trans,
    float swingSpan1,float swingSpan2,float twistSpan);
  virtual csPtr<CS::Physics::iJoint> CreateSoftLinearJoint (const csVector3 position);
  virtual csPtr<CS::Physics::iJoint> CreateSoftAngularJoint (int axis);
  virtual csPtr<CS::Physics::iJoint> CreateRigidPivotJoint (CS::Physics::iRigidBody* body, const csVector3 position);

  // Vehicles
  
  virtual csPtr<CS::Physics::iVehicleFactory> CreateVehicleFactory ();
  virtual csPtr<CS::Physics::iVehicleWheelFactory> CreateVehicleWheelFactory ();
  virtual csPtr<CS::Physics::iVehicleWheelInfo> CreateVehicleWheelInfo
    (CS::Physics::iVehicleWheelFactory* factory);
  virtual CS::Physics::iVehicle* GetVehicle (CS::Collisions::iCollisionObject* obj);

  csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*>& GetVehicleMap () { return vehicleMap; }

  // Misc

/*
  virtual CS::Collisions::CollisionGroup& CreateCollisionGroup (const char* name);
  virtual CS::Collisions::CollisionGroup& FindCollisionGroup (const char* name);

  virtual void SetGroupCollision (const char* name1,
    const char* name2, bool collide);
  virtual bool GetGroupCollision (const char* name1,
    const char* name2);
*/  
  void DeleteAll ();

  virtual void InitDebugDraw ();
  virtual void DebugDraw (iView* rview);
  virtual void SetDebugMode (CS::Physics::DebugMode mode);
  virtual CS::Physics::DebugMode GetDebugMode ();

  virtual void StartProfile ();
  virtual void StopProfile ();
  virtual void DumpProfile (bool resetProfile = true);

  // Internal stuff
  inline CollisionGroup* GetDefaultGroup () const { return defaultGroup; }
  // TODO: remove that
  inline btSoftBodyWorldInfo* GetSoftBodyWorldInfo () const { return defaultInfo; }
  inline float GetInverseInternalScale () {return inverseInternalScale;}
  inline float GetWorldTimeStep() const { return worldTimeStep; }

  btTriangleMesh* CreateBulletTriMesh (iTriangleMesh* triMesh);

  void ReportWarning (const char* msg, ...);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
