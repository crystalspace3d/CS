#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"

#include "iengine/campos.h"

#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;

PhysDemo physDemo;

PhysDemo::PhysDemo()
  : DemoApplication ("CrystalSpace.PhysTut2"),
  isSoftBodyWorld (true), do_bullet_debug (false),
  do_soft_debug (false), remainingStepDuration (0.0f), allStatic (false), 
  pauseDynamic (false), dynamicStepFactor (1.0f),
  dragging (false), softDragging (false),
  debugMode (DEBUG_COLLIDERS),
  actorAirControl(.3f),
  moveSpeed(7.f),
  turnSpeed(2.f),
  selectedItem(nullptr),
  //actorMode (ActorModeKinematic)
  actorMode (ActorModeDynamic)
  ,
  cameraMode(CameraMode1stPerson),
  defaultEnvironmentName("box")
  //defaultEnvironmentName("terrain")
  //defaultEnvironmentName("portals")
{
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
  csRef<iPluginManager> plugmgr = csQueryRegistry<iPluginManager> (GetObjectRegistry());
  physicalSystem = csLoadPlugin<CS::Physics::iPhysicalSystem> (plugmgr, "crystalspace.physics.bullet2");

  if (!physicalSystem)
    return ReportError ("Could not load the bullet2 plugin!");

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
  if (!environment)
  {
    csPrintf ("Given level (%s) is invalid - Falling back to default: \"%s\"\n",
      CS::Quote::Single (levelName.GetData()),
      defaultEnvironmentName.GetData());
    //environment = PhysDemoLevelPortals;
    environment = GetEnvironmentByName(defaultEnvironmentName);
  }

  return true;
}


bool PhysDemo::Application()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application())
    return false;

  // Set camera
  cameraManager->SetCamera (view->GetCamera ());

  // Disable the camera manager
  cameraManager->SetCameraMode (CS::Utility::CAMERA_NO_MOVE);
  cameraManager->SetMouseMoveEnabled (false);

  // Initialize Player items
  CreateItemTemplates();
  for (size_t i = 0; i < ItemMgr::Instance->GetTemplateCount(); ++i)
  {
    ItemTemplate& templ = ItemMgr::Instance->GetTemplate(i);
    player.GetInventory().AddItem(templ);
  }

  // Scene setup
  if (SetLevel(environment))
  {
    // Run the application
    Run();

    return true;
  }
  return false;
}

void PhysDemo::Reset()
{
  // Remove all sectors from the physical system
  physicalSystem->DeleteAll();

  // Remove everything in the engine that existed before
  engine->DeleteAll();
  engine->ResetWorldSpecificSettings();
  engine->GetCameraPositions()->RemoveAll();

  // reset all other variables
  mainCollider = nullptr;

  stackBoxMeshPair.Collider = nullptr;
  stackBoxMeshPair.MeshFactory = nullptr;

  dragJoint = nullptr;
  draggedBody = nullptr;

  player.SetObject(nullptr);
  dynamicActor = nullptr;
  kinematicActor = nullptr;

  clipboardBody = nullptr;
  clipboardMovable = nullptr;
  
  ghostObject = nullptr;

  moddedTerrainFeeder = nullptr;
  terrainMod = nullptr;

  debugNameMap.DeleteAll();

  actorVehicle = nullptr;

  walls = nullptr;
}

bool PhysDemo::SetLevel(PhysDemoLevel level, bool concaveDecomp)
{
  environment = level;

  // Reset scene
  Reset();

  // Initialize the actor
  UpdateActorMode(actorMode);

  // Preload some meshes and materials
  if (!loader->LoadTexture ("raindrop", "/lib/std/raindrop.png")) return ReportError ("Error loading texture: raindrop");
  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif")) return ReportError ("Could not load texture: stone");
  if (!loader->LoadTexture ("objtexture", "/lib/std/blobby.jpg")) return ReportError ("Error loading texture: blobby");
  if (!loader->LoadTexture ("misty", "/lib/std/misty.jpg")) return ReportError ("Error loading texture: misty");

  // Create the environment
  bool worked = false;
  switch (environment)
  {
  case PhysDemoLevelBox:
    CreateBoxRoom();
    worked = true;
    break;

  case PhysDemoLevelPortals:
    worked = LoadLevel("/data/portals", "world", "Portals", concaveDecomp);
    break;

  case PhysDemoLevelTerrain:
    worked = LoadLevel("/lev/terraini", "worldmod", "Terrain", concaveDecomp);
    break;

  case PhysDemoLevelCastle:
    worked = LoadLevel("/data/castle", "world", "Castle", concaveDecomp);
    break;

  default:
    break;
  }

  if (!worked)
  {
    // fall back to default
    ReportWarning("Falling back to default level: Box room");
    CreateBoxRoom();
  }

  // Finalize stuff in the engine after scene setup
  engine->Prepare();

  // Update Camera Manager item
  UpdateCameraManager();

  // Initialize HUD
  SetupHUD();

  // Move actor to initial position
  TeleportObject(player.GetObject(), engine->GetCameraPositions()->Get(0));

  return true;
}

