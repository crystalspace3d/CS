/**
* Render/print/draw and handle the camera
*/

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "csgeom/quaternion.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;

void PhysDemo::PrintHelp()
{
  csCommandLineHelper commandLineHelper;

  // Command line options
  commandLineHelper.AddCommandLineOption
    ("phys_engine", "Specify which physics plugin to use", csVariant ("bullet2"));
  commandLineHelper.AddCommandLineOption
    ("soft", "Enable the soft bodies", csVariant (true));
  commandLineHelper.AddCommandLineOption
    ("level", csString().Format ("Define the level to be loaded, can be %s, %s, %s",
    CS::Quote::Single ("portals"),
    CS::Quote::Single ("box"),
    CS::Quote::Single ("terrain")),
    csVariant ("terrain"));

  // Printing help
  commandLineHelper.PrintApplicationHelp
    (GetObjectRegistry(), "phystut2",
    "phystut2 <OPTIONS>",
    "Physics tutorial 2 for Crystal Space.");
}

void PhysDemo::Frame()
{
/*if (GetCurrentSector()->GetSoftBodyCount())
  {
    csVector3 aabbMin, aabbMax;
    GetCurrentSector()->GetSoftBody(0)->GetAABB(aabbMin, aabbMax);
    csVector3 o = aabbMin + (aabbMax - aabbMin) / 2;
    csPrintf("SB #0: %f, %f, %f\n", o.x, o.y, o.z);
  }*/
  
  // Update the demo's state information
  UpdateHUD();

  // Rotate actor
  RotateActor();

  if (!pauseDynamic)
  {
    if (actorVehicle)
    {
      // Update vehicle
      MoveActorVehicle();
    }
    else
    {
      // Update actor
      MoveActor();
    }
    // Simulate one step
    DoStep();
  }

  // Update passengers of all vehicles (this should probably be assisted by the physics plugin)
  UpdateVehiclePassengers();

  // Move the camera
  MoveCamera();

  // Update position of an object currently dragged by the mouse
  UpdateDragging();

  // Weird and irritating stuff...
  ApplyGhostSlowEffect();

  // Default behavior from DemoApplication
  DemoApplication::Frame();

  // Display debug informations
  DoDebugDraw();
}

void PhysDemo::MoveActor()
{
  // Dynamics and actor simulation
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks();
  const float moveSpeed = environment == PhysDemoLevelTerrain ? 30.0f : 4.0f;

  static const float MaxVertCos = .965f;      // can't get closer than 15 degrees to UpAxis to prevent gimbal lock

  const float timeMs = elapsed_time / 1000.0;

  iCamera* cam = view->GetCamera();
  csOrthoTransform& camTrans = cam->GetTransform();
  // player.GetObject()->SetTransform(actorTrans);
  csOrthoTransform actorTrans = player.GetObject()->GetTransform();

  // handle movement
  // intended movement direction
  if (player.GetActor())
  {
    iCollisionObject* actorObj = player.GetObject();
    /*csVector3 aabbMin, aabbMax;
    actorObj->GetAABB(aabbMin, aabbMax);*/

    csVector3 newVel = GetInputDirection();
    bool hasMoveDir = newVel.x + newVel.z + newVel.z != 0;
    bool wantsToMove = hasMoveDir | kbd->GetKeyState(KeyJump);

    if (!wantsToMove)
    {
      // stop any player-controlled movement
      player.GetActor()->StopMoving();
    }
    else
    {
      // move actor
      if (hasMoveDir)
      {
        newVel = camTrans.GetT2O() * newVel;

        bool freeFall = player.GetActor()->IsFreeFalling();

        // actor is on ground, flying or has air control
        if (!freeFall || player.GetActor()->GetAirControlFactor() > 0)
        {
          // wants to move and may move
          if (!IsGravityOff())
          {
            // Only walk horizontally when gravity applies
            csVector2 newVel2 = HORIZONTAL_COMPONENT(newVel);
            player.GetActor()->WalkHorizontal(newVel2);
            //dynamicActor->Walk(newVel);
          }
          else
          {
            // move freely when gravity is off
            player.GetActor()->Walk(newVel);
          }
        }
      }
      if (!player.GetActor()->IsFreeFalling() && kbd->GetKeyState(KeyJump))
      {
        // Jump
        player.GetActor()->Jump();
      }
    }
  }
  else
  {
    // Only move camera
    csVector3 newVel = GetInputDirection();
    if (kbd->GetKeyState(KeyJump))
    {
      // add upward movement
      newVel[UpAxis] += moveSpeed;
    }
    cam->Move (newVel * moveSpeed * timeMs);
  }
}

