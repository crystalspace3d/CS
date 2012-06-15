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


using namespace CS::Physics;
using namespace CS::Collisions;

void PhysDemo::CreateBoxRoom()
{
  printf ("Loading box level...\n");

  // Default behavior from DemoApplication for the creation of the scene
  if (!DemoApplication::CreateRoom())
    return;

  // First we make a primitive for our geometry.
  using namespace CS::Geometry;
  DensityTextureMapper mapper (0.3f);
  TesselatedBox box (csVector3 (-5, -5, -5), csVector3 (5, 5, 5));
  box.SetLevel (3);
  box.SetMapper (&mapper);
  box.SetFlags (Primitives::CS_PRIMBOX_INSIDE);

  // Now we make a factory and a mesh at once.
  walls = GeneralMeshBuilder::CreateFactoryAndMesh (engine, room, "walls", "walls_factory", &box);

  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
  {
    ReportWarning ("Could not load texture %s", CS::Quote::Single ("stone"));
  }
  iMaterialWrapper* tm = engine->GetMaterialList()->FindByName ("stone");
  walls->GetMeshObject()->SetMaterialWrapper (tm);
/*
  // This works too, but doesn't behave well with soft bodies
  csRef<CS::Collisions::iCollisionObject> co = collisionSystem->CreateCollisionObject();
  csRef<CS::Collisions::iCollider> collider = collisionSystem->CreateColliderConcaveMesh (walls);
  co->AddCollider (collider, localTrans);
  co->RebuildObject();
  collisionSector->AddCollisionObject (co);
*/

  csOrthoTransform t;

  csVector3 size (10.0f, 10.0f, 10.0f); // This should be the same size as the mesh.
  t.SetOrigin(csVector3(10.0f,0.0f,0.0f));

  // Just to make sure everything works we create half of the colliders
  // using dynsys->CreateCollider() and the other half using
  // dynsys->AttachColliderBox().
  csRef<CS::Physics::iRigidBody> co = CreateStaticRigidBody("boxroom1");

  csRef<CS::Collisions::iColliderBox> collider = collisionSystem->CreateColliderBox (size);
  co->AddCollider (collider, localTrans);
  co->SetTransform (t);
  co->RebuildObject();
  collisionSector->AddCollisionObject (co);
  
  co = CreateStaticRigidBody("boxroom2");
  t.SetOrigin(csVector3(-10.0f, 0.0f, 0.0f));
  collider = collisionSystem->CreateColliderBox (size);
  co->AddCollider (collider, localTrans);
  co->SetTransform (t);
  co->RebuildObject();
  collisionSector->AddCollisionObject (co);
  
  co = CreateStaticRigidBody("boxroom3");
  t.SetOrigin(csVector3(0.0f, 10.0f, 0.0f));
  collider = collisionSystem->CreateColliderBox (size);
  co->AddCollider (collider, localTrans);
  co->SetTransform (t);
  co->RebuildObject();
  collisionSector->AddCollisionObject (co);

  // If we use the Bullet plugin, then use a plane collider for the floor
  // Also, soft bodies don't work well with planes, so use a box in this case
  //csRef<CS::Collisions::iColliderPlane> planeCollider = 
  //  collisionSystem->CreateColliderPlane (csPlane3 (
  //  csVector3 (0.0f, 1.0f, 0.0f), -5.0f));
  //csRef<CS::Physics::iRigidBody> floorBody = physicalSystem->CreateRigidBody();
  //floorBody->AddCollider (planeCollider, localTrans);
  //floorBody->SetFriction (10.0f);
  //floorBody->SetElasticity (0.0f);
  //floorBody->RebuildObject();
  //collisionSector->AddCollisionObject (floorBody);
  ////You should set the state after the body is added to a sector.
  //floorBody->SetState (CS::Physics::STATE_STATIC);
  csRef<CS::Physics::iRigidBody> rb;
  
  rb = CreateStaticRigidBody("boxroom4");
  t.SetOrigin(csVector3(0.0f, -10.0f, 0.0f));
  collider = collisionSystem->CreateColliderBox (size);
  rb->AddCollider (collider, localTrans);
  rb->SetTransform (t);
  rb->SetFriction (10.0f);
  rb->SetElasticity (0.0f);
  rb->RebuildObject();
  collisionSector->AddCollisionObject (rb);

  rb = CreateStaticRigidBody("boxroom5");
  t.SetOrigin(csVector3(0.0f, 0.0f, 10.0f));
  collider = collisionSystem->CreateColliderBox (size);
  rb->AddCollider (collider, localTrans);
  rb->SetTransform (t);
  rb->SetFriction (10.0f);
  rb->SetElasticity (0.0f);
  rb->RebuildObject();
  collisionSector->AddCollisionObject (rb);
  
  rb = CreateStaticRigidBody("boxroom6");
  t.SetOrigin(csVector3(0.0f, 0.0f, -10.0f));
  collider = collisionSystem->CreateColliderBox (size);
  rb->AddCollider (collider, localTrans);
  rb->SetTransform (t);
  rb->RebuildObject();
  collisionSector->AddCollisionObject (rb);

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

  engine->Prepare();
  CS::Lighting::SimpleStaticLighter::ShineLights (room, engine, 4);
}

