#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"


#include "cssysdef.h"
#include "collisionghost.h"


using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  csBulletCollisionGhostObject::csBulletCollisionGhostObject(csBulletSystem* sys) : scfImplementationType(this, sys)
  {
  }

  
  csBulletCollisionGhostObject::~csBulletCollisionGhostObject()
  {
  }

  void csBulletCollisionGhostObject::RebuildObject () 
  { 
    bool wasInWorld = insideWorld;
    if (insideWorld)
    {
      wasInWorld = true;
      RemoveBulletObject ();
    }

    // TODO: Recreate object only if necessary
    //CreateBulletObject();
    
    size_t colliderCount = colliders.GetSize ();
    if (colliderCount == 0)
    {  
      csFPrintf (stderr, "csBulletCollisionObject: Haven't add any collider to the object.\nRebuild failed.\n");
      return;
    }

    if (shapeChanged)
    {
      // TODO: Clean up this crap
      btCollisionShape* shape;
      if (compoundShape)
        shape = compoundShape;
      else
        shape = colliders[0]->shape;

      btTransform pricipalAxis;
      if (compoundShape)
        pricipalAxis.setIdentity ();
      else
        pricipalAxis = CSToBullet (relaTransforms[0], system->getInternalScale ());

      invPricipalAxis = pricipalAxis.inverse ();

      if (!btObject)
      {
        btObject = new btPairCachingGhostObject ();
      }

      btTransform transform = btObject->getWorldTransform();
      if (movable)
      {
        movable->SetFullTransform (BulletToCS(transform * invPricipalAxis, system->getInverseInternalScale ()));
      }

      if (camera)
      {
        camera->SetTransform (BulletToCS(transform * invPricipalAxis, system->getInverseInternalScale ()));
      }

      btObject->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
      btObject->setCollisionShape (shape);
    }

    if (wasInWorld)
    {
      AddBulletObject ();
    }
  }

  bool csBulletCollisionGhostObject::AddBulletObject()
  {
    if (insideWorld)
      RemoveBulletObject ();
    
    sector->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    sector->bulletWorld->addCollisionObject (btObject, collGroup.value, collGroup.mask);
    insideWorld = true;
    return true;
  }
  

  bool csBulletCollisionGhostObject::RemoveBulletObject ()
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