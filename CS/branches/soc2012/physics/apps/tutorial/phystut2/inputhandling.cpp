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


bool PhysDemo::OnKeyboard (iEvent &event)
{
  DemoApplication::OnKeyboard (event);

  csKeyEventType eventtype = csKeyEventHelper::GetEventType(&event);
  if (eventtype == csKeyEventTypeDown)
  {
    if (csKeyEventHelper::GetCookedCode (&event) == CSKEY_SPACE)
    {
      int primitiveCount = 7;
      switch (rand() % primitiveCount)
      {
      case 0: SpawnBox (); break;
      case 1: SpawnSphere (); break;
      case 2: SpawnConvexMesh (); break;
      case 3: SpawnConcaveMesh (); break;
      case 4: SpawnCylinder (); break;
      case 5: SpawnCapsule (); break;
      case 6: SpawnCompound (); break;
      default: break;
      }
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'b')
    {
      SpawnBox ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 's')
    {
      SpawnSphere ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'c')
    {
      SpawnCylinder ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'a')
    {
      SpawnCapsule ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'n')
    {
      SpawnCone ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'm')
    {
      SpawnConcaveMesh ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'v')
    {
      SpawnConvexMesh ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'q')
    {
      SpawnCompound ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'j')
    {
      SpawnJointed ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'k')
    {
      SpawnFilterBody ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'h')
    {
      SpawnChain ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'r')
    {
      SpawnFrankieRagdoll ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'e')
    {
      SpawnKrystalRagdoll ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'y' && isSoftBodyWorld)
    {
      SpawnRope ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'u' && isSoftBodyWorld)
    {
      SpawnCloth ();
      return true;
    }
    else if (csKeyEventHelper::GetCookedCode (&event) == 'i' && isSoftBodyWorld)
    {
      SpawnSoftBody ();
      return true;
    }

    else if (csKeyEventHelper::GetCookedCode (&event) == 'f')
    {
      // Toggle camera mode
      switch (physicalCameraMode)
      {
      case CAMERA_DYNAMIC:
        physicalCameraMode = CAMERA_FREE;
        break;

      case CAMERA_FREE:
          physicalCameraMode = CAMERA_ACTOR;
        break;

      case CAMERA_ACTOR:
        physicalCameraMode = CAMERA_KINEMATIC;
        break;

      case CAMERA_KINEMATIC:
        physicalCameraMode = CAMERA_DYNAMIC;
        break;
      }

      UpdateCameraMode ();
      return true;
    }

    else if (csKeyEventHelper::GetCookedCode (&event) == 't')
    {
      // Toggle all bodies between dynamic and static
      allStatic = !allStatic;

      if (allStatic)
      {
        printf ("Toggling all bodies to static mode\n");
        dynamicBodies.DeleteAll ();

        for (size_t i = 0; i < physicalSector->GetRigidBodyCount (); i++)
        {
          CS::Physics::iRigidBody* body = physicalSector->GetRigidBody (i);
          if (body->GetState () == CS::Physics::STATE_DYNAMIC)
          {
            body->SetState (CS::Physics::STATE_STATIC);
            dynamicBodies.Push (body);     
          }   
        }
      }
      else
      {
        for (size_t i = 0; i <dynamicBodies.GetSize (); i++)
        {
          CS::Physics::iRigidBody* body = dynamicBodies[i];
          body->SetState (CS::Physics::STATE_DYNAMIC);
          body->Enable ();
        }
        printf ("Toggling all bodies to dynamic mode\n");
      }

      
      return true;
    }

    else if (csKeyEventHelper::GetCookedCode (&event) == 'p')
    {
      // Toggle pause mode for dynamic simulation
      pauseDynamic = !pauseDynamic;
      if (pauseDynamic)
        printf ("Dynamic simulation paused\n");
      else
        printf ("Dynamic simulation resumed\n");
      return true;
    }

    else if (csKeyEventHelper::GetCookedCode (&event) == 'o')
    {
      // Toggle speed of dynamic simulation
      if (dynamicSpeed - 1.0 < 0.00001)
      {
        dynamicSpeed = 45.0;
        printf ("Dynamic simulation slowed\n");
      }
      else
      {
        dynamicSpeed = 1.0;
        printf ("Dynamic simulation at normal speed\n");
      }
    }

    else if (csKeyEventHelper::GetCookedCode (&event) == 'd')
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
    else if (csKeyEventHelper::GetCookedCode (&event) == '?')
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
    else if (csKeyEventHelper::GetCookedCode (&event) == 'g')
    {
      // Toggle gravity.
      collisionSector->SetGravity (collisionSector->GetGravity ().IsZero (EPSILON)?
        csVector3 (0.0f, -9.81f, 0.0f) : csVector3 (0));
      return true;
    }

    // Cut operation
    else if (csKeyEventHelper::GetRawCode (&event) == 'x'
      && kbd->GetKeyState (CSKEY_CTRL))
    {
      // Trace a beam to find if a rigid body was under the mouse cursor
      csRef<iCamera> camera = view->GetCamera ();
      csVector2 v2d (mouse->GetLastX (), g2d->GetHeight () - mouse->GetLastY ());
      csVector3 v3d = camera->InvPerspective (v2d, 10000);
      csVector3 startBeam = camera->GetTransform ().GetOrigin ();
      csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

      CS::Collisions::HitBeamResult hitResult =
        collisionSector->HitBeam (startBeam, endBeam);
      if (hitResult.hasHit
        && hitResult.object->GetType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
      {
        // Remove the body and the mesh from the simulation, and put them in the clipboard
        
        clipboardBody = hitResult.object->QueryPhysicalBody ();
        clipboardMovable = hitResult.object->GetAttachedMovable ();

        if (clipboardBody->GetType () == CS::Physics::BODY_RIGID)
        {
          CS::Physics::iRigidBody* rigidBody = clipboardBody->QueryRigidBody ();
          if (rigidBody->GetState () == CS::Physics::STATE_DYNAMIC)
          {
            size_t count = physicalSector->GetRigidBodyCount ();
            physicalSector->RemoveRigidBody (clipboardBody->QueryRigidBody ());
            //room->GetMeshes ()->Remove (clipboardMovable->GetSceneNode ()->QueryMesh ());
            if (physicalSector->GetRigidBodyCount () == count)
              clipboardBody.Invalidate ();
          }
        }
        else
        {
          CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody ();
          physicalSector->RemoveSoftBody (softBody);
        }

        // Update the display of the dynamics debugger
        //dynamicsDebugger->UpdateDisplay ();
      }
    }

    // Paste operation
    else if (csKeyEventHelper::GetRawCode (&event) == 'v'
      && kbd->GetKeyState (CSKEY_CTRL)
      && clipboardBody.IsValid ())
    {
      // Compute the new position of the body
      csRef<iCamera> camera = view->GetCamera ();
      csVector2 v2d (mouse->GetLastX (), g2d->GetHeight () - mouse->GetLastY ());
      csVector3 v3d = camera->InvPerspective (v2d, 10000);
      csVector3 startBeam = camera->GetTransform ().GetOrigin ();
      csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

      csVector3 newPosition = endBeam - startBeam;
      newPosition.Normalize ();
      csOrthoTransform newTransform = camera->GetTransform ();
      newTransform.SetOrigin (newTransform.GetOrigin () + newPosition * 1.5f);

      // Put back the body from the clipboard to the simulation
      if (clipboardBody->GetType () == CS::Physics::BODY_RIGID)
      {
        clipboardBody->SetTransform (newTransform);
        physicalSector->AddRigidBody (clipboardBody->QueryRigidBody ());
      }
      else
      {
        CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody ();
        physicalSector->AddSoftBody (softBody);
      }

      clipboardBody = 0;
      clipboardMovable = 0;

      // Update the display of the dynamics debugger
      //dynamicsDebugger->UpdateDisplay ();
    }
/*
#ifdef CS_HAVE_BULLET_SERIALIZER
    // Save a .bullet file
    else if (csKeyEventHelper::GetRawCode (&event) == 's'
      && kbd->GetKeyState (CSKEY_CTRL))
    {
      const char* filename = "phystut_world.bullet";
      if (bulletDynamicSystem->SaveBulletWorld (filename))
        printf ("Dynamic world successfully saved as file %s\n", filename);
      else
        printf ("Problem saving dynamic world to file %s\n", filename);

      return true;
    }
#endif
*/
    else if (csKeyEventHelper::GetRawCode (&event) == 'i'
      && kbd->GetKeyState (CSKEY_CTRL))
    {
      printf ("Starting profile...\n");
      bulletSector->StartProfile ();
      return true;
    }

    else if (csKeyEventHelper::GetRawCode (&event) == 'o'
      && kbd->GetKeyState (CSKEY_CTRL))
    {
      printf ("Stopping profile...\n");
      bulletSector->StopProfile ();
      return true;
    }

    else if (csKeyEventHelper::GetRawCode (&event) == 'p'
      && kbd->GetKeyState (CSKEY_CTRL))
    {
      bulletSector->DumpProfile ();
      return true;
    }
  }

  // Slow down the camera's body
  else if (physicalCameraMode == CAMERA_DYNAMIC
    && (eventtype == csKeyEventTypeUp)
    && ((csKeyEventHelper::GetCookedCode (&event) == CSKEY_DOWN) 
    || (csKeyEventHelper::GetCookedCode (&event) == CSKEY_UP)))
  {
    cameraBody->SetLinearVelocity(csVector3 (0, 0, 0));
    cameraBody->SetAngularVelocity (csVector3 (0, 0, 0));
  }

  return false;
}


bool PhysDemo::OnMouseDown (iEvent &event)
{
  if (csMouseEventHelper::GetButton (&event) == 0)
  {
    // Find the rigid body that was clicked on
    // Compute the end beam points
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), g2d->GetHeight () - mouse->GetLastY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    // Trace the physical beam
    CS::Collisions::HitBeamResult hitResult =
      collisionSector->HitBeamPortal (startBeam, endBeam);
    if (!hitResult.hasHit)
      return false;

    // Add a force at the point clicked
    if (hitResult.object->GetType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
    {
      csVector3 force = endBeam - startBeam;
      force.Normalize ();
      force *= 2.0f;

      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody ();
      if (physicalBody->GetType () == CS::Physics::BODY_RIGID)
      {
        csOrthoTransform trans = physicalBody->GetTransform ();
        // Check if the body hit is not static or kinematic
        csRef<CS::Physics::iRigidBody> bulletBody =
          scfQueryInterface<CS::Physics::iRigidBody> (physicalBody);
        if (bulletBody->GetState () != CS::Physics::STATE_DYNAMIC)
          return false;

        physicalBody->QueryRigidBody ()->AddForceAtPos (force, hitResult.isect);

        // This would work too
        //csOrthoTransform transform (hitResult.body->QueryRigidBody ()->GetTransform ());
        //csVector3 relativePosition = transform.Other2This (hitResult.isect);
        //hitResult.body->QueryRigidBody ()->AddForceAtRelPos (force, relativePosition);
      }
      else
      {
        force *= 200.f;
        physicalBody->QuerySoftBody ()->AddForce (force, hitResult.vertexIndex);
      }
    }
    else
      return false;
    return true;
  }

  // Right mouse button: dragging
  else if (csMouseEventHelper::GetButton (&event) == 1)
  {
    // Find the rigid body that was clicked on
    // Compute the end beam points
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), g2d->GetHeight () - mouse->GetLastY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    // Trace the physical beam
    CS::Collisions::HitBeamResult hitResult =
      collisionSector->HitBeam (startBeam, endBeam);
    if (!hitResult.hasHit || !hitResult.object)
      return false;

    // Check if we hit a rigid body
    if (hitResult.object->GetType () == CS::Collisions::COLLISION_OBJECT_PHYSICAL)
    {
      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody ();
      if (physicalBody->GetType () == CS::Physics::BODY_RIGID)
      {
        csRef<CS::Physics::iRigidBody> bulletBody =
          scfQueryInterface<CS::Physics::iRigidBody> (physicalBody);
        if (bulletBody->GetState () != CS::Physics::STATE_DYNAMIC)
          return false;

        // Create a p2p joint at the point clicked
        dragJoint = physicalSystem->CreateRigidPivotJoint (bulletBody, hitResult.isect);
        physicalSector->AddJoint (dragJoint);

        dragging = true;
        dragDistance = (hitResult.isect - startBeam).Norm ();

        // Set some dampening on the rigid body to have a more stable dragging
        linearDampening = bulletBody->GetLinearDampener ();
        angularDampening = bulletBody->GetRollingDampener ();
        bulletBody->SetLinearDampener (0.9f);
        bulletBody->SetRollingDampener (0.9f);
      }
      else
      {
        softDragging = true;
        draggedBody = physicalBody->QuerySoftBody ();
        draggedVertex = hitResult.vertexIndex;
        dragDistance = (hitResult.isect - startBeam).Norm ();
        grabAnimationControl.AttachNew (new MouseAnchorAnimationControl (this));
        physicalBody->QuerySoftBody ()->AnchorVertex (hitResult.vertexIndex, grabAnimationControl);
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
    bulletBody->SetLinearDampener (linearDampening);
    bulletBody->SetRollingDampener (angularDampening);

    // Remove the drag joint
    physicalSector->RemoveJoint (dragJoint);
    dragJoint = NULL;
    return true;
  }

  if (csMouseEventHelper::GetButton (&event) == 1
    && softDragging)
  {
    softDragging = false;
    draggedBody->RemoveAnchor (draggedVertex);
    draggedBody = 0;
  }

  return false;
}
