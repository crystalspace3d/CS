/**
* Handle keyboard/mouse/etc input
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


bool PhysDemo::OnKeyboard (iEvent &event)
{
  DemoApplication::OnKeyboard (event);

  csKeyEventType eventtype = csKeyEventHelper::GetEventType(&event);
  if (eventtype != csKeyEventTypeDown) return false;

  utf32_char code = csKeyEventHelper::GetCookedCode (&event);
  if (code >= CSKEY_F1 && code <= CSKEY_F12)
  {
    // An F-key has been pressed -> Use tool
    if (selectedItem)
    {
      ItemFunction* func = selectedItem->GetTemplate().GetSecondaryFunction(code - CSKEY_F1);
      if (func)
      {
        return func->Use(selectedItem);
      }
    }
    return false;
  }
  else if (code >= '1' && code <= '9')
  {
    int i = code - '1';
    if (player.GetInventory().GetItem(i))
    {
      // A different item has been selected: Select it and update HUD descriptions
      selectedItem = player.GetInventory().GetItem(i);
      SetupHUD();
      return true;
    }
    return false;
  }

  else if (code == 'r')
  {
    // reset
    physDemo.ResetWorld();
  }
  else if (code == 'c')
  {
    // Toggle camera mode
    switch (physicalCameraMode)
    {
    case ACTOR_KINEMATIC:
      physicalCameraMode = ACTOR_DYNAMIC;
      break;

    case ACTOR_DYNAMIC:
      physicalCameraMode = ACTOR_NOCLIP;
      break;

    case ACTOR_NOCLIP:
      physicalCameraMode = ACTOR_KINEMATIC;
      break;
    }

    UpdateCameraMode();
    return true;
  }

  else if (code == 'p')
  {
    // Toggle pause mode for dynamic simulation
    pauseDynamic = !pauseDynamic;
    if (pauseDynamic)
      printf ("Dynamic simulation paused\n");
    else
      printf ("Dynamic simulation resumed\n");
    return true;
  }

  else if (code == 'o')
  {
    // Toggle speed of dynamic simulation
    if (dynamicStepFactor - 1.0 < EPSILON)
    {
      dynamicStepFactor = 0.025f;
      printf ("Dynamic simulation slowed\n");
    }
    else
    {
      dynamicStepFactor = 1.0f;
      printf ("Dynamic simulation at normal speed\n");
    }
  }

  else if (code == 'k')
  {
    // Toggle dynamic system visual debug mode
    // TODO
    if (do_bullet_debug)
    {
      switch (debugMode)
      {
      case CS::Physics::Bullet2::DEBUG_NOTHING:
        debugMode = CS::Physics::Bullet2::DEBUG_COLLIDERS;
        break;
      case CS::Physics::Bullet2::DEBUG_COLLIDERS:
        debugMode = CS::Physics::Bullet2::DEBUG_AABB;
        break;
      case CS::Physics::Bullet2::DEBUG_AABB:
        debugMode = CS::Physics::Bullet2::DEBUG_JOINTS;
        break;
      case CS::Physics::Bullet2::DEBUG_JOINTS:
        debugMode = CS::Physics::Bullet2::DEBUG_NOTHING;
        break;
      }
      bulletSector->SetDebugMode (debugMode);
    }
    return true;
  }
  else if (code == 'l')
  {
    // Toggle collision debug mode
    if (do_bullet_debug)
    {
      do_bullet_debug = false;
      do_soft_debug = true;
    }
    else if (do_soft_debug)
      do_soft_debug = false;
    else
      do_bullet_debug = true;

    return true;
  }
  else if (code == 'g')
  {
    // Toggle gravity.
    physicalSector->SetGravity (physicalSector->GetGravity().IsZero (EPSILON)? csVector3 (0.0f, -9.81f, 0.0f) : csVector3 (0));
    return true;
  }

  // Cut operation
  else if (csKeyEventHelper::GetRawCode (&event) == 'x'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    // Trace a beam to find if a rigid body was under the mouse cursor
    csRef<iCamera> camera = view->GetCamera();
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform().GetOrigin();
    csVector3 endBeam = camera->GetTransform().This2Other (v3d);

    CS::Collisions::HitBeamResult hitResult =
      physicalSector->HitBeam (startBeam, endBeam);
    if (hitResult.hasHit && IsDynamic(hitResult.object))
    {
      // Remove the body and the mesh from the simulation, and put them in the clipboard

      clipboardBody = hitResult.object->QueryPhysicalBody();
      clipboardMovable = hitResult.object->GetAttachedMovable();

      if (clipboardBody->GetBodyType() == CS::Physics::BODY_RIGID)
      {
        CS::Physics::iRigidBody* rigidBody = clipboardBody->QueryRigidBody();
        if (rigidBody->GetState() == CS::Physics::STATE_DYNAMIC)
        {
          size_t count = physicalSector->GetRigidBodyCount();
          physicalSector->RemoveCollisionObject (clipboardBody->QueryRigidBody());
          //room->GetMeshes()->Remove (clipboardMovable->GetSceneNode()->QueryMesh());
          if (physicalSector->GetRigidBodyCount() == count)
            clipboardBody.Invalidate();
        }
      }
      else
      {
        CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody();
        physicalSector->RemoveCollisionObject (softBody);
      }

      // Update the display of the dynamics debugger
      //dynamicsDebugger->UpdateDisplay();
    }
  }

  // Paste operation
  else if (csKeyEventHelper::GetRawCode (&event) == 'v'
    && kbd->GetKeyState (CSKEY_CTRL)
    && clipboardBody.IsValid())
  {
    // Compute the new position of the body
    csRef<iCamera> camera = view->GetCamera();
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform().GetOrigin();
    csVector3 endBeam = camera->GetTransform().This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize();
    csOrthoTransform newTransform = camera->GetTransform();
    newTransform.SetOrigin (newTransform.GetOrigin() + newPosition * 1.5f);

    // Put back the body from the clipboard to the simulation
    if (clipboardBody->GetBodyType() == CS::Physics::BODY_RIGID)
    {
      clipboardBody->SetTransform (newTransform);
      physicalSector->AddCollisionObject (clipboardBody->QueryRigidBody());
    }
    else
    {
      CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody();
      physicalSector->AddCollisionObject (softBody);
    }

    clipboardBody = 0;
    clipboardMovable = 0;

    // Update the display of the dynamics debugger
    //dynamicsDebugger->UpdateDisplay();
  }
  else if (csKeyEventHelper::GetRawCode (&event) == 'i'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    printf ("Starting profile...\n");
    bulletSector->StartProfile();
    return true;
  }

  else if (csKeyEventHelper::GetRawCode (&event) == 'o'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    printf ("Stopping profile...\n");
    bulletSector->StopProfile();
    return true;
  }

  else if (csKeyEventHelper::GetRawCode (&event) == 'p'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    bulletSector->DumpProfile();
    return true;
  }
  
  csRef<iCamera> cam = view->GetCamera();
  switch (code)
  {
  case ']':
    // Terrain stuff
    if (terrainFeeder)
    {
      if (!terrainMod)
      {
        csRef<iCamera> camera = view->GetCamera();
        csVector3 pos = camera->GetTransform().GetOrigin();

        float len = 1;
        float height = 3;
        terrainMod = terrainFeeder->AddModifier(pos, len, height);
      }
      else
      {
        terrainFeeder->RemoveModifier(terrainMod);
        terrainMod = nullptr;
      }
    }
    return true;

    // particle stuff
  case '[':
    {
      // spawn particles at the actor's feet
      float dist = 2 * ActorDimensions.y;
      float colliderRadius = dist/6;

      csVector3 pos = GetPointInFrontOfFeetXZ(dist);
      csVector3 origin = pos + csVector3 (0, dist, 0);
      //csVector3 origin = cam->GetTransform().GetOrigin() + csVector3 (0, ActorDimensions.y, 2);

      AddParticles(origin, -1);

      // add collider
      SpawnSphere(pos + csVector3(0, colliderRadius + EPSILON, 0), colliderRadius, false);
      return true;
    }
  case 'v':
    {
    // Update camera follow mode
    camFollowMode = CamFollowMode(((int)camFollowMode + 1) % (int)CamFollowModeCount);
    cam->GetTransform().LookAt(cam->GetTransform().GetT2O() * csVector3(0, 0, 1), UpVector);
    return true;
    }
  }

  return false;
}

bool PhysDemo::OnMouseDown (iEvent &event)
{
  int button = csMouseEventHelper::GetButton (&event);
  if (selectedItem && selectedItem->GetTemplate().GetPrimaryFunctions().GetSize())
  {
    // Use tool
    ItemFunction* func = selectedItem->GetTemplate().GetPrimaryFunction(button);
    if (func)
    {
      return func->Use(selectedItem);
    }
  }

  // Tool has no mouse button overrides
  if (button == 0)
  {
    // Find the rigid body that was clicked on
    // Compute the end beam points
    csRef<iCamera> camera = view->GetCamera();
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform().GetOrigin();
    csVector3 endBeam = camera->GetTransform().This2Other (v3d);

    // Trace the physical beam
    CS::Collisions::HitBeamResult hitResult =
      physicalSector->HitBeamPortal (startBeam, endBeam);
    if (!hitResult.hasHit)
      return false;

    // Add a force at the point clicked
    if (IsDynamic(hitResult.object))
    {
      csVector3 force = endBeam - startBeam;
      force.Normalize();
        force *= 100.f;

      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody();
      if (physicalBody->GetBodyType() == CS::Physics::BODY_RIGID)
      {
        csOrthoTransform trans = physicalBody->GetTransform();
        // Check if the body hit is not static or kinematic
        csRef<CS::Physics::iRigidBody> bulletBody =
          scfQueryInterface<CS::Physics::iRigidBody> (physicalBody);
        if (bulletBody->GetState() != CS::Physics::STATE_DYNAMIC)
          return false;

        physicalBody->QueryRigidBody()->AddForceAtPos (force, hitResult.isect);

        // This would work too
        //csOrthoTransform transform (hitResult.body->QueryRigidBody()->GetTransform());
        //csVector3 relativePosition = transform.Other2This (hitResult.isect);
        //hitResult.body->QueryRigidBody()->AddForceAtRelPos (force, relativePosition);
      }
      else
      {
        force *= 200.f;
        physicalBody->QuerySoftBody()->AddForce (force, hitResult.vertexIndex);
      }
    }
    else
      return false;
    return true;
  }

  // Right mouse button: dragging
  else if (button == 1)
  {
    // Find the rigid body that was clicked on
    // Compute the end beam points
    csRef<iCamera> camera = view->GetCamera();
    csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform().GetOrigin();
    csVector3 endBeam = camera->GetTransform().This2Other (v3d);

    // Trace the physical beam
    CS::Collisions::HitBeamResult hitResult = physicalSector->HitBeam (startBeam, endBeam);
    if (!hitResult.hasHit || !hitResult.object) return false;

    if (IsDynamic(hitResult.object))
    {
      // Check if we hit a rigid body
      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody();
      if (physicalBody->GetBodyType() == CS::Physics::BODY_RIGID)
      {
        csRef<CS::Physics::iRigidBody> bulletBody = scfQueryInterface<CS::Physics::iRigidBody> (physicalBody);

        // Create a p2p joint at the point clicked
        dragJoint = physicalSystem->CreateRigidPivotJoint (bulletBody, hitResult.isect);
        physicalSector->AddJoint (dragJoint);

        dragging = true;
        dragDistance = (hitResult.isect - startBeam).Norm();

        // Set some dampening on the rigid body to have a more stable dragging
        linearDampening = bulletBody->GetLinearDamping();
        angularDampening = bulletBody->GetAngularDamping();
        bulletBody->SetLinearDamping (0.9f);
        bulletBody->SetAngularDamping (0.9f);
      }
      else
      {
        softDragging = true;
        draggedBody = physicalBody->QuerySoftBody();
        draggedVertex = hitResult.vertexIndex;
        dragDistance = (hitResult.isect - startBeam).Norm();
        grabAnimationControl.AttachNew (new MouseAnchorAnimationControl (this));
        physicalBody->QuerySoftBody()->AnchorVertex (hitResult.vertexIndex, grabAnimationControl);
      }
    }
    else 
      return false;
    return true;
  }

  return false;
}

bool PhysDemo::OnMouseUp (iEvent &event)
{
  if (csMouseEventHelper::GetButton (&event) == 1
    && dragging)
  {
    dragging = false;

    // Put back the original dampening on the rigid body
    csRef<CS::Physics::iRigidBody> bulletBody =
      scfQueryInterface<CS::Physics::iRigidBody> (dragJoint->GetAttachedBody (0));
    bulletBody->SetLinearDamping (linearDampening);
    bulletBody->SetAngularDamping (angularDampening);

    // Remove the drag joint
    physicalSector->RemoveJoint (dragJoint);
    dragJoint = nullptr;
    return true;
  }

  if (csMouseEventHelper::GetButton (&event) == 1
    && softDragging)
  {
    softDragging = false;
    draggedBody->RemoveAnchor (draggedVertex);
    draggedBody = nullptr;
  }

  return false;
}


// This method updates the position of the dragging for soft bodies
csVector3 MouseAnchorAnimationControl::GetAnchorPosition() const
{
  // Keep the drag joint at the same distance to the camera
  csRef<iCamera> camera = simple->view->GetCamera();
  csVector2 v2d (simple->mouse->GetLastX(), simple->g2d->GetHeight() - simple->mouse->GetLastY());
  csVector3 v3d = camera->InvPerspective (v2d, 10000);
  csVector3 startBeam = camera->GetTransform().GetOrigin();
  csVector3 endBeam = camera->GetTransform().This2Other (v3d);

  csVector3 newPosition = endBeam - startBeam;
  newPosition.Normalize();
  newPosition = camera->GetTransform().GetOrigin() + newPosition * simple->dragDistance;
  return newPosition;
}