void PhysDemo::DoStep()
{
  csTicks elapsed_time = vc->GetElapsedTicks();
  const float timeMs = elapsed_time / 1000.0;

  if (player.GetActor())
  {
    player.GetActor()->UpdatePreStep(timeMs * dynamicStepFactor);
  }
  CallOnAllSectors(Step (timeMs * dynamicStepFactor));
  if (player.GetActor())
  {
    player.GetActor()->UpdatePostStep(timeMs * dynamicStepFactor);
  }
}

void PhysDemo::RotateActor()
{
  csTicks elapsed_time = vc->GetElapsedTicks();

  const float timeMs = elapsed_time / 1000.0;

  float moveSpeed = environment == PhysDemoLevelTerrain ? 30.0f : 4.0f;

  static const float MaxVertCos = .965f;      // can't get closer than 15 degrees to UpAxis to prevent gimbal lock


  iCamera* cam = view->GetCamera();
  csOrthoTransform& camTrans = cam->GetTransform();
  // player.GetObject()->SetTransform(actorTrans);
  csOrthoTransform actorTrans = player.GetObject()->GetTransform();

  // Rotate camera
  if (kbd->GetKeyState (KeyLeft) || kbd->GetKeyState(CSKEY_LEFT) ||
    kbd->GetKeyState (KeyRight) || kbd->GetKeyState(CSKEY_RIGHT) ||
    kbd->GetKeyState (KeyUp) ||
    kbd->GetKeyState (KeyDown))
  {
    float turnAmount = turnSpeed * timeMs;

    csVector3 actorDir3 = actorTrans.GetT2O() * csVector3(0, 0, 1);
    csVector3 camDir3 = camTrans.GetT2O() * csVector3(0, 0, 1);
    float vertCos = camDir3 * UpVector;

    float yaw = 0;
    if (kbd->GetKeyState (KeyLeft) || kbd->GetKeyState(CSKEY_LEFT))
    {
      yaw -= turnAmount;
    }
    if (kbd->GetKeyState (KeyRight) || kbd->GetKeyState(CSKEY_RIGHT))
    {
      yaw += turnAmount;
    }

    csVector2 actorDir2 = HORIZONTAL_COMPONENT(actorDir3);
    if (yaw)
    {
      actorDir2.Rotate(yaw);

      csVector2 camDir2 = HORIZONTAL_COMPONENT(camDir3);
      camDir2.Rotate(yaw);
      camDir3 = HV_VECTOR3(camDir2, camDir3[UpAxis]);

      // Update horizontal panning of actor
      actorDir3 = HV_VECTOR3(actorDir2, 0);
      actorDir3.Normalize();
      actorTrans.LookAt(actorDir3, UpVector);
      player.GetObject()->SetTransform(actorTrans);
    }

    // Update up/down camera panning
    // TODO: Zoom out/in when in 3rd person mode
    if (cameraMode == CameraMode1stPerson)
    {
      camDir3.Normalize();

      csScalar upDownAmount = 0;
      if (kbd->GetKeyState (KeyUp))
      {
        if (vertCos + turnAmount < MaxVertCos) upDownAmount -= turnAmount;
      }
      if (kbd->GetKeyState (KeyDown))
      {
        if (vertCos - turnAmount > -MaxVertCos) upDownAmount += turnAmount;
      }

      if (upDownAmount)
      {
        actorDir2.Rotate(HALF_PI);
        csVector3 camOrth3 = HV_VECTOR3(actorDir2, 0);
        camOrth3.Normalize();

        // rotate by quaternion
        csQuaternion q;
        q.SetAxisAngle(camOrth3, upDownAmount);
        camDir3 = q.Rotate(camDir3);
      }
      camTrans.LookAt(camDir3, UpVector);
    }
  }
}

