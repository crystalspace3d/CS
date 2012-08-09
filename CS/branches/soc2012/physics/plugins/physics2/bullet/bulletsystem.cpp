/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
//#include "ivaria/reporter.h"
#include "ivaria/softanim.h"
#include "imesh/animesh.h"
#include "iengine/scenenode.h"
#include "iengine/movable.h"
#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "imesh/objmodel.h"
#include "csutil/sysfunc.h"
#include "iutil/objreg.h"
#include "ivaria/view.h"
#include "ivaria/collisions.h"
#include "igeom/trimesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "csutil/custom_new_enable.h"

#include "common2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "dynamicactor.h"
#include "collisionactor.h"
#include "joint2.h"

#include "vehicle.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
SCF_IMPLEMENT_FACTORY (csBulletSystem)

csBulletSystem::csBulletSystem (iBase* iParent)
  : scfImplementationType (this, iParent), internalScale (1.0f), inverseInternalScale (1.0f)
{
  defaultInfo = new btSoftBodyWorldInfo;
  

  static const CollisionGroupMask allFilter = -1;

  CollisionGroup defaultGroup ("Default");
  defaultGroup.value = CollisionGroupMaskValueDefault;
  defaultGroup.mask = allFilter;
  collGroups.Push (defaultGroup);

  CS::Collisions::CollisionGroup staticGroup ("Static");
  staticGroup.value = CollisionGroupMaskValueStatic;
  staticGroup.mask = allFilter ^ CollisionGroupMaskValueStatic;
  collGroups.Push (staticGroup);

  CS::Collisions::CollisionGroup kinematicGroup ("Kinematic");
  kinematicGroup.value = CollisionGroupMaskValueKinematic;
  kinematicGroup.mask = allFilter ^ CollisionGroupMaskValueKinematic;
  collGroups.Push (kinematicGroup);

  CS::Collisions::CollisionGroup portalGroup ("Portal");
  portalGroup.value = CollisionGroupMaskValuePortal;
  portalGroup.mask = allFilter ^ CollisionGroupMaskValueStatic;     // all but static
  collGroups.Push (portalGroup);

  CS::Collisions::CollisionGroup copyGroup ("PortalCopy");
  copyGroup.value = CollisionGroupMaskValuePortalCopy;
  copyGroup.mask = allFilter ^ CollisionGroupMaskValuePortalCopy;
  collGroups.Push (copyGroup);

  CS::Collisions::CollisionGroup characterGroup ("Actor");
  characterGroup.value = CollisionGroupMaskValueActor;
  characterGroup.mask = allFilter;
  collGroups.Push (characterGroup);

  CS::Collisions::CollisionGroup noneGroup ("None");
  noneGroup.value = CollisionGroupMaskValueNone;    
  noneGroup.mask = CollisionGroupMaskValuePortal; // only intersect with portals
  collGroups.Push (noneGroup);
  
  SetGroupCollision ("PortalCopy", "Static", false);
  SetGroupCollision ("Portal", "PortalCopy", false);

  systemFilterCount = 6;
}

csBulletSystem::~csBulletSystem ()
{
  collSectors.DeleteAll ();
  //objects.DeleteAll ();
  //colliders.DeleteAll ();
}

void csBulletSystem::SetInternalScale (float scale)
{
  // update parameters
  // TODO: Update all objects
  internalScale = scale;
  inverseInternalScale = 1.0f / scale;
}

bool csBulletSystem::Initialize (iObjectRegistry* object_reg)
{
  this->object_reg = object_reg;
  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet> (
    object_reg, "crystalspace.shared.stringset");
  baseID = strings->Request ("base");
  colldetID = strings->Request ("colldet");
  return true;
}

csPtr<CS::Collisions::iColliderCompound> csBulletSystem::CreateColliderCompound ( )
{
   csRef<iColliderCompound> collider = csPtr<iColliderCompound>(new csBulletColliderCompound(this));

  //colliders.Push (collider);
  return csPtr<iColliderCompound>(collider);
}

csPtr<CS::Collisions::iColliderConvexMesh> csBulletSystem::CreateColliderConvexMesh (iMeshWrapper* mesh, bool simplify)
{
  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh);
  if (!triMesh)
    return csPtr<CS::Collisions::iColliderConvexMesh> (nullptr);

  return CreateColliderConvexMesh(triMesh, mesh, simplify);
}