void PhysDemo::CreatePortalRoom()
{
  printf ("Loading portal level...\n");

  // Default behavior from DemoApplication for the creation of the scene
  if (!DemoApplication::CreateRoom())
    return;

  // Create the box mesh of the room
  using namespace CS::Geometry;
  DensityTextureMapper mapper (0.3f);
  TesselatedBox box (csVector3 (-5, -5, -5), csVector3 (5, 5, 5));
  box.SetLevel (3);
  box.SetMapper (&mapper);
  box.SetFlags (Primitives::CS_PRIMBOX_INSIDE);

  walls = GeneralMeshBuilder::CreateFactoryAndMesh (
    engine, room, "walls", "walls_factory", &box);

  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
    ReportWarning ("Could not load texture %s",
    CS::Quote::Single ("stone"));
  iMaterialWrapper* tm = engine->GetMaterialList()->FindByName ("stone");
  walls->GetMeshObject()->SetMaterialWrapper (tm);

  // Create the colliders of the room  
  csVector3 size (10.0f, 10.0f, 10.0f);
  csOrthoTransform transform;

  csRef<CS::Physics::iRigidBody> terrainBody = CreateStaticRigidBody("portalroom");
  csRef<CS::Collisions::iColliderBox> collider;

  // Right wall (with a portal inside)
  collider = collisionSystem->CreateColliderBox (csVector3 (4.0f, 2.0f, 4.0f));
  transform.SetOrigin (csVector3 (7.0f, -4.0f, -3.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (7.0f, -4.0f, 3.0f));
  terrainBody->AddCollider (collider, transform);

  collider = collisionSystem->CreateColliderBox (csVector3 (4.0f, 8.0f, 10.0f));
  transform.SetOrigin (csVector3 (7.0f, 1.0f, 0.0f));
  terrainBody->AddCollider (collider, transform);

  // Floor (with a portal inside)
  collider = collisionSystem->CreateColliderBox (csVector3 (6.0f, 4.0f, 4.0f));
  transform.SetOrigin (csVector3 (-2.0f, -7.0f, 3.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (2.0f, -7.0f, -3.0f));
  terrainBody->AddCollider (collider, transform);

  collider = collisionSystem->CreateColliderBox (csVector3 (4.0f, 4.0f, 6.0f));
  transform.SetOrigin (csVector3 (3.0f, -7.0f, 2.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (-3.0f, -7.0f, -2.0f));
  terrainBody->AddCollider (collider, transform);

  // Remaining walls
  collider = collisionSystem->CreateColliderBox (size);
  transform.SetOrigin (csVector3 (0.0f, 0.0f, 10.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (0.0f, 0.0f, -10.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (-10.0f, 0.0f, 0.0f));
  terrainBody->AddCollider (collider, transform);

  transform.SetOrigin (csVector3 (0.0f, 10.0f, 0.0f));
  terrainBody->AddCollider (collider, transform);

  terrainBody->RebuildObject();
  collisionSector->AddCollisionObject (terrainBody);

  // Debug the identity transform
  CS::Debug::VisualDebuggerHelper::DebugTransform (GetObjectRegistry(), csOrthoTransform(), true);

  // Create the portal on the right wall
  csPoly3D poly;
  poly.AddVertex (csVector3 (0.0f, -1.0f, 1.0f));
  poly.AddVertex (csVector3 (0.0f, 1.0f, 1.0f));
  poly.AddVertex (csVector3 (0.0f, 1.0f, -1.0f));
  poly.AddVertex (csVector3 (0.0f, -1.0f, -1.0f));
  iPortal* portal;
  csRef<iMeshWrapper> portalMesh =
    engine->CreatePortal ("right_wall", room, csVector3 (4.999f, -4.0f, 0.0f),
			  room, poly.GetVertices(), (int) poly.GetVertexCount(),
			  portal);
  portal->GetFlags().Set (CS_PORTAL_ZFILL);
  portal->GetFlags().Set (CS_PORTAL_CLIPDEST);
  portal->SetWarp (csOrthoTransform (csYRotMatrix3 (-PI * 0.5f) * csZRotMatrix3 (PI * 0.5f),
				     csVector3 (1.0f, 0.0f, 4.999f)));

  // Create a collision portal
  //collisionSector->AddPortal (portal, portalMesh->GetMovable()->GetFullTransform());

  // Debug the inverse of the warp transform
  CS::Debug::VisualDebuggerHelper::DebugTransform
    (GetObjectRegistry(), (csOrthoTransform (csYRotMatrix3 (-PI * 0.5f) * csZRotMatrix3 (PI * 0.5f),
					      csVector3 (1.0f, 0.0f, 4.999f))).GetInverse(), true);

  // Create the portal on the floor
  csPoly3D poly2;
  poly2.AddVertex (csVector3 (-1.0f, 0.0f, 1.0f));
  poly2.AddVertex (csVector3 (1.0f, 0.0f, 1.0f));
  poly2.AddVertex (csVector3 (1.0f, 0.0f, -1.0f));
  poly2.AddVertex (csVector3 (-1.0f, 0.0f, -1.0f));
  csRef<iMeshWrapper> portalMesh2 =
    engine->CreatePortal ("floor", room, csVector3 (0.0f, -4.999f, 0.0f),
			  room, poly2.GetVertices(), (int) poly2.GetVertexCount(),
			  portal);
  portal->GetFlags().Set (CS_PORTAL_ZFILL);
  portal->GetFlags().Set (CS_PORTAL_CLIPDEST);
  portal->SetWarp (csOrthoTransform (csZRotMatrix3 (-PI * 0.5f) * csYRotMatrix3 (PI * 0.5f),
				     csVector3 (0.0f, -4.999f, -1.0f)));

  // Create a collision portal
  //collisionSector->AddPortal (portal, portalMesh2->GetMovable()->GetFullTransform());

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

  engine->Prepare();
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
  engine->Prepare();

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

  // Create a terrain collider
  csRef<iColliderTerrain> terrainCollider = collisionSystem->CreateColliderTerrain (terrain);
  
  // TODO: Consider creating a terrain CollisionObject class instead
  csRef<CS::Physics::iRigidBody> co = physicalSystem->CreateStaticRigidBody();
  co->AddCollider (terrainCollider, localTrans);
  co->RebuildObject();
  collisionSector->AddCollisionObject (co);
}