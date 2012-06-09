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

#include "cssysdef.h"
//#include "ivaria/reporter.h"
#include "ivaria/softanim.h"
#include "imesh/animesh.h"
#include "iengine/scenenode.h"
#include "iengine/movable.h"
#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "csutil/sysfunc.h"
#include "iutil/objreg.h"
#include "ivaria/view.h"
#include "ivaria/collisions.h"
#include "igeom/trimesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "convexdecompose/ConvexBuilder.h"

#include "csutil/custom_new_enable.h"

#include "bulletsystem.h"
#include "common2.h"
#include "colliderprimitives.h"
#include "colliderterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "collisionactor2.h"
#include "joint2.h"
#include "portal.h"

const float COLLISION_THRESHOLD = 0.01f;
#define AABB_DIMENSIONS 10000.0f


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
struct PointContactResult : public btCollisionWorld::ContactResultCallback
{
  csArray<CS::Collisions::CollisionData>& colls;
  csBulletSystem* sys;
  PointContactResult(csBulletSystem* sys, csArray<CS::Collisions::CollisionData>& collisions) : colls(collisions), sys(sys) 
  {
  }
  virtual	btScalar	addSingleResult (btManifoldPoint& cp,	const btCollisionObject* colObj0, int partId0,int index0,const btCollisionObject* colObj1,int partId1,int index1)
  {
    CS::Collisions::CollisionData data;
    data.objectA = static_cast<CS::Collisions::iCollisionObject*>(colObj0->getUserPointer ());
    data.objectB = static_cast<CS::Collisions::iCollisionObject*>(colObj1->getUserPointer ());
    data.penetration = cp.m_distance1 * sys->getInverseInternalScale ();
    data.positionWorldOnA = BulletToCS (cp.m_positionWorldOnA, sys->getInverseInternalScale ());
    data.positionWorldOnB = BulletToCS (cp.m_positionWorldOnB, sys->getInverseInternalScale ());
    data.normalWorldOnB = BulletToCS (cp.m_normalWorldOnB, sys->getInverseInternalScale ());
    colls.Push (data);
    return 0;
  }
};

csBulletSector::csBulletSector (csBulletSystem* sys)
  :scfImplementationType (this), sys (sys), isSoftWorld (false),
  hitPortal (NULL), debugDraw (NULL), softWorldInfo (NULL),
  linearDampening (0.0f), angularDampening (0.0f),
  linearDisableThreshold (0.8f), angularDisableThreshold (1.0f),
  timeDisableThreshold (0.0f), worldTimeStep (1.0f / 60.0f), worldMaxSteps (1)
{
  configuration = new btDefaultCollisionConfiguration ();
  dispatcher = new btCollisionDispatcher (configuration);
  solver = new btSequentialImpulseConstraintSolver;

  const int maxProxies = 32766;

  btVector3 worldAabbMin (-AABB_DIMENSIONS, -AABB_DIMENSIONS, -AABB_DIMENSIONS);
  btVector3 worldAabbMax (AABB_DIMENSIONS, AABB_DIMENSIONS, AABB_DIMENSIONS);
  broadphase = new btAxisSweep3 (worldAabbMin, worldAabbMax, maxProxies);

  bulletWorld = new btDiscreteDynamicsWorld (dispatcher,
    broadphase, solver, configuration);

  broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

  SetGravity (csVector3 (0.0f, -9.81f, 0.0f));

  CollisionGroupMask allFilter = -1;

  CollisionGroup defaultGroup ("Default");
  defaultGroup.value = CollisionGroupMaskValueDefault;
  defaultGroup.mask = allFilter;
  collGroups.Push (defaultGroup);

  CS::Collisions::CollisionGroup staticGroup ("Static");
  staticGroup.value = CollisionGroupMaskValueStatic;
  staticGroup.mask = allFilter ^ CollisionGroupMaskValueStatic;
  collGroups.Push (staticGroup);

  CS::Collisions::CollisionGroup kinematicGroup ("Kinematic");
  kinematicGroup.value = CollisionGroupMaskValueKinematic;
  kinematicGroup.mask = allFilter ^ CollisionGroupMaskValueKinematic;
  collGroups.Push (kinematicGroup);

  CS::Collisions::CollisionGroup portalGroup ("Portal");
  portalGroup.value = CollisionGroupMaskValuePortal;
  portalGroup.mask = allFilter ^ CollisionGroupMaskValuePortal;
  collGroups.Push (portalGroup);

  CS::Collisions::CollisionGroup copyGroup ("PortalCopy");
  copyGroup.value = CollisionGroupMaskValuePortalCopy;
  copyGroup.mask = allFilter ^ CollisionGroupMaskValuePortalCopy;
  collGroups.Push (copyGroup);

  CS::Collisions::CollisionGroup characterGroup ("Character");
  characterGroup.value = CollisionGroupMaskValueActor;
  characterGroup.mask = allFilter ^ CollisionGroupMaskValueActor;
  collGroups.Push (characterGroup);

  SetGroupCollision ("Portal", "Static", false);
  SetGroupCollision ("Portal", "PortalCopy", false);

  systemFilterCount = 6;
}

