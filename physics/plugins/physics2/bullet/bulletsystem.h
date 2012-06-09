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
class csBulletRigidBody;
class csBulletSoftBody;
class csBulletCollisionObject;
class csBulletCollisionActor;
class csBulletCollider;
class csBulletJoint;

class csBulletSystem : public scfImplementation3<
  csBulletSystem, CS::Collisions::iCollisionSystem, 
  CS::Physics::iPhysicalSystem, iComponent>
{
  friend class csBulletColliderConvexMesh;
  friend class csBulletColliderConcaveMesh;
    friend class csBulletSector;
    friend class csBulletCollisionPortal;


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

public:
  csBulletSystem (iBase* iParent);
  virtual ~csBulletSystem ();

  // iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  // iCollisionSystem
  virtual void SetInternalScale (float scale);
  virtual csRef<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (
    iMeshWrapper* mesh, bool simplify = false);
  virtual csRef<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh);
  virtual csRef<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
      (CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale);
  virtual csRef<CS::Collisions::iColliderCylinder> CreateColliderCylinder (float length, float radius);
  virtual csRef<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
  virtual csRef<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
  virtual csRef<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
  virtual csRef<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
  virtual csRef<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
  virtual csRef<CS::Collisions::iColliderTerrain> CreateColliderTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0);

  virtual csRef<CS::Collisions::iCollisionObject> CreateCollisionObject ();
  virtual csRef<CS::Collisions::iCollisionActor> CreateCollisionActor ();
  virtual csRef<CS::Collisions::iCollisionSector> CreateCollisionSector ();
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (const iSector* sceneSector);

  virtual void DecomposeConcaveMesh (CS::Collisions::iCollisionObject* object,
    iMeshWrapper* mesh, bool simplify = false); 

  //iPhysicalSystem
  virtual csRef<CS::Physics::iRigidBody> CreateRigidBody ();

  virtual csRef<CS::Physics::iJoint> CreateJoint ();
  virtual csRef<CS::Physics::iJoint> CreateRigidP2PJoint (const csVector3 position);
  virtual csRef<CS::Physics::iJoint> CreateRigidSlideJoint (const csOrthoTransform trans,
    float minDist, float maxDist, float minAngle, float maxAngle, int axis);
  virtual csRef<CS::Physics::iJoint> CreateRigidHingeJoint (const csVector3 position,
    float minAngle, float maxAngle, int axis);
  virtual csRef<CS::Physics::iJoint> CreateRigidConeTwistJoint (const csOrthoTransform trans,
    float swingSpan1,float swingSpan2,float twistSpan);
  virtual csRef<CS::Physics::iJoint> CreateSoftLinearJoint (const csVector3 position);
  virtual csRef<CS::Physics::iJoint> CreateSoftAngularJoint (int axis);
  virtual csRef<CS::Physics::iJoint> CreateRigidPivotJoint (CS::Physics::iRigidBody* body, const csVector3 position);
 
  virtual csRef<CS::Physics::iSoftBody> CreateRope (csVector3 start,
      csVector3 end, size_t segmentCount);
  virtual csRef<CS::Physics::iSoftBody> CreateRope (csVector3* vertices, size_t vertexCount);
  virtual csRef<CS::Physics::iSoftBody> CreateCloth (csVector3 corner1, csVector3 corner2,
      csVector3 corner3, csVector3 corner4,
      size_t segmentCount1, size_t segmentCount2,
      bool withDiagonals = false);

  virtual csRef<CS::Physics::iSoftBody> CreateSoftBody (iGeneralFactoryState* genmeshFactory, 
    const csOrthoTransform& bodyTransform);

  virtual csRef<CS::Physics::iSoftBody> CreateSoftBody (csVector3* vertices,
      size_t vertexCount, csTriangle* triangles, size_t triangleCount,
      const csOrthoTransform& bodyTransform);
  float getInverseInternalScale() {return inverseInternalScale;}
  float getInternalScale() {return internalScale;}

  void ReportWarning (const char* msg, ...);
};
}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