csPtr<CS::Collisions::iColliderConvexMesh> csBulletSystem::CreateColliderConvexMesh (
  iTriangleMesh* triMesh, iMeshWrapper* mesh, bool simplify)
{
  btTriangleMesh* btTriMesh = CreateBulletTriMesh (triMesh);
  
  csRef<csBulletColliderConvexMesh> collider = csPtr<csBulletColliderConvexMesh>(
    new csBulletColliderConvexMesh (mesh, triMesh, btTriMesh, this, simplify));

  //colliders.Push (collider);
  return csPtr<iColliderConvexMesh>(collider);
}

csPtr<CS::Collisions::iColliderConcaveMesh> csBulletSystem::CreateColliderConcaveMesh (iMeshWrapper* mesh)
{
  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh);
  if (!triMesh) return csPtr<CS::Collisions::iColliderConcaveMesh> (nullptr);

  btTriangleMesh* btTriMesh = CreateBulletTriMesh (triMesh);

  csRef<csBulletColliderConcaveMesh> collider = csPtr<csBulletColliderConcaveMesh>(new csBulletColliderConcaveMesh (mesh, btTriMesh,this));

  //colliders.Push (collider);
  return csPtr<iColliderConcaveMesh>(collider);
}

csPtr<CS::Collisions::iColliderConcaveMeshScaled> csBulletSystem::CreateColliderConcaveMeshScaled (
  CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale)
{
  csRef<csBulletColliderConcaveMeshScaled> coll = csPtr<csBulletColliderConcaveMeshScaled>(new csBulletColliderConcaveMeshScaled (collider, scale,this));

  //colliders.Push (coll);
  return csPtr<iColliderConcaveMeshScaled>(coll);
}

csPtr<CS::Collisions::iColliderCylinder> csBulletSystem::CreateColliderCylinder (float length, float radius)
{
  csRef<csBulletColliderCylinder> collider = csPtr<csBulletColliderCylinder>(new csBulletColliderCylinder (length, radius, this));

  //colliders.Push (collider);
  return csPtr<iColliderCylinder>(collider);
}

csPtr<CS::Collisions::iColliderBox> csBulletSystem::CreateColliderBox (const csVector3& size)
{
  //csRef<csBulletColliderBox> collider = csPtr<csBulletColliderBox>(new csBulletColliderBox (size, this));
  csRef<CS::Collisions::iColliderBox> collider = csPtr<CS::Collisions::iColliderBox>(new csBulletColliderBox (size, this));

  //colliders.Push (collider);
  return csPtr<CS::Collisions::iColliderBox>(collider);
} 

csPtr<CS::Collisions::iColliderSphere> csBulletSystem::CreateColliderSphere (float radius)
{
  csRef<csBulletColliderSphere> collider = csPtr<csBulletColliderSphere>(new csBulletColliderSphere (radius, this));
  return csPtr<CS::Collisions::iColliderSphere>(collider);
}

csPtr<CS::Collisions::iColliderCapsule> csBulletSystem::CreateColliderCapsule (float length, float radius)
{
  csRef<csBulletColliderCapsule> collider = csPtr<csBulletColliderCapsule>(new csBulletColliderCapsule (length, radius, this));
  
  //colliders.Push (collider);
  return csPtr<CS::Collisions::iColliderCapsule>(collider);
}

csPtr<CS::Collisions::iColliderCone> csBulletSystem::CreateColliderCone (float length, float radius)
{
  csRef<csBulletColliderCone> collider = csPtr<csBulletColliderCone>(new csBulletColliderCone (length, radius, this));

  //colliders.Push (collider);
  return csPtr<iColliderCone>(collider);
}

csPtr<CS::Collisions::iColliderPlane> csBulletSystem::CreateColliderPlane (const csPlane3& plane)
{
  csRef<csBulletColliderPlane> collider = csPtr<csBulletColliderPlane>(new csBulletColliderPlane (plane, this));

  //colliders.Push (collider);
  return csPtr<iColliderPlane>(collider);
}

