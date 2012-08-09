/**
 * Load / setup / modify the static (non-interactive) background and scenery objects
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

#include "collision2util.h"

#include "iengine/campos.h"

const static csScalar StaticElasticity(csScalar(0.1));

const csString DefaultSectorName("defaultsector");


using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

/**
 * Utility function - Should eventually be added to csPoly3D.
 * Rotates polyIn by rotation and stores it in polyOut.
 */
void TransformPoly3D(const csPoly3D& polyIn, csPoly3D& polyOut, const csOrthoTransform& trans)
{
  polyOut.SetVertexCount(polyIn.GetVertexCount());
  for (size_t i = 0; i < polyIn.GetVertexCount(); ++i)
  {
    csVector3 vert = polyIn[i];
    vert = trans.GetOrigin() + trans.GetT2O() * vert;
    polyOut[i] = vert;
  }
}

void RotatePoly3D(const csPoly3D& polyIn, csPoly3D& polyOut, const csMatrix3& rot)
{
  polyOut.SetVertexCount(polyIn.GetVertexCount());
  for (size_t i = 0; i < polyIn.GetVertexCount(); ++i)
  {
    csVector3 vert = polyIn[i];
    vert = rot * vert;
    polyOut[i] = vert;
  }
}


void PhysDemo::CreateBoxRoom(const csVector3& roomExtents, const csVector3& pos, csScalar wallThickness)
{
  // The boxes that make up floor and ceiling
  // AABB over these two is the entire room, including walls
  csVector3 yExtents(
    roomExtents[HorizontalAxis1] + 2 * wallThickness,
    wallThickness,
    roomExtents[HorizontalAxis2] + 2 * wallThickness);
  RenderMeshColliderPair yLimitingPair;
  CreateBoxMeshColliderPair(yLimitingPair, yExtents);

  // The boxes that make up one pair of walls
  // AABB over these two is the y-limited part of the room
  csVector3 xExtents(
    wallThickness,
    roomExtents[HorizontalAxis1],
    roomExtents[HorizontalAxis2] + 2 * wallThickness);
  RenderMeshColliderPair xLimitingPair;
  CreateBoxMeshColliderPair(xLimitingPair, xExtents);

  // The boxes that make up the other pair of walls
  // AABB over these two is the y- and x- limited part of the room
  csVector3 zExtents(
    roomExtents[HorizontalAxis2],
    roomExtents[HorizontalAxis1],
    wallThickness);
  RenderMeshColliderPair zLimitingPair;
  CreateBoxMeshColliderPair(zLimitingPair, zExtents);
  
  //csVector3 halfRoomExtents = roomExtents / 2;
  csVector3 distances = (roomExtents + csVector3(wallThickness)) / 2;

  SpawnRigidBody(xLimitingPair, pos + csVector3(distances[HorizontalAxis1], 0, 0), "wall", 0, 0)->SetElasticity(StaticElasticity);
  SpawnRigidBody(xLimitingPair, pos + csVector3(-distances[HorizontalAxis1], 0, 0), "wall", 0, 0)->SetElasticity(StaticElasticity);

  SpawnRigidBody(yLimitingPair, pos + csVector3(0, distances[UpAxis], 0), "ceiling", 0, 0)->SetElasticity(StaticElasticity);
  SpawnRigidBody(yLimitingPair, pos + csVector3(0, -distances[UpAxis], 0), "floor", 15, 0)->SetElasticity(StaticElasticity);

  SpawnRigidBody(zLimitingPair, pos + csVector3(0, 0, distances[HorizontalAxis2]), "wall", 0, 0)->SetElasticity(StaticElasticity);
  SpawnRigidBody(zLimitingPair, pos + csVector3(0, 0, -distances[HorizontalAxis2]), "wall", 0, 0)->SetElasticity(StaticElasticity);
  
}