csBulletSector::~csBulletSector ()
{
  for (size_t i = 0; i < portals.GetSize (); i++)
  {
    bulletWorld->removeCollisionObject (portals[i]->ghostPortal);
    delete portals[i];
  }

  joints.DeleteAll ();
  softBodies.DeleteAll ();
  rigidBodies.DeleteAll ();
  collisionObjects.DeleteAll ();
  portals.DeleteAll ();
  collisionActor = NULL;
  
  delete dispatcher;
  delete configuration;
  delete solver;
  delete broadphase;
  if (debugDraw)
  {
    delete debugDraw;
  }
  if (softWorldInfo)
  {
    delete softWorldInfo;
  }
}

void csBulletSector::SetGravity (const csVector3& v)
{
  gravity = v;
  btVector3 gravity = CSToBullet (v, sys->getInternalScale ());
  bulletWorld->setGravity (gravity);

  if (isSoftWorld)
    softWorldInfo->m_gravity = gravity;
}

void csBulletSector::AddCollisionObject (CS::Collisions::iCollisionObject* object)
{
  csBulletCollisionObject* obj (dynamic_cast<csBulletCollisionObject*>(object));

  if (obj->GetObjectType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
  {
    iPhysicalBody* phyBody = obj->QueryPhysicalBody ();
    if (phyBody->GetBodyType () == CS::Physics::BODY_RIGID)
      AddRigidBody (phyBody->QueryRigidBody ());
    else
      AddSoftBody (phyBody->QuerySoftBody ());
  }
  else
  {
    collisionObjects.Push (obj);
    obj->sector = this;
    obj->collGroup = collGroups[CollisionGroupTypeStatic]; // Static Group.
    obj->AddBulletObject ();

    AddMovableToSector (object);
  }
}

void csBulletSector::RemoveCollisionObject (CS::Collisions::iCollisionObject* object)
{
  csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
  if (!collObject)
    return;

  if (collObject->GetObjectType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
  {
    iPhysicalBody* phyBody = dynamic_cast<iPhysicalBody*> (object);
    if (phyBody->GetBodyType () == CS::Physics::BODY_RIGID)
      RemoveRigidBody (phyBody->QueryRigidBody ());
    else
      RemoveSoftBody (phyBody->QuerySoftBody ());
  }
  else
  {
    bool removed = collObject->RemoveBulletObject ();

    if (removed)
    {
      RemoveMovableFromSector (object);
      collisionObjects.Delete (collObject);
    }
  }
}

CS::Collisions::iCollisionObject* csBulletSector::GetCollisionObject (size_t index)
{
  if (index >= 0 && index < collisionObjects.GetSize ())
    return collisionObjects[index]->QueryCollisionObject ();
  else
    return NULL;
}

CS::Collisions::iCollisionObject* csBulletSector::FindCollisionObject (const char* name)
{
  return collisionObjects.FindByName (name);
}

void csBulletSector::AddPortal (iPortal* portal, const csOrthoTransform& meshTrans)
{
  csBulletCollisionPortal* newPortal = new csBulletCollisionPortal (portal, meshTrans, this);
  portals.Push (newPortal);
}

void csBulletSector::RemovePortal (iPortal* portal)
{
  for (size_t i = 0; i < portals.GetSize (); i++)
  {
    if (portals[i]->portal == portal)
    {
      for (size_t j = 0; j < portals[i]->objects.GetSize (); j++)
      {
        if (portals[i]->objects[j]->objectCopy)
        {
          portals[i]->targetSector->RemoveCollisionObject (portals[i]->objects[j]->objectCopy);
        }
      }
      bulletWorld->removeCollisionObject (portals[i]->ghostPortal);
      portals.DeleteIndexFast (i);
      return;
    }
  }
}

void csBulletSector::SetSector (iSector* sector)
{
   this->sector = sector;

   const csSet<csPtrKey<iMeshWrapper> >& portal_meshes = 
     sector->GetPortalMeshes ();
   csSet<csPtrKey<iMeshWrapper> >::GlobalIterator it = 
     portal_meshes.GetIterator ();

   while (it.HasNext ())
   {
     iMeshWrapper* portalMesh = it.Next ();
     iPortalContainer* portalContainer = portalMesh->GetPortalContainer ();
     int i; 
     for (i = 0; i < portalContainer->GetPortalCount (); i++)
     {
       iPortal* portal = portalContainer->GetPortal (i);
       AddPortal (portal, portalMesh->GetMovable ()->GetFullTransform ());
     }
   }
}

CS::Collisions::HitBeamResult csBulletSector::HitBeam (const csVector3& start, const csVector3& end)
{
  btVector3 rayFrom = CSToBullet (start, sys->getInternalScale ());
  btVector3 rayTo = CSToBullet (end, sys->getInternalScale ());

  btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);
  rayCallback.m_collisionFilterMask = collGroups[CollisionGroupTypePortalCopy].mask;
  rayCallback.m_collisionFilterGroup = collGroups[CollisionGroupTypeDefault].value;
  bulletWorld->rayTest (rayFrom, rayTo, rayCallback);

  CS::Collisions::HitBeamResult result;

  if(rayCallback.hasHit ())
  {
    CS::Collisions::iCollisionObject* collObject = static_cast<CS::Collisions::iCollisionObject*> (
      rayCallback.m_collisionObject->getUserPointer ());

    result.hasHit = true;

    if (rayCallback.m_collisionObject->getInternalType () == btCollisionObject::CO_GHOST_OBJECT
      && rayCallback.m_collisionObject->getUserPointer () == NULL)
    {
      // hit a ghost object (potentially a portal...)
      collObject = NULL;
      result.hasHit = false;
      hitPortal = btGhostObject::upcast (rayCallback.m_collisionObject);
    }
    else if (rayCallback.m_collisionObject->getInternalType () == btCollisionObject::CO_SOFT_BODY)
    {
      // hit a soft body
      btSoftBody* body = btSoftBody::upcast (rayCallback.m_collisionObject);
      btSoftBody::sRayCast ray;

      if (body->rayTest (rayFrom, rayTo, ray))
      {
        result.object = collObject;
        result.isect = BulletToCS (rayCallback.m_hitPointWorld,
          sys->getInverseInternalScale ());
        result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
          sys->getInverseInternalScale ());	

        // Find the closest vertex that was hit
        // TODO: there must be something more efficient than a second ray test
        btVector3 impact = rayFrom + (rayTo - rayFrom) * ray.fraction;
        switch (ray.feature)
        {
        case btSoftBody::eFeature::Face:
          {
            btSoftBody::Face& face = body->m_faces[ray.index];
            btSoftBody::Node* node = face.m_n[0];
            float distance = (node->m_x - impact).length2 ();

            for (int i = 1; i < 3; i++)
            {
              float nodeDistance = (face.m_n[i]->m_x - impact).length2 ();
              if (nodeDistance < distance)
              {
                node = face.m_n[i];
                distance = nodeDistance;
              }
            }

            result.vertexIndex = size_t (node - &body->m_nodes[0]);
            break;
          }
        default:
          break;
        }
        return result;
      } //has hit softbody
    } //softBody
    else
    { 
      // "normal" object
      result.object = collObject;
      result.isect = BulletToCS (rayCallback.m_hitPointWorld,
        sys->getInverseInternalScale ());
      result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
        sys->getInverseInternalScale ());
      return result;
    } // not softBody
  } //has hit
  return result;
}

