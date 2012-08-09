#ifndef __PHYSTUT2_H__
#define __PHYSTUT2_H__

#include "cssysdef.h"
#include "csutil/weakref.h"
#include "cstool/demoapplication.h"

#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/ivehicle.h"
#include "ivaria/convexdecompose.h"

#include "imesh/animesh.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/modifiableterrain.h"

#include "agent.h"

struct iCameraPosition;

// Some global variables


static const csScalar DefaultDensity(500);
static const csScalar DefaultFriction(.5);
static const csScalar DefaultElasticity(0.1f);


// Actor/Camera modes
enum ActorMode
{
  ActorModeNone,
  ActorModeDynamic,
  ActorModeKinematic,
  ActorModeNoclip
};

// Levels
enum PhysDemoLevel
{
  PhysDemoLevelNone,
  PhysDemoLevelBox,
  PhysDemoLevelPortals,
  PhysDemoLevelTerrain,
  PhysDemoLevelCastle
};

enum CameraMode
{
  CameraMode1stPerson,
  CameraMode3rdPerson,
  CameraMode3rdPersonFar,
  CameraModeCount
};

// Navigation input (use WASD controls)
static const int KeyUp = CSKEY_PGUP;
static const int KeyDown = CSKEY_PGDN;
//static const int KeyLeft = CSKEY_LEFT;
//static const int KeyRight = CSKEY_RIGHT;
//static const int KeyForward = CSKEY_UP;
//static const int KeyBack = CSKEY_DOWN;
static const int KeyLeft = 'q';
static const int KeyRight = 'e';
static const int KeyForward = 'w';
static const int KeyBack = 's';
static const int KeyStrafeLeft = 'a';
static const int KeyStrafeRight = 'd';
static const int KeyJump = CSKEY_SPACE;
static const int KeyHandbrake = CSKEY_SPACE;


class PhysDemo;

/// Retreives folder and file information from a full (unix-style) path
inline void GetFolderAndFile(const csString& path, csString& folder, csString& filename)
{
  size_t index = path.FindLast ('/');
  if (index != (size_t) -1)
  {
    folder = path.Slice (0, index);
    filename = path.Slice (index + 1);
  }
  else
  {
    folder = "";
    filename = path;
    filename.Trim();
  }
}

/**
 * Re-usable pair of a collider with a render mesh. 
 * Can be used to create new RigidBody objects.
 */
class RenderMeshColliderPair
{
public:
  csRef<CS::Collisions::iCollider> Collider;
  csRef<iMeshFactoryWrapper> MeshFactory;

  /// Creates a new RigidBody from the given collider and render mesh
  csPtr<CS::Physics::iRigidBody> SpawnRigidBody(const csString& name, const csOrthoTransform& trans, csScalar friction = 1, csScalar density = 30);
};

//static const csVector3 ActorDimensions(0.8);
//static const csVector3 ActorDimensions(0.1f, 0.6f, 0.1f);
static const csVector3 ActorDimensions(0.3f, 1.8f, 0.3f);

class PhysDemo : public CS::Utility::DemoApplication
{
public:
  csRef<CS::Physics::iPhysicalSystem> physicalSystem;
  csRef<iConvexDecomposer> convexDecomposer;

  csRef<CS::Physics::iSoftBodyAnimationControlType> softBodyAnimationType;

  csRef<iGenMeshAnimationControlFactory> softBodyAnimationFactory;
  bool isSoftBodyWorld;

  // Meshes
  csRef<CS::Collisions::iColliderConcaveMesh> mainCollider;

  RenderMeshColliderPair stackBoxMeshPair;

  // Configuration related
  bool autodisable;
  int phys_engine_id;
  bool do_bullet_debug;
  bool do_soft_debug;
  float remainingStepDuration;

  // Dynamic simulation related
  bool allStatic;
  bool pauseDynamic;
  float dynamicStepFactor;

