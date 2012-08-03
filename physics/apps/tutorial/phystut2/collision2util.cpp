#include "cssysdef.h"

#include "collision2util.h"

#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"

#include "imesh/objmodel.h"
#include "imesh/terrain2.h"

using namespace CS::Collisions;
using namespace CS::Physics;

void Collision2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iEngine* engine, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* sp = meshes->Get (i);
    if ((collection && !collection->IsParentOf(sp->QueryObject ())) || !sp->GetMovable()) continue;
    InitializeCollisionObjects (colsys, sp->GetMovable()->GetSectors()->Get(0), sp);
  }
}

void Collision2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iSector* sector, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* sp = meshes->Get (i);
    if (collection && !collection->IsParentOf(sp->QueryObject ())) continue;
    InitializeCollisionObjects (colsys, sector, sp);
  }
}

void Collision2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colSys, iSector* sector, iMeshWrapper* mesh)
{
  // Get iCollisionSector from iSector
  iCollisionSector* colSect = colSys->GetOrCreateCollisionSector (sector);
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();

  // Check if we have a (partial) heightfield
  iTerrainSystem* terrainSys = objModel->GetTerrainColldet ();
  if (terrainSys)
  {
    // Check whether the sector did not add this terrain yet
    if (!colSect->GetCollisionTerrain(terrainSys))
    {
      // Create and add a collision Terrain
      csRef<iCollisionTerrain> colTerrain = colSys->CreateCollisionTerrain(terrainSys);
      colSect->AddCollisionTerrain(colTerrain);
    }
  }

  // Check if we have portals
  iPortalContainer* portalCont = mesh->GetPortalContainer ();
  if (portalCont)
  {
    for (size_t i = 0; i < portalCont->GetPortalCount(); ++i)
    {
      iPortal* portal = portalCont->GetPortal(i);

      // Ignore all portals that don't do warping
      // TODO: Flag portals as see-through only (for example in-game monitors that display a video camera stream)
    if (!portal->GetFlags().Check(CS_PORTAL_WARP)) continue;

      // This is very odd: Multiple portals with the same mesh transform?
      // TODO: Mesh transform can/should be retreived from the iPortal object - Don't need to pass it as an argument
      colSect->AddPortal (portal, mesh->GetMovable ()->GetFullTransform ());
    }
  }

  
  csRef<iCollisionObject> collObj;
  
  // Get mesh factory
  iMeshFactoryWrapper* meshFactory = mesh->GetFactory ();
  if (meshFactory)
  {
    ::iObject* meshFactoryObj = meshFactory->QueryObject ();
    if (meshFactoryObj)
    {
      // Add all collision objects of the mesh
      for (csRef<iObjectIterator> it = meshFactoryObj->GetIterator (); it->HasNext (); )
      {
        ::iObject* next = it->Next();
        csRef<iCollisionObjectFactory> nextFactory = scfQueryInterface<iCollisionObjectFactory>(next);
        if (nextFactory)
        {
          collObj = nextFactory->CreateCollisionObject();

          // TODO: Movables need to be able to define an offset transform relative to the collision object


          // TODO: Can only add mesh to a single CO right now (quite limiting)
          break;
        }
      }
    }
  }

  if (!terrainSys && !portalCont && !collObj)
  {
    // did not find a specific physical factory and its not a heightfield 
    // -> Create a static CO from the mesh, using default values for friction, etc (given it has an underlying physical model)
    csRef<iColliderConcaveMesh> collider = colSys->CreateColliderConcaveMesh(mesh);
    csRef<iCollisionObjectFactory> collObjFact = colSys->CreateCollisionObjectFactory(collider);
    collObj = collObjFact->CreateCollisionObject();
    mesh->QueryObject()->ObjAdd(collObj->QueryObject());
  }

  if (collObj)
  {
    // set movable stuff
    iMovable* movable = mesh->GetMovable();
    if (movable)
    {
      collObj->SetTransform(movable->GetFullTransform());
      collObj->SetAttachedMovable(movable);
    }

    // set name
    collObj->QueryObject()->SetName(mesh->QueryObject()->GetName());

    // add to sector
    colSect->AddCollisionObject(collObj);
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
      InitializeCollisionObjects (colSys, sector, child);
    }
  }
}