CS::Collisions::HitBeamResult csBulletSector::HitBeamPortal (const csVector3& start, const csVector3& end)
{
  
  hitPortal = NULL;

  CS::Collisions::HitBeamResult result = HitBeam (start, end);

  if (result.object == NULL && hitPortal)
  {
    //Portals are not included.
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      if (portals[i]->ghostPortal == hitPortal)
      {
        if (!portals[i]->portal->CompleteSector (0))
        {
          result.hasHit = false;
          return result;
        }
        else
        {
          /// Something wrong with the transform.
          csOrthoTransform warpWor;
          csVector3 newStart = result.isect;
          csVector3 newEnd = end;
          if (portals[i]->portal->GetFlags ().Check (CS_PORTAL_WARP))
          {
            newStart = portals[i]->warpTrans.This2Other (newStart);
            newEnd = portals[i]->warpTrans.This2Other (newEnd);
          }

          result = portals[i]->targetSector->HitBeamPortal (newStart, newEnd);
          return result;
        }
      }
    }
    
  }
  return result;
}

CS::Collisions::CollisionGroup& csBulletSector::CreateCollisionGroup (const char* name)
{
  size_t groupCount = collGroups.GetSize ();
  if (groupCount >= sizeof (CS::Collisions::CollisionGroupMask) * 8)
    return collGroups[CollisionGroupTypeDefault];

  CS::Collisions::CollisionGroup newGroup(name);
  newGroup.value = 1 << groupCount;
  newGroup.mask = ~newGroup.value;
  collGroups.Push (newGroup);
  return collGroups[groupCount];
}

CS::Collisions::CollisionGroup& csBulletSector::FindCollisionGroup (const char* name)
{
  size_t index = collGroups.FindKey (CollisionGroupVector::KeyCmp (name));
  if (index == csArrayItemNotFound)
    return collGroups[CollisionGroupTypeDefault];
  else
    return collGroups[index];
}

void csBulletSector::SetGroupCollision (const char* name1,
                                        const char* name2,
                                        bool collide)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return;
  if (!collide)
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].mask &= ~(1 << index2);
    if (index2 >= systemFilterCount)
      collGroups[index2].mask &= ~(1 << index1);
  }
  else
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].mask |= 1 << index2;
    if (index2 >= systemFilterCount)
      collGroups[index2].mask |= 1 << index1;
  }
}

bool csBulletSector::GetGroupCollision (const char* name1,
                                        const char* name2)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return false;
  if ((collGroups[index1].mask & (1 << index2)) != 0 
    || (collGroups[index2].mask & (1 << index1)) != 0)
    return true;
  else
    return false;
}