void PhysDemo::CreateBoxRoom(csScalar size)
{ 
  // Create and setup sector
  room = engine->CreateSector (DefaultSectorName);
  SetCurrentSector(csRef<iPhysicalSector>(CreatePhysicalSector(room)));

  // Add cam pos
  iCameraPosition* pos = engine->GetCameraPositions()->NewCameraPosition("Center");
  pos->Set(DefaultSectorName, csVector3 (0, 0, 0), csVector3 (0, 0, 1), UpVector);

  // Room parameters
  csVector3 roomExtents(size); csVector3 halfRoomExtents(csScalar(.5) * roomExtents);
  csVector3 roomPos(0);
  csScalar wallThickness = 5;
  
  // Portal parameters
  csScalar portalEpsilon = csScalar(0.01);
  //csVector2 halfPortalExtents(1, 2);                              // a portal has width = 2, height = 4
  csVector2 halfPortalExtents(1);
  
  //csMatrix3 rotation = csZRotMatrix3 (HALF_PI);                   // the rotation between the two portals
  
  // Default behavior from DemoApplication for the creation of the scene
  if (!DemoApplication::CreateRoom()) return;

  // Create room
  CreateBoxRoom(roomExtents, roomPos, wallThickness);

  //// Create portals

  //// Positions of the portals
  //csVector3 portal1Pos = csVector3(
  //  halfRoomExtents.x - portalEpsilon, 
  //  -halfRoomExtents.y + halfPortalExtents.y, 
  //  0.0f);

  //csVector3 portal2Pos = csVector3(
  //  0, 
  //  -halfRoomExtents.y + portalEpsilon , 
  //  0);
  
  // TODO: Fix programmatic portals
  // TODO: Add mechanism to more easily create a portal pair

  //printf ("Creating portal room...\n");       // let's go
  //
  //
  //// Add portals

  //// Define the plane of the wall poly
  //csPoly3D poly;
  //poly.AddVertex (csVector3 (0.0f, -halfPortalExtents.y, halfPortalExtents.x));
  //poly.AddVertex (csVector3 (0.0f, halfPortalExtents.y, halfPortalExtents.x));
  //poly.AddVertex (csVector3 (0.0f, halfPortalExtents.y, -halfPortalExtents.x));
  //poly.AddVertex (csVector3 (0.0f, -halfPortalExtents.y, -halfPortalExtents.x));

  //// Portal transformations are absolute
  //csOrthoTransform portal1Trans(
  //  csMatrix3(),
  //  portal1Pos);
  //csOrthoTransform portal2Trans(
  //  rotation,
  //  portal2Pos);

  //// transform the portals into place
  //csPoly3D poly1, poly2;
  //RotatePoly3D(poly, poly1, portal1Trans.GetT2O());
  //RotatePoly3D(poly, poly2, portal2Trans.GetT2O());


  //iPortal *portal1, *portal2;

  //// Create the portal meshes
  //csRef<iMeshWrapper> portalMesh1 = engine->CreatePortal ("right_wall", 
  //  room,
  //  portal1Trans.GetOrigin(),
  //  room, 
  //  poly1.GetVertices(), 
  //  (int) poly1.GetVertexCount(),
  //  portal1);

  //csRef<iMeshWrapper> portalMesh2 = engine->CreatePortal ("floor", 
  //  room,
  //  portal2Trans.GetOrigin(),
  //  room, 
  //  poly2.GetVertices(), 
  //  (int) poly2.GetVertexCount(),
  //  portal2);

  //
  //portal1->GetWorldVertices ();

  //csVector3 nor = portal1->GetObjectPlane ().GetNormal ();
  //
  //// place & configure portals and warp transforms
  ////portalMesh1->QuerySceneNode()->GetMovable()->SetTransform(portal1Trans);
  ////portalMesh2->QuerySceneNode()->GetMovable()->SetTransform(portal2Trans);
  //
  //portal1->GetFlags().Set (CS_PORTAL_ZFILL);
  //portal1->GetFlags().Set (CS_PORTAL_CLIPDEST);

  //portal2->GetFlags().Set (CS_PORTAL_ZFILL);
  //portal2->GetFlags().Set (CS_PORTAL_CLIPDEST);
  //
  ////csOrthoTransform portal1Warp(portal2Trans.GetInverse() * portal1Trans);
  ////csOrthoTransform portal2Warp(portal1Trans.GetInverse() * portal2Trans);
  //
  ////portal1Warp.SetT2O(portal1Warp.GetT2O() * csXRotMatrix3(PI) * csYRotMatrix3(PI));
  ////portal1Warp.SetT2O(portal1Warp.GetT2O() * csZRotMatrix3(PI));
  ////portal2Warp.SetT2O(portal2Warp.GetT2O() * csYRotMatrix3(PI) * csZRotMatrix3(PI));

  //// distance between the two portals
  //csVector3 portalDist(portal2Trans.GetOrigin() - portal1Trans.GetOrigin());

  //csOrthoTransform portal1Warp(csYRotMatrix3 (-PI * 0.5f) * csZRotMatrix3 (PI * 0.5f),
		//		     csVector3 (-portalDist.y, -portalDist.z, -portalDist.x));

  //csOrthoTransform portal2Warp(csZRotMatrix3 (-PI * 0.5f) * csYRotMatrix3 (PI * 0.5f),
		//		     csVector3 (portalDist.z, portalDist.x, portalDist.y));

  //portal1->SetWarp (portal1Warp);
  //portal2->SetWarp (portal2Warp);
  //
  //// Create collision portals
  //static const csOrthoTransform identity;
  //GetCurrentSector()->AddPortal (portal1, portal1Trans);
  //GetCurrentSector()->AddPortal (portal2, portal2Trans);

  // Set up some lights
  room->SetDynamicAmbientLight (csColor (0.3f, 0.3f, 0.3f));

  csRef<iLight> light;
  iLightList* lightList = room->GetLights();
  lightList->RemoveAll();

  light = engine->CreateLight(0, csVector3(10), 9000, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (3, 0, 0), 8, csColor (1, 0, 0));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (-3, 0,  0), 8, csColor (0, 0, 1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0, 0, 3), 8, csColor (0, 1, 0));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0, -3, 0), 8, csColor (1, 1, 0));
  lightList->Add (light);

  CS::Lighting::SimpleStaticLighter::ShineLights (room, engine, 4);
}

