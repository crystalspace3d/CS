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

  const float timeMs = elapsed_time / 1000.0;

  // handle movement & camera rotation
  if (kbd->GetKeyState (KeyLeft) ||
    kbd->GetKeyState (KeyRight) ||
    kbd->GetKeyState (KeyUp) ||
    kbd->GetKeyState (KeyDown))
  {
    // Rotate camera
    float turnAmount = turnSpeed * timeMs;
    static const float MaxVertCos = .965f;      // can't get closer than 15 degrees to up axis

    csOrthoTransform& camTrans = view->GetCamera()->GetTransform();
    csVector3 camDir3 = camTrans.GetT2O() * csVector3(0, 0, 1);
    float vertCos = camDir3 * UpVector;

    float leftRightAmount = 0;
    if (kbd->GetKeyState (KeyLeft))
    {
      leftRightAmount -= turnAmount;
    }
    if (kbd->GetKeyState (KeyRight))
    {
      leftRightAmount += turnAmount;
    }

    float upDownAmount = 0;
    if (kbd->GetKeyState (KeyUp))
    {
      if (vertCos + turnAmount < MaxVertCos) upDownAmount -= turnAmount;
    }
    if (kbd->GetKeyState (KeyDown))
    {
      if (vertCos - turnAmount > -MaxVertCos) upDownAmount += turnAmount;
    }

    csVector2 camDir2 = HORIZONTAL_COMPONENT(camDir3);
    if (leftRightAmount)
    {
      camDir2.Rotate(leftRightAmount);
      camDir3 = HV_VECTOR3(camDir2, camDir3[UpAxis]);
    }
    camDir3.Normalize();

    if (upDownAmount)
    {
      camDir2.Rotate(HALF_PI);
      csVector3 camOrth3 = HV_VECTOR3(camDir2, 0);
      camOrth3.Normalize();

      // rotate by quaternion
      csQuaternion q;
      q.SetAxisAngle(camOrth3, upDownAmount);
      camDir3 = q.Rotate(camDir3);
    }
    camTrans.LookAt(camDir3, UpVector);
  }

  // handle movement
  bool gravityOff = fabs(physicalSector->GetGravity()[UpAxis]) < EPSILON;
  if (physicalCameraMode == CAMERA_DYNAMIC)
  {
    // Check if actor is on ground
    static const float groundAngleCosThresh = 0.7f;             // min cos of angle between ground and up-axis (45 degrees)
    static const float jumpAccel = 15.f * cameraBody->GetMass();

    // Find any objects beneath the actor's feet
    csArray<CollisionData> collisions;
    physicalSector->CollisionTest(cameraBody, collisions);

    int objBeneathCount = 0;
    for (size_t i = 0; i < collisions.GetSize (); ++i)
    {
      CollisionData& coll = collisions[i];

      float groundAngleCos = coll.normalWorldOnB * UpVector;
      if (groundAngleCos > groundAngleCosThresh)
      {
        ++objBeneathCount;
      }
    }

    const csVector3& vel = cameraBody->GetLinearVelocity();
    bool freeFall = !objBeneathCount && !gravityOff;
    bool movingUp = vel[UpAxis] > 1;

    if ((!movingUp || gravityOff || actorAirControl) &&
      (kbd->GetKeyState (KeyForward) || kbd->GetKeyState (KeyBack) || kbd->GetKeyState(KeyJump)))
    {
      // actor is on ground, flying or has air control, and tries to move
      if (kbd->GetKeyState (KeyForward) || kbd->GetKeyState (KeyBack))
      {
        // horizontal movement
        csVector3 newVelRel(0);
        if (kbd->GetKeyState (KeyForward))
        {
          newVelRel = csVector3(0, 0, 1);
        }
        else if (kbd->GetKeyState (KeyBack))
        {
          newVelRel = csVector3(0, 0, -1);
        }

        csVector3 newVel = view->GetCamera()->GetTransform().GetT2O() * newVelRel;

        csVector2 newVel2 = HORIZONTAL_COMPONENT(newVel);   // horizontal component of velocity
        newVel2.Normalize();
        newVel2 *= moveSpeed;

        if (freeFall || movingUp)
        {
          // cannot entirely control movement mid-air
          newVel2 = actorAirControl * newVel2;
          newVel2 += (1.f - actorAirControl) * HORIZONTAL_COMPONENT(vel);
        }

        newVel = HV_VECTOR3(newVel2, vel[UpAxis]);

        cameraBody->SetLinearVelocity(newVel);
      }
      if (!freeFall && !movingUp && kbd->GetKeyState(KeyJump))
      {
        // jump

        // we stand on top of something -> Apply force to both objects
        // of course this is not quite correct, since the up force on the actor depends on the restitution of both objects
        // (imagine jumping from a soft piece of cloth vs. jumping from concrete)
        csVector3 force(jumpAccel * UpVector);

        // jump accelerates the body upward
        cameraBody->AddForce(force);

        // apply inverse of force to objects beneath
        float currentSpeed = vel.Norm();
        if (currentSpeed > 1)
        {
          // apply a 45¢X backwards force on the other object when walking forward
          force = UpVector * currentSpeed + vel;
          force /= 2 * currentSpeed;
          force.Normalize();
          force *= -jumpAccel / objBeneathCount;   // split force between all objects beneath
        }

        for (size_t i = 0; i < collisions.GetSize (); ++i)
        {
          CollisionData& coll = collisions[i];

          if (coll.normalWorldOnB * UpAxis < groundAngleCosThresh)
          {
            iPhysicalBody* pb = coll.objectB->QueryPhysicalBody();
            pb->AddForce(force);
          }
        }
      }
    }
    else
    {
      if (!freeFall)
      {
        // no movement keys pressed and not drifting mid-air
        // stop moving
        cameraBody->SetLinearVelocity(0);
      }
    }
  }
  else if (physicalCameraMode == CAMERA_ACTOR)
  {
    if (cameraActor->IsOnGround() || gravityOff)    // actor can only be moved while on ground or when there is no gravity
    {
      if (kbd->GetKeyState (KeyForward))
      {
        cameraActor->SetPlanarVelocity(csVector2(moveSpeed, 0));
      }
      else if (kbd->GetKeyState (KeyBack))
      {
        cameraActor->SetPlanarVelocity(csVector2(-moveSpeed, 0));
      }
      else
      {
        cameraActor->SetPlanarVelocity(csVector2(0, 0));
      }
      if (kbd->GetKeyState(KeyJump))
      {
        cameraActor->Jump();
      }
    }
  }
  else
  {
    // Camera is free
    iCamera* c = view->GetCamera();

    if (kbd->GetKeyState (CSKEY_SHIFT))
    {
      // If the user is holding down shift, the arrow keys will cause
      // the camera to strafe up, down, left or right from it's
      // current position.
      if (kbd->GetKeyState (KeyRight))
        c->Move (CS_VEC_RIGHT * moveSpeed * timeMs);
      if (kbd->GetKeyState (KeyLeft))
        c->Move (CS_VEC_LEFT * moveSpeed * timeMs);
      if (kbd->GetKeyState (KeyForward))
        c->Move (CS_VEC_UP * moveSpeed * timeMs);
      if (kbd->GetKeyState (KeyBack))
        c->Move (CS_VEC_DOWN * moveSpeed * timeMs);
    }
    else
    {
      if (kbd->GetKeyState (KeyForward))
        c->Move (CS_VEC_FORWARD * moveSpeed * timeMs);
      if (kbd->GetKeyState (KeyBack))
        c->Move (CS_VEC_BACKWARD * moveSpeed * timeMs);
    }
  }

  if (dragging)
  {
    // Keep the drag joint at the same distance to the camera
    csRef<iCamera> camera = view->GetCamera();
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform().GetOrigin();
    csVector3 endBeam = camera->GetTransform().This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize();
    newPosition = camera->GetTransform().GetOrigin() + newPosition * dragDistance;
    dragJoint->SetPosition (newPosition);
  }

  if (!pauseDynamic)
  {
    physicalSector->Step (timeMs * dynamicStepFactor);
  }

  GripContactBodies();


  // update camera object
  switch (physicalCameraMode)
  {
  case CAMERA_DYNAMIC:
    {
      view->GetCamera()->GetTransform().SetOrigin(cameraBody->GetTransform().GetOrigin());
      break;
    }

  case CAMERA_ACTOR:
    {
      if (cameraActor->IsInWorld())
      {
        cameraActor->UpdateAction(timeMs * dynamicStepFactor);
      }
      break;
    }
  }


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
  case CAMERA_DYNAMIC:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: dynamic"));
    break;

  case CAMERA_FREE:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: free"));
    break;

  case CAMERA_KINEMATIC:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: kinematic"));
    break;

  case CAMERA_ACTOR:
    hudManager->GetStateDescriptions()->Push (csString ("Camera mode: actor"));

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