bool csBulletSector::CollisionTest (CS::Collisions::iCollisionObject* object, 
                                    csArray<CS::Collisions::CollisionData>& collisions)
{

  if (!object)
    return false;

  size_t length = collisions.GetSize ();
  PointContactResult result(sys, collisions);

  csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
  if (collObject->GetObjectType () == CS::Collisions::COLLISION_OBJECT_BASE
    || collObject->GetObjectType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
  {
    if (collObject->isTerrain)
    {
      //Here is a question. Should we let user to do collision test on terrain object?
      csBulletColliderTerrain* terrainShape = dynamic_cast<csBulletColliderTerrain*> (collObject->colliders[0]);
      for (size_t i = 0; i< terrainShape->colliders.GetSize (); i++)
      {
        btRigidBody* body = terrainShape->GetBulletObject (i);
        bulletWorld->contactTest (body, result);
      }
    }
    else
      bulletWorld->contactTest (collObject->btObject, result);
  }
  else
  {
    btPairCachingGhostObject* ghost = dynamic_cast<btPairCachingGhostObject*> (
      btGhostObject::upcast (collObject->btObject));

    bulletWorld->getDispatcher()->dispatchAllCollisionPairs(
      ghost->getOverlappingPairCache(), bulletWorld->getDispatchInfo(), bulletWorld->getDispatcher());

    for (int i = 0; i < ghost->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
    {
      btManifoldArray manifoldArray;
      btBroadphasePair* collisionPair = &ghost->getOverlappingPairCache()->getOverlappingPairArray()[i];

      if (collisionPair->m_algorithm)
        collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

      for (int j=0;j<manifoldArray.size();j++)
      {
        btPersistentManifold* manifold = manifoldArray[j];
        btCollisionObject* objA = static_cast<btCollisionObject*> (manifold->getBody0 ());
        btCollisionObject* objB = static_cast<btCollisionObject*> (manifold->getBody1 ());
        CS::Collisions::iCollisionObject* csCOA = static_cast<CS::Collisions::iCollisionObject*>(objA->getUserPointer ());
        CS::Collisions::iCollisionObject* csCOB = static_cast<CS::Collisions::iCollisionObject*>(objB->getUserPointer ());
        for (int p=0;p<manifold->getNumContacts();p++)
        {
          CS::Collisions::CollisionData data;
          data.objectA = csCOA;
          data.objectB = csCOB;

          const btManifoldPoint& pt = manifold->getContactPoint(p);
          data.penetration = pt.m_distance1 * sys->getInverseInternalScale ();
          data.positionWorldOnA = BulletToCS (pt.m_positionWorldOnA, sys->getInverseInternalScale ());
          data.positionWorldOnB = BulletToCS (pt.m_positionWorldOnB, sys->getInverseInternalScale ());
          data.normalWorldOnB = BulletToCS (pt.m_normalWorldOnB, sys->getInverseInternalScale ());
          collisions.Push (data);
        }
      }
    }
  }

  if (collObject->objectCopy)
  {
    csArray<CS::Collisions::CollisionData> copyData;
    collObject->objectCopy->sector->CollisionTest (collObject->objectCopy, copyData);
    for (size_t i = 0; i < copyData.GetSize (); i++)
    {
      CS::Collisions::CollisionData data;
      if (copyData[i].objectA == collObject->objectCopy->QueryCollisionObject ())
      {
        data.objectA = object;
        data.objectB = copyData[i].objectB;
        csVector3 vec = collObject->objectCopy->GetTransform ().Other2This (copyData[i].positionWorldOnA);
        data.positionWorldOnA = collObject->GetTransform ().This2Other (vec);
        // What's the position of the other object? Still in the other side of the portal?
        data.positionWorldOnB = copyData[i].positionWorldOnB;
        vec = collObject->objectCopy->GetTransform ().Other2ThisRelative (copyData[i].normalWorldOnB);
        data.normalWorldOnB = collObject->GetTransform ().This2OtherRelative (vec);
      }
      else
      {
        data.objectB = object;
        data.objectA = copyData[i].objectA;
        csVector3 vec = collObject->objectCopy->GetTransform ().Other2This (copyData[i].positionWorldOnB);
        data.positionWorldOnB = collObject->GetTransform ().This2Other (vec);
        // What's the position of the other object? Still in the other side of the portal?
        data.positionWorldOnA = copyData[i].positionWorldOnA;
        vec = collObject->objectCopy->GetTransform ().Other2ThisRelative (copyData[i].normalWorldOnB);
        data.normalWorldOnB = collObject->GetTransform ().This2OtherRelative (vec);
      }
      data.penetration = copyData[i].penetration;
      collisions.Push (data);
    }
  }

  if (length != collisions.GetSize ())
    return true;
  else
    return false;
}

void csBulletSector::AddCollisionActor (CS::Collisions::iCollisionActor* actor)
{
  csRef<csBulletCollisionActor> obj (dynamic_cast<csBulletCollisionActor*>(actor));
  collisionActor = obj;
  obj->sector = this;
  obj->collGroup = collGroups[CollisionGroupTypeActor]; // Actor Group.
  obj->AddBulletObject ();
}

void csBulletSector::RemoveCollisionActor ()
{
  collisionActor->RemoveBulletObject ();
  collisionActor->insideWorld = false;
  collisionActor = NULL;
}

CS::Collisions::iCollisionActor* csBulletSector::GetCollisionActor ()
{
  return collisionActor;
}

bool csBulletSector::BulletCollide (btCollisionObject* objectA,
                                    btCollisionObject* objectB,
                                    csArray<CS::Collisions::CollisionData>& data)
{
  //contactPairTest
  PointContactResult result(sys, data);
  bulletWorld->contactPairTest (objectA, objectB, result);
  if (data.IsEmpty ())
    return false;
  else
    return true;
}

CS::Collisions::HitBeamResult csBulletSector::RigidHitBeam (btCollisionObject* object, 
                                            const csVector3& start,
                                            const csVector3& end)
{
  btVector3 rayFrom = CSToBullet (start, sys->getInternalScale ());
  btVector3 rayTo = CSToBullet (end, sys->getInternalScale ());

  CS::Collisions::HitBeamResult result;

  if (object->getInternalType () == btCollisionObject::CO_SOFT_BODY)
  {
    btSoftBody* body = btSoftBody::upcast (object);
    btSoftBody::sRayCast ray;

    if (body->rayTest (rayFrom, rayTo, ray))
    {
      result.hasHit = true;
      result.object = static_cast<csBulletCollisionObject*> (object->getUserPointer ());

      btVector3 impact = rayFrom + (rayTo - rayFrom) * ray.fraction;
      switch (ray.feature)
      {
      case btSoftBody::eFeature::Face:
        {
          btSoftBody::Face& face = body->m_faces[ray.index];
          btSoftBody::Node* node = face.m_n[0];
          float distance = (node->m_x - impact).length2 ();

          for (int i = 1; i < 3; i++)
          {
            float nodeDistance = (face.m_n[i]->m_x - impact).length2 ();
            if (nodeDistance < distance)
            {
              node = face.m_n[i];
              distance = nodeDistance;
            }
          }

          result.isect = BulletToCS (node->m_x,
            sys->getInverseInternalScale ());
          result.normal = BulletToCS (node->m_n,
            sys->getInverseInternalScale ());	
          result.vertexIndex = size_t (node - &body->m_nodes[0]);
          break;
        }
      default:
        break;
      }
      return result;
    }
  }
  else
  {
    btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);

    //Ghost Object part?

    btTransform	rayFromTrans;
    btTransform	rayToTrans;

    rayFromTrans.setIdentity();
    rayFromTrans.setOrigin(rayFrom);
    rayToTrans.setIdentity();
    rayToTrans.setOrigin(rayTo);

    bulletWorld->rayTestSingle (rayFromTrans, rayToTrans, object,
      object->getCollisionShape(),
      object->getWorldTransform(),
      rayCallback);

    if(rayCallback.hasHit ())
    {
      result.hasHit = true;
      result.object = static_cast<csBulletCollisionObject*> (object->getUserPointer ());
      result.isect = BulletToCS (rayCallback.m_hitPointWorld,
        sys->getInverseInternalScale ());
      result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
        sys->getInverseInternalScale ());
      return result;
    }
  }
  return result;
}