csPtr<CS::Collisions::iCollisionTerrain> csBulletSystem::CreateCollisionTerrain (iTerrainSystem* terrain, 
                                                               float minHeight /* = 0 */, 
                                                               float maxHeight /* = 0 */)
{
  csRef<csBulletCollisionTerrain> collider = csPtr<csBulletCollisionTerrain>(new csBulletCollisionTerrain (terrain, minHeight, maxHeight, this));

  //colliders.Push (collider);
  return csPtr<iCollisionTerrain>(collider);
}

csPtr<iCollisionObjectFactory> csBulletSystem::CreateCollisionObjectFactory (int id)
{
  // Compiler can optimize this into a table-lookup
  iCollisionObjectFactory* factory;
  switch (id)
  {
  case COLLISION_OBJECT_ACTOR:
    {
    factory = new BulletCollisionActorFactory();
    }
  case COLLISION_OBJECT_GHOST:
    {
      factory = new BulletGhostCollisionObjectFactory();
    }
  case PHYSICAL_OBJECT_DYNAMICACTOR:
    {
      factory = new BulletDynamicActorFactory();
    }
  case PHYSICAL_OBJECT_RIGIDBODY:
    {
      factory = new BulletRigidBodyFactory();
    }

    // TODO: SoftBody factories
  //case InternalCollisionObjectTypeSoftRope:
  //  {
  //  }
  //case InternalCollisionObjectTypeSoftCloth:
  //  {
  //  }
  //case InternalCollisionObjectTypeSoftMesh:
  //  {
  //  }
  default:
    // Just create a rigid body from it, for now
    factory = new BulletRigidBodyFactory();
  }
  return csPtr<iCollisionObjectFactory> (factory);
}

CS::Collisions::iCollisionSector* csBulletSystem::CreateCollisionSector ()
{
  csRef<csBulletSector> collSector = csPtr<csBulletSector>(new csBulletSector (this));

  collSectors.Push (collSector);
  return collSector;
}

CS::Collisions::iCollisionSector* csBulletSystem::GetOrCreateCollisionSector (iSector* sector)
{
  iCollisionSector* collSector = GetCollisionSector(sector);
  if (!collSector)
  {
    collSector = CreateCollisionSector();
    collSector->SetSector(sector);
  }
  return collSector;
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const csString& name)
{
  return this->collSectors.FindByName (name.GetData());
}

CS::Collisions::iCollisionSector* csBulletSystem::GetCollisionSector (const iSector* sec)
{
  for (size_t i = 0; i < collSectors.GetSize (); i++)
  {
    if (collSectors[i]->GetSector () == sec)
    {
      return collSectors[i];
    }
  }
  return nullptr;
}

// ###############################################################################################################
// Physical Objects


// Joints