  // Dragging related
  bool dragging;
  bool softDragging;
  csRef<CS::Physics::iJoint> dragJoint;
  csRef<CS::Physics::iSoftBody> draggedBody;
  
  size_t draggedVertex;
  float dragDistance;
  float linearDampening, angularDampening;

  // Camera & actors
  CS::Physics::DebugMode debugMode;
  float actorAirControl;
  float moveSpeed, turnSpeed;
  ActorMode actorMode;
  CameraMode cameraMode;

  Agent player;
  Item* selectedItem;
  csRef<CS::Physics::iDynamicActor> dynamicActor;
  csRef<CS::Collisions::iCollisionActor> kinematicActor;

  // Ragdoll related
  csRef<CS::Animation::iSkeletonRagdollNodeManager2> ragdollManager;

  // Cut & Paste related
  csRef<CS::Physics::iPhysicalBody> clipboardBody;
  csRef<iMovable> clipboardMovable;

  // Collider

  // Ghost
  csWeakRef<CS::Collisions::iGhostCollisionObject> ghostObject;

  // Terrain
  /// The feeder to which the current terrain mod is applied (if any)
  csRef<iModifiableDataFeeder> moddedTerrainFeeder;
  /// The currently applied terrain mode (if any)
  csRef<iTerrainModifier> terrainMod;

  csHash<int, csString> debugNameMap;

  // Vehicles
  csRef<CS::Physics::iVehicle> actorVehicle;

  // Static environment
  csString currentMap;
  csRef<iMeshWrapper> walls;

private:
  void Frame();
  bool OnKeyboard (iEvent &event);

  bool OnMouseDown (iEvent &event);
  bool OnMouseUp (iEvent &event);

public:
  // Camera
  void UpdateActorMode(ActorMode newActorMode);

  // Spawning objects
  bool SpawnStarCollider();

  void CreateBoxCollider (csRef<CS::Collisions::iColliderBox>& colliderPtr, csRef<iMeshWrapper>& meshPtr, const csVector3& extents);

