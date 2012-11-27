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


#include "cssysdef.h"
#include "iengine/scenenode.h"
#include "collisionghost.h"

#include "portal.h"


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csBulletGhostCollisionObject::CreateGhostCollisionObject(CS::Collisions::iGhostCollisionObjectFactory* props)
  {
    CreateCollisionObject(props);
    
    // create object and set shape and user pointer
    btObject = new btPairCachingGhostObject ();
    btObject->setCollisionShape (collider->GetOrCreateBulletShape());
    btObject->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
  }

  csBulletGhostCollisionObject::csBulletGhostCollisionObject(csBulletSystem* sys) : scfImplementationType(this, sys)
  {
  }

  
  csBulletGhostCollisionObject::~csBulletGhostCollisionObject()
  {
  }

  void csBulletGhostCollisionObject::RebuildObject () 
  { 
    bool wasInWorld = insideWorld;
    if (insideWorld)
    {
      wasInWorld = true;
      RemoveBulletObject ();
    }
    
    btObject->setCollisionShape (collider->GetOrCreateBulletShape());
    
    // set transform
    btTransform transform = btObject->getWorldTransform();
    iSceneNode* sceneNode = GetAttachedSceneNode ();
    if (sceneNode)
    {
      // Update movable
      sceneNode->GetMovable ()->SetFullTransform
	(BulletToCS(transform, system->GetInverseInternalScale ()));
      sceneNode->GetMovable ()->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS(transform, system->GetInverseInternalScale ()));
    }

    // add back to world
    if (wasInWorld)
    {
      AddBulletObject ();
    }
  }

  bool csBulletGhostCollisionObject::AddBulletObject()
  {
    if (insideWorld)
      RemoveBulletObject ();
    
    sector->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    sector->GetBulletWorld()->addCollisionObject (btObject, group->value, group->mask);
    insideWorld = true;
    return true;
  }
  

  bool csBulletGhostCollisionObject::RemoveBulletObject ()
  {
    sector->bulletWorld->removeCollisionObject (btObject);
    insideWorld = false;

    if (portalData && portalData->Portal)
    {
      // Remove from portal
      portalData->Portal->RemoveTraversingObject(this);
    }
    return true;
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
