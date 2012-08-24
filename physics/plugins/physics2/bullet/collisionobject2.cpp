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

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "cssysdef.h"
#include "iengine/movable.h"
#include "iengine/scenenode.h"
#include "collisionobject2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"

#include "portal.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csBulletCollisionObject::CreateCollisionObject(iCollisionObjectFactory* props)
  {
    collider = dynamic_cast<csBulletCollider*>(props->GetCollider());
    SetName(props->QueryObject()->GetName());

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
    : scfImplementationType (this), portalWarp (btQuaternion::getIdentity ()), sector (nullptr),
    system (sys), btObject (nullptr),
    insideWorld (false), portalData(nullptr)
  {
  }

  csBulletCollisionObject::~csBulletCollisionObject ()
  {
    if (btObject)
      delete btObject;

    if (portalData)
    {
      // must not still be traversing
      CS_ASSERT(!portalData->Portal);
      delete portalData;
    }
  }
  
  void csBulletCollisionObject::SetAttachedSceneNode (iSceneNode* newSceneNode) 
  {
    if (sceneNode == newSceneNode) return;

    if (sceneNode)
    {
      // remove old SceneNode from sector
      if (sector) sector->RemoveSceneNodeFromSector(sceneNode); 
    }
    sceneNode = newSceneNode; 
    if (sceneNode) 
    {
      // add new movable to sector
      sceneNode->QueryMesh()->GetMovable()->SetFullTransform(GetTransform()); 
      sceneNode->QueryMesh()->GetMovable()->UpdateMove ();
      if (sector)
      {
        sector->AddSceneNodeToSector(sceneNode); 
      }
    }
  }
  
  iMovable* csBulletCollisionObject::GetAttachedMovable () const
  {
    if (!sceneNode) return nullptr;
    return sceneNode->QueryMesh()->GetMovable();
  }

  void csBulletCollisionObject::SetCollider (CS::Collisions::iCollider* newCollider)
  {
    if (newCollider)
    {
      collider = dynamic_cast<csBulletCollider*>(newCollider);

      RebuildObject();
    }
  }

  void csBulletCollisionObject::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT(btObject);

    btTransform btTrans = CSToBullet (trans, system->getInternalScale ());

    iMovable* movable = GetAttachedMovable();
    if (movable)
    {
      movable->SetFullTransform (trans);
      movable->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (trans);
    }

    btObject->setWorldTransform(btTrans);
  }

  csOrthoTransform csBulletCollisionObject::GetTransform () const
  {
    //float inverseScale = system->getInverseInternalScale ();
    CS_ASSERT(btObject);
    return BulletToCS (btObject->getWorldTransform(), system->getInverseInternalScale ());
  }

  void csBulletCollisionObject::GetAABB(csVector3& aabbMin, csVector3& aabbMax) const
  {
    btVector3 bmin, bmax;
    collider->GetOrCreateBulletShape()->getAabb(btObject->getWorldTransform(), bmin, bmax);
    
    aabbMin = BulletToCS(bmin, system->getInverseInternalScale ());
    aabbMax = BulletToCS(bmax, system->getInverseInternalScale ());
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

      // re-add object
      RemoveBulletObject();
      AddBulletObject();
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
    if (IsPassive()) return 0;

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

    if (portalData && portalData->OtherObject)
    {
      result += portalData->OtherObject->GetContactObjectsCount ();
    }

    return result;
  }

  CS::Collisions::iCollisionObject* csBulletCollisionObject::GetContactObject (size_t index)
  {
    if (IsPassive()) return nullptr;

    // TODO: Fix this method and split it up into the corresponding sub-classes
    if (IsPhysicalObject())
    {
      if (index < contactObjects.GetSize () && index >= 0)
      {
        return contactObjects[index];
      }
      else
      {
        if (portalData && portalData->OtherObject)
        {
          index -= contactObjects.GetSize ();
          return portalData->OtherObject->GetContactObject (index);
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
          if (portalData && portalData->OtherObject)
          {
            index -= ghost->getNumOverlappingObjects ();
            return portalData->OtherObject->GetContactObject (index);
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
    iMovable* movable = GetAttachedMovable();
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
    iMovable* movable = GetAttachedMovable();
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

  bool csBulletCollisionObject::TestOnGround()
  { 
    static const float groundAngleCosThresh = .7f;

    // Find any objects that can at least remotely support the object
    csArray<CollisionData> collisions;
    sector->CollisionTest(this, collisions);

    //int objBeneathCount = 0;
    for (size_t i = 0; i < collisions.GetSize (); ++i)
    {
      CollisionData& coll = collisions[i];
      
      int dir = coll.objectA == this ? 1 : -1;

      float groundAngleCos = coll.normalWorldOnB * UpVector;
      if (dir * groundAngleCos > groundAngleCosThresh)
      {
        return true;
      }
    }
    return false;
  }

  
  bool csBulletCollisionObject::IsPassive() const
  {
    return portalData != nullptr && !portalData->IsOriginal;
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