csPtr<CS::Physics::iJoint> csBulletSystem::CreateJoint ()
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidP2PJoint (const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  csVector3 trans (0.0f,0.0f,0.0f);
  joint->SetMaximumDistance (trans);
  joint->SetMinimumDistance (trans);
  joint->SetPosition (position);
  joint->SetType (RIGID_P2P_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidSlideJoint (const csOrthoTransform trans,
                                                    float minDist, float maxDist, 
                                                    float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJoint> (nullptr);
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  joint->SetRotConstraints (true, true, true);
  csVector3 minDistant (0.0f, 0.0f, 0.0f);
  csVector3 maxDistant (0.0f, 0.0f, 0.0f);

  minDistant[axis] = minDist;
  maxDistant[axis] = maxDist;
  joint->SetMinimumDistance (minDistant);
  joint->SetMaximumDistance (maxDistant);
  minDistant[axis] = minAngle;
  maxDistant[axis] = maxAngle;
  joint->SetMinimumAngle (minDistant);
  joint->SetMaximumAngle (maxDistant);
  joint->SetTransform (trans);
  joint->SetType (RIGID_SLIDE_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidHingeJoint (const csVector3 position, 
                                                     float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJoint> (nullptr);
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  csVector3 minDistant (0.0f, 0.0f, 0.0f);
  csVector3 maxDistant (0.0f, 0.0f, 0.0f);
  minDistant[axis] = minAngle;
  maxDistant[axis] = maxAngle;
  joint->SetMinimumAngle (minDistant);
  joint->SetMaximumAngle (maxDistant);
  joint->SetPosition (position);  
  joint->SetType (RIGID_HINGE_JOINT);
  joint->axis = axis;
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidConeTwistJoint (const csOrthoTransform trans, 
                                                                       float swingSpan1,float swingSpan2,
                                                                       float twistSpan) 
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  joint->SetRotConstraints (true, true, true);

  csVector3 minDistant (0.0f, 0.0f, 0.0f);
  csVector3 maxDistant (0.0f, 0.0f, 0.0f);
  joint->SetMaximumDistance (minDistant);
  joint->SetMinimumDistance (maxDistant);
  minDistant.Set (-twistSpan, -swingSpan2, -swingSpan1);  
  maxDistant.Set (twistSpan, swingSpan2, swingSpan1); 
  joint->SetMinimumAngle (minDistant);
  joint->SetMaximumAngle (maxDistant);
  joint->SetTransform (trans);
  joint->SetType (RIGID_CONETWIST_JOINT);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateSoftLinearJoint (const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  joint->SetPosition (position);
  joint->SetType (SOFT_LINEAR_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateSoftAngularJoint (int axis)
{
  csBulletJoint* joint = nullptr;
  if (axis < 0 || axis > 2)
    return joint;
  joint = new csBulletJoint (this);
  if (axis == 0)
    joint->SetRotConstraints (false, true, true);
  else if (axis == 1)
    joint->SetRotConstraints (true, false, false);
  else if (axis == 2)
    joint->SetRotConstraints (false, false, true);

  joint->SetType (SOFT_ANGULAR_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidPivotJoint (iRigidBody* body, const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint>(new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  csVector3 trans (0.0f,0.0f,0.0f);
  joint->SetMaximumDistance (trans);
  joint->SetMinimumDistance (trans);
  joint->SetPosition (position);
  joint->SetType (RIGID_PIVOT_JOINT);
  joint->Attach (body, nullptr);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint>(joint);
}


// Vehicles

/// Creates a new factory to produce vehicles
csPtr<iVehicleFactory> csBulletSystem::CreateVehicleFactory ()
{
  return csPtr<iVehicleFactory>(new BulletVehicleFactory(this));
}

/// Creates a new factory to produce vehicle wheels
csPtr<iVehicleWheelFactory> csBulletSystem::CreateVehicleWheelFactory ()
{
  return csPtr<iVehicleWheelFactory>(new BulletVehicleWheelFactory(this));
}

csPtr<iVehicleWheelInfo> csBulletSystem::CreateVehicleWheelInfo (iVehicleWheelFactory* factory)
{
  return csPtr<iVehicleWheelInfo>(new BulletVehicleWheelInfo(factory));
}

iVehicle* csBulletSystem::GetVehicle (iCollisionObject* obj)
{
  return vehicleMap.Get(obj, nullptr);
}


// Misc

void csBulletSystem::ReportWarning (const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csReportV (object_reg, CS_REPORTER_SEVERITY_WARNING,
	     "crystalspace.dynamics.bullet2",
	     msg, arg);
  va_end (arg);
}

CS::Collisions::CollisionGroup& csBulletSystem::CreateCollisionGroup (const char* name)
{
  size_t groupCount = collGroups.GetSize ();
  if (groupCount >= sizeof (CS::Collisions::CollisionGroupMask) * 8)
    return collGroups[CollisionGroupTypeDefault];

  CS::Collisions::CollisionGroup newGroup(name);
  newGroup.value = 1 << groupCount;
  newGroup.mask = ~newGroup.value;
  collGroups.Push (newGroup);
  return collGroups[groupCount];
}

CS::Collisions::CollisionGroup& csBulletSystem::FindCollisionGroup (const char* name)
{
  size_t index = collGroups.FindKey (CollisionGroupVector::KeyCmp (name));
  if (index == csArrayItemNotFound)
    return collGroups[CollisionGroupTypeDefault];
  else
    return collGroups[index];
}

void csBulletSystem::SetGroupCollision (const char* name1,
                                        const char* name2,
                                        bool collide)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return;
  if (!collide)
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].mask &= ~(1 << index2);
    if (index2 >= systemFilterCount)
      collGroups[index2].mask &= ~(1 << index1);
  }
  else
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].mask |= 1 << index2;
    if (index2 >= systemFilterCount)
      collGroups[index2].mask |= 1 << index1;
  }
}

bool csBulletSystem::GetGroupCollision (const char* name1,
                                        const char* name2)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return false;
  if ((collGroups[index1].mask & (1 << index2)) != 0 
    || (collGroups[index2].mask & (1 << index1)) != 0)
    return true;
  else
    return false;
}

bool dummyWorldInit = false;
void csBulletSystem::SeparateDisconnectedSubMeshes(iColliderCompound* compoundCollider, iColliderCompoundResult* results)
{
  // Make sure we have a valid dispatcher
  static btDefaultCollisionConfiguration* configuration;
  static btCollisionDispatcher* dispatcher;
  static btAxisSweep3* broadphase;
  static btCollisionWorld* bulletWorld;
  if (!dummyWorldInit)
  {
    // init dummy world
    dummyWorldInit = true;
    configuration = new btDefaultCollisionConfiguration ();
    dispatcher = new btCollisionDispatcher (configuration);

    btVector3 worldAabbMin (-WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS);
    btVector3 worldAabbMax (WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS);

    const int maxProxies = 32766;   // TODO: Tune this down for the dummy world?
    broadphase = new btAxisSweep3 (worldAabbMin, worldAabbMax, maxProxies);

    bulletWorld = new btCollisionWorld (dispatcher, broadphase, configuration);
  }

  // Add all partial colliders as objects to the collision world
  CS_ALLOC_STACK_ARRAY(btCollisionObject, objs, compoundCollider->GetColliderCount());
  for (size_t i = 0; i < compoundCollider->GetColliderCount(); ++i)
  {
    btCollisionObject& obj = objs[i];

    CS::Collisions::iCollider* ipartialCollider;
    csOrthoTransform transform;
    compoundCollider->GetCollider(i, ipartialCollider, transform);

    csBulletCollider* partialCollider = dynamic_cast<csBulletCollider*>(ipartialCollider);
    obj.setCollisionShape(partialCollider->GetOrCreateBulletShape());
    obj.setWorldTransform(CSToBullet(transform, getInternalScale()));
    bulletWorld->addCollisionObject(&obj);
  }
  
  // Test all partial colliders against each other
  bulletWorld->performDiscreteCollisionDetection();
  
  // TODO: Build islands
  //btPersistentManifold** manifold = dispatcher->getInternalManifoldPointer();
  //int maxNumManifolds = dispatcher->getNumManifolds();
  //for (i=0;i<maxNumManifolds ;i++)
  //{
  //  btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);

  //  btCollisionObject* colObj0 = static_cast<btCollisionObject*>(manifold->getBody0());
  //  btCollisionObject* colObj1 = static_cast<btCollisionObject*>(manifold->getBody1());

  //  ///@todo: check sleeping conditions!
  //  if (((colObj0) && colObj0->getActivationState() != ISLAND_SLEEPING) ||
  //    ((colObj1) && colObj1->getActivationState() != ISLAND_SLEEPING))
  //  {

  //    //kinematic objects don't merge islands, but wake up all connected objects
  //    if (colObj0->isKinematicObject() && colObj0->getActivationState() != ISLAND_SLEEPING)
  //    {
  //      if (colObj0->hasContactResponse())
  //        colObj1->activate();
  //    }
  //    if (colObj1->isKinematicObject() && colObj1->getActivationState() != ISLAND_SLEEPING)
  //    {
  //      if (colObj1->hasContactResponse())
  //        colObj0->activate();
  //    }
  //    if(m_splitIslands)
  //    { 
  //      //filtering for response
  //      if (dispatcher->needsResponse(colObj0,colObj1))
  //        m_islandmanifold.push_back(manifold);
  //    }
  //  }
  //}
}

iTriangleMesh* csBulletSystem::FindColdetTriangleMesh (iMeshWrapper* mesh)
{
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();
  csRef<iTriangleMesh> triMesh;
  if  (objModel->GetTriangleData (colldetID))
    triMesh = objModel->GetTriangleData (colldetID);
  else
    triMesh = objModel->GetTriangleData (baseID);

  if (!triMesh || triMesh->GetVertexCount () == 0
      || triMesh->GetTriangleCount () == 0)
  {
    /*csFPrintf (stderr, "iCollider: No collision polygons, triangles or vertices on mesh factory %s\n",
      CS::Quote::Single (mesh->QueryObject ()->GetName ()));*/

    return csRef<iTriangleMesh> (nullptr);
  }
  return triMesh;
}


// Factory


csPtr<CS::Collisions::iGhostCollisionObjectFactory> 
  csBulletSystem::CreateGhostCollisionObjectFactory (CS::Collisions::iCollider* collider, const csString& name) 
{ 
  BulletGhostCollisionObjectFactory* fact = new BulletGhostCollisionObjectFactory(collider, name);
  fact->system = this;

  return csPtr<iGhostCollisionObjectFactory>(fact); 
}

csPtr<CS::Collisions::iCollisionActorFactory> 
  csBulletSystem::CreateCollisionActorFactory (CS::Collisions::iCollider* collider, const csString& name) 
{
  BulletCollisionActorFactory* fact = new BulletCollisionActorFactory(collider, name);
  fact->system = this;

  return csPtr<iCollisionActorFactory>(fact); 
}

csPtr<CS::Physics::iRigidBodyFactory> 
  csBulletSystem::CreateRigidBodyFactory (CS::Collisions::iCollider* collider, const csString& name)
{
  BulletRigidBodyFactory* fact = new BulletRigidBodyFactory(collider, name);
  fact->system = this;

  return csPtr<iRigidBodyFactory>(fact); 
}

csPtr<CS::Physics::iDynamicActorFactory> 
  csBulletSystem::CreateDynamicActorFactory (CS::Collisions::iCollider* collider, const csString& name)
{
  BulletDynamicActorFactory* fact = new BulletDynamicActorFactory(collider, name);
  fact->system = this;

  return csPtr<iDynamicActorFactory>(fact);
}

csPtr<CS::Physics::iSoftRopeFactory> csBulletSystem::CreateSoftRopeFactory ()
{
  BulletSoftRopeFactory* fact = new BulletSoftRopeFactory();
  fact->system = this;

  return csPtr<iSoftRopeFactory>(fact);
}

csPtr<CS::Physics::iSoftClothFactory> csBulletSystem::CreateSoftClothFactory ()
{
  BulletSoftClothFactory* fact = new BulletSoftClothFactory();
  fact->system = this;

  return csPtr<iSoftClothFactory>(fact);
}

csPtr<CS::Physics::iSoftMeshFactory> csBulletSystem::CreateSoftMeshFactory ()
{
  BulletSoftMeshFactory* fact = new BulletSoftMeshFactory();
  fact->system = this;

  return csPtr<iSoftMeshFactory>(fact);
}

void csBulletSystem::DeleteAll() 
{
  collSectors.DeleteAll ();
}

btTriangleMesh* csBulletSystem::CreateBulletTriMesh (iTriangleMesh* triMesh)
{
  btTriangleMesh* btMesh = new btTriangleMesh ();
  
  size_t triangleCount = triMesh->GetTriangleCount ();
  //size_t vertexCount = triMesh->GetVertexCount ();

  size_t i;
  csTriangle *c_triangle = triMesh->GetTriangles ();
  csVector3 *c_vertex = triMesh->GetVertices ();
  for (i =0;i<triangleCount;i++)
  {
    int index0 = c_triangle[i].a;
    int index1 = c_triangle[i].b;
    int index2 = c_triangle[i].c;

    btVector3 vertex0 (c_vertex[index0].x, c_vertex[index0].y, c_vertex[index0].z);
    btVector3 vertex1 (c_vertex[index1].x, c_vertex[index1].y, c_vertex[index1].z);
    btVector3 vertex2 (c_vertex[index2].x, c_vertex[index2].y, c_vertex[index2].z);

    vertex0 *= getInternalScale ();
    vertex1 *= getInternalScale ();
    vertex2 *= getInternalScale ();

    btMesh->addTriangle (vertex0,vertex1,vertex2);
  }
  return btMesh;
}

}
CS_PLUGIN_NAMESPACE_END(Bullet2)
