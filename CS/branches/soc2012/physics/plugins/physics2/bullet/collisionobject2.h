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
  friend class csBulletColliderTerrain;
  friend class csBulletCollisionActor;
  friend class csBulletCollisionGhostObject;
  friend class csBulletCollisionPortal;

protected:
  csRefArray<csBulletCollider> colliders;
  csRefArray<csBulletCollisionObject> contactObjects;
  csArray<csOrthoTransform> relaTransforms;
  csArray<CS::Physics::iJoint*> joints;
  CS::Collisions::CollisionGroup collGroup;
  csWeakRef<iMovable> movable;
  csWeakRef<iCamera> camera;
  csRef<CS::Collisions::iCollisionCallback> collCb;

  btTransform invPricipalAxis;
  btQuaternion portalWarp;

  csBulletSector* sector;
  csBulletSystem* system;
  btCollisionObject* btObject;
  csBulletMotionState* motionState;
  btCompoundShape* compoundShape;
  csBulletCollisionObject* objectOrigin;
  csBulletCollisionObject* objectCopy;

  short haveStaticColliders;
  bool insideWorld;
  bool shapeChanged;
  bool isTerrain;

#ifdef OBJECT_DEBUG_NAMES
  csString debugName;
#endif

public:
  csBulletCollisionObject (csBulletSystem* sys);
  virtual ~csBulletCollisionObject ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () { return dynamic_cast<CS::Collisions::iCollisionObject*> (this); }
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return nullptr;}

  /// Returns the sector to which is this object has been added or nullptr, if not in world
  virtual CS::Collisions::iCollisionSector* GetSector () const { return sector; }

  /// Whether this object is currently added to the world
  virtual bool IsInWorld () const { return sector != nullptr; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const = 0;

  virtual void SetAttachedMovable (iMovable* movable){this->movable = movable;}
  virtual iMovable* GetAttachedMovable (){return movable;}

  virtual void SetAttachedCamera (iCamera* camera){this->camera = camera;}
  virtual iCamera* GetAttachedCamera () const {return camera;}

  virtual void SetTransform (const csOrthoTransform& trans);
  virtual csOrthoTransform GetTransform () const;
  virtual void SetRotation (const csMatrix3& rot);
  virtual void Rotate (const csVector3& v, float angle);
  virtual void IncreaseYaw(float yawDelta);
  virtual void IncreasePitch(float pitchDelta);

  virtual void AddCollider (CS::Collisions::iCollider* collider, const csOrthoTransform& relaTrans
    = csOrthoTransform (csMatrix3 (), csVector3 (0)));
  virtual void RemoveCollider (CS::Collisions::iCollider* collider);
  virtual void RemoveCollider (size_t index);

  virtual CS::Collisions::iCollider* GetCollider (size_t index) ;
  virtual size_t GetColliderCount () {return colliders.GetSize ();}

  virtual void RebuildObject () = 0;

  virtual void SetCollisionGroup (const char* name);
  virtual const char* GetCollisionGroup () const {return collGroup.name.GetData ();}

  virtual void SetCollisionCallback (CS::Collisions::iCollisionCallback* cb) {collCb = cb;}
  virtual CS::Collisions::iCollisionCallback* GetCollisionCallback () {return collCb;}

  virtual bool Collide (CS::Collisions::iCollisionObject* otherObject);
  virtual CS::Collisions::HitBeamResult HitBeam (const csVector3& start, const csVector3& end);

  virtual size_t GetContactObjectsCount ();
  virtual CS::Collisions::iCollisionObject* GetContactObject (size_t index);

  btCollisionObject* GetBulletCollisionPointer () {return btObject;}
  virtual void CreateBulletObject() {}
  virtual bool RemoveBulletObject () = 0;
  virtual bool AddBulletObject () = 0;
  void RemoveObjectCopy () 
  {
    csBulletSector* sec = objectCopy->sector;
    sec->RemoveCollisionObject (objectCopy);
    objectCopy = nullptr;
  }
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