void csBulletSector::SetSimulationSpeed (float speed)
{
  //TODO
}

void csBulletSector::SetStepParameters (float timeStep,
                                        size_t maxSteps,
                                        size_t interations)
{
  worldTimeStep = timeStep;
  worldMaxSteps = maxSteps;
  btContactSolverInfo& info = bulletWorld->getSolverInfo ();
  info.m_numIterations = (int)interations;
}

void csBulletSector::Step (float duration)
{
  // Update the soft body anchors
  for (csWeakRefArray<csBulletSoftBody>::Iterator it = anchoredSoftBodies.GetIterator (); it.HasNext (); )
  {
    csBulletSoftBody* body = static_cast<csBulletSoftBody*> (it.Next ());
    body->UpdateAnchorPositions ();
  }

  // Update the state of collision collide with portals.
  UpdatecsBulletCollisionPortals ();

  // Step the simulation
  bulletWorld->stepSimulation (duration, (int)worldMaxSteps, worldTimeStep);

  // Send the collision response of copies to source object.
  for (size_t i = 0; i < collisionObjects.GetSize (); i++)
    if (collisionObjects[i]->objectOrigin)
      GetInformationFromCopy (collisionObjects[i]->objectOrigin, collisionObjects[i]);

  for (size_t i = 0; i < rigidBodies.GetSize (); i++)
    if (rigidBodies[i]->objectOrigin)
      GetInformationFromCopy (rigidBodies[i]->objectOrigin, rigidBodies[i]);

  // Check for collisions
  CheckCollisions();
}

void csBulletSector::SetLinearDampener (float d)
{
  linearDampening = d;
}

void csBulletSector::SetRollingDampener (float d)
{
  angularDampening = d;
}

void csBulletSector::SetAutoDisableParams (float linear, float angular,
                                           float time)
{
  linearDisableThreshold = linear;
  angularDisableThreshold = angular;
  timeDisableThreshold = time;
}

