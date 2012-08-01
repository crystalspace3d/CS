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
 * iCollider implementation base class
 */

#ifndef __CS_BULLET_COLLIDER_H__
#define __CS_BULLET_COLLIDER_H__

#include "csutil/weakref.h"
#include "csgeom/plane3.h"
#include "ivaria/collisions.h"
#include "common2.h"

struct csLockedHeightData;
struct iTerrainSystem;
struct iTriangleMesh;


CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

class csBulletSector;
class csBulletSystem;
class csBulletCollider;

csRef<iTriangleMesh> FindColdetTriangleMesh (iMeshWrapper* mesh, 
                                             csStringID baseID, csStringID colldetID);


/**
 * A collection of colliders that belong to another collider
 */
struct csColliderCollection
{
  btCompoundShape compoundShape;

  short staticColliderCount;
  csRefArray<csBulletCollider> colliders;
  csArray<csOrthoTransform> transforms;

  csColliderCollection() :
    staticColliderCount(0)
  {
  }
};


/**
 * Implementation of iCollider. Supports hierarchy of colliders.
 */
class csBulletCollider : public scfVirtImplementation1<csBulletCollider, CS::Collisions::iCollider>
  //public virtual CS::Collisions::iCollider
{
  friend class csBulletCollisionObject;
  friend class csBulletCollisionActor;
  friend class csBulletRigidBody;
  friend class csBulletGhostCollisionObject;

protected:
  btCollisionShape* shape, *usedShape;
  float margin;
  csWeakRef<csBulletSystem> collSystem;

  bool dirty;
  csColliderCollection* children;
  
  float volume;
  btVector3 localInertia;
  btTransform principalAxisTransform;
  bool customPrincipalAxis;

  virtual float ComputeShapeVolume() const = 0;

  bool IsDirty() const;

  inline csColliderCollection* GetOrCreateChildren() 
  {
    if (!children)
    {
      children = new csColliderCollection;
    }
    return children;
  }

public:
  csBulletCollider ();
  virtual ~csBulletCollider();
  virtual CS::Collisions::ColliderType GetColliderType () const = 0;
  virtual void SetLocalScale (const csVector3& scale);
  virtual csVector3 GetLocalScale () const;

  virtual void SetMargin (float margin);
  virtual float GetMargin () const;
  virtual float GetVolume () const { return volume; }

  /**
   * Whether this collider (and all its children) can be used in a dynamic environment
   */
  virtual bool IsDynamic() const;

  /// Returns the AABB of this collider, centered at it's center of mass (assuming uniform density)
  virtual void GetAABB(csVector3& aabbMin, csVector3& aabbMax) const;

  virtual void AddCollider (CS::Collisions::iCollider* collider, const csOrthoTransform& relaTrans = csOrthoTransform ());
  virtual void RemoveCollider (CS::Collisions::iCollider* collider);
  virtual void RemoveCollider (size_t index);

  virtual CS::Collisions::iCollider* GetCollider (size_t index) ;
  virtual size_t GetColliderCount () {return 1 + children ? children->colliders.GetSize () : 0;}
  
  btCollisionShape* GetOrCreateBulletShape();

  inline const btVector3& GetLocalInertia() const 
  { 
    //return btVector3(0, 0, 0); 
    return localInertia; 
  }

  inline const btTransform& GetBtPrincipalAxisTransform() const 
  {
    return principalAxisTransform; 
  }
  
  /// Get the frame of reference
  virtual csOrthoTransform GetPrincipalAxisTransform() const
  {
    return BulletToCS(principalAxisTransform, 1);
  }

  /// Set the frame of reference
  virtual void SetPrincipalAxisTransform(const csOrthoTransform& trans)
  {
    principalAxisTransform = CSToBullet(trans, 1);
    customPrincipalAxis = true;
  }
};

}
CS_PLUGIN_NAMESPACE_END(Bullet2)
#endif
