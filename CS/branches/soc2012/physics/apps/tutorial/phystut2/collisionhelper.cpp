/*
    Copyright (C) 2012 by Dominik Seifert

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

#include "cssysdef.h"

#include "collisionhelper.h"

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

void CollisionHelper::Initialize
(iObjectRegistry* objectRegistry, CS::Collisions::iCollisionSystem* collisionSystem)
{
  CS_ASSERT (objectRegistry && collisionSystem);

  this->objectRegistry = objectRegistry;
  this->collisionSystem = collisionSystem;

  // Initialize the ID's for the collision models of the meshes
  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>
    (objectRegistry, "crystalspace.shared.stringset");
  baseID = strings->Request ("base");
  collisionID = strings->Request ("colldet");
}

void CollisionHelper::InitializeCollisionObjects
(iEngine* engine, iConvexDecomposer* decomposer, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if ((collection && !collection->IsParentOf (mesh->QueryObject ())) || !mesh->GetMovable ()) continue;
    InitializeCollisionObjects (mesh->GetMovable ()->GetSectors ()->Get (0), mesh, decomposer);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iConvexDecomposer* decomposer, iCollection* collection)
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if (collection && !collection->IsParentOf (mesh->QueryObject ())) continue;
    InitializeCollisionObjects (sector, mesh, decomposer);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iMeshWrapper* mesh, iConvexDecomposer* decomposer)
{
  // Get iCollisionSector from iSector
  iCollisionSector* colSect = collisionSystem->GetOrCreateCollisionSector (sector);
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();

  // Check if we have a terrain mesh
  iTerrainSystem* terrainSys = objModel->GetTerrainColldet ();
  if (terrainSys)
  {
    // Check whether the sector did not add this terrain yet
    if (!colSect->GetCollisionTerrain (terrainSys))
    {
      // Create and add a collision Terrain
      csRef<iCollisionTerrain> colTerrain = collisionSystem->CreateCollisionTerrain (terrainSys);
      colSect->AddCollisionTerrain (colTerrain);
    }
  }

  // Check if we have a portal mesh
  iPortalContainer* portalCont = mesh->GetPortalContainer ();
  if (portalCont)
  {
    for (size_t i = 0; i < (size_t)portalCont->GetPortalCount (); ++i)
    {
      iPortal* portal = portalCont->GetPortal (i);

      // TODO: Ignore all portals that don't do warping
      // TODO: Flag portals as see-through only (for example in-game monitors that display a video camera stream)
      //if (!portal->GetFlags ().Check (CS_PORTAL_WARP)) continue;
      // TODO: use CS_PORTAL_COLLDET and put on this flag by default for all portals?
      //if (!portal->GetFlags ().Check (CS_PORTAL_COLLDET)) continue;

      // This is very odd: Multiple portals with the same mesh transform?
      // TODO: Mesh transform can/should be retreived from the iPortal object - Don't need to pass it as an argument
      colSect->AddPortal (portal, mesh->GetMovable ()->GetFullTransform ());
    }
  }

  
  // Create the collision object of the mesh
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
        ::iObject* next = it->Next ();
        csRef<iCollisionObjectFactory> nextFactory = scfQueryInterface<iCollisionObjectFactory>(next);
        if (nextFactory)
        {
          collObj = nextFactory->CreateCollisionObject ();

          // TODO: Movables need to be able to define an offset transform relative to the collision object


          // TODO: Can only add mesh to a single collision object right now (quite limiting)
          break;
        }
      }
    }
  }

  if (!terrainSys && !portalCont && !collObj)
  {
    // did not find a specific physical factory and its not a placeholder
    // -> Create a static collision object from the mesh, using default values for physical values (if available)

    csRef<iTriangleMesh> triMesh = FindCollisionMesh (mesh);
    if (triMesh)
    {
      //csRef<iColliderConvexMesh> collider = collisionSystem->CreateColliderConvexMesh (mesh);

      csRef<CS::Collisions::iCollider> collider;
      if (decomposer)
      {
        collider = csRef<CS::Collisions::iColliderCompound>(PerformConvexDecomposition (decomposer, triMesh));
      }
      else
      {
        collider = csRef<CS::Collisions::iColliderConcaveMesh>(collisionSystem->CreateColliderConcaveMesh (triMesh));
      }

      // Colliders cannot be created for meshes that have no triangle data
      if (collider)
      {
        csRef<iCollisionObjectFactory> collObjFact = collisionSystem->CreateCollisionObjectFactory (collider);
        collObj = collObjFact->CreateCollisionObject ();
        mesh->QueryObject ()->ObjAdd (collObj->QueryObject ());
      }
    }
  }

  if (collObj)
  {
    // set scenenode
    collObj->SetTransform (mesh->GetMovable ()->GetFullTransform ());
    collObj->SetAttachedSceneNode (mesh->QuerySceneNode ());

    // set name
    collObj->QueryObject ()->SetName (mesh->QueryObject ()->GetName ());

    // add to sector
    colSect->AddCollisionObject (collObj);
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
      InitializeCollisionObjects (sector, child);
    }
  }
}

iTriangleMesh* CollisionHelper::FindCollisionMesh
(iMeshWrapper* mesh)
{
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();
  iTriangleMesh* triMesh;
  if (objModel->IsTriangleDataSet (collisionID))
    triMesh = objModel->GetTriangleData (collisionID);
  else
    triMesh = objModel->GetTriangleData (baseID);

  if (!triMesh || triMesh->GetVertexCount () == 0
      || triMesh->GetTriangleCount () == 0)
    return nullptr;

  return triMesh;
}

csPtr<iColliderCompound> CollisionHelper::PerformConvexDecomposition
(iConvexDecomposer* decomposer, iTriangleMesh* concaveMesh)
{
  struct ConvexDecomposedMeshResult : iConvexDecomposedMeshResult
  {
    iCollisionSystem* collisionSystem;
    csRef<iColliderCompound> collider;

    ConvexDecomposedMeshResult (iCollisionSystem* collisionSystem)
      : collisionSystem (collisionSystem)
    {
      collider = collisionSystem->CreateColliderCompound ();
    }

    virtual void YieldMesh (iTriangleMesh* convexMeshPart)
    {
      csRef<iColliderConvexMesh> partialCollider = collisionSystem->CreateColliderConvexMesh (convexMeshPart);
      collider->AddCollider (partialCollider);
    }
  };

  // Create result "accumulator"
  ConvexDecomposedMeshResult nestedResults (collisionSystem);

  // Actual Decomposition
  decomposer->Decompose (concaveMesh, &nestedResults);
  
  // Return the new collider to the caller
  return csPtr<iColliderCompound>(nestedResults.collider);
}