bool PhysDemo::LoadLevel(const csString& pathname, bool convexDecomp)
{
  CS_ASSERT(pathname.Length());

  csString folder, filename;
  GetFolderAndFile(pathname, folder, filename);

  const csString& levelname = pathname;
  printf ("Loading level: %s...\n", levelname.GetData());

  // Load the level file
  csRef<iVFS> VFS (csQueryRegistry<iVFS> (GetObjectRegistry()));

  VFS->PushDir();
  if (!VFS->ChDir (folder.GetData()))
  {
    ReportError("ERROR: Couldn't find directory \"%s\" for level: %s", folder.GetData(), levelname.GetData());
    return false;
  }

  if (!loader->LoadMapFile (filename, false))
  {
    ReportError("ERROR: Couldn't load file \"%s\" for level: %s", filename.GetData(), levelname.GetData());
    return false;
  }
  VFS->PopDir();
  
  // create physical world from render world
  Collision2Helper::InitializeCollisionObjects(physicalSystem, engine, convexDecomp ? convexDecomposer : nullptr);

  // Set default sector
  room = engine->GetSectors()->Get(0);

  CS_ASSERT(room && "Invalid level - Has no sectors.");

  printf ("Done - level loaded: %s\n", levelname.GetData());
  return true;
}


iModifiableDataFeeder* PhysDemo::GetFirstTerrainModDataFeeder(CS::Collisions::iCollisionSector* sector)
{
  // iterate over all terrains in the sector
  for (size_t i = 0; i < sector->GetCollisionTerrainCount(); ++i)
  {
    iTerrainSystem* terrain = sector->GetCollisionTerrain(i)->GetTerrain();
    csRef<iMeshObject> terrainObj = scfQueryInterface<iMeshObject>(terrain);
    if (terrainObj)
    {
      // Get the factory
      csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory>(terrainObj->GetFactory());
      CS_ASSERT(factory);

      // Get the data feeder and check if its modifiable
      // NOTE: By default, data feeders are not modifiable
      csRef<iModifiableDataFeeder> terrainFeeder = scfQueryInterface<iModifiableDataFeeder> (factory->GetFeeder());
      if (terrainFeeder)
      {
        return terrainFeeder;
      }
    }
  }
  return nullptr;
}