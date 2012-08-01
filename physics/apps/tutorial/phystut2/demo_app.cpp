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
  //actorMode (ACTOR_KINEMATIC)
  actorMode (ACTOR_DYNAMIC)
  ,
  cameraMode(CamFollowMode1stPerson),
  //defaultEnvironmentName("terrain")
  defaultEnvironmentName("portals")
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

  physicalSystem->SetInternalScale (1.0f);

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
    csPrintf ("Given level (%s) is not one of {%s, %s, %s}. Falling back to \"%s\"\n",
      CS::Quote::Single (levelName.GetData()),
      CS::Quote::Single ("portals"),
      CS::Quote::Single ("box"),
      CS::Quote::Single ("terrain"),
      defaultEnvironmentName.GetData());
    //environment = ENVIRONMENT_PORTALS;
    environment = GetEnvironmentByName(defaultEnvironmentName);
  }

  if (!physicalSystem)
    return ReportError ("No bullet system plugin!");

  return true;
}


bool PhysDemo::Application()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application())
    return false;

  // Prepare engine (whatever that means)
  engine->Prepare();


  // Create the dynamic system
  room = engine->CreateSector ("room");
  physicalSector = physicalSystem->CreatePhysicalSector();
  physicalSector->SetSector(room);

  // Set some linear and angular dampening in order to have a reduction of
  // the movements of the objects
  physicalSector->SetLinearDamping(0.1f);
  physicalSector->SetAngularDamping(0.1f);

  // Enable soft bodies
  if (isSoftBodyWorld)
  {
    physicalSector->SetSoftBodyEnabled (true);
  }

  bulletSector = scfQueryInterface<iPhysicalSector> (physicalSector);
  bulletSector->SetDebugMode (debugMode);
  
  // Preload some meshes and materials
  if (!loader->LoadTexture ("raindrop", "/lib/std/raindrop.png")) return ReportError ("Error loading texture: raindrop");
  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif")) return ReportError ("Could not load texture: stone");
  if (!loader->LoadTexture ("objtexture", "/lib/std/blobby.jpg")) return ReportError ("Error loading texture: blobby");
  if (!loader->LoadTexture ("misty", "/lib/std/misty.jpg")) return ReportError ("Error loading texture: misty");

  // Create the environment
  switch (environment)
  {
  case ENVIRONMENT_PORTALS:
    CreatePortalRoom();
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 0, -3.5f));
    break;

  case ENVIRONMENT_TERRAIN:
    CreateTerrainRoom();
    //view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 30, -3));
    view->GetCamera()->GetTransform().SetOrigin (csVector3 (0, 5, 3));
    break;

  default:
    break;
  }

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

  // Initialize Player items
  CreateItemTemplates();
  for (size_t i = 0; i < ItemMgr::Instance->GetTemplateCount(); ++i)
  {
    ItemTemplate& templ = ItemMgr::Instance->GetTemplate(i);
    player.GetInventory().AddItem(templ);
  }

  // Initialize HUD
  SetupHUD();


  // Run the application
  Run();

  return true;
}

void PhysDemo::SetupHUD()
{
  // Setup the descriptions in the HUD
  iStringArray& desc = *hudManager->GetKeyDescriptions();
  desc.Empty();
  
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
      desc.Push (csString().Format(" F%d: %s", i+1, func->GetName().GetData()));
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
    desc.Push (csString().Format(" %d: %s", i+1, item->GetName()));
  }
}



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

