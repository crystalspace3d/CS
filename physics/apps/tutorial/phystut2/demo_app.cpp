#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

PhysDemo::PhysDemo()
  : DemoApplication ("CrystalSpace.PhysTut2"),
    isSoftBodyWorld (true), solver (0), do_bullet_debug (false),
    do_soft_debug (false), remainingStepDuration (0.0f), allStatic (false), 
    pauseDynamic (false), dynamicSpeed (1.0f),
    debugMode (CS::Physics::Bullet2::DEBUG_COLLIDERS),
    physicalCameraMode (CAMERA_DYNAMIC), dragging (false), softDragging (false)
{
  localTrans.Identity();
}

PhysDemo::~PhysDemo()
{
}

bool PhysDemo::OnInitialize (int argc, char* argv[])
{
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  csBaseEventHandler::Initialize (GetObjectRegistry());
  if (!RegisterQueue (GetObjectRegistry(), csevAllEvents (GetObjectRegistry())))
    return ReportError ("Failed to set up event handler!");

  // Checking for choosen dynamic system
  csRef<iCommandLineParser> clp = csQueryRegistry<iCommandLineParser> (GetObjectRegistry());
  phys_engine_name = clp->GetOption ("phys_engine");
  
  phys_engine_name = "Bullet";
  csRef<iPluginManager> plugmgr = 
    csQueryRegistry<iPluginManager> (GetObjectRegistry());
  collisionSystem = csLoadPlugin<CS::Collisions::iCollisionSystem> (plugmgr, "crystalspace.physics.bullet2");
  physicalSystem = scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);

  // We have some objects of size smaller than 0.035 units, so we scale up the
  // whole world for a better behavior of the dynamic simulation.
  collisionSystem->SetInternalScale (10.0f);

  // Check whether the soft bodies are enabled or not
  isSoftBodyWorld = clp->GetBoolOption ("soft", true);

  // Load the soft body animation control plugin & factory
  if (isSoftBodyWorld)
  {
    softBodyAnimationType = csLoadPlugin<CS::Physics::iSoftBodyAnimationControlType>(plugmgr, "crystalspace.physics.softanim2");
    if (!softBodyAnimationType)
      return ReportError ("Could not load soft body animation for genmeshes plugin!");

    softBodyAnimationFactory = softBodyAnimationType->CreateAnimationControlFactory();
  }

  // Load the ragdoll plugin
  ragdollManager = csLoadPlugin<CS::Animation::iSkeletonRagdollNodeManager2>
    (plugmgr, "crystalspace.mesh.animesh.animnode.ragdoll2");
  if (!ragdollManager)
    return ReportError ("Failed to locate ragdoll manager!");

  // Check which environment has to be loaded
  csString levelName = clp->GetOption ("level");
  environment = GetEnvironmentByName(levelName);
  //csString defaultEnvironmentName = "terrain";
  csString defaultEnvironmentName = "portals";
  if (!environment)
  {
      csPrintf ("Given level (%s) is not one of {%s, %s, %s}. Falling back to \"%s\"\n",
          CS::Quote::Single (levelName.GetData()),
          CS::Quote::Single ("portals"),
          CS::Quote::Single ("box"),
          CS::Quote::Single ("terrain"),
          defaultEnvironmentName.GetData());
      //environment = ENVIRONMENT_PORTALS;
      environment = GetEnvironmentByName(defaultEnvironmentName);
  }

  if (!collisionSystem)
    return ReportError ("No bullet system plugin!");

  return true;
}