void csBulletSector::AddRigidBody (CS::Physics::iRigidBody* body)
{
  csRef<csBulletRigidBody> bulletBody (dynamic_cast<csBulletRigidBody*>(body));
  rigidBodies.Push (bulletBody);

  bulletBody->sector = this;
  bulletBody->collGroup = collGroups[CollisionGroupTypeDefault]; // Default Group.
  bulletBody->SetLinearDampener(linearDampening);
  bulletBody->SetRollingDampener(angularDampening);
  bulletBody->AddBulletObject ();
  bulletBody->btBody->setSleepingThresholds (linearDisableThreshold, angularDisableThreshold);
  bulletBody->btBody->setDeactivationTime (timeDisableThreshold);

  AddMovableToSector (body);
}

void csBulletSector::RemoveRigidBody (CS::Physics::iRigidBody* body)
{
  csBulletRigidBody* btBody = dynamic_cast<csBulletRigidBody*> (body);
  CS_ASSERT (btBody);

  bool removed = btBody->RemoveBulletObject ();
  if (removed)
  {
    RemoveMovableFromSector (body);
    rigidBodies.Delete (btBody);
  }
}

CS::Physics::iRigidBody* csBulletSector::GetRigidBody (size_t index)
{
  return rigidBodies[index]->QueryRigidBody ();
}

CS::Physics::iRigidBody* csBulletSector::FindRigidBody (const char* name)
{
  return rigidBodies.FindByName (name);
}

void csBulletSector::AddSoftBody (CS::Physics::iSoftBody* body)
{
  csRef<csBulletSoftBody> btBody (dynamic_cast<csBulletSoftBody*>(body));
  softBodies.Push (btBody);
  btBody->sector = this;
  btBody->collGroup = collGroups[CollisionGroupTypeDefault];
  btBody->AddBulletObject ();

  iMovable* movable = body->GetAttachedMovable ();
  if (movable)
  {
    iMeshWrapper* mesh = movable->GetSceneNode ()->QueryMesh ();
    if (!mesh)
      return;
    csRef<iGeneralMeshState> meshState =
      scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject ());
    csRef<CS::Physics::iSoftBodyAnimationControl> animationControl =
      scfQueryInterface<CS::Physics::iSoftBodyAnimationControl> (meshState->GetAnimationControl ());
    if (!animationControl->GetSoftBody ())
      animationControl->SetSoftBody (body);

    sector->GetMeshes ()->Add (mesh);
  }
}

void csBulletSector::RemoveSoftBody (CS::Physics::iSoftBody* body)
{
  csBulletSoftBody* btBody = dynamic_cast<csBulletSoftBody*> (body);
  CS_ASSERT (btBody);

  bool removed = btBody->RemoveBulletObject ();
  if (removed)
  {
    RemoveMovableFromSector (body);
    softBodies.Delete (btBody);
  }
}

CS::Physics::iSoftBody* csBulletSector::GetSoftBody (size_t index)
{
  return softBodies[index]->QuerySoftBody ();
}

CS::Physics::iSoftBody* csBulletSector::FindSoftBody (const char* name)
{
  return softBodies.FindByName (name);
}

void csBulletSector::AddJoint (CS::Physics::iJoint* joint)
{
  csBulletJoint* btJoint = dynamic_cast<csBulletJoint*> (joint);
  CS_ASSERT(btJoint);
  btJoint->sector = this;
  btJoint->AddBulletJoint ();
  joints.Push (btJoint);
}

void csBulletSector::RemoveJoint (CS::Physics::iJoint* joint)
{
  csBulletJoint* btJoint = dynamic_cast<csBulletJoint*> (joint);
  CS_ASSERT(btJoint);

  btJoint->RemoveBulletJoint ();
  joints.Delete (btJoint);
}

void PreTickCallback (btDynamicsWorld* world, btScalar timeStep)
{
  csBulletSector* sector = (csBulletSector*) (world->getWorldUserInfo ());
  sector->UpdateSoftBodies (timeStep);
}

void csBulletSector::SetSoftBodyEnabled (bool enabled)
{
  CS_ASSERT(!rigidBodies.GetSize ()
    && !collisionObjects.GetSize ()
    && !softBodies.GetSize ());

  if (enabled == isSoftWorld)
    return;

  isSoftWorld = enabled;
  // re-create configuration, dispatcher & dynamics world
  btVector3 gra = bulletWorld->getGravity ();
  delete bulletWorld;
  delete dispatcher;
  delete configuration;

  if (isSoftWorld)
  {
    configuration = new btSoftBodyRigidBodyCollisionConfiguration ();
    dispatcher = new btCollisionDispatcher (configuration);
    bulletWorld = new btSoftRigidDynamicsWorld
      (dispatcher, broadphase, solver, configuration);

    softWorldInfo = new btSoftBodyWorldInfo ();
    softWorldInfo->m_broadphase = broadphase;
    softWorldInfo->m_dispatcher = dispatcher;
    softWorldInfo->m_gravity = gra;
    softWorldInfo->air_density = 1.2f;
    softWorldInfo->water_density = 0.0f;
    softWorldInfo->water_offset = 0.0f;
    softWorldInfo->water_normal = btVector3 (0.0f, 1.0f, 0.0f);
    softWorldInfo->m_sparsesdf.Initialize ();
  }
  else
  {
    configuration = new btDefaultCollisionConfiguration ();
    dispatcher = new btCollisionDispatcher (configuration);
    bulletWorld = new btDiscreteDynamicsWorld
      (dispatcher, broadphase, solver, configuration);

    if (softWorldInfo)
    {
        delete softWorldInfo;
        softWorldInfo = NULL;
    }
  }

  bulletWorld->setGravity (gra);
  bulletWorld->setDebugDrawer (debugDraw);

  // Register a pre-tick callback
  bulletWorld->setInternalTickCallback (PreTickCallback, this, true);
}

