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

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/nobjvec.h"
#include "csutil/scf_implementation.h"
#include "ivaria/reporter.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/view.h"
#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "csutil/csobject.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#ifndef __CS_BULLET_SECTOR_H__
#define __CS_BULLET_SECTOR_H__

struct iSector;
class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;

//class btGhostObject;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
class csBulletSystem;
class csBulletSector;
class csBulletDebugDraw;
class csBulletRigidBody;
class csBulletSoftBody;
class csBulletCollisionObject;
class csBulletCollisionActor;
class csBulletCollider;
class csBulletJoint;
class csBulletCollisionPortal;

// Also implements iPhysicalSector
class csBulletSector : public scfImplementationExt1<
  csBulletSector, csObject,
  CS::Physics::iPhysicalSector>
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
  friend class csBulletCollisionPortal;

  csWeakRef<csBulletSystem> sys;

  bool isSoftWorld;
  csVector3 gravity;

  // TODO: Get rid of this hitPortal field
  btGhostObject* hitPortal;

  csBulletDebugDraw* debugDraw;
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
  float worldTimeStep;
  size_t worldMaxSteps;

  csRefArray<csBulletJoint> joints;
  csArray<csBulletCollisionPortal*> portals;
  csRefArrayObject<csBulletCollisionObject> collisionObjects;
  csRefArrayObject<csBulletRigidBody> rigidBodies;
  csRefArrayObject<csBulletSoftBody> softBodies;
  csWeakRefArray<csBulletSoftBody> anchoredSoftBodies;
  csRef<iSector> sector;

  void CheckCollisions();
  void UpdatecsBulletCollisionPortals ();
  void SetInformationToCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy,
    const csOrthoTransform& warpTrans);
  void GetInformationFromCopy (csBulletCollisionObject* obj, csBulletCollisionObject* cpy);
  

  void AddCollisionActor (CS::Collisions::iCollisionActor* actor);

  virtual void AddRigidBody (CS::Physics::iRigidBody* body);

  virtual void AddSoftBody (CS::Physics::iSoftBody* body);

public:
  csBulletSector (csBulletSystem* sys);
  virtual ~csBulletSector ();
  
  virtual CS::Collisions::iCollisionSystem* GetSystem();

  virtual iObject* QueryObject () {return (iObject*) this;}
  //iCollisionSector
  virtual void SetGravity (const csVector3& v);
  virtual csVector3 GetGravity () const {return gravity;}

  inline btDynamicsWorld* GetBulletWorld() const { return bulletWorld; }

  virtual void AddCollisionObject(CS::Collisions::iCollisionObject* object);
  virtual void RemoveCollisionObject(CS::Collisions::iCollisionObject* object);

  virtual size_t GetCollisionObjectCount () {return collisionObjects.GetSize ();}
  virtual CS::Collisions::iCollisionObject* GetCollisionObject (size_t index);
  virtual CS::Collisions::iCollisionObject* FindCollisionObject (const char* name);

  virtual void AddCollisionTerrain(CS::Collisions::iCollisionTerrain* terrain);

  virtual void AddPortal(iPortal* portal, const csOrthoTransform& meshTrans);
  virtual void RemovePortal(iPortal* portal);

  virtual void SetSector(iSector* sector);
  virtual iSector* GetSector(){return sector;}

  virtual CS::Collisions::HitBeamResult HitBeam(const csVector3& start, 
    const csVector3& end);

  virtual CS::Collisions::HitBeamResult HitBeamPortal(const csVector3& start, 
    const csVector3& end);

  virtual bool CollisionTest(CS::Collisions::iCollisionObject* object, 
    csArray<CS::Collisions::CollisionData>& collisions);

  /*virtual MoveResult MoveTest (iCollisionObject* object,
    const csOrthoTransform& fromWorld, const csOrthoTransform& toWorld);*/

  //iPhysicalSector
  virtual void SetSimulationSpeed (float speed);
  virtual void SetStepParameters (float timeStep,
    size_t maxSteps, size_t iterations);
  virtual void Step (float duration);

  virtual void SetLinearDamping (float d);
  virtual float GetLinearDamping () const {return linearDampening;}

  virtual void SetAngularDamping (float d);
  virtual float GetAngularDamping () const {return angularDampening;}

  virtual void SetAutoDisableParams (float linear,
    float angular, float time);

  virtual size_t GetRigidBodyCount () {return rigidBodies.GetSize ();}
  virtual CS::Physics::iRigidBody* GetRigidBody (size_t index);
  virtual CS::Physics::iRigidBody* FindRigidBody (const char* name);

  virtual size_t GetSoftBodyCount () {return softBodies.GetSize ();}
  virtual CS::Physics::iSoftBody* GetSoftBody (size_t index);
  virtual CS::Physics::iSoftBody* FindSoftBody (const char* name);

  virtual void AddJoint (CS::Physics::iJoint* joint);
  virtual void RemoveJoint (CS::Physics::iJoint* joint);

  virtual void SetSoftBodyEnabled (bool enabled);
  virtual bool GetSoftBodyEnabled () {return isSoftWorld;}

  //Bullet::iPhysicalSector
  //Currently will not use gimpact shape...
  //virtual void SetGimpactEnabled (bool enabled);
  //virtual bool GetGimpactEnabled ();

  virtual bool SaveWorld (const char* filename);

  virtual void DebugDraw (iView* rview);
  virtual void SetDebugMode (CS::Physics::DebugMode mode);
  virtual CS::Physics::DebugMode GetDebugMode ();

  virtual void StartProfile ();

  virtual void StopProfile ();

  virtual void DumpProfile (bool resetProfile = true);

  bool BulletCollide (btCollisionObject* objectA,
    btCollisionObject* objectB,
    csArray<CS::Collisions::CollisionData>& data);

  CS::Collisions::HitBeamResult RigidHitBeam(btCollisionObject* object, 
			     const csVector3& start,
			     const csVector3& end);

  void UpdateSoftBodies (float timeStep);

  void AddMovableToSector (CS::Collisions::iCollisionObject* obj);

  void RemoveMovableFromSector (CS::Collisions::iCollisionObject* obj);

  inline csScalar GetWorldTimeStep() const { return worldTimeStep; }
};
}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif