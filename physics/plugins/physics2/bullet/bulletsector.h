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

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/nobjvec.h"
#include "csutil/scf_implementation.h"
#include "ivaria/reporter.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/physicscommon.h"
#include "ivaria/view.h"
#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "csutil/csobject.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#ifndef __CS_BULLET_SECTOR_H__
#define __CS_BULLET_SECTOR_H__

struct iSector;
struct iMovable;

class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;
class btActionInterface;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csActor;
class csBulletSystem;
class csBulletSector;
class csBulletDebugDraw;
class csBulletRigidBody;
class csBulletSoftBody;
class csBulletCollisionActor;
class csBulletCollisionObject;
class csBulletCollisionTerrain;
class csBulletCollider;
class csBulletJoint;
class CollisionPortal;

// TODO: what's this?
struct BulletActionWrapper : public virtual CS::Physics::iUpdatable
{
  SCF_INTERFACE (CS::Plugin::Bullet2::BulletActionWrapper, 1, 0, 0);

  virtual btActionInterface* GetBulletAction () = 0;
};

// Also implements iPhysicalSector
class csBulletSector : public scfVirtImplementationExt2<
  csBulletSector, csObject,
  CS::Physics::iPhysicalSector,
  CS::Collisions::iCollisionSector>
{
  friend class csBulletCollisionObject;
  friend class csBulletCollisionActor;
  friend class csBulletRigidBody;
  friend class csBulletSoftBody;
  friend class csBulletJoint;
  friend class csBulletCollisionTerrain;
  friend class csBulletKinematicMotionState;
  friend class csBulletMotionState;
  friend class csBulletSystem;
  friend class csBulletGhostCollisionObject;
  friend class CollisionPortal;

  csBulletSystem* system;

  // TODO: Get rid of this hitPortal field
  btGhostObject* hitPortal;

  btDynamicsWorld* bulletWorld;
  btCollisionDispatcher* dispatcher;
  btDefaultCollisionConfiguration* configuration;
  btSequentialImpulseConstraintSolver* solver;
  btBroadphaseInterface* broadphase;
  btSoftBodyWorldInfo* softWorldInfo;

  float linearDampening;
  float angularDampening;

  float linearDisableThreshold;
  float angularDisableThreshold;
  float timeDisableThreshold;

  csRef<iSector> sector;

  csRefArray<csBulletJoint> joints;
  // TODO: csRefArray of portals?
  csArray<CollisionPortal*> portals;
  csRefArrayObject<csBulletCollisionObject> collisionObjects;
  csRefArrayObject<csBulletRigidBody> rigidBodies;
  csRefArrayObject<csBulletSoftBody> softBodies;
  csWeakRefArray<csBulletSoftBody> anchoredSoftBodies;
  csRefArray<csActor> actors;
  csRefArray<csBulletCollisionTerrain> terrains;
  csRefArray<CS::Physics::iUpdatable> updatables;

  void CheckCollisions ();
  void UpdateCollisionPortalsPreStep ();
  void UpdateCollisionPortalsPostStep ();

  void AddRigidBody (CS::Physics::iRigidBody* body);
  void AddSoftBody (CS::Physics::iSoftBody* body);

public:
  csBulletSector (csBulletSystem* system);
  virtual ~csBulletSector ();
  
  //-- iCollisionSector
  virtual CS::Collisions::iCollisionSystem* GetSystem () const
  { return (CS::Collisions::iCollisionSystem*) system; }

  virtual iObject* QueryObject () const
  { return (iObject*) this; }

  virtual CS::Collisions::CollisionObjectType GetSectorType () const
  { return CS::Collisions::COLLISION_OBJECT_PHYSICAL; }
  virtual iPhysicalSector* QueryPhysicalSector () const
  { return (iPhysicalSector*) this; }

  virtual void SetGravity (const csVector3& v);
  virtual csVector3 GetGravity () const;

  inline btDynamicsWorld* GetBulletWorld () const { return bulletWorld; }

  virtual void AddCollisionObject (CS::Collisions::iCollisionObject* object);
  virtual void RemoveCollisionObject (CS::Collisions::iCollisionObject* object);

  virtual size_t GetCollisionObjectCount () {return collisionObjects.GetSize ();}
  virtual CS::Collisions::iCollisionObject* GetCollisionObject (size_t index);

  virtual void AddCollisionTerrain (CS::Collisions::iCollisionTerrain* terrain);
  virtual void RemoveCollisionTerrain (CS::Collisions::iCollisionTerrain* terrain);
  virtual size_t GetCollisionTerrainCount () const { return terrains.GetSize (); }
  virtual CS::Collisions::iCollisionTerrain* GetCollisionTerrain (size_t index) const;
  virtual CS::Collisions::iCollisionTerrain* GetCollisionTerrain (iTerrainSystem* terrain);

  virtual void AddPortal (iPortal* portal, const csOrthoTransform& meshTrans);
  virtual void RemovePortal (iPortal* portal);

  virtual void SetSector (iSector* sector);
  virtual iSector* GetSector () { return sector; }

  virtual CS::Collisions::HitBeamResult HitBeam (const csVector3& start, 
    const csVector3& end);
  virtual CS::Collisions::HitBeamResult HitBeamPortal (const csVector3& start, 
    const csVector3& end);

  virtual bool CollisionTest (CS::Collisions::iCollisionObject* object, 
    csArray<CS::Collisions::CollisionData>& collisions);

  virtual void DeleteAll ();

  //-- iPhysicalSector
  virtual void SetLinearDamping (float d);
  virtual float GetLinearDamping () const {return linearDampening;}

  virtual void SetAngularDamping (float d);
  virtual float GetAngularDamping () const {return angularDampening;}

  virtual void SetAutoDisableParams (float linear,
    float angular, float time);

  virtual size_t GetRigidBodyCount () {return rigidBodies.GetSize ();}
  virtual CS::Physics::iRigidBody* GetRigidBody (size_t index);

  virtual size_t GetSoftBodyCount () {return softBodies.GetSize ();}
  virtual CS::Physics::iSoftBody* GetSoftBody (size_t index);

  virtual void AddJoint (CS::Physics::iJoint* joint);
  virtual void RemoveJoint (CS::Physics::iJoint* joint);

  //Bullet::iPhysicalSector
  //Currently will not use gimpact shape...
  //virtual void SetGimpactEnabled (bool enabled);
  //virtual bool GetGimpactEnabled ();

  bool BulletCollide (btCollisionObject* objectA,
    btCollisionObject* objectB,
    csArray<CS::Collisions::CollisionData>& data);

  CS::Collisions::HitBeamResult RigidHitBeam (btCollisionObject* object, 
			     const csVector3& start,
			     const csVector3& end);

  virtual void AddUpdatable (CS::Physics::iUpdatable* u);
  virtual void RemoveUpdatable (CS::Physics::iUpdatable* u);

  // Internal methods
  void Step (float duration);
  void UpdateSoftBodies (float timeStep);

  void AddSceneNodeToSector (iSceneNode* sceneNode);
  void RemoveSceneNodeFromSector (iSceneNode* sceneNode);

  virtual bool Save (const char* filename);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
