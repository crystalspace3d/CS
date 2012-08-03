/**
 * Manipulate and interact with objects
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"


#include "iengine/campos.h"

#include "physdemo.h"

#include "ivaria/colliders.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

static const csScalar DefaultDensity(100);
static const csScalar DefaultFriction(10);
static const csScalar DefaultElasticity(0.1f);


bool PhysDemo::PickCursorObject(CS::Collisions::HitBeamResult& result)
{ 
  // Find the rigid body that was clicked on
  // Compute the end beam points
  csRef<iCamera> camera = view->GetCamera();
  csVector2 v2d (mouse->GetLastX(), g2d->GetHeight() - mouse->GetLastY());
  csVector3 v3d = camera->InvPerspective (v2d, 10000);
  csVector3 startBeam = camera->GetTransform().GetOrigin();
  csVector3 endBeam = camera->GetTransform().This2Other (v3d);

  // Trace the physical beam
  result = GetCurrentSector()->HitBeamPortal (startBeam, endBeam);

  // debug print name
  if (result.hasHit)
  {
    csPrintf("Picked object: %s\n", result.object->QueryObject()->GetName());
  }

  return result.hasHit;
}

::iBase* PhysDemo::GetOwner(CS::Collisions::iCollisionObject* obj)
{
  // TODO: Need a generic mechanism to determine ownership of objects

  // check for actor
  if (obj->QueryActor())
  {
    return obj->QueryActor();
  }

  // check for vehicle
  iVehicle* vehicle = physicalSystem->GetVehicle(obj);
  if (vehicle)
  {
    return vehicle;
  }

  // no owner
  return nullptr;
}


bool PhysDemo::TestOnGround(CS::Collisions::iCollisionObject* obj)
{
  static const float groundAngleCosThresh = .7f;

  // Find any objects that can at least remotely support the object
  csArray<CollisionData> collisions;
  GetCurrentSector()->CollisionTest(obj, collisions);

  for (size_t i = 0; i < collisions.GetSize (); ++i)
  {
    CollisionData& coll = collisions[i];

    int dir = coll.objectA == obj ? 1 : -1;

    float groundAngleCos = coll.normalWorldOnB * UpVector;
    if (dir * groundAngleCos > groundAngleCosThresh)
    {
      return true;
    }
  }
  return false;
}

void PhysDemo::PullObject(CS::Collisions::iCollisionObject* obj)
{
  csVector3 from;
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject(result) || !IsDynamic(result.object)) return;    // didn't hit anything dynamic
    obj = result.object;
    from = result.isect;
  }
  else
  {
    from = obj->GetTransform().GetOrigin();
  }
   
  iPhysicalBody* pb = obj->QueryPhysicalBody();

  csVector3 posCorrection(2  * UpVector);

  csVector3 force(GetActorPos() - from - posCorrection);
  force.Normalize();
  force *= 30 * pb->GetMass();

  // prevent sliding problem
  csOrthoTransform trans = pb->GetTransform();
  trans.SetOrigin(trans.GetOrigin() + posCorrection);
  pb->SetTransform(trans);

  pb->QueryRigidBody()->AddForce (force);
}

void PhysDemo::DeleteObject(CS::Collisions::iCollisionObject* obj)
{
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject(result)) return;    // didn't hit anything
    obj = result.object;
  }
   
  if (!GetOwner(obj))
  {
    // can only remove it, if it has no owner
    obj->GetSector()->RemoveCollisionObject(obj);
  }
  else
  {
    // TODO: Handle removal of complex game entities generically
    ReportWarning("Cannot trivially remove the given object because it is part of a complex object.\n");
  }
}

void PhysDemo::ToggleObjectDynamic(CS::Collisions::iCollisionObject* obj)
{
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject(result)) return;    // didn't hit anything
    obj = result.object;
  }

  if (!obj->QueryPhysicalBody())
  {
    // must be physical in order to be toggled
    return;
  }

  iPhysicalBody* physObj = obj->QueryPhysicalBody();
  bool isDynamic = physObj->GetDensity();
  if (isDynamic)
  {
    // Set mass to 0 (makes it static)
    physObj->SetDensity(0);
  }
  else
  {
    // Give it mass (makes it dynamic)
    if (physObj->GetCollider()->GetColliderType() == COLLIDER_CONCAVE_MESH && obj->GetAttachedMovable())
    {
      // First decompose it
      csPrintf("Performing convex decomposition on object: \"%s\"...\n", obj->QueryObject()->GetName());

      csRef<iColliderCompound> collider = physicalSystem->CreateColliderCompound();
      physicalSystem->DecomposeConcaveMesh(&*collider, obj->GetAttachedMovable()->GetSceneNode ()->QueryMesh ());
      obj->SetCollider(collider);

      csPrintf("Done - Performed convex decomposition on object: \"%s\".\n", obj->QueryObject()->GetName());
    }
    
    physObj->SetDensity(DefaultDensity);
  }

  if (physObj->GetDensity() == isDynamic)
  {
    ReportWarning("Cannot make object \"%s\" %s.\n", physObj->QueryObject()->GetName(), isDynamic ? "STATIC" : "DYNAMIC");
  }
}


void PhysDemo::TeleportObject(CS::Collisions::iCollisionObject* obj, iCameraPosition* pos)
{
  // set transform
  csOrthoTransform trans(csMatrix3(), pos->GetPosition());
  trans.LookAt(pos->GetForwardVector(), pos->GetUpwardVector());
  obj->SetTransform(trans);
  
  // set sector
  iSector* isector = engine->FindSector(pos->GetSector());
  iCollisionSector* collSector = physicalSystem->GetOrCreateCollisionSector(isector);
  CS_ASSERT(collSector);
  collSector->AddCollisionObject(obj);
}