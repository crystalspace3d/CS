#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"


#include "cssysdef.h"
#include "collisionghost.h"


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
    if (movable)
    {
      movable->SetFullTransform (BulletToCS(transform, system->getInverseInternalScale ()));
      movable->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS(transform, system->getInverseInternalScale ()));
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
    sector->GetBulletWorld()->addCollisionObject (btObject, collGroup.value, collGroup.mask);
    insideWorld = true;
    return true;
  }
  

  bool csBulletGhostCollisionObject::RemoveBulletObject ()
  {
    if (insideWorld)
    {
      sector->bulletWorld->removeCollisionObject (btObject);
      insideWorld = false;

      if (objectCopy)
        objectCopy->sector->RemoveCollisionObject (objectCopy);
      if (objectOrigin)
        objectOrigin->objectCopy = nullptr;

      objectCopy = nullptr;
      objectOrigin = nullptr;
      return true;
    }
    return false;
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)