#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
<<<<<<< .mine
=======
#include "physdemo.h"
>>>>>>> .r38138

//
//
//
//int runDebug(int argc, char* argv[]);
//
//int main (int argc, char* argv[])
//{
//    if (runDebug(argc, argv)) 
//    {
//        return Simple ().Main(argc, argv);
//    }
//    return 0;
//}
//

class VerySimple : public CS::Utility::DemoApplication
{
private:
  csRef<CS::Collisions::iCollisionSystem> collisionSystem;
  csRef<CS::Physics::iPhysicalSystem> physicalSystem;
  csRef<CS::Collisions::iCollisionSector> collisionSector;
  csRef<CS::Physics::iPhysicalSector> physicalSector;
  csRef<CS::Physics::Bullet2::iPhysicalSector> bulletSector;

  // Edit1: Not necessary to cast to sub-type
  //csRef<CS::Physics::iSoftBodyAnimationControlFactory> softBodyAnimationFactory;
  csRef<iGenMeshAnimationControlFactory> softBodyAnimationFactory;
  csRefArray<CS::Physics::iRigidBody> dynamicBodies;
  bool isSoftBodyWorld;

  // Meshes
  csRef<iMeshFactoryWrapper> boxFact;
  csRef<iMeshFactoryWrapper> meshFact;
  csRef<CS::Collisions::iColliderConcaveMesh> mainCollider;


  // Environments
  int environment;
  csRef<iMeshWrapper> walls;

  // Configuration related
  int solver;
  bool autodisable;
  csString phys_engine_name;
  int phys_engine_id;
  bool do_bullet_debug;
  bool do_soft_debug;
  float remainingStepDuration;

  // Dynamic simulation related
  bool allStatic;
  bool pauseDynamic;
  float dynamicSpeed;

  // Camera related
  CS::Physics::Bullet2::DebugMode debugMode;
  int physicalCameraMode;
  csRef<CS::Physics::iRigidBody> cameraBody;
  csRef<CS::Collisions::iCollisionActor> cameraActor;

  // Ragdoll related
  csRef<CS::Animation::iSkeletonRagdollNodeManager2> ragdollManager;

  // Dragging related
  bool dragging;
  bool softDragging;
  csRef<CS::Physics::iJoint> dragJoint;
  csRef<CS::Physics::iSoftBody> draggedBody;
  
  size_t draggedVertex;
  float dragDistance;
  float linearDampening, angularDampening;

  // Cut & Paste related
  csRef<CS::Physics::iPhysicalBody> clipboardBody;
  csRef<iMovable> clipboardMovable;

  // Collider
  csOrthoTransform localTrans;

  // Ghost
  csRef<CS::Collisions::iCollisionObject> ghostObject;

private:
  void Frame ();
  bool OnKeyboard (iEvent &event);

  bool OnMouseDown (iEvent &event);
  bool OnMouseUp (iEvent &event);

  // Camera
  void UpdateCameraMode ();

  // Spawning objects
  CS::Physics::iSoftBody* SpawnSoftBody (bool setVelocity = true);

public:
  VerySimple ();
  virtual ~VerySimple ();

  void PrintHelp ();
  bool OnInitialize (int argc, char* argv[]);
  bool Application ();
};

int runDebug(int argc, char* argv[])
{
    //VerySimple ().Main(argc, argv);
    return 1;
}



// ####################################################################################################
// VerySimple implementation

#define CAMERA_DYNAMIC 1
#define CAMERA_KINEMATIC 2
#define CAMERA_FREE 3
#define CAMERA_ACTOR 4

#define ENVIRONMENT_PORTALS 1
#define ENVIRONMENT_BOX 2
#define ENVIRONMENT_TERRAIN 3

VerySimple::VerySimple()
  : DemoApplication ("CrystalSpace.PhysTut2"),
    isSoftBodyWorld (true), solver (0), do_bullet_debug (false),
    do_soft_debug (true), remainingStepDuration (0.0f), allStatic (false), 
    pauseDynamic (false), dynamicSpeed (1.0f),
    debugMode (CS::Physics::Bullet2::DEBUG_COLLIDERS),
    physicalCameraMode (CAMERA_DYNAMIC), dragging (false), softDragging (false)
{
  localTrans.Identity ();
}

VerySimple::~VerySimple ()
{
}

void VerySimple::PrintHelp (){}

void VerySimple::Frame ()
{
}

bool VerySimple::OnKeyboard (iEvent &event)
{
    return false;
}

bool VerySimple::OnMouseDown (iEvent &event)
{
  return false;
}

bool VerySimple::OnMouseUp (iEvent &event)
{
  return false;
}


