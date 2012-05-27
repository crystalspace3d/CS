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

void PhysDemo::PrintHelp ()
{
  csCommandLineHelper commandLineHelper;

  // Command line options
  commandLineHelper.AddCommandLineOption
    ("phys_engine", "Specify which physics plugin to use", csVariant ("bullet2"));
  commandLineHelper.AddCommandLineOption
    ("soft", "Enable the soft bodies", csVariant (true));
  commandLineHelper.AddCommandLineOption
    ("level", csString ().Format ("Define the level to be loaded, can be %s, %s, %s",
				  CS::Quote::Single ("portals"),
				  CS::Quote::Single ("box"),
				  CS::Quote::Single ("terrain")),
     csVariant ("terrain"));

  // Printing help
  commandLineHelper.PrintApplicationHelp
    (GetObjectRegistry (), "phystut2",
    "phystut2 <OPTIONS>",
    "Physics tutorial 2 for Crystal Space.");
}

void PhysDemo::Frame ()
{
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks ();

  // Now rotate the camera according to keyboard state
  const float speed = elapsed_time / 1000.0;

  // Camera is controlled by a rigid body
  if (physicalCameraMode == CAMERA_DYNAMIC || physicalCameraMode == CAMERA_KINEMATIC)
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_ROT_RIGHT, speed);
    if (kbd->GetKeyState (CSKEY_LEFT))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_ROT_LEFT, speed);
    if (kbd->GetKeyState (CSKEY_PGUP))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_TILT_UP, speed);
    if (kbd->GetKeyState (CSKEY_PGDN))
      view->GetCamera()->GetTransform().RotateThis (CS_VEC_TILT_DOWN, speed);
    if (physicalCameraMode == CAMERA_DYNAMIC)
    {
      if (kbd->GetKeyState (CSKEY_UP))
      {
        cameraBody->SetLinearVelocity (view->GetCamera()->GetTransform()
          .GetT2O () * csVector3 (0, 0, 5));
      }
      if (kbd->GetKeyState (CSKEY_DOWN))
      {
        cameraBody->SetLinearVelocity (view->GetCamera()->GetTransform()
          .GetT2O () * csVector3 (0, 0, -5));
      }
    }
    else
    {
      csOrthoTransform trans = view->GetCamera()->GetTransform();
      if (kbd->GetKeyState (CSKEY_UP))
        trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0, 0, 5) * speed);
      if (kbd->GetKeyState (CSKEY_DOWN))
        trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0, 0, -5) * speed);
      cameraBody->SetTransform (trans);
    }
  }
  else if (physicalCameraMode == CAMERA_ACTOR)
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      cameraActor->Rotate (CS_VEC_ROT_RIGHT, speed);
    if (kbd->GetKeyState (CSKEY_LEFT))
      cameraActor->Rotate (CS_VEC_ROT_LEFT, speed);
    if (kbd->GetKeyState (CSKEY_PGUP))
      cameraActor->Rotate (CS_VEC_TILT_UP, speed);
    if (kbd->GetKeyState (CSKEY_PGDN))
      cameraActor->Rotate (CS_VEC_TILT_DOWN, speed);
    if (kbd->GetKeyState (CSKEY_UP))
      cameraActor->SetVelocity (5);
    if (kbd->GetKeyState (CSKEY_DOWN))
      cameraActor->SetVelocity (-5);
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
        c->Move (CS_VEC_RIGHT * cameraSpeed * speed);
      if (kbd->GetKeyState (CSKEY_LEFT))
        c->Move (CS_VEC_LEFT * cameraSpeed * speed);
      if (kbd->GetKeyState (CSKEY_UP))
        c->Move (CS_VEC_UP * cameraSpeed * speed);
      if (kbd->GetKeyState (CSKEY_DOWN))
        c->Move (CS_VEC_DOWN * cameraSpeed * speed);
    }
    else
    {
      // left and right cause the camera to rotate on the global Y
      // axis; page up and page down cause the camera to rotate on the
      // _camera's_ X axis (more on this in a second) and up and down
      // arrows cause the camera to go forwards and backwards.
      if (kbd->GetKeyState (CSKEY_RIGHT))
	c->GetTransform ().RotateThis (csVector3 (0.0f, 1.0f, 0.0f), speed);
      if (kbd->GetKeyState (CSKEY_LEFT))
	c->GetTransform ().RotateThis (csVector3 (0.0f, 1.0f, 0.0f), -speed);
      if (kbd->GetKeyState (CSKEY_PGUP))
	c->GetTransform ().RotateThis (csVector3 (1.0f, 0.0f, 0.0f), -speed);
      if (kbd->GetKeyState (CSKEY_PGDN))
	c->GetTransform ().RotateThis (csVector3 (1.0f, 0.0f, 0.0f), speed);
      if (kbd->GetKeyState (CSKEY_UP))
        c->Move (CS_VEC_FORWARD * cameraSpeed * speed);
      if (kbd->GetKeyState (CSKEY_DOWN))
        c->Move (CS_VEC_BACKWARD * cameraSpeed * speed);
    }
  }

  if (dragging)
  {
    // Keep the drag joint at the same distance to the camera
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), g2d->GetHeight () - mouse->GetLastY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize ();
    newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
    dragJoint->SetPosition (newPosition);
  }

  if (!pauseDynamic)
    physicalSector->Step (speed / dynamicSpeed);

  GripContactBodies ();

  if (physicalCameraMode == CAMERA_DYNAMIC/* || physicalCameraMode == CAMERA_KINEMATIC*/)
  {
    view->GetCamera ()->GetTransform ().SetOrigin
    (cameraBody->GetTransform ().GetOrigin ());
  }
  if (physicalCameraMode == CAMERA_ACTOR)
    cameraActor->UpdateAction (speed / dynamicSpeed);

  // Update the demo's state information
  hudManager->GetStateDescriptions ()->Empty ();
  csString txt;

  hudManager->GetStateDescriptions ()->Push (csString ("Physics engine: ") + phys_engine_name);

  txt.Format ("Collision objects count: %zu", collisionSector->GetCollisionObjectCount ());
  hudManager->GetStateDescriptions ()->Push (txt);

  txt.Format ("Rigid bodies count: %zu", physicalSector->GetRigidBodyCount ());
  hudManager->GetStateDescriptions ()->Push (txt);

  if (isSoftBodyWorld)
  {
    txt.Format ("Soft bodies count: %zu", physicalSector->GetSoftBodyCount ());
    hudManager->GetStateDescriptions ()->Push (txt);
  }

  switch (physicalCameraMode)
  {
  case CAMERA_DYNAMIC:
    hudManager->GetStateDescriptions ()->Push (csString ("Camera mode: dynamic"));
    break;

  case CAMERA_FREE:
    hudManager->GetStateDescriptions ()->Push (csString ("Camera mode: free"));
    break;

  case CAMERA_KINEMATIC:
    hudManager->GetStateDescriptions ()->Push (csString ("Camera mode: kinematic"));
    break;

  case CAMERA_ACTOR:
    hudManager->GetStateDescriptions ()->Push (csString ("Camera mode: actor"));

  default:
    break;
  }

  // Default behavior from DemoApplication
  DemoApplication::Frame ();

  // Display debug informations
  if (do_bullet_debug)
    bulletSector->DebugDraw (view);
  else if (isSoftBodyWorld && do_soft_debug)
    for (size_t i = 0; i < physicalSector->GetSoftBodyCount (); i++)
    {
      CS::Physics::iSoftBody* softBody = physicalSector->GetSoftBody (i);
      csRef<CS::Physics::Bullet2::iSoftBody> bulletSoftBody = 
        scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (softBody);
      if (softBody->GetVertexCount ())
        bulletSoftBody->DebugDraw (view);
    }
}

