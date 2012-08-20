#include "cssysdef.h"

#include "collision2util.h"

#include "iengine/sector.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"

#include "igeom/trimesh.h"

#include "imesh/objmodel.h"
#include "imesh/terrain2.h"

#include "ivaria/convexdecompose.h"

using namespace CS::Collisions;
using namespace CS::Physics;

void Collision2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iEngine* engine, 
    iConvexDecomposer* decomposer,
    iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if ((collection && !collection->IsParentOf(mesh->QueryObject ())) || !mesh->GetMovable()) continue;
    InitializeCollisionObjects (colsys, mesh->GetMovable()->GetSectors()->Get(0), mesh, decomposer);
  }
}

void Collision2Helper::InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iSector* sector, 
    iConvexDecomposer* decomposer, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if (collection && !collection->IsParentOf(mesh->QueryObject ())) continue;
    InitializeCollisionObjects (colsys, sector, mesh, decomposer);
  }
}

void Collision2Helper::InitializeCollisionObjects (
  CS::Collisions::iCollisionSystem* colSys, 
  iSector* sector,
  iMeshWrapper* mesh, 
  iConvexDecomposer* decomposer)
{
  // Get iCollisionSector from iSector
  iCollisionSector* colSect = colSys->GetOrCreateCollisionSector (sector);
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();

  // Check if we have a terrain mesh
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

  // Check if we have a portal mesh
  iPortalContainer* portalCont = mesh->GetPortalContainer ();
  if (portalCont)
  {
    for (size_t i = 0; i < (size_t)portalCont->GetPortalCount(); ++i)
    {
      iPortal* portal = portalCont->GetPortal(i);

      // TODO: Ignore all portals that don't do warping
      // TODO: Flag portals as see-through only (for example in-game monitors that display a video camera stream)
      //if (!portal->GetFlags().Check(CS_PORTAL_WARP)) continue;

      // This is very odd: Multiple portals with the same mesh transform?
      // TODO: Mesh transform can/should be retreived from the iPortal object - Don't need to pass it as an argument
      colSect->AddPortal (portal, mesh->GetMovable ()->GetFullTransform ());
    }
  }

  
  // Create the CO of the mesh
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
    // did not find a specific physical factory and its not a placeholder
    // -> Create a static CO from the mesh, using default values for physical values (if available)

    csRef<iTriangleMesh> triMesh = colSys->FindColdetTriangleMesh(mesh);
    if (triMesh)
    {
      //csRef<iColliderConvexMesh> collider = colSys->CreateColliderConvexMesh(mesh);

      csRef<CS::Collisions::iCollider> collider;
      if (decomposer)
      {
        collider = csRef<CS::Collisions::iColliderCompound>(PerformConvexDecomposition(colSys, decomposer, triMesh));
      }
      else
      {
        collider = csRef<CS::Collisions::iColliderConcaveMesh>(colSys->CreateColliderConcaveMesh(mesh));
      }

      // Colliders cannot be created for meshes that have no triangle data
      if (collider)
      {
        csRef<iCollisionObjectFactory> collObjFact = colSys->CreateCollisionObjectFactory(collider);
        collObj = collObjFact->CreateCollisionObject();
        mesh->QueryObject()->ObjAdd(collObj->QueryObject());
      }
    }
  }

  if (collObj)
  {
    // set scenenode
    collObj->SetTransform(mesh->GetMovable()->GetFullTransform());
    collObj->SetAttachedSceneNode(mesh->QuerySceneNode());

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

csPtr<iColliderCompound> Collision2Helper::PerformConvexDecomposition(
  iCollisionSystem* colSys, 
  iConvexDecomposer* decomposer,
  iTriangleMesh* concaveMesh)
{
  struct ConvexDecomposedMeshResult : iConvexDecomposedMeshResult
  {
    iCollisionSystem* colSys;
    csRef<iColliderCompound> collider;

    ConvexDecomposedMeshResult(iCollisionSystem* colSys) :

    colSys(colSys)
    {
      collider = colSys->CreateColliderCompound();
    }

    virtual void YieldMesh(iTriangleMesh* convexMeshPart)
    {
      csRef<iColliderConvexMesh> partialCollider = colSys->CreateColliderConvexMesh(convexMeshPart);
      collider->AddCollider(partialCollider);
    }
  };

  // Create result "accumulator"
  ConvexDecomposedMeshResult nestedResults(colSys);

  // Actual Decomposition
  decomposer->Decompose(concaveMesh, &nestedResults);
  
  // Return the new collider to the caller
  return csPtr<iColliderCompound>(nestedResults.collider);
}