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

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

void PhysDemo::CreatePortalRoom()
{
  // Parameters to use for customizing the room
  // Room is the inside of a cuboid of the given size, and wall thickness
  static const csScalar WallThickness = 5;
  static const csVector3 RoomExtents(20);

  printf ("Creating portal room...\n");

  // Default behavior from DemoApplication for the creation of the scene
  if (!DemoApplication::CreateRoom())
    return;

  // The boxes that make up floor and ceiling
  // AABB of these is the entire room
  csVector3 yExtents(
    RoomExtents[HorizontalAxis1] + 2 * WallThickness,
    WallThickness,
    RoomExtents[HorizontalAxis2] + 2 * WallThickness);
  RenderMeshColliderPair yLimitingPair;
  CreateBoxMeshColliderPair(yLimitingPair, yExtents);

  // The boxes that make up one pair of walls
  // AABB of these is the y-limited part of the room
  csVector3 xExtents(
    WallThickness,
    RoomExtents[HorizontalAxis1],
    RoomExtents[HorizontalAxis2] + 2 * WallThickness);
  RenderMeshColliderPair xLimitingPair;
  CreateBoxMeshColliderPair(xLimitingPair, xExtents);

  // The boxes that make up the other pair of walls
  // AABB of these is the y- and x- limited part of the room
  csVector3 zExtents(
    RoomExtents[HorizontalAxis2],
    RoomExtents[HorizontalAxis1],
    WallThickness);
  RenderMeshColliderPair zLimitingPair;
  CreateBoxMeshColliderPair(zLimitingPair, zExtents);
  
  // TODO: Create box objects and allow for their hole digging for a portal and restoring of original shape after portal is gone
  
  csVector3 halfRoomExtents = RoomExtents / 2;
  csVector3 distances = (RoomExtents + csVector3(WallThickness)) / 2;
  SpawnRigidBody(xLimitingPair, csVector3(distances[HorizontalAxis1], 0, 0), "wall", 0, 0); 
  SpawnRigidBody(xLimitingPair, csVector3(-distances[HorizontalAxis1], 0, 0), "wall", 0, 0);

  SpawnRigidBody(yLimitingPair, csVector3(0, distances[UpAxis], 0), "ceiling", 0, 0);
  SpawnRigidBody(yLimitingPair, csVector3(0, -distances[UpAxis], 0), "floor", 15, 0);

  SpawnRigidBody(zLimitingPair, csVector3(0, 0, distances[HorizontalAxis2]), "wall", 0, 0);
  SpawnRigidBody(zLimitingPair, csVector3(0, 0, -distances[HorizontalAxis2]), "wall", 0, 0);

  // Debug the identity transform
  CS::Debug::VisualDebuggerHelper::DebugTransform (GetObjectRegistry(), csOrthoTransform(), true);
  
  
  // Add portals
  // TODO: Add mechanism to more easily create a portal pair

  csScalar portalEpsilon = csScalar(0.01);
  csVector2 halfPortalExtents(2, 2); halfPortalExtents /= 2;      // a portal has size 2 x 2

  csOrthoTransform portal1Trans;
  portal1Trans.SetOrigin(
    csVector3(halfRoomExtents.x - portalEpsilon, -halfRoomExtents.y + halfPortalExtents.y, 0.0f));
  csOrthoTransform portal2Trans(
    csYRotMatrix3 (-HALF_PI) * csZRotMatrix3 (HALF_PI),
    csVector3(0, -halfRoomExtents.y - 2 , 0));

  // Define the plane of the first poly
  csPoly3D poly;
  poly.AddVertex (csVector3 (0.0f, -halfPortalExtents.y, halfPortalExtents.x));
  poly.AddVertex (csVector3 (0.0f, halfPortalExtents.y, halfPortalExtents.x));
  poly.AddVertex (csVector3 (0.0f, halfPortalExtents.y, -halfPortalExtents.x));
  poly.AddVertex (csVector3 (0.0f, -halfPortalExtents.y, -halfPortalExtents.x));
  
  iPortal* portal;
  csRef<iMeshWrapper> portalMesh = engine->CreatePortal ("right_wall", 
    room,
    csVector3(0),
    room, 
    poly.GetVertices(), 
    (int) poly.GetVertexCount(),
    portal);

  portalMesh->QuerySceneNode()->GetMovable()->SetTransform(portal1Trans);
  
  portal->GetFlags().Set (CS_PORTAL_ZFILL);
  portal->GetFlags().Set (CS_PORTAL_CLIPDEST);
  portal->SetWarp (portal2Trans);

  // Create a collision portal
  //physicalSector->AddPortal (portal, portalMesh->GetMovable()->GetFullTransform());

  // Debug the inverse of the warp transform
  CS::Debug::VisualDebuggerHelper::DebugTransform
    (GetObjectRegistry(), portal2Trans.GetInverse(), true);

  // Create the portal on the floor
  csRef<iMeshWrapper> portalMesh2 = engine->CreatePortal ("floor", 
    room,
    csVector3(0),
    room, 
    poly.GetVertices(), 
    (int) poly.GetVertexCount(),
    portal);
  
  portalMesh2->QuerySceneNode()->GetMovable()->SetTransform(portal2Trans);

  portal->GetFlags().Set (CS_PORTAL_ZFILL);
  portal->GetFlags().Set (CS_PORTAL_CLIPDEST);
  portal->SetWarp (portal1Trans);

  // Create a collision portal
  //physicalSector->AddPortal (portal, portalMesh2->GetMovable()->GetFullTransform());

  // Debug the inverse of the warp transform
  CS::Debug::VisualDebuggerHelper::DebugTransform
    (GetObjectRegistry(), (csOrthoTransform (csZRotMatrix3 (-PI * 0.5f) * csYRotMatrix3 (PI * 0.5f),
					      csVector3 (0.0f, -4.999f, -1.0f))).GetInverse(), true);

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

void PhysDemo::CreateTerrainRoom()
{
  printf ("Loading terrain level...\n");

  // Load the level file
  csRef<iVFS> VFS (csQueryRegistry<iVFS> (GetObjectRegistry()));
  VFS->ChDir ("/lev/terraini");

  if (!loader->LoadMapFile ("worldmod"))
  //if (!loader->LoadMapFile ("world"))
  {
    ReportError("Error couldn't load terrain level!");
    return;
  }

  // Setup the sector
  room = engine->FindSector ("room");
  view->GetCamera()->SetSector (room);

  // Find the terrain mesh
  csRef<iMeshWrapper> terrainWrapper = engine->FindMeshObject ("Terrain");
  if (!terrainWrapper)
  {
    ReportError("Error cannot find the terrain mesh!");
    return;
  }
  
  csRef<iTerrainSystem> terrain = scfQueryInterface<iTerrainSystem> (terrainWrapper->GetMeshObject());
  if (!terrain)
  {
    ReportError("Error cannot find the terrain interface!");
    return;
  }
  
  csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory>(terrainWrapper->GetFactory()->GetMeshObjectFactory());
  CS_ASSERT(factory);

  terrainFeeder = scfQueryInterface<iModifiableDataFeeder> (factory->GetFeeder());
  if (!terrainFeeder)
  {
    ReportError("Warning: Terrain is not modifiable");
  }

  // Create a collision heightfield
  csRef<iCollisionTerrain> colTerrain = physicalSystem->CreateCollisionTerrain (terrain);
  
  physicalSector->AddCollisionTerrain(colTerrain);
}