#ifndef __CS_BULLET_COLLISIONOBJECT_H__
#define __CS_BULLET_COLLISIONOBJECT_H__

#include "bulletsystem.h"
#include "common2.h"
#include "colliderprimitives.h"

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

//struct CS::Physics::iPhysicalBody;

class csBulletCollisionObject: public scfVirtImplementationExt1<
  csBulletCollisionObject, csObject, CS::Collisions::iCollisionObject>
{
  friend class csBulletSector;
  friend class csBulletSystem;
  friend class csBulletMotionState;
  friend class csBulletKinematicMotionState;
  friend class csBulletJoint;
  friend class csBulletCollisionTerrain;
  friend class csBulletCollisionActor;
  friend class csBulletGhostCollisionObject;
  friend class csBulletCollisionPortal;

protected:
  csRef<csBulletCollider> collider;
  csRefArray<csBulletCollisionObject> contactObjects;
  csArray<CS::Physics::iJoint*> joints;
  CS::Collisions::CollisionGroup collGroup;
  csWeakRef<iMovable> movable;
  csWeakRef<iCamera> camera;
  csRef<CS::Collisions::iCollisionCallback> collCb;

  btQuaternion portalWarp;

  csBulletSector* sector;
  csBulletSystem* system;
  csBulletCollisionObject* objectOrigin;
  csBulletCollisionObject* objectCopy;
  
  btCollisionObject* btObject;
  
  bool insideWorld;

  void CreateCollisionObject(CS::Collisions::iCollisionObjectProperties* props);

public:
  csBulletCollisionObject (csBulletSystem* sys);

  virtual ~csBulletCollisionObject ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () { return dynamic_cast<CS::Collisions::iCollisionObject*> (this); }
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return nullptr;}
  virtual CS::Collisions::iActor* QueryActor () {return nullptr;}

  /// Returns the sector to which is this object has been added or nullptr, if not in world
  virtual CS::Collisions::iCollisionSector* GetSector () const { return sector; }

  /// Whether this object is currently added to the world
  virtual bool IsInWorld () const { return insideWorld; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const = 0;

  virtual void SetAttachedMovable (iMovable* movable) 
  {
    this->movable = movable; if (movable) movable->SetFullTransform(GetTransform());
  }

  virtual iMovable* GetAttachedMovable () { return movable; }

  virtual void SetAttachedCamera (iCamera* camera) 
  { 
    this->camera = camera; if (camera) camera->SetTransform(GetTransform());
  }
  virtual iCamera* GetAttachedCamera () const {return camera;}

  /// Get the collider that defines this object's shape
  virtual CS::Collisions::iCollider* GetCollider () const { return collider; }

  /// Set the collider that defines this object's shape
  virtual void SetCollider (CS::Collisions::iCollider* collider);

  virtual void SetTransform (const csOrthoTransform& trans);
  virtual csOrthoTransform GetTransform () const;
  virtual void SetRotation (const csMatrix3& rot);
  virtual void Rotate (const csVector3& v, float angle);
  virtual void IncreaseYaw(float yawDelta);
  virtual void IncreasePitch(float pitchDelta);
  
  /// Returns the AABB of this object, centered at it's center of mass
  virtual void GetAABB(csVector3& aabbMin, csVector3& aabbMax) const;

  virtual void RebuildObject () = 0;
  
  virtual void SetCollisionGroup (const char* name);
  virtual void SetCollisionGroup (const CS::Collisions::CollisionGroup& group);
  virtual const CS::Collisions::CollisionGroup& GetCollisionGroup () const { return collGroup; }

  virtual void SetCollisionCallback (CS::Collisions::iCollisionCallback* cb) {collCb = cb;}
  virtual CS::Collisions::iCollisionCallback* GetCollisionCallback () {return collCb;}

  virtual bool Collide (CS::Collisions::iCollisionObject* otherObject);
  virtual CS::Collisions::HitBeamResult HitBeam (const csVector3& start, const csVector3& end);

  virtual size_t GetContactObjectsCount ();
  virtual CS::Collisions::iCollisionObject* GetContactObject (size_t index);

  btCollisionObject* GetBulletCollisionPointer () {return btObject;}
  virtual bool RemoveBulletObject () = 0;
  virtual bool AddBulletObject () = 0;
  void RemoveObjectCopy () 
  {
    csBulletSector* sec = objectCopy->sector;
    sec->RemoveCollisionObject (objectCopy);
    objectCopy = nullptr;
  }

  bool TestOnGround();
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
