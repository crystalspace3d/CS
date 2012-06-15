#ifndef __PHYSTUT2_H__
#define __PHYSTUT2_H__

#include "cssysdef.h"
#include "csutil/weakref.h"

#include "cstool/demoapplication.h"
#include "ivaria/physics.h"
#include "ivaria/bullet2.h"
#include "ivaria/collisions.h"

#include "imesh/animesh.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/modifiableterrain.h"

#define CAMERA_DYNAMIC 1
#define CAMERA_KINEMATIC 2
#define CAMERA_FREE 3
#define CAMERA_ACTOR 4

#define ENVIRONMENT_NONE 0
#define ENVIRONMENT_PORTALS 1
#define ENVIRONMENT_BOX 2
#define ENVIRONMENT_TERRAIN 3

inline int GetEnvironmentByName(csString levelName)
{
  if (levelName == "portals")
      return ENVIRONMENT_PORTALS;

  else if (levelName == "box")
      return ENVIRONMENT_BOX;

  else if (levelName == "terrain")
      return ENVIRONMENT_TERRAIN;
  
  return ENVIRONMENT_NONE;
}
 
//static const csVector3 ActorDimensions(0.8);
static const csVector3 ActorDimensions(0.4, 1.5, 0.4);

class PhysDemo : public CS::Utility::DemoApplication
{
private:
  csRef<CS::Collisions::iCollisionSystem> collisionSystem;
  csWeakRef<CS::Physics::iPhysicalSystem> physicalSystem;

  csRef<CS::Collisions::iCollisionSector> collisionSector;
  csWeakRef<CS::Physics::iPhysicalSector> physicalSector;
  csWeakRef<CS::Physics::Bullet2::iPhysicalSector> bulletSector;

  csRef<CS::Physics::iSoftBodyAnimationControlType> softBodyAnimationType;

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
  csRef<CS::Collisions::iCollisionGhostObject> ghostObject;

  // Terrain
  csRef<iModifiableDataFeeder> terrainFeeder;
  csRef<iTerrainModifier> terrainMod;

  csHash<int, csString> debugNameMap;

private:
  void Frame();
  bool OnKeyboard (iEvent &event);

  bool OnMouseDown (iEvent &event);
  bool OnMouseUp (iEvent &event);

  // Camera
  void UpdateCameraMode();

  // Spawning objects
  bool SpawnStarCollider();
  CS::Physics::iRigidBody* SpawnBox (bool setVelocity = true);
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
  CS::Physics::iRigidBody* SpawnFilterBody (bool setVelocity = true);
  void SpawnChain();
  void LoadFrankieRagdoll();
  void LoadKrystalRagdoll();
  void SpawnFrankieRagdoll();
  void SpawnKrystalRagdoll();
  void SpawnRope();
  CS::Physics::iSoftBody* SpawnCloth();
  CS::Physics::iSoftBody* SpawnSoftBody (bool setVelocity = true);

  void CreateBoxRoom();
  void CreatePortalRoom();
  void CreateTerrainRoom();

  void CreateGhostCylinder();
  void GripContactBodies();


  // particles
  void AddParticles(const csVector3& origin, float yFactor = 1, int num = 256);

public:
  PhysDemo();
  virtual ~PhysDemo();

  void PrintHelp();
  bool OnInitialize (int argc, char* argv[]);
  bool Application();

  friend class MouseAnchorAnimationControl;
  csRef<CS::Physics::iAnchorAnimationControl> grabAnimationControl;


  /**
   * The location of the actor's head, i.e. the location of the camera
   */
  csVector3 GetActorHeadPos() const { return view->GetCamera()->GetTransform().GetOrigin(); }

  /**
   * The location where the actor stands
   */
  csVector3 GetActorFeetPos() const { return GetActorHeadPos() - csVector3(0, ActorDimensions.y, 0); }

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
  csVector3 GetPointInFront(float distance) const { return GetActorHeadPos() + (GetCameraDirection() * distance); }

  /**
   * Point in the given distance in front of the actor's feet
   */
  csVector3 GetPointInFrontOfFeet(float distance) const { return GetActorFeetPos() + (GetCameraDirection() * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontXZ(float distance) const { return GetActorHeadPos() + (GetCameraDirectionXZ() * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontOfFeetXZ(float distance) const { return GetActorFeetPos() + (GetCameraDirectionXZ() * distance); }

  csPtr<CS::Physics::iRigidBody> CreateRigidBody(const csString& name);
  
  csPtr<CS::Physics::iRigidBody> CreateStaticRigidBody(const csString& name);
  
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

#endif
