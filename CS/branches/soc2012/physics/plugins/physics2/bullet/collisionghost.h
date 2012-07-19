#ifndef __CS_BULLET_COLLISIONGHOST_H__
#define __CS_BULLET_COLLISIONGHOST_H__

#include "bulletsystem.h"
#include "common2.h"
#include "collisionobject2.h"

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
class csBulletGhostCollisionObject : public scfVirtImplementationExt1<
  csBulletGhostCollisionObject, csBulletCollisionObject, CS::Collisions::iGhostCollisionObject>
{
public:
  void CreateGhostCollisionObject(CS::Collisions::iGhostCollisionObjectFactory* props);

public:
  csBulletGhostCollisionObject(csBulletSystem* sys);
  virtual ~csBulletGhostCollisionObject();
  virtual bool IsPhysicalObject() const { return false; }
  virtual CS::Collisions::CollisionObjectType GetObjectType () const { return CS::Collisions::COLLISION_OBJECT_GHOST; }

  btPairCachingGhostObject* GetPairCachingGhostObject() const
  { 
    btGhostObject* go = btGhostObject::upcast(btObject);
    btPairCachingGhostObject* obj = (btPairCachingGhostObject*) (go); 
    return obj;
  }
  
  virtual void RebuildObject();
  virtual bool AddBulletObject();
  virtual bool RemoveBulletObject();
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif