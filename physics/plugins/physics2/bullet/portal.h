#ifndef __CS_BULLET_PORTAL_H__
#define __CS_BULLET_PORTAL_H__

#include "bulletsector.h"
#include "bulletsystem.h"
#include "collisionobject2.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
class csBulletCollisionPortal
{
//private:
public:
  iPortal* portal;
  csOrthoTransform warpTrans;
  csBulletSector* sourceSector;
  csBulletSector* targetSector;
  btGhostObject* ghostPortal;
  csRefArray<csBulletCollisionObject> objects;
  csArray<csOrthoTransform> transforms;

public:
  csBulletCollisionPortal (iPortal* portal, const csOrthoTransform& meshTrans, csBulletSector* sourceSector);
  ~csBulletCollisionPortal ();

  void AddObject (csRef<csBulletCollisionObject> object)
    { objects.Push (object); }

  void UpdateCollisions (csBulletSector* sector);

  void SetInformationToCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy,
    const csOrthoTransform& warpTrans);

  void GetInformationFromCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy, float duration);
};

}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
