/*
    Copyright (C) 2012 by Dominik Seifert
    Copyright (C) 2011 by Liu Lu

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_BULLET_COLLISIONOBJECT_H__
#define __CS_BULLET_COLLISIONOBJECT_H__

#include "bulletsystem.h"
#include "common2.h"
#include "colliderprimitives.h"

struct iSceneNode;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
class PortalTraversalData;

//struct CS::Physics::iPhysicalBody;

class csBulletCollisionObject: public scfVirtImplementationExt1<
  csBulletCollisionObject, csObject,
  CS::Collisions::iCollisionObject
  //,CS::Collisions::iCollisionObjectFactory
>
{
  friend class csBulletSector;
  friend class csBulletSystem;
  friend class csBulletMotionState;
  friend class csBulletKinematicMotionState;
  friend class csBulletJoint;
  friend class csBulletCollisionTerrain;
  friend class csBulletCollisionActor;
  friend class csBulletGhostCollisionObject;
  friend class CollisionPortal;

protected:
  csRef<csBulletCollider> collider;
  csRefArray<csBulletCollisionObject> contactObjects;
  csArray<CS::Physics::iJoint*> joints;
  CS::Collisions::CollisionGroup collGroup;
  csRef<iSceneNode> sceneNode;
  csWeakRef<iCamera> camera;
  csRef<CS::Collisions::iCollisionCallback> collCb;

  btQuaternion portalWarp;

  csBulletSector* sector;
  csBulletSystem* system;

  PortalTraversalData* portalData;
  
  btCollisionObject* btObject;
  
  bool insideWorld;

  void CreateCollisionObject(CS::Collisions::iCollisionObjectFactory* props);

public:
  csBulletCollisionObject (csBulletSystem* sys);

  virtual ~csBulletCollisionObject ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () { return dynamic_cast<CS::Collisions::iCollisionObject*> (this); }
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return nullptr;}
  virtual CS::Collisions::iActor* QueryActor () {return nullptr;}

  virtual CS::Collisions::iCollisionSystem* GetSystem() const { return system; }

  /// Returns the sector to which is this object has been added or nullptr, if not in world
  virtual CS::Collisions::iCollisionSector* GetSector () const { return sector; }

  /// Whether this object is currently added to the world
  virtual bool IsInWorld () const { return insideWorld; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const = 0;

  virtual void SetAttachedSceneNode (iSceneNode* newSceneNode);
  virtual iSceneNode* GetAttachedSceneNode () const { return sceneNode; }

  virtual iMovable* GetAttachedMovable () const;

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

  bool TestOnGround();

  /// Whether this object may be excluded from deactivation.
  virtual bool GetMayBeDeactivated() const { return btObject->getActivationState() == DISABLE_DEACTIVATION; }
  /// Whether this object may be excluded from deactivation.
  virtual void SetMayBeDeactivated(bool d) { btObject->setActivationState(d ? 0 : DISABLE_DEACTIVATION); }

  /// Clone this object
  virtual btCollisionObject* CreateBulletObject()
  {
    return nullptr;
  }
  
  /// Clone this object
  virtual csPtr<CS::Collisions::iCollisionObject> CloneObject() 
  { 
    return csPtr<CS::Collisions::iCollisionObject>(nullptr); 
  }
  
  /**
   * Clone this object for use with portals.
   * Might adjust some object properties necessary for an accurate portal simulation.
   */
  virtual csPtr<CS::Collisions::iCollisionObject> ClonePassivePortalObject() 
  { 
    return CloneObject();
  }

  /**
   * Passive objects, such as portal clones, are not supposed to be tempered with
   */
  virtual bool IsPassive() const;

  inline PortalTraversalData* GetPortalData() const
  {
    return portalData;
  }
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
