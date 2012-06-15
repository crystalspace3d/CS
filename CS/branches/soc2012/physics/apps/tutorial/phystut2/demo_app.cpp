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

PhysDemo::PhysDemo()
  : DemoApplication ("CrystalSpace.PhysTut2"),
    isSoftBodyWorld (true), solver (0), do_bullet_debug (false),
    do_soft_debug (false), remainingStepDuration (0.0f), allStatic (false), 
    pauseDynamic (false), dynamicSpeed (1.0f),
    debugMode (Bullet2::DEBUG_COLLIDERS),
    dragging (false), softDragging (false),
    physicalCameraMode (CAMERA_ACTOR)
    //physicalCameraMode (CAMERA_DYNAMIC)
{
  localTrans.Identity();
  actorSpeed = 5;
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
  physicalSystem = scfQueryInterface<iPhysicalSystem> (collisionSystem);

  collisionSystem->SetInternalScale (1.0f);

  // Check whether the soft bodies are enabled or not
  isSoftBodyWorld = clp->GetBoolOption ("soft", true);

  // Load the soft body animation control plugin & factory
  if (isSoftBodyWorld)
  {
    softBodyAnimationType = csLoadPlugin<iSoftBodyAnimationControlType>(plugmgr, "crystalspace.physics.softanim2");
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
  physicalSector = scfQueryInterface<iPhysicalSector> (collisionSector);

  // Set some linear and angular dampening in order to have a reduction of
  // the movements of the objects
  physicalSector->SetLinearDampener(0.1f);
  physicalSector->SetRollingDampener(0.1f);

  // Enable soft bodies
  if (isSoftBodyWorld)
    physicalSector->SetSoftBodyEnabled (true);

  bulletSector = scfQueryInterface<Bullet2::iPhysicalSector> (physicalSector);
  bulletSector->SetDebugMode (debugMode);
  collisionSector->SetGravity(0);

  // Create the environment
  switch (environment)
  {
  case ENVIRONMENT_PORTALS:
    CreatePortalRoom();
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 0, -5));
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
  hudManager->GetKeyDescriptions()->Push ("r: reset");

  //hudManager->GetKeyDescriptions()->Push ("b: spawn a box");
  //hudManager->GetKeyDescriptions()->Push ("a: spawn a capsule");
  hudManager->GetKeyDescriptions()->Push ("c: spawn a cylinder");
  hudManager->GetKeyDescriptions()->Push ("n: spawn a cone");
 
  hudManager->GetKeyDescriptions()->Push ("v: spawn a sphere");
  //hudManager->GetKeyDescriptions()->Push ("v: spawn a convex mesh");
  hudManager->GetKeyDescriptions()->Push ("m: spawn a static concave mesh");
  
  hudManager->GetKeyDescriptions()->Push ("q: spawn a compound body");
  hudManager->GetKeyDescriptions()->Push ("j: spawn a joint");
  hudManager->GetKeyDescriptions()->Push ("k: spawn a filter body");
  
  hudManager->GetKeyDescriptions()->Push ("h: spawn a chain");
  hudManager->GetKeyDescriptions()->Push ("e: spawn a Krystal's ragdoll");
  hudManager->GetKeyDescriptions()->Push ("': spawn a Frankie's ragdoll");

  if (isSoftBodyWorld)
  {
    hudManager->GetKeyDescriptions()->Push ("y: spawn a rope");
    hudManager->GetKeyDescriptions()->Push ("u: spawn a cloth");
    hudManager->GetKeyDescriptions()->Push ("i: spawn a soft body");
  }
  hudManager->GetKeyDescriptions()->Push ("SPACE: Jump");
  
  hudManager->GetKeyDescriptions()->Push ("left mouse: fire!");
  hudManager->GetKeyDescriptions()->Push ("right mouse: drag object");
  hudManager->GetKeyDescriptions()->Push ("CTRL-x: cut selected object");
  hudManager->GetKeyDescriptions()->Push ("CTRL-v: paste object");

  hudManager->GetKeyDescriptions()->Push ("f: toggle camera modes");
  hudManager->GetKeyDescriptions()->Push ("t: toggle all bodies dynamic/static");
  hudManager->GetKeyDescriptions()->Push ("p: pause the simulation");
  hudManager->GetKeyDescriptions()->Push ("o: toggle speed of simulation");
  hudManager->GetKeyDescriptions()->Push ("l: toggle Bullet debug display");
  
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
  

  // Run the application
  Run();

  return true;
}



