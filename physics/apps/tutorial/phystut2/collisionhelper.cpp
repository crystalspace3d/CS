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

#include "iengine/engine.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"
#include "iengine/sector.h"
#include "igeom/trimesh.h"
#include "imesh/objmodel.h"
#include "imesh/terrain2.h"
#include "iutil/plugin.h"
#include "ivaria/convexdecompose.h"
#include "ivaria/collisions.h"
#include "ivaria/reporter.h"

using namespace CS::Collisions;
//using namespace CS::Physics;

void CollisionHelper::ReportError (const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csReportV (objectRegistry, CS_REPORTER_SEVERITY_ERROR,
	     "crystalspace.collisions.helper",
	     msg, arg);
  va_end (arg);
}

bool CollisionHelper::Initialize
(iObjectRegistry* objectRegistry, CS::Collisions::iCollisionSystem* collisionSystem,
 CS::Collisions::iConvexDecomposer* decomposer)
{
  CS_ASSERT (objectRegistry && collisionSystem);

  this->objectRegistry = objectRegistry;
  this->collisionSystem = collisionSystem;
  this->decomposer = decomposer;
/*
  // Load the convex decomposer if it has not been specified by the user
  if (!this->decomposer)
  {
    csRef<iPluginManager> plugmgr = csQueryRegistry<iPluginManager> (objectRegistry);
    this->decomposer = csLoadPlugin<iConvexDecomposer>
      (plugmgr, "crystalspace.mesh.convexdecompose.hacd");
    if (!this->decomposer)
    {
      ReportError ("Could not find any plugin for convex decomposition");
      return false;
    }
  }
*/
  // Initialize the ID's for the collision models of the meshes
  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>
    (objectRegistry, "crystalspace.shared.stringset");
  baseID = strings->Request ("base");
  collisionID = strings->Request ("colldet");

  return true;
}

void CollisionHelper::InitializeCollisionObjects
(iEngine* engine, iCollection* collection) const
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if ((collection && !collection->IsParentOf (mesh->QueryObject ())) || !mesh->GetMovable ()) continue;
    InitializeCollisionObjects (mesh->GetMovable ()->GetSectors ()->Get (0), mesh);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iCollection* collection) const
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if (collection && !collection->IsParentOf (mesh->QueryObject ())) continue;
    InitializeCollisionObjects (sector, mesh);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iMeshWrapper* mesh) const
{
  // Create the iCollisionSector from the iSector if not yet made
  iCollisionSector* colSect = collisionSystem->FindCollisionSector (sector);
  if (!colSect)
  {
    colSect = collisionSystem->CreateCollisionSector ();
    colSect->SetSector (sector);
  }

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
      csRef<CS::Collisions::iCollider> collider;
      if (decomposer)
      {
	collider = csRef<CS::Collisions::iCollider> (collisionSystem->CreateCollider ());
	DecomposeConcaveMesh (triMesh, collider, decomposer);
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
(iMeshWrapper* mesh) const
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

void CollisionHelper::DecomposeConcaveMesh
(iTriangleMesh* mesh, iCollider* collider,
 CS::Collisions::iConvexDecomposer* decomposer) const
{
  // Decompose the mesh in its convex parts
  csRefArray<iTriangleMesh> convexParts;
  decomposer->DecomposeMesh (mesh, convexParts);

  // Merge the convex parts into the given collider
  for (size_t i = 0; i < convexParts.GetSize (); i++)
  {
    csRef<iColliderConvexMesh> convexCollider =
      collisionSystem->CreateColliderConvexMesh (convexParts[i]);
    collider->AddChild (convexCollider);
  }    
}

void CollisionHelper::DecomposeConcaveMesh
(iMeshWrapper* mesh, CS::Collisions::iCollider* collider,
 CS::Collisions::iConvexDecomposer* decomposer) const
{
  iTriangleMesh* triangleMesh = FindCollisionMesh (mesh);
  if (triangleMesh)
    DecomposeConcaveMesh (triangleMesh, collider, decomposer);
}
