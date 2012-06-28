/**
 * Render/print/draw and handle the camera
 */

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

  // Now rotate the camera according to keyboard state
  const float timeMs = elapsed_time / 1000.0;

  // Camera is attached to a rigid body
  // TODO: Clean this mess up
  if (physicalCameraMode == CAMERA_DYNAMIC || physicalCameraMode == CAMERA_KINEMATIC)
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_ROT_RIGHT, timeMs);
    if (kbd->GetKeyState (CSKEY_LEFT))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_ROT_LEFT, timeMs);
    if (kbd->GetKeyState (CSKEY_PGUP))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_TILT_UP, timeMs);
    if (kbd->GetKeyState (CSKEY_PGDN))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_TILT_DOWN, timeMs);

    if (physicalCameraMode == CAMERA_DYNAMIC)
    {
      // Check if actor is on ground
      const csVector3& vel = cameraBody->GetLinearVelocity();
      if (vel[UpAxis] < 1)  // check that the actor is not already lifting off
      {
        // cast ray from center of mass downward (assume it is not a straight-standing shape)
        // to right beneath the actor's feet
        static const float groundAngleCosThresh = 0.7f;             // min cos of angle between ground and up-axis
        static const float walkSpeed = 5.f;
        static const float jumpAccel = 6.f * cameraBody->GetMass();

        csVector3 aabbMin, aabbMax;
        cameraBody->GetAABB(aabbMin, aabbMax);
        
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

        if (objBeneathCount)
        {
          // actor is on ground
          if (kbd->GetKeyState (CSKEY_UP) || kbd->GetKeyState (CSKEY_DOWN))
          {
            csVector3 newVelRel(0);
            if (kbd->GetKeyState (CSKEY_UP))
            {
              newVelRel = csVector3(0, 0, 1);
            }
            else if (kbd->GetKeyState (CSKEY_DOWN))
            {
              newVelRel = csVector3(0, 0, -1);
            }

            csVector3 newVel = view->GetCamera()->GetTransform().GetT2O() * newVelRel;

            // aim a little bit higher to overcome slopes
            newVel[UpAxis] = 0.01f;
            newVel.Normalize();

            newVel *= walkSpeed;

            cameraBody->SetLinearVelocity(newVel);
          }
          if (kbd->GetKeyState(CSKEY_SPACE))
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
      }
    }
    else
    {
      csOrthoTransform trans = view->GetCamera()->GetTransform();
      if (kbd->GetKeyState (CSKEY_UP))
        trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0, 0, 5) * timeMs);
      if (kbd->GetKeyState (CSKEY_DOWN))
        trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0, 0, -5) * timeMs);
      cameraBody->SetTransform (trans);
    }
  }
  else if (physicalCameraMode == CAMERA_ACTOR)
  {
    iCamera* camera = view->GetCamera();
    if (kbd->GetKeyState (CSKEY_RIGHT))
    {
      cameraActor->IncreaseYaw (timeMs);
    }
    else if (kbd->GetKeyState (CSKEY_LEFT))
    {
      cameraActor->IncreaseYaw (-timeMs);
    }

    if (kbd->GetKeyState (CSKEY_PGUP))
    {
      cameraActor->IncreasePitch(timeMs);
    }
    else if (kbd->GetKeyState (CSKEY_PGDN))
    {
      cameraActor->IncreasePitch(-timeMs);
    }

    if (cameraActor->IsOnGround() || physicalSector->GetGravity()[1] == 0)    // actor can only be moved while on ground or when flying
    {
      if (kbd->GetKeyState (CSKEY_UP))
      {
        cameraActor->SetPlanarVelocity(csVector2(actorSpeed, 0));
      }
      else if (kbd->GetKeyState (CSKEY_DOWN))
      {
        cameraActor->SetPlanarVelocity(csVector2(-actorSpeed, 0));
      }
      else
      {
        cameraActor->SetPlanarVelocity(csVector2(0, 0));
      }
      if (kbd->GetKeyState(CSKEY_SPACE))
      {
        cameraActor->Jump();
      }
    }
  }

  // Camera is free
  else
  {
    iCamera* c = view->GetCamera();
    
    float cameraSpeed = environment == ENVIRONMENT_TERRAIN ? 30.0f : 4.0f;
    if (kbd->GetKeyState (CSKEY_SHIFT))
    {
      // If the user is holding down shift, the arrow keys will cause
      // the camera to strafe up, down, left or right from it's
      // current position.
      if (kbd->GetKeyState (CSKEY_RIGHT))
        c->Move (CS_VEC_RIGHT * cameraSpeed * timeMs);
      if (kbd->GetKeyState (CSKEY_LEFT))
        c->Move (CS_VEC_LEFT * cameraSpeed * timeMs);
      if (kbd->GetKeyState (CSKEY_UP))
        c->Move (CS_VEC_UP * cameraSpeed * timeMs);
      if (kbd->GetKeyState (CSKEY_DOWN))
        c->Move (CS_VEC_DOWN * cameraSpeed * timeMs);
    }
    else
    {
      // left and right cause the camera to rotate on the global Y
      // axis; page up and page down cause the camera to rotate on the
      // _camera's_ X axis (more on this in a second) and up and down
      // arrows cause the camera to go forwards and backwards.
      if (kbd->GetKeyState (CSKEY_RIGHT))
	c->GetTransform().RotateThis (csVector3 (0.0f, 1.0f, 0.0f), timeMs);
      if (kbd->GetKeyState (CSKEY_LEFT))
	c->GetTransform().RotateThis (csVector3 (0.0f, 1.0f, 0.0f), -timeMs);
      if (kbd->GetKeyState (CSKEY_PGUP))
	c->GetTransform().RotateThis (csVector3 (1.0f, 0.0f, 0.0f), -timeMs);
      if (kbd->GetKeyState (CSKEY_PGDN))
	c->GetTransform().RotateThis (csVector3 (1.0f, 0.0f, 0.0f), timeMs);
      if (kbd->GetKeyState (CSKEY_UP))
        c->Move (CS_VEC_FORWARD * cameraSpeed * timeMs);
      if (kbd->GetKeyState (CSKEY_DOWN))
        c->Move (CS_VEC_BACKWARD * cameraSpeed * timeMs);
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
    physicalSector->Step (timeMs / dynamicSpeed);
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
        cameraActor->UpdateAction(timeMs / dynamicSpeed);
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