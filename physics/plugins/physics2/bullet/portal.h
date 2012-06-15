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
  /**
   * The portal as described by the scene
   */
  iPortal* portal;

  /**
   * The transform that converts coordinates of this portal to coordinates of the other side
   */
  csOrthoTransform warpTrans;


  csBulletSector* sourceSector;
  csBulletSector* targetSector;

  /**
   * Represents the portal in the bullet world
   */ 
  btGhostObject* ghostPortal;

  /**
   * All objects that are currently touching this portal
   */
  csRefArray<csBulletCollisionObject> objects;

  /**
   * All transforms of all objects that are currently touching this portal
   */
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