void PhysDemo::SetupHUD()
{
  // Setup the descriptions in the HUD
  iStringArray& desc = *hudManager->GetKeyDescriptions();
  desc.Empty();

  desc.Push("N: Next page");
  
  if (selectedItem && selectedItem->GetTemplate().GetPrimaryFunctions().GetSize())
  {
    ItemFunction* func;
    func = selectedItem->GetTemplate().GetPrimaryFunction(0);
    desc.Push (csString("Left Mouse Button: ") + func->GetName());
    func = selectedItem->GetTemplate().GetPrimaryFunction(1);
    desc.Push (csString("Right Mouse Button: ") + func->GetName());
  }
  else
  {
    desc.Push ("Left Mouse: fire!");
    desc.Push ("Right Mouse: drag object");
  }
  
  desc.Push ("Q, E, PGUP, PGDN: Pan Camera");
  desc.Push ("W, A, S, D: Move");
  desc.Push ("SPACE: Jump");

  desc.Push ("R: reset");

  desc.Push ("CTRL-x: cut selected object");
  desc.Push ("CTRL-v: paste object");
  
  desc.Push ("C: switch between actor modes (dynamic, kinematic, noclip)");
  desc.Push ("V: switch between camera follow modes (1st person, 3rd person)");
  desc.Push ("P: pause the simulation");
  desc.Push ("O: toggle speed of simulation");
  desc.Push ("L: toggle Bullet debug display");

  desc.Push ("K: toggle display of collisions");
  desc.Push ("G: toggle gravity");
  /*  
  #ifdef CS_HAVE_BULLET_SERIALIZER
  if (phys_engine_id == BULLET_ID)
  desc.Push ("CTRL-s: save the dynamic world");
  #endif
  */
  /*
  if (phys_engine_id == BULLET_ID)
  desc.Push ("CTRL-n: next environment");
  */

  desc.Push ("CTRL-i: start profiling");
  desc.Push ("CTRL-o: stop profiling");
  desc.Push ("CTRL-p: dump profile");
  
  if (selectedItem)
  {
    ItemTemplate& templ = selectedItem->GetTemplate();
    desc.Push ("--Current Tool (" + templ.GetName() + ")--");
    for (size_t i = 0; i < csMin(templ.GetPrimaryFunctions().GetSize(), size_t(2)); ++i)
    {
      ItemFunction* func = templ.GetPrimaryFunction(i);
      desc.Push (csString().Format(" %s: %s", i == 0 ? "LMB" : "RMB", func->GetName().GetData()));
    }
    for (size_t i = 0; i < templ.GetSecondaryFunctions().GetSize(); ++i)
    {
      ItemFunction* func = templ.GetSecondaryFunction(i);
      desc.Push (csString().Format(" %d: %s", i+1, func->GetName().GetData()));
    }
  }
  else
  {
    desc.Push ("--(No Tool selected)--");
  }

  desc.Push ("--Tools--");
  for (size_t i = 0; i < player.GetInventory().GetItems().GetSize(); ++i)
  {
    Item* item = player.GetInventory().GetItem(i);
    desc.Push (csString().Format(" F%d: %s", i+1, item->GetName()));
  }
}


csPtr<iPhysicalSector> PhysDemo::CreatePhysicalSector(iSector* isector)
{
  csRef<iPhysicalSector> sector = physicalSystem->CreatePhysicalSector();
  sector->SetSector(isector);
  sector->SetDebugMode (debugMode);
  if (isSoftBodyWorld)
  {
    sector->SetSoftBodyEnabled (true);
  }
  return csPtr<iPhysicalSector>(sector);
}


// ####################################################################################################################
// Misc stuff


void PhysDemo::ApplyGhostSlowEffect()
{
  if (!ghostObject) return;

  size_t count = ghostObject->GetContactObjectsCount();
  for (size_t i = 0; i < count; i++)
  {
    iPhysicalBody* pb = ghostObject->GetContactObject (i)->QueryPhysicalBody();
    if (pb && IsDynamic(pb))
    {
      if (pb->QueryRigidBody())
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
        sb->QueryPhysicalBody()->SetLinearVelocity (csVector3 (.0f,.0f,.0f));
        //sb->SetLinearVelocity (csVector3 (0,0,-1.0f));
      }
    }
  }
}