bool VerySimple::OnInitialize (int argc, char* argv[])
{
csRef<CS::Physics::iSoftBodyAnimationControlType> softBodyAnimationType;
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  csBaseEventHandler::Initialize (GetObjectRegistry ());
  if (!RegisterQueue (GetObjectRegistry (), csevAllEvents (GetObjectRegistry ())))
    return ReportError ("Failed to set up event handler!");

  // Checking for choosen dynamic system
  csRef<iCommandLineParser> clp = csQueryRegistry<iCommandLineParser> (GetObjectRegistry ());
  phys_engine_name = clp->GetOption ("phys_engine");
  
  phys_engine_name = "Bullet";
  csRef<iPluginManager> plugmgr = 
    csQueryRegistry<iPluginManager> (GetObjectRegistry ());
  collisionSystem = csLoadPlugin<CS::Collisions::iCollisionSystem> (plugmgr, "crystalspace.physics.bullet2");
  physicalSystem = scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);

  // We have some objects of size smaller than 0.035 units, so we scale up the
  // whole world for a better behavior of the dynamic simulation.
  collisionSystem->SetInternalScale (10.0f);

  softBodyAnimationType =
      csLoadPlugin<CS::Physics::iSoftBodyAnimationControlType>
      (plugmgr, "crystalspace.physics.softanim2");

  if (!softBodyAnimationType)
      return ReportError ("Could not load soft body animation for genmeshes plugin!");

  // Edit1: The 2 lines return the same object.
  //csRef<iGenMeshAnimationControlFactory> animationFactory =
  //  softBodyAnimationType->CreateAnimationControlFactory ();
  //softBodyAnimationFactory =
  //  scfQueryInterface<CS::Physics::iSoftBodyAnimationControlFactory> (animationFactory);
  softBodyAnimationFactory = softBodyAnimationType->CreateAnimationControlFactory ();

  if (!collisionSystem)
    return ReportError ("No bullet system plugin!");

  return true;
}


bool VerySimple::Application ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

  // Create the dynamic system
  collisionSector = collisionSystem->CreateCollisionSector ();
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

  // Preload some meshes and materials
  iTextureWrapper* txt = loader->LoadTexture ("spark",
    "/lib/std/spark.png");
  if (!txt) return ReportError ("Error loading texture!");

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
  UpdateCameraMode ();

  SpawnSoftBody(false);

  collisionSector->SetSector (room);

  collisionSector->CreateCollisionGroup ("Box");
  collisionSector->CreateCollisionGroup ("BoxFiltered");

  bool coll = collisionSector->GetGroupCollision ("Box", "BoxFiltered");
  if (coll)
    collisionSector->SetGroupCollision ("Box", "BoxFiltered", false);

  // Run the application
  Run();

  return true;
}

void VerySimple::UpdateCameraMode ()
{
}

CS::Physics::iSoftBody* VerySimple::SpawnSoftBody (bool setVelocity /* = true */)
{
  // Create the ball mesh factory.
  csRef<iMeshFactoryWrapper> ballFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "ballFact");
  if (!ballFact)
  {
    ReportError ("Error creating mesh object factory!");
    return NULL;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (ballFact->GetMeshObjectFactory ());

  
  gmstate->SetAnimationControlFactory (softBodyAnimationFactory);


  const float r = 0.4f;
  csVector3 radius (r, r, r);
  csEllipsoid ellips (csVector3 (0), radius);
  gmstate->GenerateSphere (ellips, 16);

  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the soft body
  //csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateSoftBody(gmstate,
  //  csOrthoTransform (csMatrix3 (), csVector3 (0.0f, 0.0f, 1.0f)) * tc);
  // This would have worked too
  csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateSoftBody
    (gmstate->GetVertices (), gmstate->GetVertexCount (),
     gmstate->GetTriangles (), gmstate->GetTriangleCount (),
     csOrthoTransform (csMatrix3 (), csVector3 (0.0f, 0.0f, 1.0f)) * tc);
  body->SetMass (5.0f);
  body->SetRigidity (0.8f);
  csRef<CS::Physics::Bullet2::iSoftBody> bulletbody = 
    scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (body);
  bulletbody->SetBendingConstraint (true);
  
  if (setVelocity)
  {
    // Fling the body.
    body->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
  }

  // Create the mesh
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    ballFact, "soft_body"));
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("spark");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  body->SetAttachedMovable (mesh->GetMovable ());

  body->RebuildObject ();
  physicalSector->AddSoftBody (body);

  // Init the animation control for the animation of the genmesh
  /*csRef<iGeneralMeshState> meshState =
    scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject ());
  csRef<CS::Physics::iSoftBodyAnimationControl> animationControl =
    scfQueryInterface<CS::Physics::iSoftBodyAnimationControl> (meshState->GetAnimationControl ());
  animationControl->SetSoftBody (body);*/


  // This would have worked too
  //for (size_t i = 0; i < body->GetVertexCount (); i++)
  //  body->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5), i);
  return body;
}
