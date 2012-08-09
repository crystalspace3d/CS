#ifndef __CS_BULLET_PORTAL_H__
#define __CS_BULLET_PORTAL_H__

#include "bulletsector.h"
#include "bulletsystem.h"
#include "collisionobject2.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
class CollisionPortal;
class PortalTraversalData;

/**
 * Represents all the data required by portals to manage traversal of physical objects
 */
class PortalTraversalData
{
public:
  /// Whether this is the data of a copy or the original
  bool Copy;

  /// The traversing object
  csWeakRef<csBulletCollisionObject> Object;

  /// The portal being traversed
  csWeakRef<CollisionPortal> Portal;
  
  /// The other guy
  PortalTraversalData* OtherData;
};

/**
 * A collision portal guides the traversal of physical objects through a portal, and ensures that impulses on either side of the portal
 * effect the other side. The portal achieves that by creating a copy of the object and placing it on the other side and syncing force,
 * velocity and position between the two.
 */
class CollisionPortal
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
  CollisionPortal (iPortal* portal, const csOrthoTransform& meshTrans, csBulletSector* sourceSector);
  ~CollisionPortal ();

  bool CanTraverse(csBulletCollisionObject* obj);

  void AddObject (csRef<csBulletCollisionObject> object)
    { objects.Push (object); }

  void UpdateCollisionsPreStep (csBulletSector* sector);
  void UpdateCollisionsPostStep (csBulletSector* sector) {}

  void SetInformationToCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy,
    const csOrthoTransform& warpTrans);

  void GetInformationFromCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy, float duration);
};

}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