void PhysDemo::MoveCamera()
{
  iCamera* cam = view->GetCamera();
  csOrthoTransform& camTrans = cam->GetTransform();

  if (player.GetObject())
  {
    // set sector
    if (cam->GetSector() != player.GetObject()->GetSector()->GetSector())
    {
      cam->SetSector(player.GetObject()->GetSector()->GetSector());
    }

    // adjust camera relative to actor
    csOrthoTransform actorTrans = player.GetObject()->GetTransform();
    csVector3 targetPos = player.GetObject()->GetTransform().GetOrigin();

    if (cameraMode != CameraMode1stPerson)
    {
      csVector3 pos = camTrans.GetOrigin();

      // camera follows the actor, looking over the shoulder
      csScalar camDistFactor = cameraMode == CameraMode3rdPersonFar ? 3 : 1;
      csScalar camDistance = 2 * camDistFactor * ActorDimensions.Norm();

      targetPos -= camDistance * actorTrans.GetT2O() * csVector3(0, -1, 1); // * (1 / SQRT2)

      // interpolate between current pos and target pos
      static const csScalar targetWeight = csScalar(0.1);
      targetPos = targetWeight * targetPos + (1 - targetWeight) * pos;
      camTrans.SetOrigin(targetPos);

      // let the camera look at a point in front of the actor
      csVector3 lookAtPos = actorTrans.GetOrigin() + actorTrans.GetT2O() * csVector3(0, 0, 1) - targetPos;
      camTrans.LookAt(lookAtPos, UpVector);
    }
    else
    {
      // Move camera eye level (~ 0.9 * actorheight)
      camTrans.SetOrigin(targetPos + csVector3(0, csScalar(.4) * ActorDimensions.y, 0));
    }
  }
}

void PhysDemo::UpdateDragging()
{
  if (dragging)
  {
    iCamera* cam = view->GetCamera();
    csOrthoTransform& camTrans = cam->GetTransform();

    // Keep the drag joint at the same distance to the camera
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = cam->InvPerspective (v2d, 10000);
    csVector3 startBeam = camTrans.GetOrigin();
    csVector3 endBeam = camTrans.This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize();
    newPosition = camTrans.GetOrigin() + newPosition * dragDistance;
    dragJoint->SetPosition (newPosition);
  }
}

void PhysDemo::UpdateHUD()
{
  hudManager->GetStateDescriptions()->Empty();
  csString txt;

  hudManager->GetStateDescriptions()->Push (csString ("Physics engine: ") + phys_engine_name);

  if (actorVehicle || player.GetObject()->QueryPhysicalBody())
  {
    csScalar speed;
    if (actorVehicle)
    {
      speed = actorVehicle->GetSpeedKMH();
    }
    else
    {
      speed = csScalar(3.6) * player.GetObject()->QueryPhysicalBody()->GetLinearVelocity().Norm();
    }
    txt.Format ("Speed : %.3f km/h", speed);
    hudManager->GetStateDescriptions()->Push (txt);
  }

  txt.Format ("Collision objects: %zu", GetCurrentSector()->GetCollisionObjectCount());
  hudManager->GetStateDescriptions()->Push (txt);

  txt.Format ("Rigid bodies: %zu", GetCurrentSector()->GetRigidBodyCount());
  hudManager->GetStateDescriptions()->Push (txt);

  if (isSoftBodyWorld)
  {
    txt.Format ("Soft bodies: %zu", GetCurrentSector()->GetSoftBodyCount());
    hudManager->GetStateDescriptions()->Push (txt);
  }

  switch (actorMode)
  {
  case ActorModeDynamic:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: Dynamic"));
    break;

  case ActorModeNoclip:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: Free"));
    break;

  case ActorModeKinematic:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: Kinematic"));
    break;

  default:
    break;
  }
}

void PhysDemo::DoDebugDraw()
{
  if (do_bullet_debug)
    GetCurrentSector()->DebugDraw (view);
  else if (isSoftBodyWorld && do_soft_debug)
    for (size_t i = 0; i < GetCurrentSector()->GetSoftBodyCount(); i++)
    {
      CS::Physics::iSoftBody* softBody = GetCurrentSector()->GetSoftBody (i);
      csRef<CS::Physics::iSoftBody> bulletSoftBody = 
        scfQueryInterface<CS::Physics::iSoftBody> (softBody);
      if (softBody->GetVertexCount())
        bulletSoftBody->DebugDraw (view);
    }
}