#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "cssysdef.h"
#include "iengine/movable.h"
#include "collisionobject2.h"
#include "colliderprimitives.h"
#include "colliderterrain.h"


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  csBulletCollisionObject::csBulletCollisionObject (csBulletSystem* sys)
    : scfImplementationType (this), movable (nullptr), camera (nullptr), collCb (nullptr),
    portalWarp (btQuaternion::getIdentity ()), sector (nullptr), system (sys),
    btObject (nullptr), compoundShape (nullptr), objectOrigin (nullptr), objectCopy (nullptr),
    haveStaticColliders(0), insideWorld (false), shapeChanged (false), isTerrain (false)
  {
    btTransform identity;
    identity.setIdentity ();
    motionState = new csBulletMotionState (this, identity, identity);
  }

  csBulletCollisionObject::~csBulletCollisionObject ()
  {
    //RemoveBulletObject ();  <- this won't be necessary, since it should not be deleted until the object has been removed from the world, anyway
    colliders.DeleteAll ();
    if (btObject)
      delete btObject;
    if (compoundShape)
      delete compoundShape;
    if (motionState)
      delete motionState;
  }

  void csBulletCollisionObject::SetTransform (const csOrthoTransform& trans)
  {
    btTransform transform = CSToBullet (trans, system->getInternalScale ());

    if (IsPhysicalObject())
    {
      // TODO: Clean this up
      if (!isTerrain)
      {
        if (insideWorld)
          sector->bulletWorld->removeRigidBody (btRigidBody::upcast (btObject));

        btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();

        delete motionState;
        motionState = new csBulletMotionState (this, transform * principalAxis, principalAxis);

        if (btObject)
        {
          btRigidBody::upcast (btObject)->setMotionState (motionState);
          btRigidBody::upcast (btObject)->setCenterOfMassTransform (motionState->m_graphicsWorldTrans);
        }

        if (insideWorld)
        {
          sector->bulletWorld->addRigidBody (btRigidBody::upcast (btObject), collGroup.value, collGroup.mask);
        }
      }
      else
      {
        //Do not support change transform of terrain?
        //Currently in CS it's not supported.
      }
    }
    if (movable)
      movable->SetFullTransform (BulletToCS (transform * invPricipalAxis, system->getInverseInternalScale ()));
    if (camera)
      camera->SetTransform (BulletToCS (transform * invPricipalAxis, system->getInverseInternalScale ()));
    if (btObject)
      btObject->setWorldTransform(transform);
  }

  csOrthoTransform csBulletCollisionObject::GetTransform () const
  {
    float inverseScale = system->getInverseInternalScale ();

    if (isTerrain)
    {
      csBulletColliderTerrain* terrainCollider = dynamic_cast<csBulletColliderTerrain*> (colliders[0]);
      return terrainCollider->terrainTransform;   
    }
    else 
    {
      CS_ASSERT(btObject);
      return BulletToCS (btObject->getWorldTransform(), system->getInverseInternalScale ());
    }
  }

  void csBulletCollisionObject::AddCollider (CS::Collisions::iCollider* collider,
    const csOrthoTransform& relaTrans)
  {
    csRef<csBulletCollider> coll (dynamic_cast<csBulletCollider*>(collider));

    CS::Collisions::ColliderType type = collider->GetType ();
    if (type == CS::Collisions::COLLIDER_CONCAVE_MESH
      ||type == CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED
      ||type == CS::Collisions::COLLIDER_PLANE)
      haveStaticColliders ++;

    if(type == CS::Collisions::COLLIDER_TERRAIN)
    {
      colliders.Empty ();
      relaTransforms.Empty ();
      colliders.Push (coll);
      relaTransforms.Push (relaTrans);
      isTerrain = true;
    }
    else if (!isTerrain)
    {
      // If a collision object has a terrain collider. Then it is not allowed to add other colliders.
      colliders.Push (coll);
      relaTransforms.Push (relaTrans);
    }
    shapeChanged = true;
    //User must call RebuildObject() after this.
  }

  void csBulletCollisionObject::RemoveCollider (CS::Collisions::iCollider* collider)
  {
    for (size_t i =0; i < colliders.GetSize(); i++)
    {
      if (colliders[i] == collider)
      {
        RemoveCollider (i);
        return;
      }
    }
    //User must call RebuildObject() after this.
  }

  void csBulletCollisionObject::RemoveCollider (size_t index)
  {
    if (index < colliders.GetSize ())
    {  
      if (isTerrain && index == 0)
        isTerrain = false;

      CS::Collisions::ColliderType type = colliders[index]->GetType ();
      if (type == CS::Collisions::COLLIDER_CONCAVE_MESH
        ||type == CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED
        ||type == CS::Collisions::COLLIDER_PLANE)
        haveStaticColliders --;

      colliders.DeleteIndex (index);
      relaTransforms.DeleteIndex (index);
    }
    //User must call RebuildObject() after this.
  }

  CS::Collisions::iCollider* csBulletCollisionObject::GetCollider (size_t index)
  {
    if (index < colliders.GetSize ())
      return colliders[index];
    return nullptr;
  }

  void csBulletCollisionObject::SetCollisionGroup (const char* name)
  {
    if (!sector)
      return;

    CS::Collisions::CollisionGroup& group = sector->FindCollisionGroup (name);
    this->collGroup = group;

    if (btObject && insideWorld)
    {
      btObject->getBroadphaseHandle ()->m_collisionFilterGroup = collGroup.value;
      btObject->getBroadphaseHandle ()->m_collisionFilterMask = collGroup.mask;
    }
  }

  bool csBulletCollisionObject::Collide (CS::Collisions::iCollisionObject* otherObject)
  {
    csArray<CS::Collisions::CollisionData> data;
    csBulletCollisionObject* otherObj = dynamic_cast<csBulletCollisionObject*> (otherObject);
    if (isTerrain)
    {
      //Terrain VS Terrain???
      if (otherObj->isTerrain == true)
        return false;

      //Terrain VS object/Body.
      btCollisionObject* otherBtObject = otherObj->GetBulletCollisionPointer ();
      csBulletColliderTerrain* terrainColl = dynamic_cast<csBulletColliderTerrain*> (colliders[0]);
      for (size_t i = 0; i < terrainColl->bodies.GetSize (); i++)
      {
        btRigidBody* body = terrainColl->GetBulletObject (i);
        if (sector->BulletCollide (body, otherBtObject, data))
        {
          if (otherObj->collCb)
            otherObj->collCb->OnCollision (otherObj, this, data);
          return true;
        }
      }
      return false;
    }
    else
    {
      //Object/Body CS terrain.
      if (otherObj->isTerrain == true)
        return otherObject->Collide (this);

      //Object/Body VS object.
      btCollisionObject* otherBtObject = dynamic_cast<csBulletCollisionObject*> (otherObject)->GetBulletCollisionPointer ();
      bool result = sector->BulletCollide (btObject, otherBtObject, data);
      if (result)
        if (collCb)
          collCb->OnCollision (this, otherObj, data);

      return result;
    }
    return false;
  }

  CS::Collisions::HitBeamResult csBulletCollisionObject::HitBeam (const csVector3& start,
    const csVector3& end)
  {
    //Terrain part
    if (isTerrain)
    {
      csBulletColliderTerrain* terrainColl = dynamic_cast<csBulletColliderTerrain*> (colliders[0]);
      for (size_t i = 0; i < terrainColl->bodies.GetSize (); i++)
      {
        btRigidBody* body = terrainColl->GetBulletObject (i);
        CS::Collisions::HitBeamResult result = sector->RigidHitBeam (body, start, end);
        if (result.hasHit)
          return result;
      }
      return CS::Collisions::HitBeamResult();
    }
    //Others part
    else
      return sector->RigidHitBeam (btObject, start, end);
  }

  size_t csBulletCollisionObject::GetContactObjectsCount ()
  {
    size_t result = 0;
    if (IsPhysicalObject())
    {
      result = contactObjects.GetSize ();
    }
    else
    {
      btGhostObject* ghost = btGhostObject::upcast (btObject);
      if (ghost)
        result = ghost->getNumOverlappingObjects ();
      else 
        return 0;
    }

    if (objectCopy)
      result += objectCopy->GetContactObjectsCount ();

    return result;
  }

  CS::Collisions::iCollisionObject* csBulletCollisionObject::GetContactObject (size_t index)
  {
    if (IsPhysicalObject())
    {
      if (index < contactObjects.GetSize () && index >= 0)
      {
        return contactObjects[index];
      }
      else
      {
        // TODO: Highly inconsistent
        if (objectCopy)
        {
          index -= contactObjects.GetSize ();
          return objectCopy->GetContactObject (index);
        }
        else
        {
          return nullptr;
        }
      }
    }
    else
    {
      btGhostObject* ghost = btGhostObject::upcast (btObject);
      if (ghost)
      {
        if (index < (size_t) ghost->getNumOverlappingObjects () && index >= 0)
        {
          btCollisionObject* obj = ghost->getOverlappingObject (index);
          if (obj)
            return static_cast<CS::Collisions::iCollisionObject*> (obj->getUserPointer ());
          else 
            return nullptr;
        }
        else
        {
          // TODO: Highly inconsistent
          if (objectCopy)
          {
            index -= ghost->getNumOverlappingObjects ();
            return objectCopy->GetContactObject (index);
          }
          else
            return nullptr;
        }
      }
      else 
        return nullptr;
    }
  }

  void csBulletCollisionObject::SetRotation (const csMatrix3& rot)
  {
    if (movable)
      movable->GetTransform().SetT2O(rot);
    if (camera)
      camera->GetTransform().SetT2O(rot);
    if (btObject)
    {
      csOrthoTransform trans = GetTransform ();
      trans.SetT2O (rot);
      btObject->setWorldTransform(CSToBullet (trans, system->getInternalScale ()));
    }
  }

  void csBulletCollisionObject::Rotate (const csVector3& v, float angle)
  {
    if (movable)
      movable->GetTransform().RotateThis (v, angle);
    if (camera)
      camera->GetTransform().RotateThis (v, angle);
    if (btObject)
    {
      csOrthoTransform trans = GetTransform ();
      trans.RotateThis (v, angle);
      btObject->setWorldTransform(CSToBullet (trans, system->getInternalScale ()));
    }
  }

  void csBulletCollisionObject::IncreasePitch(float pitchDelta)
  {
    Rotate(CS_VEC_TILT_UP, pitchDelta);
  }

  void csBulletCollisionObject::IncreaseYaw(float yawDelta)
  {
    Rotate (CS_VEC_ROT_RIGHT, yawDelta);
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