void PhysDemo::UpdateCameraMode ()
{
  switch (physicalCameraMode)
  {
    // The camera is controlled by a rigid body
  case CAMERA_DYNAMIC:
    {
      // Check if there is already a rigid body created for the 'kinematic' mode
      if (cameraBody)
      {
        cameraBody->SetState (CS::Physics::STATE_DYNAMIC);

        // Remove the attached camera (in this mode we want to control
        // the orientation of the camera, so we update the camera
        // position by ourselves)
        cameraBody->SetAttachedCamera (0);
      }

      // Create a new rigid body
      else
      {
        csRef<CS::Collisions::iCollider> sphere = collisionSystem->CreateColliderSphere (0.8f);
        cameraBody = physicalSystem->CreateRigidBody ();
        cameraBody->SetDensity (0.3f);
        cameraBody->SetElasticity (0.8f);
        cameraBody->SetFriction (100.0f);

        cameraBody->AddCollider(sphere, localTrans);
        cameraBody->RebuildObject ();
        physicalSector->AddRigidBody (cameraBody);
      }

      const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();
      cameraBody->SetTransform (tc);

      break;
    }

    // The camera is free
  case CAMERA_FREE:
    {
      if (cameraBody)
      {
        physicalSector->RemoveRigidBody (cameraBody);
        cameraBody = NULL;
      }

      // Update the display of the dynamics debugger
      //dynamicsDebugger->UpdateDisplay ();

      break;
    }

  case CAMERA_ACTOR:
    {
     
      cameraActor = collisionSystem->CreateCollisionActor ();

      csRef<CS::Collisions::iColliderSphere> sphere = collisionSystem->CreateColliderSphere (0.8f);
      csOrthoTransform localTrans;
      cameraActor->AddCollider (sphere, localTrans);
      cameraActor->SetCamera (view->GetCamera ());
      cameraActor->RebuildObject ();

      collisionSector->AddCollisionActor (cameraActor);
      
      break;
    }

    // The camera is kinematic
  case CAMERA_KINEMATIC:
    {
      collisionSector->RemoveCollisionActor ();
      // Create a body
      csRef<CS::Collisions::iColliderSphere> sphere = collisionSystem->CreateColliderSphere (0.8f);
      csOrthoTransform localTrans;
      cameraBody = physicalSystem->CreateRigidBody ();
      cameraBody->AddCollider(sphere, localTrans);
      const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();
      cameraBody->SetTransform (tc);

      cameraBody->SetDensity (1.0f);
      cameraBody->SetElasticity (0.8f);
      cameraBody->SetFriction (100.0f);
      cameraBody->RebuildObject ();

      // Attach the camera to the body so as to benefit of the default
      // kinematic callback
      cameraBody->SetAttachedCamera (view->GetCamera ());

      // Make it kinematic
      cameraBody->SetState (CS::Physics::STATE_KINEMATIC);

      physicalSector->AddRigidBody (cameraBody);

      break;
    }

  default:
    break;
  }
}

// This method updates the position of the dragging for soft bodies
csVector3 MouseAnchorAnimationControl::GetAnchorPosition () const
{
  // Keep the drag joint at the same distance to the camera
  csRef<iCamera> camera = simple->view->GetCamera ();
  csVector2 v2d (simple->mouse->GetLastX (), simple->g2d->GetHeight () - simple->mouse->GetLastY ());
  csVector3 v3d = camera->InvPerspective (v2d, 10000);
  csVector3 startBeam = camera->GetTransform ().GetOrigin ();
  csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

  csVector3 newPosition = endBeam - startBeam;
  newPosition.Normalize ();
  newPosition = camera->GetTransform ().GetOrigin () + newPosition * simple->dragDistance;
  return newPosition;
}