void PhysDemo::UpdateActorMode(ActorMode newActorMode)
{
  actorMode = newActorMode;

  iCollisionObject* lastActorObj = player.GetObject();

  switch (actorMode)
  {
    // The camera is controlled by a rigid body
  case ActorModeDynamic:
    {
      // Check if there is already a rigid body created for the 'kinematic' mode
      if (!dynamicActor)
      {
        //csRef<CS::Collisions::iColliderSphere> collider = physicalSystem->CreateColliderSphere (ActorDimensions.y);
        //csRef<CS::Collisions::iColliderCylinder> collider = physicalSystem->CreateColliderCylinder (ActorDimensions.y, ActorDimensions.x/2);
        csRef<CS::Collisions::iColliderBox> collider = physicalSystem->CreateColliderBox (ActorDimensions);
        csRef<iDynamicActorFactory> factory = physicalSystem->CreateDynamicActorFactory(collider);
        factory->SetMass(csScalar(80.));
        factory->SetElasticity(csScalar(0));
        factory->SetFriction(csScalar(.1));

        factory->SetAirControlFactor(actorAirControl);
        factory->SetStepHeight(csScalar(0.5));

        dynamicActor = factory->CreateDynamicActor();
      }

      player.SetObject(dynamicActor);
      break;
    }

  case ActorModeKinematic:
    {
      if (!kinematicActor)
      {
        //csRef<CS::Collisions::iColliderSphere> collider = physicalSystem->CreateColliderSphere (ActorDimensions.y/2);
        csRef<CS::Collisions::iColliderCylinder> collider = physicalSystem->CreateColliderCylinder (ActorDimensions.y, ActorDimensions.x/2);
        //csRef<CS::Collisions::iColliderBox> collider = physicalSystem->CreateColliderBox (ActorDimensions);

        /*csOrthoTransform trans;
        trans.RotateThis(csVector3(1, 0, 0), HALF_PI);
        csRef<CS::Collisions::iColliderCompound> parent = physicalSystem->CreateColliderCompound ();
        parent->AddCollider(collider, trans);
        csRef<iCollisionActorFactory> factory = physicalSystem->CreateCollisionActorFactory(parent);*/
        csRef<iCollisionActorFactory> factory = physicalSystem->CreateCollisionActorFactory(collider);
        factory->SetAirControlFactor(actorAirControl);
        factory->SetJumpSpeed(moveSpeed);

        kinematicActor = factory->CreateCollisionActor();
      }

      kinematicActor->SetCollisionGroup(physicalSystem->FindCollisionGroup("Actor"));

      player.SetObject(kinematicActor);
    }
    break;

  default:
    break;
  }

  CS_ASSERT(player.GetObject()->QueryActor());

  if (actorMode != ActorModeNoclip)
  {
    if (lastActorObj)
    {
      // remove previous actor
      lastActorObj->GetSector()->RemoveCollisionObject(lastActorObj);
      
      // move new actor to old transform
      player.GetObject()->SetTransform(lastActorObj->GetTransform());
      lastActorObj->GetSector()->AddCollisionObject(player.GetObject());
    }
    player.GetObject()->SetCollisionGroup(physicalSystem->FindCollisionGroup("Actor"));
    SetGravity(-10 * UpVector);
  }
  else
  {
    // The camera is free now -> Requires actor object to already be created & set
    player.GetObject()->SetCollisionGroup(physicalSystem->FindCollisionGroup("None"));
    SetGravity(0);
  }
}

bool PhysDemo::IsDynamic(CS::Collisions::iCollisionObject* obj) const
{
  return 
    obj->QueryPhysicalBody() &&
    (
    !obj->QueryPhysicalBody()->QueryRigidBody() || 
    obj->QueryPhysicalBody()->QueryRigidBody()->GetState() == STATE_DYNAMIC
    );
}

bool PhysDemo::IsActor(CS::Collisions::iCollisionObject* obj) const
{
  return obj->QueryActor();
}

void PhysDemo::SetGravity(const csVector3& g)
{
  for (size_t i = 0; i < physicalSystem->GetCollisionSectorCount(); ++i)
  {
    csRef<iPhysicalSector> sector = scfQueryInterface<iPhysicalSector>(physicalSystem->GetCollisionSector(i));
    sector->SetGravity(g);
  }
}

void PhysDemo::ResetCurrentLevel()
{
  Reset();

  SetLevel(environment);
}

bool PhysDemo::GetPointOnGroundBeneathPos(const csVector3& pos, csVector3& groundPos) const
{
  csVector3 to = pos - 10000 * UpVector;
  HitBeamResult result = GetCurrentSector()->HitBeam(pos, to);

  if (result.hasHit)
  {
    groundPos = result.isect;
    return true;
  }
  return false;
}

bool PhysDemo::GetPointOnGroundAbovePos(const csVector3& pos, csVector3& groundPos) const
{
  csVector3 to = pos + 10000 * UpVector;
  HitBeamResult result = GetCurrentSector()->HitBeam(pos, to);

  if (result.hasHit)
  {
    groundPos = result.isect;
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION
  
/// We use this to keep the physical system alive until the end to make sure its destroyed last
csRef<iPhysicalSystem> _physSys;

int main (int argc, char* argv[])
{
  _physSys = physDemo.physicalSystem;
  return physDemo.Main(argc, argv);
}