void PhysDemo::UpdateCameraMode()
{
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  iCollisionObject* lastActorObj = player.GetObject();

  switch (actorMode)
  {
    // The camera is controlled by a rigid body
  case ACTOR_DYNAMIC:
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
        factory->SetFriction(csScalar(1.));

        factory->SetAirControlFactor(actorAirControl);
        factory->SetStepHeight(csScalar(0.1));

        dynamicActor = factory->CreateDynamicActor();
      }

      csOrthoTransform trans(tc);
      dynamicActor->SetTransform (trans);
      physicalSector->AddCollisionObject(dynamicActor);

      player.SetObject(dynamicActor);
      break;
    }

  case ACTOR_KINEMATIC:
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

      csOrthoTransform trans;
      //trans.RotateThis(csVector3(1, 0, 0), HALF_PI);
      trans.SetOrigin(tc.GetOrigin());
      kinematicActor->SetTransform(trans);
      //kinematicActor->SetTransform(tc);

      physicalSector->AddCollisionObject(kinematicActor);
      kinematicActor->SetCollisionGroup(physicalSystem->FindCollisionGroup("Actor"));

      player.SetObject(kinematicActor);
    }
    break;

  default:
    break;
  }

  if (actorMode != ACTOR_NOCLIP)
  {   
    if (lastActorObj)
    {
      // remove previous actor
      physicalSector->RemoveCollisionObject(lastActorObj);
    }
    player.GetObject()->SetCollisionGroup(physicalSystem->FindCollisionGroup("Actor"));
  }
  else
  {
    // The camera is free
    player.GetObject()->SetCollisionGroup(physicalSystem->FindCollisionGroup("None"));
    physicalSector->SetGravity(0);
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

void PhysDemo::ResetWorld()
{
  for (int i = physicalSector->GetCollisionObjectCount() - 1; i >= 0; --i)
  {
    iCollisionObject* obj = physicalSector->GetCollisionObject(i);
    if (IsDynamic(obj) && !IsActor(obj))
    {
      physicalSector->RemoveCollisionObject(obj);
    }
  }
}

bool PhysDemo::GetPointOnGroundBeneathPos(const csVector3& pos, csVector3& groundPos) const
{
  csVector3 to = pos - 10000 * UpVector;
  HitBeamResult result = physicalSector->HitBeam(pos, to);

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
  HitBeamResult result = physicalSector->HitBeam(pos, to);

  if (result.hasHit)
  {
    groundPos = result.isect;
    return true;
  }
  return false;
}

bool PhysDemo::TestOnGround(CS::Collisions::iCollisionObject* obj)
{
  static const float groundAngleCosThresh = .7f;

  // Find any objects that can at least remotely support the object
  csArray<CollisionData> collisions;
  physicalSector->CollisionTest(obj, collisions);

  for (size_t i = 0; i < collisions.GetSize (); ++i)
  {
    CollisionData& coll = collisions[i];

    int dir = coll.objectA == obj ? 1 : -1;

    float groundAngleCos = coll.normalWorldOnB * UpVector;
    if (dir * groundAngleCos > groundAngleCosThresh)
    {
      return true;
    }
  }
  return false;
}

bool PhysDemo::GetObjectInFrontOfMe(CS::Collisions::HitBeamResult& result)
{ 
  // Find the rigid body that was clicked on
  // Compute the end beam points
  csRef<iCamera> camera = view->GetCamera();
  csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
  csVector3 v3d = camera->InvPerspective (v2d, 10000);
  csVector3 startBeam = camera->GetTransform().GetOrigin();
  csVector3 endBeam = camera->GetTransform().This2Other (v3d);

  // Trace the physical beam
  return (result = physicalSector->HitBeamPortal (startBeam, endBeam)).hasHit;
}

void PhysDemo::PullObject()
{
  HitBeamResult result;
  if (GetObjectInFrontOfMe(result) && IsDynamic(result.object))
  {
    iPhysicalBody* pb = result.object->QueryPhysicalBody();

    csVector3 posCorrection(2  * UpVector);

    csVector3 force(GetActorPos() - result.isect - posCorrection);
    force.Normalize();
    force *= 30 * pb->GetMass();

    // prevent sliding problem
    csOrthoTransform trans = pb->GetTransform();
    trans.SetOrigin(trans.GetOrigin() + posCorrection);
    pb->SetTransform(trans);

    pb->QueryRigidBody()->AddForce (force);
  }
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
