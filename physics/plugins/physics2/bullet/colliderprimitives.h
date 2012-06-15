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

/**
 * Collection of primitive colliders
 */

#ifndef __CS_BULLET_COLLIDERS_H__
#define __CS_BULLET_COLLIDERS_H__

#include "csgeom/plane3.h"
#include "ivaria/collisions.h"
#include "common2.h"

class btBoxShape;
class btSphereShape;
class btCylinderShapeZ;
class btCapsuleShapeZ;
class btConeShapeZ;
class btStaticPlaneShape;
class btConvexHullShape;
class btCollisionShape;
class btScaledBvhTriangleMeshShape;
class btBvhTriangleMeshShape;
class btGImpactMeshShape;
class btTriangleMesh;
struct csLockedHeightData;
struct iTerrainSystem;
struct iTriangleMesh;


CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

class csBulletSector;
class csBulletSystem;

csRef<iTriangleMesh> FindColdetTriangleMesh (iMeshWrapper* mesh, 
                                             csStringID baseID, csStringID colldetID);

class csBulletCollider: public virtual CS::Collisions::iCollider
{
  friend class csBulletCollisionObject;
  friend class csBulletCollisionActor;
  friend class csBulletRigidBody;
  friend class csBulletCollisionGhostObject;

protected:
  csVector3 scale;
  btCollisionShape* shape;
  float margin;
  float volume;
  csBulletSystem* collSystem;

public:
  csBulletCollider ();
  virtual ~csBulletCollider() {}
  virtual CS::Collisions::ColliderType GetType () const = 0;
  virtual void SetLocalScale (const csVector3& scale);
  virtual const csVector3& GetLocalScale () const {return scale;}
  virtual void SetMargin (float margin);
  virtual float GetMargin () const;
  virtual float GetVolume () const {return volume;}
};

class csBulletColliderBox: 
  public scfImplementation2<csBulletColliderBox,
  csBulletCollider, CS::Collisions::iColliderBox>
{
  csVector3 boxSize;

public:
  csBulletColliderBox (const csVector3& boxSize, csBulletSystem* sys);
  virtual ~csBulletColliderBox ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_BOX;}

  virtual csVector3 GetBoxGeometry () {return boxSize;}
};

class csBulletColliderSphere:
  public scfImplementation2<csBulletColliderSphere,
  csBulletCollider, CS::Collisions::iColliderSphere>
{
  float radius;

public:
  csBulletColliderSphere (float radius, csBulletSystem* sys);
  virtual ~csBulletColliderSphere ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_SPHERE;}
  virtual void SetMargin (float margin);

  virtual float GetSphereGeometry () {return radius;}
};

class csBulletColliderCylinder:
  public scfImplementation2<csBulletColliderCylinder,
  csBulletCollider, CS::Collisions::iColliderCylinder>
{
  float length;
  float radius;

public:
  csBulletColliderCylinder (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCylinder ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_CYLINDER;}

  virtual void GetCylinderGeometry (float& length, float& radius);
};

class csBulletColliderCapsule: 
  public scfImplementation2<csBulletColliderCapsule,
  csBulletCollider, CS::Collisions::iColliderCapsule>
{
  float length;
  float radius;

public:
  csBulletColliderCapsule (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCapsule ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_CAPSULE;}

  virtual void GetCapsuleGeometry (float& length, float& radius);
};

class csBulletColliderCone:
  public scfImplementation2<csBulletColliderCone,
  csBulletCollider, CS::Collisions::iColliderCone>
{
  float length;
  float radius;

public:
  csBulletColliderCone (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCone ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_CONE;}

  virtual void GetConeGeometry (float& length, float& radius);
};

class csBulletColliderPlane:
  public scfImplementation2<csBulletColliderPlane,
  csBulletCollider, CS::Collisions::iColliderPlane>
{
  csPlane3 plane;

public:
  csBulletColliderPlane (const csPlane3& plane, csBulletSystem* sys);
  virtual ~csBulletColliderPlane ();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_PLANE;}
  virtual void SetLocalScale (const csVector3& scale) {}

  virtual csPlane3 GetPlaneGeometry () {return plane;}
};

class csBulletColliderConvexMesh:
  public scfImplementation2<csBulletColliderConvexMesh,
  csBulletCollider, CS::Collisions::iColliderConvexMesh>
{
  iMeshWrapper* mesh;
  
public:
  csBulletColliderConvexMesh (iMeshWrapper* mesh, csBulletSystem* sys, bool simplify);
  csBulletColliderConvexMesh (btConvexHullShape* shape, float volume, csBulletSystem* sys) 
    : scfImplementationType (this)
  {
    this->shape = shape; 
    collSystem = sys;
    this->volume = volume;
  }
  virtual ~csBulletColliderConvexMesh ();
  virtual CS::Collisions::ColliderType GetType () const
 {return CS::Collisions::COLLIDER_CONVEX_MESH;}

  virtual iMeshWrapper* GetMesh () {return mesh;}
};

class csBulletColliderConcaveMesh:
  public scfImplementation2<csBulletColliderConcaveMesh, 
  csBulletCollider, CS::Collisions::iColliderConcaveMesh>
{
  friend class csBulletColliderConcaveMeshScaled;
  btTriangleMesh* triMesh;
  iMeshWrapper* mesh;

public:
  csBulletColliderConcaveMesh (iMeshWrapper* mesh, csBulletSystem* sys);
  virtual ~csBulletColliderConcaveMesh ();
  virtual CS::Collisions::ColliderType GetType () const
 {return CS::Collisions::COLLIDER_CONCAVE_MESH;}

  virtual iMeshWrapper* GetMesh () {return mesh;}
};

class csBulletColliderConcaveMeshScaled:
  public scfImplementation2<csBulletColliderConcaveMeshScaled,
  csBulletCollider, CS::Collisions::iColliderConcaveMeshScaled>
{
  csBulletColliderConcaveMesh* originalCollider;

public:
  csBulletColliderConcaveMeshScaled (CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale, csBulletSystem* sys);
  virtual ~csBulletColliderConcaveMeshScaled();
  virtual CS::Collisions::ColliderType GetType () const
  {return CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED;}

  virtual CS::Collisions::iColliderConcaveMesh* GetCollider () 
  {return dynamic_cast<CS::Collisions::iColliderConcaveMesh*>(originalCollider);}
};
}
CS_PLUGIN_NAMESPACE_END(Bullet2)
#endif
