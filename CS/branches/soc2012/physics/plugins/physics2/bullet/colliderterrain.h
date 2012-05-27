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

#ifndef __CS_BULLET_COLLISIONTERRAIN_H__
#define __CS_BULLET_COLLISIONTERRAIN_H__

#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "csgeom/plane3.h"
#include "imesh/terrain2.h"
#include "ivaria/collisions.h"
#include "common2.h"
#include "colliderprimitives.h"

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


/**
 * The collider of a single terrain cell
 */
class HeightMapCollider : public btHeightfieldTerrainShape
{
  friend class csBulletColliderTerrain;

  btVector3 localScale;
  iTerrainCell* cell;
  float* heightData;

  HeightMapCollider (float* gridData,
    iTerrainCell* cell,
    float minHeight, float maxHeight,
    float internalScale);

  virtual ~HeightMapCollider();

  void UpdataMinHeight (float minHeight);
  void UpdateMaxHeight (float maxHeight);
  void SetLocalScale (const csVector3& scale);

  void UpdateHeight(const csRect& area);

public:
};

/**
 * Wrapper for physically responsive (but static) terrain
 */ 
class csBulletColliderTerrain:
  public scfImplementation4<csBulletColliderTerrain, 
  csBulletCollider, CS::Collisions::iColliderTerrain, iTerrainCellLoadCallback, iTerrainCellHeightDataCallback>
{
  friend class csBulletSector;
  friend class csBulletCollisionObject;
  
  csArray<HeightMapCollider*> colliders;
  csArray<btRigidBody*> bodies;
  csOrthoTransform terrainTransform;
  csBulletSector* collSector;
  csBulletSystem* collSystem;
  csBulletCollisionObject* collBody;
  iTerrainSystem* terrainSystem;
  float minimumHeight;
  float maximumHeight;
  bool unload;

  void LoadCellToCollider(iTerrainCell* cell);
public:
  csBulletColliderTerrain (iTerrainSystem* terrain,
    float minimumHeight, float maximumHeight,
    csBulletSystem* sys);
  virtual ~csBulletColliderTerrain ();
  virtual CS::Collisions::ColliderType GetType () const {return CS::Collisions::COLLIDER_TERRAIN;}
  virtual void SetLocalScale (const csVector3& scale);
  virtual void SetMargin (float margin);

  virtual iTerrainSystem* GetTerrain () const {return terrainSystem;}

  //-- iTerrainCellLoadCallback
  virtual void OnCellLoad (iTerrainCell *cell);
  virtual void OnCellPreLoad (iTerrainCell *cell);
  virtual void OnCellUnload (iTerrainCell *cell);

  //-- iTerrainCellHeightDataCallback
  virtual void OnHeightUpdate (iTerrainCell* cell, const csRect& rectangle);

  /**
   * Returns the collider that represents the given cell in the physical world
   */
  HeightMapCollider* GetCellCollider(iTerrainCell* cell);

  btRigidBody* GetBulletObject (size_t index) {return bodies[index];}
  void RemoveRigidBodies ();
  void AddRigidBodies (csBulletSector* sector, csBulletCollisionObject* body);
};


}
CS_PLUGIN_NAMESPACE_END(Bullet2)
#endif