void PhysDemo::GripContactBodies()
{
  size_t count = ghostObject->GetContactObjectsCount();
  for (size_t i = 0; i < count; i++)
  {
    iPhysicalBody* pb = ghostObject->GetContactObject (i)->QueryPhysicalBody();
    if (pb && pb->GetObjectType() == COLLISION_OBJECT_PHYSICAL_DYNAMIC)
    {
      if (pb->GetBodyType() == BODY_RIGID)
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
        iSoftBody* sb = pb->QuerySoftBody();
	sb->SetLinearVelocity (csVector3 (.0f,.0f,.0f));
        //sb->SetLinearVelocity (csVector3 (0,0,-1.0f));
      }
    }
  }
}

void PhysDemo::UpdateCameraMode()
{
  switch (physicalCameraMode)
  {
    // The camera is controlled by a rigid body
  case CAMERA_DYNAMIC:
    {
      const csOrthoTransform& tc = view->GetCamera()->GetTransform();

      // Check if there is already a rigid body created for the 'kinematic' mode
      if (cameraBody)
      {
        cameraBody->SetState (STATE_DYNAMIC);

        // Remove the attached camera (in this mode we want to control
        // the orientation of the camera, so we update the camera
        // position by ourselves)
        cameraBody->SetAttachedCamera (0);

        cameraBody->SetTransform (tc);
      }

      // Create a new rigid body
      else
      {
        csRef<CS::Collisions::iColliderBox> sphere = //collisionSystem->CreateColliderSphere (ActorDimensions.x);
          collisionSystem->CreateColliderBox(ActorDimensions);
        cameraBody = CreateRigidBody("Camera");
        cameraBody->SetDensity(0.3f);
        cameraBody->SetElasticity(0.8f);
        cameraBody->SetFriction(100.0f);
        
        cameraBody->AddCollider(sphere, localTrans);
        cameraBody->RebuildObject();


        cameraBody->SetTransform (tc);

        collisionSector->AddCollisionObject(cameraBody);
      }

      break;
    }

    // The camera is free
  case CAMERA_FREE:
    {
      if (cameraBody)
      {
        collisionSector->AddCollisionObject(cameraBody);
        cameraBody = NULL;
      }

      // Update the display of the dynamics debugger
      //dynamicsDebugger->UpdateDisplay();

      break;
    }

  case CAMERA_ACTOR:
    {
     
      csRef<CS::Collisions::iColliderBox> actorCollider = //collisionSystem->CreateColliderSphere (ActorDimensions.x);
        collisionSystem->CreateColliderBox(ActorDimensions);

      cameraActor = collisionSystem->CreateCollisionActor(actorCollider);
      cameraActor->QueryObject()->SetName("actor");
      cameraActor->SetAttachedCamera(view->GetCamera());
      cameraActor->RebuildObject();

      collisionSector->AddCollisionObject(cameraActor);
      
      cameraActor->SetJumpSpeed(actorSpeed);
      
    }
    break;

    // The camera is kinematic
  case CAMERA_KINEMATIC:
    {
      if (cameraActor)
      {
        collisionSector->RemoveCollisionObject(cameraActor);
      }
      // Create a body
      if (!cameraBody)
      {
        csRef<CS::Collisions::iColliderSphere> sphere = collisionSystem->CreateColliderSphere (0.8f);
        csOrthoTransform localTrans;
        cameraBody = physicalSystem->CreateRigidBody();
        cameraBody->AddCollider(sphere, localTrans);

        cameraBody->SetDensity (1.0f);
        cameraBody->SetElasticity (0.8f);
        cameraBody->SetFriction (100.0f);
        cameraBody->RebuildObject();
        
        const csOrthoTransform& tc = view->GetCamera()->GetTransform();
        cameraBody->SetTransform (tc);
      }

      // Attach the camera to the body so as to benefit of the default
      // kinematic callback
      cameraBody->SetAttachedCamera (view->GetCamera());

      // Make it kinematic
      cameraBody->SetState (STATE_KINEMATIC);

      collisionSector->AddCollisionObject (cameraBody);

    }
    break;

  default:
    break;
  }
}



//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

int main (int argc, char* argv[])
{
  return PhysDemo().Main(argc, argv);
}