bool csBulletSector::SaveWorld (const char* filename)
{
  //What's this?
#ifndef CS_HAVE_BULLET_SERIALIZER
  return false;
#else

  // create a large enough buffer
  int maxSerializeBufferSize = 1024 * 1024 * 5;

  btDefaultSerializer* serializer = new btDefaultSerializer (maxSerializeBufferSize);
  bulletWorld->serialize (serializer);

  FILE* file = fopen (filename,"wb");
  if (!file) return false;

  if (fwrite (serializer->getBufferPointer (), serializer->getCurrentBufferSize (), 1, file)
    != 1)
    return false;

  if (fclose(file) == EOF) return false;

  return true;

#endif
}

void csBulletSector::DebugDraw (iView* rview)
{
  if (!debugDraw)
  {
    debugDraw = new csBulletDebugDraw (sys->getInverseInternalScale ());
    bulletWorld->setDebugDrawer (debugDraw);
  }

  bulletWorld->debugDrawWorld ();
  debugDraw->DebugDraw (rview);
}

void csBulletSector::SetDebugMode (CS::Physics::Bullet2::DebugMode mode)
{
  if (!debugDraw && mode)
  {
    debugDraw = new csBulletDebugDraw (sys->getInverseInternalScale ());
    bulletWorld->setDebugDrawer (debugDraw);
  }

  debugDraw->SetDebugMode (mode);
}

CS::Physics::Bullet2::DebugMode csBulletSector::GetDebugMode ()
{
  if (!debugDraw)
    return CS::Physics::Bullet2::DEBUG_NOTHING;

  return debugDraw->GetDebugMode ();
}

void csBulletSector::StartProfile ()
{
  CProfileManager::Start_Profile ("Crystal Space scene");

  if (!debugDraw)
  {
    debugDraw = new csBulletDebugDraw (sys->getInverseInternalScale ());
    bulletWorld->setDebugDrawer (debugDraw);
  }
  debugDraw->StartProfile ();
}

void csBulletSector::StopProfile ()
{
  CProfileManager::Stop_Profile ();
  debugDraw->StopProfile ();
}

void csBulletSector::DumpProfile (bool resetProfile /* = true */)
{
  printf ("\n");
  printf ("==========================================================\n");
  printf ("====           Bullet dynamic scene profile           ====\n");
  printf ("==========================================================\n");
  CProfileManager::dumpAll ();
  printf ("==========================================================\n");
  printf ("\n");

  if (resetProfile)
    CProfileManager::Reset ();
}

void csBulletSector::UpdateSoftBodies (float timeStep)
{
  for (csWeakRefArray<csBulletSoftBody>::Iterator it = anchoredSoftBodies.GetIterator (); it.HasNext (); )
  {
    csBulletSoftBody* body = static_cast<csBulletSoftBody*> (it.Next ());
    body->UpdateAnchorInternalTick (timeStep);
  }
}

void csBulletSector::AddMovableToSector (CS::Collisions::iCollisionObject* obj)
{
  iMovable* movable = obj->GetAttachedMovable ();
  if (movable && sector)
  {
    iMeshWrapper* mesh = movable->GetSceneNode ()->QueryMesh ();
    iLight* light = movable->GetSceneNode ()->QueryLight ();
    if (mesh)
      sector->GetMeshes ()->Add (mesh);
    else
      sector->GetLights ()->Add (light);
  }
}

void csBulletSector::RemoveMovableFromSector (CS::Collisions::iCollisionObject* obj)
{
  iMovable* movable = obj->GetAttachedMovable ();
  if (movable && sector)
  {
    iMeshWrapper* mesh = movable->GetSceneNode ()->QueryMesh ();
    iLight* light = movable->GetSceneNode ()->QueryLight ();
    if (mesh)
      sector->GetMeshes ()->Remove (mesh);
    else
      sector->GetLights ()->Remove (light);
  }
}

