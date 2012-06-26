#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "cssysdef.h"
#include "iengine/movable.h"
#include "collisionobject2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csBulletCollisionObject::CreateCollisionObject(CollisionObjectProperties* props)
  {
    SetCollider(props->GetCollider());
    SetName(props->GetName());

    if (props->GetCollisionGroup().name.Length())
    {
      SetCollisionGroup(props->GetCollisionGroup());
    }
    else
    {
      SetCollisionGroup(system->FindCollisionGroup("Default"));
    }
  }

  csBulletCollisionObject::csBulletCollisionObject (csBulletSystem* sys)
    : scfImplementationType (this), movable (nullptr), camera (nullptr), collCb (nullptr),
    portalWarp (btQuaternion::getIdentity ()), sector (nullptr), system (sys),
    btObject (nullptr), objectOrigin (nullptr), objectCopy (nullptr),
    insideWorld (false), btShape(nullptr)
  {
  }

  csBulletCollisionObject::~csBulletCollisionObject ()
  {
    if (btObject)
      delete btObject;
  }

  void csBulletCollisionObject::SetCollider (CS::Collisions::iCollider* newCollider)
  {
    if (newCollider)
    {
      this->collider = dynamic_cast<csBulletCollider*>(newCollider);
    }

    //RebuildObject();
  }

  void csBulletCollisionObject::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT(btObject);

    btTransform btTrans = CSToBullet (trans, system->getInternalScale ());

    if (movable)
    {
      movable->SetFullTransform (BulletToCS (btTrans, system->getInverseInternalScale ()));
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS (btTrans, system->getInverseInternalScale ()));
    }

    btObject->setWorldTransform(btTrans);
  }

  csOrthoTransform csBulletCollisionObject::GetTransform () const
  {
    //float inverseScale = system->getInverseInternalScale ();
    CS_ASSERT(btObject);
    return BulletToCS (btObject->getWorldTransform(), system->getInverseInternalScale ());
  }
  
  void csBulletCollisionObject::SetCollisionGroup (const char* name)
  {
    SetCollisionGroup(system->FindCollisionGroup(name));
  }

  void csBulletCollisionObject::SetCollisionGroup (const CollisionGroup& group)
  {
    this->collGroup = group;

    if (btObject && IsInWorld())
    {
      btObject->getBroadphaseHandle ()->m_collisionFilterGroup = group.value;
      btObject->getBroadphaseHandle ()->m_collisionFilterMask = group.mask;
    }
  }

  bool csBulletCollisionObject::Collide (CS::Collisions::iCollisionObject* otherObject)
  {
    csArray<CS::Collisions::CollisionData> data;
    csBulletCollisionObject* otherObj = dynamic_cast<csBulletCollisionObject*> (otherObject);

    //Object/Body VS object.
    btCollisionObject* otherBtObject = dynamic_cast<csBulletCollisionObject*> (otherObject)->GetBulletCollisionPointer ();
    bool result = sector->BulletCollide (btObject, otherBtObject, data);
    if (result && collCb)
    {
      collCb->OnCollision (this, otherObj, data);
    }

    return result;
  }

  CS::Collisions::HitBeamResult csBulletCollisionObject::HitBeam (const csVector3& start,
    const csVector3& end)
  {
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