bool PhysDemo::Application()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application())
    return false;

  // Create the dynamic system
  collisionSector = collisionSystem->CreateCollisionSector();
  if (!collisionSector) return ReportError ("Error creating collision sector!");
  physicalSector = scfQueryInterface<CS::Physics::iPhysicalSector> (collisionSector);

  // Set some linear and angular dampening in order to have a reduction of
  // the movements of the objects
  physicalSector->SetLinearDampener(0.1f);
  physicalSector->SetRollingDampener(0.1f);

  // Enable soft bodies
  if (isSoftBodyWorld)
    physicalSector->SetSoftBodyEnabled (true);

  bulletSector = scfQueryInterface<CS::Physics::Bullet2::iPhysicalSector> (physicalSector);
  bulletSector->SetDebugMode (debugMode);

  // Create the environment
  switch (environment)
  {
  case ENVIRONMENT_PORTALS:
    CreatePortalRoom();
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 0, -3));
    break;
    
  case ENVIRONMENT_BOX:
    CreateBoxRoom();
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 0, -3));
    break;
    
  case ENVIRONMENT_TERRAIN:
    CreateTerrainRoom();
    //view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 30, -3));
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 5, -3));
    break;

  default:
    break;
  }

  collisionSector->SetSector (room);

  collisionSector->CreateCollisionGroup ("Box");
  collisionSector->CreateCollisionGroup ("BoxFiltered");

  bool coll = collisionSector->GetGroupCollision ("Box", "BoxFiltered");
  if (coll)
    collisionSector->SetGroupCollision ("Box", "BoxFiltered", false);

  // Preload some meshes and materials
  if (!loader->LoadTexture ("spark", "/lib/std/spark.png")) return ReportError ("Error loading texture: spark");
  if (!loader->LoadTexture ("raindrop", "/lib/std/raindrop.png")) return ReportError ("Error loading texture: raindrop");

  // Load the box mesh factory.
  boxFact = loader->LoadMeshObjectFactory ("/lib/std/sprite1");
  if (!boxFact) return ReportError ("Error loading mesh object factory!");

  // Double the size.
  csMatrix3 m; m *= .5;
  csReversibleTransform t = csReversibleTransform (m, csVector3 (0));
  boxFact->HardTransform (t);

  // Load the mesh factory.
  meshFact = loader->LoadMeshObjectFactory ("/varia/physmesh");
  if (!meshFact) return ReportError ("Error loading mesh object factory!");

  // Disable the camera manager
  cameraManager->SetCameraMode (CS::Utility::CAMERA_NO_MOVE);
  cameraManager->SetMouseMoveEnabled (false);

  // Initialize the camera
  UpdateCameraMode();

  CreateGhostCylinder();

  // Initialize the HUD manager
  hudManager->GetKeyDescriptions()->Empty();
  hudManager->GetKeyDescriptions()->Push ("b: spawn a box");
  hudManager->GetKeyDescriptions()->Push ("s: spawn a sphere");
  
  hudManager->GetKeyDescriptions()->Push ("c: spawn a cylinder");
  hudManager->GetKeyDescriptions()->Push ("a: spawn a capsule");
  hudManager->GetKeyDescriptions()->Push ("n: spawn a cone");
 
  hudManager->GetKeyDescriptions()->Push ("v: spawn a convex mesh");
  hudManager->GetKeyDescriptions()->Push ("m: spawn a static concave mesh");
  
  hudManager->GetKeyDescriptions()->Push ("q: spawn a compound body");
  hudManager->GetKeyDescriptions()->Push ("j: spawn a joint");
  hudManager->GetKeyDescriptions()->Push ("k: spawn a filter body");
  
  hudManager->GetKeyDescriptions()->Push ("h: spawn a chain");
  hudManager->GetKeyDescriptions()->Push ("r: spawn a Frankie's ragdoll");
  hudManager->GetKeyDescriptions()->Push ("e: spawn a Krystal's ragdoll");

  if (isSoftBodyWorld)
  {
    hudManager->GetKeyDescriptions()->Push ("y: spawn a rope");
    hudManager->GetKeyDescriptions()->Push ("u: spawn a cloth");
    hudManager->GetKeyDescriptions()->Push ("i: spawn a soft body");
  }
  hudManager->GetKeyDescriptions()->Push ("SPACE: spawn random object");
  
  hudManager->GetKeyDescriptions()->Push ("left mouse: fire!");
  hudManager->GetKeyDescriptions()->Push ("right mouse: drag object");
  hudManager->GetKeyDescriptions()->Push ("CTRL-x: cut selected object");
  hudManager->GetKeyDescriptions()->Push ("CTRL-v: paste object");

  hudManager->GetKeyDescriptions()->Push ("f: toggle camera modes");
  hudManager->GetKeyDescriptions()->Push ("t: toggle all bodies dynamic/static");
  hudManager->GetKeyDescriptions()->Push ("p: pause the simulation");
  hudManager->GetKeyDescriptions()->Push ("o: toggle speed of simulation");
  hudManager->GetKeyDescriptions()->Push ("d: toggle Bullet debug display");
  
  hudManager->GetKeyDescriptions()->Push ("?: toggle display of collisions");
  hudManager->GetKeyDescriptions()->Push ("g: toggle gravity");
/*  
#ifdef CS_HAVE_BULLET_SERIALIZER
  if (phys_engine_id == BULLET_ID)
    hudManager->GetKeyDescriptions()->Push ("CTRL-s: save the dynamic world");
#endif
*/
  /*
  if (phys_engine_id == BULLET_ID)
    hudManager->GetKeyDescriptions()->Push ("CTRL-n: next environment");
  */
  
  hudManager->GetKeyDescriptions()->Push ("CTRL-i: start profiling");
  hudManager->GetKeyDescriptions()->Push ("CTRL-o: stop profiling");
  hudManager->GetKeyDescriptions()->Push ("CTRL-p: dump profile");
  
  // Pre-load the animated mesh and the ragdoll animation node data
  LoadFrankieRagdoll();
  LoadKrystalRagdoll();

  // Run the application
  Run();

  return true;
}



void PhysDemo::GripContactBodies()
{
  size_t count = ghostObject->GetContactObjectsCount();
  for (size_t i = 0; i < count; i++)
  {
    CS::Physics::iPhysicalBody* pb = ghostObject->GetContactObject (i)->QueryPhysicalBody();
    if (pb)
    {
      if (pb->GetBodyType() == CS::Physics::BODY_RIGID)
      {
        CS::Physics::iRigidBody* rb = pb->QueryRigidBody();
        csVector3 velo = pb->GetLinearVelocity();
        velo = - velo;
        //rb->Disable();
        rb->SetLinearVelocity (csVector3 (.0f,.0f,.0f));
        rb->SetAngularVelocity (csVector3 (.0f,.0f,.0f));
      }
      else
      {
        CS::Physics::iSoftBody* sb = pb->QuerySoftBody();
	sb->SetLinearVelocity (csVector3 (.0f,.0f,.0f));
        //sb->SetLinearVelocity (csVector3 (0,0,-1.0f));
      }
    }
  }
}


//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

int main (int argc, char* argv[])
{
  return PhysDemo().Main(argc, argv);
}
