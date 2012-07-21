#include "cssysdef.h"

#include "collision2util.h"

#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/scenenode.h"

using namespace CS::Collisions;
using namespace CS::Physics;

void Collider2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iEngine* engine, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* sp = meshes->Get (i);
    if (collection && !collection->IsParentOf(sp->QueryObject ())) continue;
    InitializeCollisionObjects (colsys, sp);
  }
}

void Collider2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iSector* sector, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* sp = meshes->Get (i);
    if (collection && !collection->IsParentOf(sp->QueryObject ())) continue;
    InitializeCollisionObjects (colsys, sp);
  }
}

void Collider2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colSys, iMeshWrapper* mesh)
{
  iMeshFactoryWrapper* meshFactory = mesh->GetFactory ();

  if (meshFactory)
  {
    csRef<::iObject> meshFactoryObj = scfQueryInterface<::iObject>(meshFactory);
    if (meshFactoryObj)
    {
      iMovable* movable = mesh->GetMovable();
      if (movable)
      {
        // Get iSector
        // TODO: Since the object could be in multiple sectors at once, 
        //        it might have to check against collisions in all of them
        iSector* sect = movable->GetSectors()->Get(0);

        // get iCollisionSector from iSector
        iCollisionSector* colSect = colSys->GetCollisionSector (sect);
        if (colSect)
        {
          // Add all collision objects of the mesh
          for (csRef<iObjectIterator> it = meshFactoryObj->GetIterator (); it->HasNext (); )
          {
            ::iObject* next = it->Next();
            csRef<iCollisionObjectFactory> nextFactory = scfQueryInterface<iCollisionObjectFactory>(next);
            if (nextFactory)
            {
              csRef<iCollisionObject> obj = nextFactory->CreateCollisionObject();
              
              // TODO: We have a problem if we set the mesh as movable of multiple collision objects
              // TODO: Movables need to be able to define an offset transform relative to the collision object
              // TODO: Inconsistent ownership model:
              //      Movables are children of CollisionObject, but CollisionObjects are created from children of the mesh's factory
              obj->SetTransform(movable->GetFullTransform());
              obj->SetAttachedMovable(movable);
              
              colSect->AddCollisionObject(obj);
            }
          }
        }
      }
    }
  }

  // recurse
  const csRef<iSceneNodeArray> ml = mesh->QuerySceneNode ()->GetChildrenArray ();
  size_t i;
  for (i = 0 ; i < ml->GetSize () ; i++)
  {
    iMeshWrapper* child = ml->Get (i)->QueryMesh ();
    
    // @@@ What if we have a light containing another mesh?
    if (child)
    {
      InitializeCollisionObjects (colSys, child);
    }
  }
}