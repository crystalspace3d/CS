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

#include "cssysdef.h"
//#include "ivaria/reporter.h"
#include "imesh/softanim.h"
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

#include "csutil/custom_new_enable.h"

#include "bulletsystem.h"
#include "common2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "collisionactor.h"
#include "joint2.h"
#include "portal.h"

const float COLLISION_THRESHOLD = 0.01f;


using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  //----------------------------------- PointContactResult -----------------------------------

  struct PointContactResult : public btCollisionWorld::ContactResultCallback
  {
    csArray<CS::Collisions::CollisionData>& collisions;
    csBulletSystem* system;

    PointContactResult (csBulletSystem* system, csArray<CS::Collisions::CollisionData>& collisions)
      : collisions (collisions), system (system) 
    {
    }

    virtual btScalar addSingleResult (btManifoldPoint& cp, const btCollisionObject* colObj0,
				      int partId0, int index0, const btCollisionObject* colObj1,
				      int partId1, int index1)
    {
      CS::Collisions::CollisionData data;
      data.objectA = static_cast<CS::Collisions::iCollisionObject*>(colObj0->getUserPointer ());
      data.objectB = static_cast<CS::Collisions::iCollisionObject*>(colObj1->getUserPointer ());
      data.penetration = cp.m_distance1 * system->GetInverseInternalScale ();
      data.positionWorldOnA = BulletToCS (cp.m_positionWorldOnA, system->GetInverseInternalScale ());
      data.positionWorldOnB = BulletToCS (cp.m_positionWorldOnB, system->GetInverseInternalScale ());
      data.normalWorldOnB = BulletToCS (cp.m_normalWorldOnB, system->GetInverseInternalScale ());
      collisions.Push (data);
      return 0;
    }
  };

  //----------------------------------- csBulletSector -----------------------------------

  void PreTickCallback (btDynamicsWorld* world, btScalar timeStep)
  {
    csBulletSector* sector = (csBulletSector*) (world->getWorldUserInfo ());
    sector->UpdateSoftBodies (timeStep);
  }

  csBulletSector::csBulletSector (csBulletSystem* system)
    : scfImplementationType (this), system (system), hitPortal (nullptr), softWorldInfo (nullptr),
    linearDampening (system->linearDampening), angularDampening (system->angularDampening),
    linearDisableThreshold (system->linearDisableThreshold),
    angularDisableThreshold (system->angularDisableThreshold),
    timeDisableThreshold (system->timeDisableThreshold)
  {
    solver = new btSequentialImpulseConstraintSolver ();

    // TODO: those dimensions will not fit every worlds
    const int maxProxies = 32766;
    btVector3 worldAabbMin (-WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS);
    btVector3 worldAabbMax (WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS);
    broadphase = new btAxisSweep3 (worldAabbMin, worldAabbMax, maxProxies);
    broadphase->getOverlappingPairCache ()->setInternalGhostPairCallback (new btGhostPairCallback ());

    csVector3 v (0.0f, -9.81f, 0.0f);
    btVector3 gravity = CSToBullet (v, system->GetInternalScale ());

    if (system->isSoftWorld)
    {
      configuration = new btSoftBodyRigidBodyCollisionConfiguration ();
      dispatcher = new btCollisionDispatcher (configuration);
      bulletWorld = new btSoftRigidDynamicsWorld
        (dispatcher, broadphase, solver, configuration);

      softWorldInfo = new btSoftBodyWorldInfo ();
      softWorldInfo->m_broadphase = broadphase;
      softWorldInfo->m_dispatcher = dispatcher;
      softWorldInfo->m_gravity = gravity;
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
    }

    btContactSolverInfo& info = bulletWorld->getSolverInfo ();
    info.m_numIterations = system->stepIterations;

    bulletWorld->setGravity (gravity);

    if (system->debugDraw)
      bulletWorld->setDebugDrawer (system->debugDraw);

    // Register a pre-tick callback
    // TODO: remove?
    if (system->isSoftWorld)
      bulletWorld->setInternalTickCallback (PreTickCallback, this, true);
  }

  csBulletSector::~csBulletSector ()
  {
    DeleteAll ();

    delete bulletWorld;
    delete dispatcher;
    delete configuration;
    delete solver;
    delete softWorldInfo;
    delete broadphase;
  }

  void csBulletSector::DeleteAll ()
  {
    // remove updatables
    while (updatables.GetSize ())
    {
      RemoveUpdatable (updatables[updatables.GetSize () - 1]);
    }

    // remove portals
    for (size_t i = 0; i < portals.GetSize (); ++i)
    {
      bulletWorld->removeCollisionObject (portals[i]->ghostPortal);
    }

    // remove collision objects
    for (size_t i = 0; i < collisionObjects.GetSize (); ++i)
    {
      collisionObjects[i]->RemoveBulletObject ();
    }

    // remove constraints
    for (size_t i = 0; i < joints.GetSize (); ++i)
    {
      joints[i]->RemoveBulletJoint ();
    }

    // remove terrains
    for (size_t i = 0; i < terrains.GetSize (); ++i)
    {
      terrains[i]->RemoveRigidBodies ();
    }
    terrains.DeleteAll ();

    joints.DeleteAll ();
    softBodies.DeleteAll ();
    rigidBodies.DeleteAll ();
    collisionObjects.DeleteAll ();
    portals.DeleteAll ();
  }

  CS::Collisions::iCollisionSystem* csBulletSector::GetSystem () { return system; }

  void csBulletSector::SetGravity (const csVector3& v)
  {
    // first re-activate all objects
    for (size_t i = 0; i < collisionObjects.GetSize (); ++i)
    {
      if (collisionObjects[i]->GetObjectType () == COLLISION_OBJECT_PHYSICAL
	  && collisionObjects[i]->QueryPhysicalBody ()->IsDynamic ())
      {
        collisionObjects[i]->btObject->activate (true);
      }
    }

    btVector3 gravity = CSToBullet (v, system->GetInternalScale ());
    bulletWorld->setGravity (gravity);

    if (softWorldInfo)
      softWorldInfo->m_gravity = gravity;
  }

  csVector3 csBulletSector::GetGravity () const
  {
    btVector3 gravity = bulletWorld->getGravity ();
    return BulletToCS (gravity, system->GetInverseInternalScale ());
  }

  void csBulletSector::AddCollisionObject (CS::Collisions::iCollisionObject* object)
  {
    if (object->GetSector ())
    {
      object->GetSector ()->RemoveCollisionObject (object);
    }

    csBulletCollisionObject* obj (dynamic_cast<csBulletCollisionObject*>(object));

#ifdef _DEBUG
    printf ("Adding object \"%s\" (0x%lx) to sector: 0x%lx\n", object->QueryObject ()->GetName (), obj->btObject, this);
#endif

    switch (obj->GetObjectType ())
    {
    case CS::Collisions::COLLISION_OBJECT_ACTOR:
      {
        AddCollisionActor (static_cast<csBulletCollisionActor*>(obj));
      }
      break;
    case CS::Collisions::COLLISION_OBJECT_PHYSICAL:
      {
        iPhysicalBody* phyBody = obj->QueryPhysicalBody ();
        //int cflags = obj->btObject->getCollisionFlags ();
        //obj->btObject->setCollisionFlags (obj->btObject->getCollisionFlags () | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
        if (phyBody->QueryRigidBody ())
        {
          AddRigidBody (phyBody->QueryRigidBody ());
        }
        else
        {
	  CS_ASSERT (system->isSoftWorld);
          AddSoftBody (phyBody->QuerySoftBody ());
        }
      }
      break;
    default:
      {
        // Ghost objects
        obj->sector = this;
        //obj->btObject->setCollisionFlags (obj->btObject->getCollisionFlags () | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);

        obj->AddBulletObject ();
      }
      break;
    }

    AddSceneNodeToSector (object->GetAttachedSceneNode ());

    // add all objects to the collisionObjects list
    collisionObjects.Push (obj);
  }

  void csBulletSector::RemoveCollisionObject (CS::Collisions::iCollisionObject* object)
  {
    csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
    if (!collObject)
      return;

    bool removed = collObject->RemoveBulletObject ();
    if (removed)
    {
      collisionObjects.Delete (collObject);
      RemoveSceneNodeFromSector (object->GetAttachedSceneNode ());

      iPhysicalBody* phyBody = dynamic_cast<iPhysicalBody*> (object);
      if (phyBody->QueryRigidBody ())
      {
	rigidBodies.Delete (dynamic_cast<csBulletRigidBody*>(phyBody->QueryRigidBody ()));
      }
      else
      {
	csBulletSoftBody* btBody = dynamic_cast<csBulletSoftBody*>(phyBody->QuerySoftBody ());
	softBodies.Delete (btBody);
	RemoveUpdatable (btBody);
      }
    }
  }

  CS::Collisions::iCollisionObject* csBulletSector::GetCollisionObject (size_t index)
  {
    if (index >= 0 && index < collisionObjects.GetSize ())
    {
      return collisionObjects[index]->QueryCollisionObject ();
    }
    else
    {
      return nullptr;
    }
  }

  void csBulletSector::AddCollisionTerrain (CS::Collisions::iCollisionTerrain* terrain)
  {
    csBulletCollisionTerrain* btTerrain = dynamic_cast<csBulletCollisionTerrain*>(terrain);

    btTerrain->RemoveRigidBodies ();
    btTerrain->AddRigidBodies (this);

    terrains.Push (btTerrain);
  }
  
  CS::Collisions::iCollisionTerrain* csBulletSector::GetCollisionTerrain (size_t index) const 
  { 
    return csRef<CS::Collisions::iCollisionTerrain>(scfQueryInterface<CS::Collisions::iCollisionTerrain>(terrains.Get (index)));
  }

  CS::Collisions::iCollisionTerrain* csBulletSector::GetCollisionTerrain (iTerrainSystem* terrain) 
  {
    for (size_t i = 0; i < terrains.GetSize (); ++i)
    {
      if (terrains.Get (i)->GetTerrain () == terrain)
      {
        return terrains.Get (i);
      }
    }
    return nullptr;
  }

  CS::Collisions::iCollisionObject* csBulletSector::FindCollisionObject (const char* name)
  {
    return collisionObjects.FindByName (name);
  }

  void csBulletSector::AddPortal (iPortal* portal, const csOrthoTransform& meshTrans)
  {
    CollisionPortal* newPortal = new CollisionPortal (portal, meshTrans, this);
    portals.Push (newPortal);
  }

  void csBulletSector::RemovePortal (iPortal* portal)
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      if (portals[i]->portal == portal)
      {
        portals[i]->RemoveFromSector ();
        portals.DeleteIndexFast (i);
        return;
      }
    }
  }

  void csBulletSector::SetSector (iSector* sector)
  {
    if (this->sector == sector) return;
    
    this->sector = sector;

    if (sector)
    {
      // sector is set

      // set name
      // TODO: remove?
      QueryObject ()->SetName (sector->QueryObject ()->GetName ());

      // add portal meshes
      /*const csSet<csPtrKey<iMeshWrapper> >& portal_meshes = 
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
      }*/

      // add object meshes
      for (size_t i = 0; i < collisionObjects.GetSize (); i++)
      {
        iCollisionObject* obj = collisionObjects[i];
        AddSceneNodeToSector (obj->GetAttachedSceneNode ());
      }
    }
    else
    {
      // sector is unset
      // TODO: Remove meshes
    }
  }

  CS::Collisions::HitBeamResult csBulletSector::HitBeam (const csVector3& start, const csVector3& end)
  {
    btVector3 rayFrom = CSToBullet (start, system->GetInternalScale ());
    btVector3 rayTo = CSToBullet (end, system->GetInternalScale ());

    btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);
    rayCallback.m_collisionFilterMask = system->collGroups[CollisionGroupTypePortalCopy].mask;
    rayCallback.m_collisionFilterGroup = system->collGroups[CollisionGroupTypeDefault].value;
    bulletWorld->rayTest (rayFrom, rayTo, rayCallback);

    CS::Collisions::HitBeamResult result;

    if (rayCallback.hasHit ())
    {
      CS::Collisions::iCollisionObject* collObject = static_cast<CS::Collisions::iCollisionObject*> (
        rayCallback.m_collisionObject->getUserPointer ());

      result.hasHit = true;

      if (rayCallback.m_collisionObject->getInternalType () == btCollisionObject::CO_GHOST_OBJECT
        && rayCallback.m_collisionObject->getUserPointer () == nullptr)
      {
        // hit a ghost object (potentially a portal...)
        collObject = nullptr;
        result.hasHit = false;
        hitPortal = btGhostObject::upcast (rayCallback.m_collisionObject);
        if (hitPortal)
        {
          // this might be of interest to the caller
          result.object = collObject;
          result.isect = BulletToCS (rayCallback.m_hitPointWorld,
            system->GetInverseInternalScale ());
          result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
            system->GetInverseInternalScale ());
        }
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
            system->GetInverseInternalScale ());
          result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
            system->GetInverseInternalScale ());	

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
          system->GetInverseInternalScale ());
        result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
          system->GetInverseInternalScale ());
        return result;
      } // not softBody
    } //has hit
    return result;
  }

  CS::Collisions::HitBeamResult csBulletSector::HitBeamPortal (const csVector3& start, const csVector3& end)
  {

    hitPortal = nullptr;

    CS::Collisions::HitBeamResult result = HitBeam (start, end);

    if (result.object == nullptr && hitPortal)
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

  bool csBulletSector::CollisionTest (CS::Collisions::iCollisionObject* object, 
    csArray<CS::Collisions::CollisionData>& collisions)
  {

    if (!object || object->IsPassive ())
      return false;

    size_t length = collisions.GetSize ();
    PointContactResult result (system, collisions);

    csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
    if (collObject->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
    {
      bulletWorld->contactTest (collObject->btObject, result);
    }
    else
    {
      btPairCachingGhostObject* ghost = static_cast<btPairCachingGhostObject*> (btGhostObject::upcast (collObject->btObject));

      bulletWorld->getDispatcher ()->dispatchAllCollisionPairs (ghost->getOverlappingPairCache (), bulletWorld->getDispatchInfo (), bulletWorld->getDispatcher ());

      for (int i = 0; i < ghost->getOverlappingPairCache ()->getNumOverlappingPairs (); i++)
      {
        btManifoldArray manifoldArray;
        btBroadphasePair* collisionPair = &ghost->getOverlappingPairCache ()->getOverlappingPairArray ()[i];

        if (collisionPair->m_algorithm)
          collisionPair->m_algorithm->getAllContactManifolds (manifoldArray);

        for (int j=0;j<manifoldArray.size ();j++)
        {
          btPersistentManifold* manifold = manifoldArray[j];
          btCollisionObject* objA = static_cast<btCollisionObject*> (manifold->getBody0 ());
          btCollisionObject* objB = static_cast<btCollisionObject*> (manifold->getBody1 ());
          CS::Collisions::iCollisionObject* csCOA = static_cast<CS::Collisions::iCollisionObject*>(objA->getUserPointer ());
          CS::Collisions::iCollisionObject* csCOB = static_cast<CS::Collisions::iCollisionObject*>(objB->getUserPointer ());
          for (int p=0;p<manifold->getNumContacts ();p++)
          {
            CS::Collisions::CollisionData data;
            data.objectA = csCOA;
            data.objectB = csCOB;

            const btManifoldPoint& pt = manifold->getContactPoint (p);
            data.penetration = pt.m_distance1 * system->GetInverseInternalScale ();
            data.positionWorldOnA = BulletToCS (pt.m_positionWorldOnA, system->GetInverseInternalScale ());
            data.positionWorldOnB = BulletToCS (pt.m_positionWorldOnB, system->GetInverseInternalScale ());
            data.normalWorldOnB = BulletToCS (pt.m_normalWorldOnB, system->GetInverseInternalScale ());
            collisions.Push (data);
          }
        }
      }
    }

    if (collObject->GetPortalData () && collObject->GetPortalData ()->OtherObject)
    {
      // Object is traversing a portal and thus has a clone that is in symbiosis with itself
      csBulletCollisionObject* portalClone = collObject->GetPortalData ()->OtherObject;
      csArray<CS::Collisions::CollisionData> copyData;
      portalClone->sector->CollisionTest (portalClone, copyData);
      for (size_t i = 0; i < copyData.GetSize (); i++)
      {
        CS::Collisions::CollisionData data;
        if (copyData[i].objectA == portalClone->QueryCollisionObject ())
        {
          data.objectA = object;
          data.objectB = copyData[i].objectB;
          csVector3 vec = portalClone->GetTransform ().Other2This (copyData[i].positionWorldOnA);
          data.positionWorldOnA = collObject->GetTransform ().This2Other (vec);
          // What's the position of the other object? Still in the other side of the portal?
          data.positionWorldOnB = copyData[i].positionWorldOnB;
          vec = portalClone->GetTransform ().Other2ThisRelative (copyData[i].normalWorldOnB);
          data.normalWorldOnB = collObject->GetTransform ().This2OtherRelative (vec);
        }
        else
        {
          data.objectB = object;
          data.objectA = copyData[i].objectA;
          csVector3 vec = portalClone->GetTransform ().Other2This (copyData[i].positionWorldOnB);
          data.positionWorldOnB = collObject->GetTransform ().This2Other (vec);
          // What's the position of the other object? Still in the other side of the portal?
          data.positionWorldOnA = copyData[i].positionWorldOnA;
          vec = portalClone->GetTransform ().Other2ThisRelative (copyData[i].normalWorldOnB);
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
    collisionObjects.Push (obj);
    obj->sector = this;
    obj->collGroup = system->collGroups[CollisionGroupTypeActor]; // Actor Group.
    obj->AddBulletObject ();
  }

  bool csBulletSector::BulletCollide (btCollisionObject* objectA,
    btCollisionObject* objectB,
    csArray<CS::Collisions::CollisionData>& data)
  {
    //contactPairTest
    PointContactResult result (system, data);
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
    btVector3 rayFrom = CSToBullet (start, system->GetInternalScale ());
    btVector3 rayTo = CSToBullet (end, system->GetInternalScale ());

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
              system->GetInverseInternalScale ());
            result.normal = BulletToCS (node->m_n,
              system->GetInverseInternalScale ());	
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

      rayFromTrans.setIdentity ();
      rayFromTrans.setOrigin (rayFrom);
      rayToTrans.setIdentity ();
      rayToTrans.setOrigin (rayTo);

      bulletWorld->rayTestSingle (rayFromTrans, rayToTrans, object,
        object->getCollisionShape (),
        object->getWorldTransform (),
        rayCallback);

      if (rayCallback.hasHit ())
      {
        result.hasHit = true;
        result.object = static_cast<csBulletCollisionObject*> (object->getUserPointer ());
        result.isect = BulletToCS (rayCallback.m_hitPointWorld,
          system->GetInverseInternalScale ());
        result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
          system->GetInverseInternalScale ());
        return result;
      }
    }
    return result;
  }

  void csBulletSector::Step (float duration)
  {
    // Call updatable steps
    for (size_t i = 0; i < updatables.GetSize (); i++)
    {
      updatables[i]->PreStep (duration);
    }

    // Update traversing objects before simulation
    UpdateCollisionPortalsPreStep ();

    // Step the simulation
    bulletWorld->stepSimulation (duration, system->worldMaxSteps, system->worldTimeStep);
    
    // Update traversing objects after simulation
    UpdateCollisionPortalsPostStep ();

    // Check for collisions
    //CheckCollisions ();

    for (size_t i = 0; i < updatables.GetSize (); i++)
    {
      updatables[i]->PostStep (duration);
    }
  }

  void csBulletSector::SetLinearDamping (float d)
  {
    linearDampening = d;
  }

  void csBulletSector::SetAngularDamping (float d)
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

    // TODO: Body might have set its own damping
    bulletBody->SetLinearDamping (linearDampening);
    bulletBody->SetAngularDamping (angularDampening);
    bulletBody->btBody->setSleepingThresholds (linearDisableThreshold, angularDisableThreshold);
    bulletBody->btBody->setDeactivationTime (timeDisableThreshold);
    bulletBody->AddBulletObject ();
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
    CS_ASSERT (system->isSoftWorld);

    csRef<csBulletSoftBody> btBody (dynamic_cast<csBulletSoftBody*>(body));
    softBodies.Push (btBody);
    AddUpdatable (btBody);

    btBody->sector = this;
    btBody->AddBulletObject ();

    iSceneNode* sceneNode = body->GetAttachedSceneNode ();
    if (sceneNode)
    {
      iMeshWrapper* mesh = sceneNode->QueryMesh ();
      if (!mesh)
        return;

      csRef<iGeneralMeshState> meshState =
        scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject ());

      // TODO: manage that from the animation controller
      csRef<CS::Animation::iSoftBodyAnimationControl> animationControl =
        scfQueryInterface<CS::Animation::iSoftBodyAnimationControl> (meshState->GetAnimationControl ());

      if (!animationControl->GetSoftBody ())
        animationControl->SetSoftBody (body);
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
    CS_ASSERT (btJoint);
    btJoint->sector = this;
    btJoint->AddBulletJoint ();
    joints.Push (btJoint);
  }

  void csBulletSector::RemoveJoint (CS::Physics::iJoint* joint)
  {
    csBulletJoint* btJoint = dynamic_cast<csBulletJoint*> (joint);
    CS_ASSERT (btJoint);

    // re-activate the now free bodies
    if (btJoint->bodies[0])
    {
      dynamic_cast<csBulletCollisionObject*>(btJoint->bodies[0])->btObject->activate (true);
    }
    if (btJoint->bodies[1])
    {
      dynamic_cast<csBulletCollisionObject*>(btJoint->bodies[1])->btObject->activate (true);
    }

    btJoint->RemoveBulletJoint ();
    joints.Delete (btJoint);
  }

  bool csBulletSector::SaveWorld (const char* filename)
  {
    // Check that the version of the Bullet library can handle serialization.
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

    if (fclose (file) == EOF) return false;

    return true;

#endif
  }

  void csBulletSector::UpdateSoftBodies (float timeStep)
  {
    for (csWeakRefArray<csBulletSoftBody>::Iterator it = anchoredSoftBodies.GetIterator (); it.HasNext (); )
    {
      csBulletSoftBody* body = static_cast<csBulletSoftBody*> (it.Next ());
      body->UpdateAnchorInternalTick (timeStep);
    }
  }

  void csBulletSector::AddSceneNodeToSector (iSceneNode* sceneNode)
  {
    // TODO: use iMovable::SetSector () instead (same everywhere)
    if (sceneNode && sector)
    {
      iMeshWrapper* mesh = sceneNode->QueryMesh ();
      iLight* light = sceneNode->QueryLight ();

      if (mesh && size_t (sector->GetMeshes ()->Find (mesh)) == csArrayItemNotFound)
      {
        sector->GetMeshes ()->Add (mesh);
      }

      if (light && size_t (sector->GetLights ()->Find (light)) == csArrayItemNotFound)
      {
        sector->GetLights ()->Add (light);
      }
    }
  }

  void csBulletSector::RemoveSceneNodeFromSector (iSceneNode* sceneNode)
  {
    if (sceneNode && sector)
    {
      iMeshWrapper* mesh = sceneNode->QueryMesh ();
      iLight* light = sceneNode->QueryLight ();
      if (mesh)
        sector->GetMeshes ()->Remove (mesh);
      if (light)
        sector->GetLights ()->Remove (light);
    }
  }

  void csBulletSector::CheckCollisions ()
  {
    int numManifolds = bulletWorld->getDispatcher ()->getNumManifolds ();

    for (size_t i = 0; i < collisionObjects.GetSize (); i++)
      collisionObjects[i]->contactObjects.Empty ();

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

        csBulletCollisionObject* csCOA = dynamic_cast <csBulletCollisionObject*>
	  (static_cast<CS::Collisions::iCollisionObject*>(obA->getUserPointer ()));
        csBulletCollisionObject* csCOB = dynamic_cast <csBulletCollisionObject*>
	  (static_cast<CS::Collisions::iCollisionObject*>(obB->getUserPointer ()));

        if (csCOA && csCOA->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
          if (csCOA->contactObjects.Contains (csCOB) == csArrayItemNotFound)
            csCOA->contactObjects.Push (csCOB);

        if (csCOB && csCOB->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
          if (csCOB->contactObjects.Contains (csCOA) == csArrayItemNotFound)
            csCOB->contactObjects.Push (csCOA);
      }
    }
  }

  void csBulletSector::UpdateCollisionPortalsPreStep ()
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      portals[i]->UpdateCollisionsPreStep (this);
    }
  }

  void csBulletSector::UpdateCollisionPortalsPostStep ()
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      portals[i]->UpdateCollisionsPostStep (this);
    }
  }

  /**
   * Will cause the step function to be called on this updatable every step
   */
  void csBulletSector::AddUpdatable (iUpdatable* u)
  {
    updatables.Push (u);

    if (u->GetCollisionObject ())
    {
      AddCollisionObject (u->GetCollisionObject ());
    }

    csRef<BulletActionWrapper> wrapper = scfQueryInterface<BulletActionWrapper>(u);
    if (wrapper && wrapper->GetBulletAction ())
    {
      // It was an internal action that also defines a bullet action
      bulletWorld->addAction (wrapper->GetBulletAction ());
    }

    u->OnAdded (this);
  }
  
  /**
   * Removes the given updatable
   */
  void csBulletSector::RemoveUpdatable (iUpdatable* u)
  {
    if (u->GetCollisionObject ())
    {
      RemoveCollisionObject (u->GetCollisionObject ());
    }

    csRef<BulletActionWrapper> wrapper = scfQueryInterface<BulletActionWrapper>(u);
    if (wrapper && wrapper->GetBulletAction ())
    {
      bulletWorld->removeAction (wrapper->GetBulletAction ());
    }
    
    u->OnRemoved (this);

    updatables.Delete (u);   // delete last because that might be the last hard reference
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