  csRef<CS::Physics::iRigidBody> SpawnBox (bool setVelocity = true);
  csRef<CS::Physics::iRigidBody> SpawnBox (const csVector3& extents, const csVector3& pos, bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnBox (const RenderMeshColliderPair& pair, const csVector3& pos, bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnSphere (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnSphere (const csVector3& pos, float radius, bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCone (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCylinder (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCapsule (float length = rand() % 3 / 50.f + .7f,
    float radius = rand() % 10 / 50.f + .2f, bool setVelocity = true);
  CS::Collisions::iCollisionObject* SpawnConcaveMesh();
  CS::Physics::iRigidBody* SpawnConvexMesh (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCompound (bool setVelocity = true);
  CS::Physics::iJoint* SpawnJointed();
  void SpawnChain();
  void LoadFrankieRagdoll();
  void LoadKrystalRagdoll();
  void SpawnFrankieRagdoll();
  void SpawnKrystalRagdoll();
  void SpawnRope();
  CS::Physics::iSoftBody* SpawnCloth();
  CS::Physics::iSoftBody* SpawnSoftBody (bool setVelocity = true);
  void SpawnBoxStacks(int stackNum = 4, int stackHeight = 4, float boxLen = .5f, float mass = 20.f);

  /**
   * Room is the inside of a cuboid of the given size, and wall thickness
   */
  void CreateBoxRoom(const csVector3& roomExtents, const csVector3& pos = csVector3(0), csScalar wallThickness = 5);

  /// Create the default box room with a given side-length
  void CreateBoxRoom(csScalar size = 100);
  
  /// Load a scene from file
  bool LoadLevel(const csString& filepath, bool convexDecomp = false);

  void CreateGhostCylinder();
  void ApplyGhostSlowEffect();

  /// Creates a new rigid body, places it at the given pos and, optionally, gives it some initial momentum
  csRef<CS::Physics::iRigidBody> SpawnRigidBody (RenderMeshColliderPair& pair, const csVector3& pos, 
    const csString& name, csScalar friction = 1, csScalar density = 30, bool setVelocity = false);


  // particles
  void AddParticles(const csVector3& origin, float yFactor = 1, int num = 256);

  // removes everything and resets things
  void ResetCurrentLevel();

public:
  PhysDemo();
  virtual ~PhysDemo();

  void PrintHelp();
  bool OnInitialize (int argc, char* argv[]);
  bool Application();

  void SetupHUD();

  friend class MouseAnchorAnimationControl;
  csRef<CS::Physics::iAnchorAnimationControl> grabAnimationControl;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Transformation utilities

  /**
   * The location of the actor's head, i.e. the location of the camera
   */
  csVector3 GetActorPos() const { return player.GetObject()->GetTransform().GetOrigin(); }

  /**
   * The location where the actor stands
   */
  csVector3 GetActorFeetPos() const { return GetActorPos() - csVector3(0, csScalar(.5) * ActorDimensions.y, 0); }

  /**
   * Position of the camera
   */
  csVector3 GetCameraPosition() const { return view->GetCamera()->GetTransform().GetOrigin(); }

  /**
   * Normalized direction of the camera
   */
  csVector3 GetCameraDirection() const { return view->GetCamera()->GetTransform().GetT2O() * csVector3(0, 0, 1); }

  /**
   * Normalized direction of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetCameraDirectionXZ() const { csVector3 dist = view->GetCamera()->GetTransform().GetT2O() * csVector3(0, 0, 1); dist.y = 0; dist.Normalize(); return dist; }

  /**
   * Point in the given distance in front of the camera
   */
  csVector3 GetPointInFront(float distance) const { return GetActorPos() + (GetCameraDirection() * distance); }

  /**
   * Point in the given distance in front of the actor's feet
   */
  csVector3 GetPointInFrontOfFeet(float distance) const { return GetActorFeetPos() + (GetCameraDirection() * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontXZ(float distance) const { return GetActorPos() + (GetCameraDirectionXZ() * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontOfFeetXZ(float distance) const { return GetActorFeetPos() + (GetCameraDirectionXZ() * distance); }

  /// Find the ground contact point beneath pos
  bool GetPointOnGroundBeneathPos(const csVector3& pos, csVector3& groundPos) const;

  /// Find the ground contact point above pos
  bool GetPointOnGroundAbovePos(const csVector3& pos, csVector3& groundPos) const;
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Geometry utilities

  csPtr<iMeshWrapper> CreateCylinderYMesh(csScalar length, csScalar radius, const char* materialName = "objtexture", const char* meshName = "cylinder");

  csPtr<iMeshWrapper> CreateBoxMesh(const csVector3& extents, const char* materialName = "objtexture", const char* meshName = "box");
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Object & Actor utilities

  /// Pick the object that is under the cursor
  bool PickCursorObject(CS::Collisions::HitBeamResult& result);

  /// Get the owning entity of the given collision object (actor, vehicle, etc)
  ::iBase* GetOwner(CS::Collisions::iCollisionObject* obj);

  /// Teleport given object to given location
  void TeleportObject(CS::Collisions::iCollisionObject* obj, iCameraPosition* pos);

  /// Test whether there are any objects beneath obj (that obj can collide with)
  bool TestOnGround(CS::Collisions::iCollisionObject* obj);

  bool IsActor(CS::Collisions::iCollisionObject* obj) const;
  
  /// Adds a force to the given object that pulls it toward the actor. If not given, picks currently targeted object.
  void PullObject(CS::Collisions::iCollisionObject* obj = nullptr);

  /// Toggles between DYNAMIC/STATIC state of the given object. If not given, picks currently targeted object.
  void ToggleObjectDynamic(CS::Collisions::iCollisionObject* obj = nullptr);

  /// Deletes the given object. If not given, picks currently targeted object.
  void DeleteObject(CS::Collisions::iCollisionObject* obj = nullptr);
  
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Items

  void CreateItemTemplates();

  void UpdateCameraManager();
  

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Mesh & Collider Utilities

  void CreateBoxMeshColliderPair(RenderMeshColliderPair& pair, const csVector3& extents);
    

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Vehicles
  
  csPtr<CS::Physics::iVehicle> CreateVehicle();

  void EnterTargetVehicle();
  void LeaveCurrentVehicle();

  void SpawnVehicle();
  void DeleteTargetVehicle();

  void AccelerateTargetVehicle();

  CS::Physics::iVehicle* GetTargetVehicle();


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Input Tools

  csVector3 GetInputDirection();

  /// 0 or 1, depending on whether the forward key is currently pressed (possibly values in between, depending on input device)
  csScalar GetForward();
  
  /// 0 or 1, depending on whether the backward key is currently pressed (possibly values in between, depending on input device)
  csScalar GetBackward();
  
  /// 0 or 1, depending on whether the left/right keys are currently pressed (possibly values in between, depending on input device)
  csScalar GetLeftRight();

  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Frame

  void DoStep();

  void MoveActor();
  void MoveActorVehicle();

  void UpdateVehiclePassengers();

  void RotateActor();

  void MoveCamera();

  void UpdateDragging();

  void UpdateHUD();

  void DoDebugDraw();

  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Level setup & management
  
  /// Create new iPhysicalSector for the given iSector
  csPtr<CS::Physics::iPhysicalSector> CreatePhysicalSector(iSector* sector);
  
  inline void SetCurrentSector(CS::Physics::iPhysicalSector* sector) { sector->AddCollisionObject(player.GetObject()); }
  inline CS::Physics::iPhysicalSector* GetCurrentSector() const 
  { 
    return csRef<CS::Physics::iPhysicalSector>(scfQueryInterface<CS::Physics::iPhysicalSector>(player.GetObject()->GetSector()));
  }

  /// Clears & deletes the scene
  void Reset();

  /// Reset the current scene and setup the given level
  bool SetLevel(PhysDemoLevel level, bool convexDecomp = false);
  /// Reset the current scene and setup the given level
  bool SetLevel(const csString& pathname, bool convexDecomp = false);

  /// Get the iModifiableDataFeeder of the first terrain that has one in the given sector (if any)
  iModifiableDataFeeder* GetFirstTerrainModDataFeeder(CS::Collisions::iCollisionSector* sector);

  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Physics stuff

  bool IsDynamic(CS::Collisions::iCollisionObject* obj) const;

  void SetGravity(const csVector3& g);
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Other

  bool IsGravityOff() { return GetCurrentSector()->GetGravity().SquaredNorm() == 0; }
};

class MouseAnchorAnimationControl : public scfImplementation1
  <MouseAnchorAnimationControl, CS::Physics::iAnchorAnimationControl>
{
public:
  MouseAnchorAnimationControl (PhysDemo* simple)
    : scfImplementationType (this), simple (simple) {}

  csVector3 GetAnchorPosition() const;

private:
  PhysDemo* simple;
};

extern PhysDemo physDemo;

/// Utility: Component-wise vector multiplication
inline csVector3 ScaleVector3(const csVector3& v1, const csVector3& v2)
{
  csVector3 v3;
  v3.x = v1.x * v2.x;
  v3.y = v1.y * v2.y;
  v3.z = v1.z * v2.z;
  return v3;
}

// TODO: Replace with CS' equivalent of std::bind and std::function
#define CallOnAllSectors(code) \
  for (size_t iiiii = 0; iiiii < physDemo.physicalSystem->GetCollisionSectorCount(); ++iiiii) \
  { \
    csRef<iPhysicalSector>(scfQueryInterface<iPhysicalSector>(physDemo.physicalSystem->GetCollisionSector(iiiii)))->code; \
  }

#endif