void csBulletSector::CheckCollisions ()
{
  int numManifolds = bulletWorld->getDispatcher()->getNumManifolds();

  for (size_t i = 0; i < collisionObjects.GetSize (); i++)
    collisionObjects[i]->contactObjects.Empty ();
  for (size_t i = 0; i < rigidBodies.GetSize (); i++)
    rigidBodies[i]->contactObjects.Empty ();
  for (size_t i = 0; i < softBodies.GetSize (); i++)
    softBodies[i]->contactObjects.Empty ();

  // Could not get contacted softBody?
  for (int i = 0; i < numManifolds; i++)
  {
    btPersistentManifold* contactManifold =
      bulletWorld->getDispatcher ()->getManifoldByIndexInternal (i);
    if (contactManifold->getNumContacts ())
    {
      btCollisionObject* obA =
        static_cast<btCollisionObject*> (contactManifold->getBody0 ());
      btCollisionObject* obB =
        static_cast<btCollisionObject*> (contactManifold->getBody1 ());

      csBulletCollisionObject* csCOA = dynamic_cast <csBulletCollisionObject*> (static_cast<CS::Collisions::iCollisionObject*>(obA->getUserPointer ()));
      csBulletCollisionObject* csCOB = dynamic_cast <csBulletCollisionObject*> (static_cast<CS::Collisions::iCollisionObject*>(obB->getUserPointer ()));

      if (csCOA && (csCOA->GetObjectType () == CS::Collisions::COLLISION_OBJECT_BASE
        || csCOA->GetObjectType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL))
        if (csCOA->contactObjects.Contains (csCOB) == csArrayItemNotFound)
          csCOA->contactObjects.Push (csCOB);

      if (csCOB && (csCOB->GetObjectType () == CS::Collisions::COLLISION_OBJECT_BASE
        || csCOB->GetObjectType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL))
        if (csCOB->contactObjects.Contains (csCOA) == csArrayItemNotFound)
          csCOB->contactObjects.Push (csCOA);
    }
  }
}

void csBulletSector::UpdatecsBulletCollisionPortals ()
{
  for (size_t i = 0; i < portals.GetSize (); i++)
  {
    portals[i]->UpdateCollisions (this);
  }
}

void csBulletSector::SetInformationToCopy (csBulletCollisionObject* obj, 
                                           csBulletCollisionObject* cpy, 
                                           const csOrthoTransform& warpTrans)
{
  if (!obj || !cpy )
    return;

  csOrthoTransform trans = obj->GetTransform () * warpTrans;

  btTransform btTrans = CSToBullet (trans, sys->getInternalScale ());

  if (obj->type == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
  {
    CS::Physics::iPhysicalBody* pb = obj->QueryPhysicalBody ();
    if (pb->GetBodyType () == CS::Physics::BODY_RIGID)
    {
      csBulletRigidBody* btCopy = dynamic_cast<csBulletRigidBody*> (cpy->QueryPhysicalBody ()->QueryRigidBody ());
      csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());

      if (rb->GetState () == CS::Physics::STATE_DYNAMIC)
      {
        btQuaternion rotate;
        CSToBullet (warpTrans.GetT2O()).getRotation (rotate);
        btCopy->btBody->setLinearVelocity (quatRotate (rotate, rb->btBody->getLinearVelocity ()));
        btCopy->btBody->setAngularVelocity (quatRotate (rotate, rb->btBody->getAngularVelocity ()));

      }
      btCopy->SetTransform (trans);
    }
    else
    {
      //TODO Soft Body
    }
  }
  else if (obj->type == CS::Collisions::COLLISION_OBJECT_GHOST
    || obj->type == CS::Collisions::COLLISION_OBJECT_ACTOR)
  {
    cpy->SetTransform (trans);
  }
}

void csBulletSector::GetInformationFromCopy (csBulletCollisionObject* obj, 
                                             csBulletCollisionObject* cpy)
{
  // TODO Warp the velocity.
  if (obj->type == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
  {
    CS::Physics::iPhysicalBody* pb = obj->QueryPhysicalBody ();
    if (pb->GetBodyType () == CS::Physics::BODY_RIGID)
    {
      csBulletRigidBody* btCopy = dynamic_cast<csBulletRigidBody*> (cpy->QueryPhysicalBody ()->QueryRigidBody ());
      csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());
      if (rb->GetState () == CS::Physics::STATE_DYNAMIC)
      {
        btQuaternion qua = cpy->portalWarp;

        rb->btBody->internalGetDeltaLinearVelocity () = quatRotate (qua, btCopy->btBody->internalGetDeltaLinearVelocity ());
        rb->btBody->internalGetDeltaAngularVelocity () = quatRotate (qua, btCopy->btBody->internalGetDeltaAngularVelocity ());
        rb->btBody->internalGetPushVelocity ()= quatRotate (qua, btCopy->btBody->internalGetPushVelocity ());
        rb->btBody->internalGetTurnVelocity ()= quatRotate (qua, btCopy->btBody->internalGetTurnVelocity ());
        rb->btBody->internalWritebackVelocity ();
      }
      else if (rb->GetState () == CS::Physics::STATE_KINEMATIC)
      {
        //Nothing to do?
      }
    }
    else
    {
      //TODO Soft Body
    }
  }
  else if (obj->type == CS::Collisions::COLLISION_OBJECT_GHOST
    || obj->type == CS::Collisions::COLLISION_OBJECT_ACTOR)
  {
    //btPairCachingGhostObject* ghostCopy = btPairCachingGhostObject::upcast (cpy);
    //btPairCachingGhostObject* ghostObject = btPairCachingGhostObject::upcast (obj->btObject);

    // Need to think about the implementation of actor.
  }
}

}
CS_PLUGIN_NAMESPACE_END(Bullet2)