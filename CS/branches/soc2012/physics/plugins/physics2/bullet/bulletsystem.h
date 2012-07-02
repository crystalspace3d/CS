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
#include "ivaria/reporter.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/bullet2.h"
#include "ivaria/view.h"
#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "csutil/csobject.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

// new csRef: 
//  csBulletCollisionPortal ->  portal, desSector, ghostPortal
//  csBulletSector ->   sys, hitPortal, (debugDraw, bulletWorld, dispatcher, configuration, solver, broadphase, softWorldInfo)
//  csBulletSystem ->   

struct iSector;
class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;

//class btGhostObject;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
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

public:
  csBulletSystem (iBase* iParent);
  virtual ~csBulletSystem ();

  // iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  // iCollisionSystem
  virtual void SetInternalScale (float scale);
  virtual csPtr<CS::Collisions::iColliderCompound> CreateColliderCompound ( );
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (
    iMeshWrapper* mesh, bool simplify = false);
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

  
  virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject (CS::Collisions::CollisionObjectProperties* props);
  virtual csPtr<CS::Collisions::iGhostCollisionObject> CreateGhostCollisionObject (CS::Collisions::GhostCollisionObjectProperties* props);
  virtual csPtr<CS::Collisions::iCollisionActor> CreateCollisionActor (CS::Collisions::CollisionActorProperties* props);
  virtual csPtr<CS::Collisions::iCollisionSector> CreateCollisionSector ();
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (const iSector* sceneSector);

  virtual void DecomposeConcaveMesh (CS::Collisions::iCollider* object, iMeshWrapper* mesh, bool simplify = false); 

  //iPhysicalSystem
  virtual csPtr<CS::Physics::iPhysicalSector> CreatePhysicalSector () 
  { 
    return csPtr<CS::Physics::iPhysicalSector>(scfQueryInterface<CS::Physics::iPhysicalSector>(
      csRef<CS::Collisions::iCollisionSector>(CreateCollisionSector())));
  }
  
  virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody (CS::Physics::RigidBodyProperties* props);
  virtual csPtr<CS::Physics::iDynamicActor> CreateDynamicActor (CS::Physics::DynamicActorProperties* props);

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
 
  virtual csPtr<CS::Physics::iSoftBody> CreateRope (csVector3 start,
      csVector3 end, size_t segmentCount);
  virtual csPtr<CS::Physics::iSoftBody> CreateRope (csVector3* vertices, size_t vertexCount);
  virtual csPtr<CS::Physics::iSoftBody> CreateCloth (csVector3 corner1, csVector3 corner2,
      csVector3 corner3, csVector3 corner4,
      size_t segmentCount1, size_t segmentCount2,
      bool withDiagonals = false);

  virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody (iGeneralFactoryState* genmeshFactory, 
    const csOrthoTransform& bodyTransform);

  virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody (csVector3* vertices,
      size_t vertexCount, csTriangle* triangles, size_t triangleCount,
      const csOrthoTransform& bodyTransform);
  float getInverseInternalScale() {return inverseInternalScale;}
  float getInternalScale() {return internalScale;}

  virtual CS::Collisions::CollisionGroup& CreateCollisionGroup (const char* name);
  virtual CS::Collisions::CollisionGroup& FindCollisionGroup (const char* name);

  virtual void SetGroupCollision (const char* name1,
    const char* name2, bool collide);
  virtual bool GetGroupCollision (const char* name1,
    const char* name2);

  void ReportWarning (const char* msg, ...);
};
}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
