/*
  Copyright (C) 2011 by Liu Lu

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
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;
class btTriangleMesh;
//class btGhostObject;

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


class CollisionGroupVector : public csArray<CS::Collisions::CollisionGroup>
{
public:
  CollisionGroupVector () : csArray<CS::Collisions::CollisionGroup> () {}
  static int CompareKey (CS::Collisions::CollisionGroup const& item,
    char const* const& key)
  {
    return strcmp (item.name.GetData (), key);
  }
  static csArrayCmp<CS::Collisions::CollisionGroup, char const*>
    KeyCmp(char const* k)
  {
    return csArrayCmp<CS::Collisions::CollisionGroup, char const*> (k,CompareKey);
  }
};

class csBulletSystem : public scfImplementationExt2<
  csBulletSystem, csObject,
  CS::Physics::iPhysicalSystem, 
  iComponent>
{
  friend class csBulletColliderConvexMesh;
  friend class csBulletColliderConcaveMesh;
  friend class csBulletSector;
  friend class csBulletCollisionPortal;
  friend class csBulletRigidBody;


private:
  iObjectRegistry* object_reg;
  /*csRefArrayObject<CS::Collisions::iCollider> colliders;
  csRefArrayObject<csBulletCollisionObject> objects;
  csRefArrayObject<csBulletRigidBody> rigidBodies;
  csRefArrayObject<csBulletSoftBody> softBodies;
  csRefArrayObject<csBulletJoint> joints;
  csRefArrayObject<iCollisionActor> actors;*/
  csRefArrayObject<csBulletSector> collSectors;
  btSoftBodyWorldInfo* defaultInfo;
  float internalScale;
  float inverseInternalScale;
  csStringID baseID;
  csStringID colldetID;
  
  CollisionGroupVector collGroups;
  size_t systemFilterCount;
  csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*> vehicleMap;

public:
  csBulletSystem (iBase* iParent);
  virtual ~csBulletSystem ();

  // iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  // iCollisionSystem
  virtual void SetInternalScale (float scale);
  virtual csPtr<CS::Collisions::iColliderCompound> CreateColliderCompound ( );
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (iMeshWrapper* mesh, bool simplify = false);
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (iTriangleMesh* triMesh, iMeshWrapper* mesh = nullptr, bool simplify = false);
  virtual csPtr<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh);
  virtual csPtr<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
      (CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale);
  virtual csPtr<CS::Collisions::iColliderCylinder> CreateColliderCylinder (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
  virtual csPtr<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
  virtual csPtr<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
  virtual csPtr<CS::Collisions::iCollisionTerrain> CreateCollisionTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0);

  
  virtual CS::Collisions::iCollisionSector* CreateCollisionSector ();
  virtual CS::Collisions::iCollisionSector* GetOrCreateCollisionSector (iSector* sector);
  virtual size_t GetCollisionSectorCount () const { return collSectors.GetSize(); }
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const csString& name);
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (size_t index) 
  {
    return csRef<CS::Collisions::iCollisionSector>(scfQueryInterface<CS::Collisions::iCollisionSector>(collSectors.Get(index)));
  }
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (const iSector* sceneSector);

  //iPhysicalSystem
  virtual csPtr<CS::Physics::iPhysicalSector> CreatePhysicalSector () 
  { 
    return csPtr<CS::Physics::iPhysicalSector>(scfQueryInterface<CS::Physics::iPhysicalSector>(
      csRef<CS::Collisions::iCollisionSector>(CreateCollisionSector())));
  }
  
  
  // Factories
  virtual csPtr<CS::Collisions::iCollisionObjectFactory> CreateCollisionObjectFactory (int id);

  virtual csPtr<CS::Collisions::iCollisionObjectFactory> CreateCollisionObjectFactory (CS::Collisions::iCollider *collider, const csString & name = "CollisionObject")
  { 
    return csPtr<CS::Collisions::iCollisionObjectFactory>(csRef<CS::Physics::iRigidBodyFactory>(CreateRigidBodyFactory(collider, name))); 
  }

  virtual csPtr<CS::Collisions::iGhostCollisionObjectFactory> CreateGhostCollisionObjectFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "GhostObject");

  virtual csPtr<CS::Collisions::iCollisionActorFactory> CreateCollisionActorFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "CollisionActor");

  virtual csPtr<CS::Physics::iRigidBodyFactory> CreateRigidBodyFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "RigidBody");

  virtual csPtr<CS::Physics::iDynamicActorFactory> CreateDynamicActorFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "DynamicActor");

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
  
  /// Creates a new factory to produce vehicles
  virtual csPtr<CS::Physics::iVehicleFactory> CreateVehicleFactory ();

  /// Creates a new factory to produce vehicle wheels
  virtual csPtr<CS::Physics::iVehicleWheelFactory> CreateVehicleWheelFactory ();

  /// Creates a new factory to produce a new iVehicleWheelInfo object which defines a wheel factory and wheel geometry.
  virtual csPtr<CS::Physics::iVehicleWheelInfo> CreateVehicleWheelInfo (CS::Physics::iVehicleWheelFactory* factory);
  
  /// Returns the vehicle that the given object is a part of, or nullptr
  virtual CS::Physics::iVehicle* GetVehicle (CS::Collisions::iCollisionObject* obj);

  csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*>& GetVehicleMap() { return vehicleMap; }


  // Misc

  virtual CS::Collisions::CollisionGroup& CreateCollisionGroup (const char* name);
  virtual CS::Collisions::CollisionGroup& FindCollisionGroup (const char* name);

  virtual void SetGroupCollision (const char* name1,
    const char* name2, bool collide);
  virtual bool GetGroupCollision (const char* name1,
    const char* name2);
  
  virtual void SeparateDisconnectedSubMeshes(CS::Collisions::iColliderCompound* mesh, CS::Collisions::iColliderCompoundResult* results);

  virtual iTriangleMesh* FindColdetTriangleMesh (iMeshWrapper* mesh);

  void DeleteAll();

  // Internal stuff
  btSoftBodyWorldInfo* GetSoftBodyWorldInfo() const { return defaultInfo; }
  float getInverseInternalScale() {return inverseInternalScale;}
  float getInternalScale() {return internalScale;}
  void ReportWarning (const char* msg, ...);
  
  btTriangleMesh* CreateBulletTriMesh (iTriangleMesh* triMesh);
};

}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
