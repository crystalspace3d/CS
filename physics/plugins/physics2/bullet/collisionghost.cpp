#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"


#include "cssysdef.h"
#include "collisionghost.h"


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csBulletGhostCollisionObject::CreateGhostCollisionObject(CS::Collisions::GhostCollisionObjectProperties* props)
  {
    CreateCollisionObject(props);
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
      // TODO: Is it necessary to remove/re-insert the object?
      wasInWorld = true;
      RemoveBulletObject ();
    }

    if (!btObject)
    {
      btObject = new btPairCachingGhostObject ();
    }

    // create and set shape
    btShape = collider->GetOrCreateBulletShape();
    btObject->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
    btObject->setCollisionShape (btShape);

    // set transform
    btTransform transform = btObject->getWorldTransform();
    if (movable)
    {
      movable->SetFullTransform (BulletToCS(transform, system->getInverseInternalScale ()));
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
    sector->bulletWorld->addCollisionObject (btObject, collGroup.value, collGroup.mask);
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