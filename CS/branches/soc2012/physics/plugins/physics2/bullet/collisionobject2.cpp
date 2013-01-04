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
  // TODO: factory in constructor (everywhere)
  void csBulletCollisionObject::CreateCollisionObject (iCollisionObjectFactory* props)
  {
    collider = dynamic_cast<csBulletCollider*>(props->GetCollider ());
    group = dynamic_cast<CollisionGroup*> (props->GetCollisionGroup ());
  }

  csBulletCollisionObject::csBulletCollisionObject (csBulletSystem* sys)
    : scfImplementationType (this), portalWarp (btQuaternion::getIdentity ()), sector (nullptr),
    system (sys), portalData (nullptr), btObject (nullptr), insideWorld (false)
  {
  }

  csBulletCollisionObject::~csBulletCollisionObject ()
  {
    if (btObject)
      delete btObject;

    if (portalData)
    {
      // must not still be traversing
      CS_ASSERT (!portalData->Portal);
      delete portalData;
    }
  }
  
  void csBulletCollisionObject::SetSector (CS::Collisions::iCollisionSector* sector)
  {
    if (this->sector) this->sector->RemoveCollisionObject (this);
    if (!sector) this->sector = nullptr;
    else sector->AddCollisionObject (this);
  }

  void csBulletCollisionObject::SetAttachedSceneNode (iSceneNode* newSceneNode) 
  {
    if (sceneNode == newSceneNode) return;

    if (sceneNode)
    {
      // remove old SceneNode from sector
      if (sector) sector->RemoveSceneNodeFromSector (sceneNode); 
    }
    sceneNode = newSceneNode; 
    if (sceneNode) 
    {
      // add new movable to sector
      sceneNode->GetMovable ()->SetFullTransform (GetTransform ()); 
      sceneNode->GetMovable ()->UpdateMove ();
      if (sector)
      {
        sector->AddSceneNodeToSector (sceneNode); 
      }
    }
  }
  
  void csBulletCollisionObject::SetCollider (CS::Collisions::iCollider* newCollider,
					     const csOrthoTransform& transform)
  {
    colliderTransform = transform;
    if (newCollider)
    {
      collider = dynamic_cast<csBulletCollider*>(newCollider);
      RebuildObject ();
    }
  }

  void csBulletCollisionObject::SetColliderTransform (const csOrthoTransform& transform)
  {
    colliderTransform = transform;
  }

  const csOrthoTransform& csBulletCollisionObject::GetColliderTransform () const
  {
    return colliderTransform;
  }

  void csBulletCollisionObject::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT (btObject);

    btTransform btTrans = CSToBullet (trans, system->GetInternalScale ());

    iSceneNode* sceneNode = GetAttachedSceneNode ();
    if (sceneNode)
    {
      sceneNode->GetMovable ()->SetFullTransform (trans);
      sceneNode->GetMovable ()->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (trans);
    }

    btObject->setWorldTransform (btTrans);
  }

  csOrthoTransform csBulletCollisionObject::GetTransform () const
  {
    CS_ASSERT (btObject);
    return BulletToCS (btObject->getWorldTransform (), system->GetInverseInternalScale ());
  }

  void csBulletCollisionObject::SetCollisionGroup (iCollisionGroup* group)
  {
    // TODO: fallback to the factory value
    this->group = dynamic_cast<CollisionGroup*> (group);
    if (!this->group) this->group = system->GetDefaultGroup ();
  }

  iCollisionGroup* csBulletCollisionObject::GetCollisionGroup () const
  {
    return group;
  }

/*
  void csBulletCollisionObject::SetCollisionGroup (const char* name)
  {
    SetCollisionGroup (system->FindCollisionGroup (name));
  }

  void csBulletCollisionObject::SetCollisionGroup (const CollisionGroup& group)
  {
    this->collGroup = group;

    if (btObject && insideWorld)
    {
      btObject->getBroadphaseHandle ()->m_collisionFilterGroup = group.value;
      btObject->getBroadphaseHandle ()->m_collisionFilterMask = group.mask;

      // re-add object
      RemoveBulletObject ();
      AddBulletObject ();
    }
  }
*/
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
    if (IsPassive ()) return 0;

    size_t result = 0;
    if (GetObjectType () == COLLISION_OBJECT_PHYSICAL)
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
    if (IsPassive ()) return nullptr;

    // TODO: Fix this method and split it up into the corresponding sub-classes
    if (GetObjectType () == COLLISION_OBJECT_PHYSICAL)
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
    iSceneNode* sceneNode = GetAttachedSceneNode ();
    if (sceneNode)
      sceneNode->GetMovable ()->GetTransform ().SetT2O (rot);
    if (camera)
      camera->GetTransform ().SetT2O (rot);
    if (btObject)
    {
      csOrthoTransform trans = GetTransform ();
      trans.SetT2O (rot);
      btObject->setWorldTransform (CSToBullet (trans, system->GetInternalScale ()));
    }
  }

  bool csBulletCollisionObject::TestOnGround ()
  { 
    static const float groundAngleCosThresh = .7f;

    // Find any objects that can at least remotely support the object
    csArray<CollisionData> collisions;
    sector->CollisionTest (this, collisions);

    //int objBeneathCount = 0;
    for (size_t i = 0; i < collisions.GetSize (); ++i)
    {
      CollisionData& coll = collisions[i];
      
      int dir = coll.objectA == this ? 1 : -1;

      float groundAngleCos = coll.normalWorldOnB * csVector3 (0.0f, 1.0f, 0.0f);
      if (dir * groundAngleCos > groundAngleCosThresh)
      {
        return true;
      }
    }
    return false;
  }

  bool csBulletCollisionObject::IsPassive () const
  {
    return portalData != nullptr && !portalData->IsOriginal;
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
