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
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks();

  float moveSpeed = environment == ENVIRONMENT_TERRAIN ? 30.0f : 4.0f;

  static const float MaxVertCos = .965f;      // can't get closer than 15 degrees to UpAxis to prevent gimbal lock

  const float timeMs = elapsed_time / 1000.0;

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

    float leftRightAmount = 0;
    if (kbd->GetKeyState (KeyLeft) || kbd->GetKeyState(CSKEY_LEFT))
    {
      leftRightAmount -= turnAmount;
    }
    if (kbd->GetKeyState (KeyRight) || kbd->GetKeyState(CSKEY_RIGHT))
    {
      leftRightAmount += turnAmount;
    }

    csVector2 actorDir2 = HORIZONTAL_COMPONENT(actorDir3);
    if (leftRightAmount)
    {
      actorDir2.Rotate(leftRightAmount);

      csVector2 camDir2 = HORIZONTAL_COMPONENT(camDir3);
      camDir2.Rotate(leftRightAmount);
      camDir3 = HV_VECTOR3(camDir2, camDir3[UpAxis]);

      // Update horizontal panning of actor
      actorDir3 = HV_VECTOR3(actorDir2, 0);
      actorDir3.Normalize();
      actorTrans.LookAt(actorDir3, UpVector);
      player.GetObject()->SetTransform(actorTrans);
    }

    // Update up/down camera panning
    // TODO: Zoom out/in when in 3rd person mode
    if (camFollowMode == CamFollowMode1stPerson)
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

  // Dynamics and actor simulation
  if (!pauseDynamic)
  {
    if (physicalSector->GetSoftBodyCount())
    {
      csVector3 aabbMin, aabbMax;
      physicalSector->GetSoftBody(0)->GetAABB(aabbMin, aabbMax);
      csVector3 o = aabbMin + (aabbMax - aabbMin) / 2;
      csPrintf("SB #0: %f, %f, %f\n", o.x, o.y, o.z);
    }

    // handle movement
    // intended movement direction
    bool gravityOff = physicalSector->GetGravity().SquaredNorm() == 0;

    csVector3 newVel(0);
    if (kbd->GetKeyState (KeyForward) || kbd->GetKeyState(CSKEY_UP))
    {
      newVel.z += 1;
    }
    if (kbd->GetKeyState (KeyBack) || kbd->GetKeyState(CSKEY_DOWN))
    {
      newVel.z -= 1;
    }
    if (kbd->GetKeyState (KeyStrafeLeft))
    {
      newVel.x -= 1;
    }
    if (kbd->GetKeyState (KeyStrafeRight))
    {
      newVel.x += 1;
    }

    bool hasMoveDir = newVel.x + newVel.z + newVel.z != 0;
    bool wantsToMove = hasMoveDir | kbd->GetKeyState(KeyJump);
    if (player.GetActor())
    {
      iCollisionObject* colCurrentActor = player.GetObject();
      /*csVector3 aabbMin, aabbMax;
      colCurrentActor->GetAABB(aabbMin, aabbMax);*/

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
        }

        bool freeFall = player.GetActor()->IsFreeFalling();
        bool gravityOff = physicalSector->GetGravity().SquaredNorm() == 0;

        // actor is on ground, flying or has air control
        if (hasMoveDir && (!freeFall || player.GetActor()->GetAirControlFactor() > 0)) 
        {
          // wants to move and may move
          if (!gravityOff)
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
      if (kbd->GetKeyState(KeyJump))
      {
        // add upward movement
        newVel[UpAxis] += moveSpeed;
      }
      cam->Move (newVel * moveSpeed * timeMs);
    }

    if (player.GetActor())
    {
      player.GetActor()->UpdatePreStep(timeMs * dynamicStepFactor);
    }
    physicalSector->Step (timeMs * dynamicStepFactor);
    if (player.GetActor())
    {
      player.GetActor()->UpdatePostStep(timeMs * dynamicStepFactor);
    }
  }

  if (dragging)
  {
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

  if (player.GetActor())
  {
    // adjust camera relative to actor
    csVector3 targetPos = player.GetObject()->GetTransform().GetOrigin();

    if (camFollowMode != CamFollowMode1stPerson)
    {
      csVector3 pos = camTrans.GetOrigin();

      // camera follows behind the actor, looking over the shoulder
      csScalar camDistFactor = camFollowMode == CamFollowMode3rdPersonFar ? 3 : 1;
      csScalar camDistance = 2 * camDistFactor * ActorDimensions.Norm();

      targetPos -= camDistance * actorTrans.GetT2O() * csVector3(0, -1, 1); // * (1 / SQRT2)

      // interpolate between current pos and target pos
      static const csScalar targetWeight = csScalar(0.1);
      targetPos = targetWeight * targetPos + (1 - targetWeight) * pos;
      camTrans.SetOrigin(targetPos);

      // let the camera look in front of the actor
      csVector3 lookAtPos = actorTrans.GetOrigin() + actorTrans.GetT2O() * csVector3(0, 0, 1) - targetPos;
      camTrans.LookAt(lookAtPos, UpVector);
    }
    else
    {
      camTrans.SetOrigin(targetPos);
    }
  }

  GripContactBodies();

  // Update the demo's state information
  hudManager->GetStateDescriptions()->Empty();
  csString txt;

  hudManager->GetStateDescriptions()->Push (csString ("Physics engine: ") + phys_engine_name);

  txt.Format ("Collision objects count: %zu", physicalSector->GetCollisionObjectCount());
  hudManager->GetStateDescriptions()->Push (txt);

  txt.Format ("Rigid bodies count: %zu", physicalSector->GetRigidBodyCount());
  hudManager->GetStateDescriptions()->Push (txt);

  if (isSoftBodyWorld)
  {
    txt.Format ("Soft bodies count: %zu", physicalSector->GetSoftBodyCount());
    hudManager->GetStateDescriptions()->Push (txt);
  }

  switch (physicalCameraMode)
  {
  case ACTOR_DYNAMIC:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: dynamic"));
    break;

  case ACTOR_NOCLIP:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: free"));
    break;

  case ACTOR_KINEMATIC:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: kinematic"));
    break;

  default:
    break;
  }

  // Default behavior from DemoApplication
  DemoApplication::Frame();

  // Display debug informations
  if (do_bullet_debug)
    bulletSector->DebugDraw (view);
  else if (isSoftBodyWorld && do_soft_debug)
    for (size_t i = 0; i < physicalSector->GetSoftBodyCount(); i++)
    {
      CS::Physics::iSoftBody* softBody = physicalSector->GetSoftBody (i);
      csRef<CS::Physics::Bullet2::iSoftBody> bulletSoftBody = 
        scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (softBody);
      if (softBody->GetVertexCount())
        bulletSoftBody->DebugDraw (view);
    }
}