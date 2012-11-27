/*
    Copyright (C) 2011-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html
    Copyright (C) 2012 by Dominik Seifert
    Copyright (C) 2011 by Liu Lu

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
//#include "ivaria/reporter.h"
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

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

SCF_IMPLEMENT_FACTORY (csBulletSystem)

//--------------------------------- CollisionGroup ---------------------------------

CollisionGroup::CollisionGroup (const char* name, char index)
  : scfImplementationType (this), name (name), index (index)
{
  value = 1 << index;
  //mask = ~value;
  mask = -1;
}

void CollisionGroup::SetCollisionEnabled (iCollisionGroup* other, bool enabled)
{
  CollisionGroup* group = dynamic_cast<CollisionGroup*> (other);
  if (enabled)
  {
    mask |= 1 << group->index;
    group->mask |= 1 << index;
  }
  else
  {
    mask &= ~(1 << group->index);
    group->mask &= ~(1 << index);
  }
}

bool CollisionGroup::GetCollisionEnabled (iCollisionGroup* other)
{
  CollisionGroup* group = dynamic_cast<CollisionGroup*> (other);
  return mask & (1 << group->index);
}

//--------------------------------- csBulletSystem ---------------------------------

// TODO: listen to the 'Frame' event in order to step automatically the simulation

csBulletSystem::csBulletSystem (iBase* iParent)
  : scfImplementationType (this, iParent), internalScale (1.0f), inverseInternalScale (1.0f),
  simulationSpeed (1.0f), worldTimeStep (1.0f / 60.0f), worldMaxSteps (1),
  stepIterations (10), isSoftWorld (true), linearDampening (0.1f), angularDampening (0.1f),
  linearDisableThreshold (0.8f), angularDisableThreshold (1.0f),
  timeDisableThreshold (0.0f), debugDraw (nullptr)
{
  defaultInfo = new btSoftBodyWorldInfo;

  // Create the default collision group
  csRef<CollisionGroup> group;
  group.AttachNew (new CollisionGroup ("Default", 0));
  defaultGroup = collisionGroups.Put ("Default", group);
}

csBulletSystem::~csBulletSystem ()
{
  collSectors.DeleteAll ();
  collisionGroups.DeleteAll ();
  // TODO: delete vehicle map
  delete debugDraw;
}

bool csBulletSystem::Initialize (iObjectRegistry* object_reg)
{
  this->object_reg = object_reg;
  // TODO: register to the registry if not yet made? to be done in the tool class?
  return true;
}

void csBulletSystem::SetSimulationSpeed (float speed)
{
  simulationSpeed = speed;
}

void csBulletSystem::SetStepParameters (float timeStep,
					size_t maxSteps,
					size_t iterations)
{
  worldTimeStep = timeStep;
  worldMaxSteps = maxSteps;
  stepIterations = (int) iterations;

  for (size_t i = 0; i < collSectors.GetSize (); i++)
  {
    btContactSolverInfo& info = collSectors[i]->bulletWorld->getSolverInfo ();
    info.m_numIterations = (int) iterations;
  }
}

void csBulletSystem::Step (csTicks duration)
{
  float fduration = duration * simulationSpeed * 0.001f;
  for (size_t i = 0; i < collSectors.GetSize (); i++)
    collSectors[i]->Step (fduration);
}

void csBulletSystem::SetSoftBodyEnabled (bool enabled)
{
  CS_ASSERT (!collSectors.GetSize ());
  isSoftWorld = enabled;
}

void csBulletSystem::SetInternalScale (float scale)
{
  // update parameters
  internalScale = scale;
  inverseInternalScale = 1.0f / scale;
}

void csBulletSystem::SetLinearDamping (float damping)
{
  linearDampening = damping;
}

void csBulletSystem::SetAngularDamping (float damping)
{
  angularDampening = damping;
}

void csBulletSystem::SetAutoDisableParams (float linear, float angular,
					   float time)
{
  linearDisableThreshold = linear;
  angularDisableThreshold = angular;
  timeDisableThreshold = time;
}

csPtr<CS::Collisions::iCollider> csBulletSystem::CreateCollider ()
{
  return csPtr<CS::Collisions::iCollider> (new csBulletCollider (this));
}

csPtr<CS::Collisions::iColliderConvexMesh> csBulletSystem::CreateColliderConvexMesh (
  iTriangleMesh* triMesh, bool simplify)
{
  btTriangleMesh* btTriMesh = CreateBulletTriMesh (triMesh);
  csRef<csBulletColliderConvexMesh> collider;
  collider.AttachNew (new csBulletColliderConvexMesh (triMesh, btTriMesh, this, simplify));
  return csPtr<iColliderConvexMesh> (collider);
}

csPtr<CS::Collisions::iColliderConcaveMesh> csBulletSystem::CreateColliderConcaveMesh (iTriangleMesh* mesh)
{
  btTriangleMesh* btTriMesh = CreateBulletTriMesh (mesh);
  csRef<csBulletColliderConcaveMesh> collider = csPtr<csBulletColliderConcaveMesh>
    (new csBulletColliderConcaveMesh (mesh, btTriMesh, this));
  return csPtr<iColliderConcaveMesh> (collider);
}

csPtr<CS::Collisions::iColliderConcaveMeshScaled> csBulletSystem::CreateColliderConcaveMeshScaled (
  CS::Collisions::iColliderConcaveMesh* collider, const csVector3& scale)
{
  csRef<csBulletColliderConcaveMeshScaled> coll = csPtr<csBulletColliderConcaveMeshScaled>
    (new csBulletColliderConcaveMeshScaled (collider, scale, this));
  return csPtr<iColliderConcaveMeshScaled> (coll);
}

csPtr<CS::Collisions::iColliderCylinder> csBulletSystem::CreateColliderCylinder (float length, float radius)
{
  csRef<csBulletColliderCylinder> collider = csPtr<csBulletColliderCylinder>
    (new csBulletColliderCylinder (length, radius, this));
  return csPtr<iColliderCylinder> (collider);
}

csPtr<CS::Collisions::iColliderBox> csBulletSystem::CreateColliderBox (const csVector3& size)
{
  csRef<CS::Collisions::iColliderBox> collider = csPtr<CS::Collisions::iColliderBox>
    (new csBulletColliderBox (size, this));
  return csPtr<CS::Collisions::iColliderBox> (collider);
} 

csPtr<CS::Collisions::iColliderSphere> csBulletSystem::CreateColliderSphere (float radius)
{
  csRef<csBulletColliderSphere> collider = csPtr<csBulletColliderSphere>
    (new csBulletColliderSphere (radius, this));
  return csPtr<CS::Collisions::iColliderSphere> (collider);
}

csPtr<CS::Collisions::iColliderCapsule> csBulletSystem::CreateColliderCapsule (float length, float radius)
{
  csRef<csBulletColliderCapsule> collider = csPtr<csBulletColliderCapsule>
    (new csBulletColliderCapsule (length, radius, this));
  return csPtr<CS::Collisions::iColliderCapsule> (collider);
}

csPtr<CS::Collisions::iColliderCone> csBulletSystem::CreateColliderCone (float length, float radius)
{
  csRef<csBulletColliderCone> collider = csPtr<csBulletColliderCone>
    (new csBulletColliderCone (length, radius, this));
  return csPtr<iColliderCone> (collider);
}

csPtr<CS::Collisions::iColliderPlane> csBulletSystem::CreateColliderPlane (const csPlane3& plane)
{
  csRef<csBulletColliderPlane> collider = csPtr<csBulletColliderPlane>
    (new csBulletColliderPlane (plane, this));
  return csPtr<iColliderPlane> (collider);
}

csPtr<CS::Collisions::iCollisionTerrain> csBulletSystem::CreateCollisionTerrain (iTerrainSystem* terrain, 
                                                               float minHeight /* = 0 */, 
                                                               float maxHeight /* = 0 */)
{
  csRef<csBulletCollisionTerrain> collider = csPtr<csBulletCollisionTerrain>
    (new csBulletCollisionTerrain (terrain, minHeight, maxHeight, this));
  return csPtr<iCollisionTerrain> (collider);
}

CS::Collisions::iCollisionSector* csBulletSystem::CreateCollisionSector (iSector* sector)
{
  csRef<csBulletSector> collSector = csPtr<csBulletSector> (new csBulletSector (this));
  if (sector) collSector->SetSector (sector);
  collSectors.Push (collSector);
  return collSector;
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const char* name)
{
  return this->collSectors.FindByName (name);
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const iSector* sec)
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
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidP2PJoint (const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  csVector3 trans (0.0f,0.0f,0.0f);
  joint->SetMaximumDistance (trans);
  joint->SetMinimumDistance (trans);
  joint->SetPosition (position);
  joint->SetType (RIGID_P2P_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidSlideJoint (const csOrthoTransform trans,
                                                    float minDist, float maxDist, 
                                                    float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJoint> (nullptr);
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
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
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidHingeJoint (const csVector3 position, 
                                                     float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJoint> (nullptr);
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
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
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidConeTwistJoint (const csOrthoTransform trans, 
                                                                       float swingSpan1,float swingSpan2,
                                                                       float twistSpan) 
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
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
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateSoftLinearJoint (const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
  joint->SetPosition (position);
  joint->SetType (SOFT_LINEAR_JOINT);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint> (joint);
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
  return csPtr<CS::Physics::iJoint> (joint);
}

csPtr<CS::Physics::iJoint> csBulletSystem::CreateRigidPivotJoint (iRigidBody* body, const csVector3 position)
{
  csRef<csBulletJoint> joint = csPtr<csBulletJoint> (new csBulletJoint (this));
  joint->SetTransConstraints (true, true, true);
  csVector3 trans (0.0f,0.0f,0.0f);
  joint->SetMaximumDistance (trans);
  joint->SetMinimumDistance (trans);
  joint->SetPosition (position);
  joint->SetType (RIGID_PIVOT_JOINT);
  joint->Attach (body, nullptr);
  //joints.Push (joint);
  return csPtr<CS::Physics::iJoint> (joint);
}

// Vehicles

/// Creates a new factory to produce vehicles
csPtr<iVehicleFactory> csBulletSystem::CreateVehicleFactory ()
{
  return csPtr<iVehicleFactory> (new BulletVehicleFactory (this));
}

/// Creates a new factory to produce vehicle wheels
csPtr<iVehicleWheelFactory> csBulletSystem::CreateVehicleWheelFactory ()
{
  return csPtr<iVehicleWheelFactory> (new BulletVehicleWheelFactory (this));
}

csPtr<iVehicleWheelInfo> csBulletSystem::CreateVehicleWheelInfo (iVehicleWheelFactory* factory)
{
  return csPtr<iVehicleWheelInfo> (new BulletVehicleWheelInfo (factory));
}

iVehicle* csBulletSystem::GetVehicle (iCollisionObject* obj)
{
  return vehicleMap.Get (obj, nullptr);
}

// Groups

CS::Collisions::iCollisionGroup* csBulletSystem::CreateCollisionGroup (const char* name)
{
  csRef<CollisionGroup>* group = collisionGroups.GetElementPointer (name);
  if (group) return *group;

  size_t groupCount = collisionGroups.GetSize ();
  // It is safer to use 16 as the global lower bound limit
  //if (groupCount >= sizeof (CS::Collisions::CollisionGroupMask) * 8)
  if (groupCount >= 16)
    return nullptr;

  csRef<CollisionGroup> newGroup;
  newGroup.AttachNew (new CollisionGroup (name, groupCount));
  collisionGroups.Put (name, newGroup);
  return newGroup;
}

CS::Collisions::iCollisionGroup* csBulletSystem::FindCollisionGroup (const char* name) const
{
  const csRef<CollisionGroup>* group = collisionGroups.GetElementPointer (name);
  if (group) return *group;
  else return nullptr;
}

size_t csBulletSystem::GetCollisionGroupCount () const
{
  return collisionGroups.GetSize ();
}

iCollisionGroup* csBulletSystem::GetCollisionGroup (size_t index) const
{
  CS_ASSERT (index < collisionGroups.GetSize ());
  csHash< csRef<CollisionGroup>, const char*>::ConstGlobalIterator it =
    collisionGroups.GetIterator ();
  for (int i = 0; i < ((int) index) - 1; i++) it.Next ();
  return it.Next ();
}

// Factory

csPtr<iCollisionObjectFactory> csBulletSystem::CreateCollisionObjectFactory
  (CS::Collisions::iCollider *collider, const char* name)
{
  BulletRigidBodyFactory* factory = new BulletRigidBodyFactory (this, collider, name);
  factory->system = this;
  factory->SetState (STATE_STATIC);
  return csPtr<iCollisionObjectFactory> (factory);
}

csPtr<CS::Collisions::iGhostCollisionObjectFactory> 
  csBulletSystem::CreateGhostCollisionObjectFactory (CS::Collisions::iCollider* collider, const char* name) 
{ 
  BulletGhostCollisionObjectFactory* fact = new BulletGhostCollisionObjectFactory (this, collider, name);
  fact->system = this;
  return csPtr<iGhostCollisionObjectFactory> (fact); 
}

csPtr<CS::Collisions::iCollisionActorFactory> 
  csBulletSystem::CreateCollisionActorFactory (CS::Collisions::iCollider* collider, const char* name) 
{
  BulletCollisionActorFactory* fact = new BulletCollisionActorFactory (this, collider, name);
  fact->system = this;
  return csPtr<iCollisionActorFactory> (fact); 
}

csPtr<CS::Physics::iRigidBodyFactory> 
  csBulletSystem::CreateRigidBodyFactory (CS::Collisions::iCollider* collider, const char* name)
{
  BulletRigidBodyFactory* fact = new BulletRigidBodyFactory (this, collider, name);
  fact->system = this;
  return csPtr<iRigidBodyFactory> (fact); 
}

csPtr<CS::Physics::iDynamicActorFactory> 
  csBulletSystem::CreateDynamicActorFactory (CS::Collisions::iCollider* collider, const char* name)
{
  BulletDynamicActorFactory* fact = new BulletDynamicActorFactory (this, collider, name);
  fact->system = this;
  return csPtr<iDynamicActorFactory> (fact);
}

csPtr<CS::Physics::iSoftRopeFactory> csBulletSystem::CreateSoftRopeFactory ()
{
  BulletSoftRopeFactory* fact = new BulletSoftRopeFactory (this);
  fact->system = this;
  return csPtr<iSoftRopeFactory> (fact);
}

csPtr<CS::Physics::iSoftClothFactory> csBulletSystem::CreateSoftClothFactory ()
{
  BulletSoftClothFactory* fact = new BulletSoftClothFactory (this);
  fact->system = this;
  return csPtr<iSoftClothFactory> (fact);
}

csPtr<CS::Physics::iSoftMeshFactory> csBulletSystem::CreateSoftMeshFactory ()
{
  BulletSoftMeshFactory* fact = new BulletSoftMeshFactory (this);
  fact->system = this;
  return csPtr<iSoftMeshFactory> (fact);
}

void csBulletSystem::DeleteAll () 
{
  collSectors.DeleteAll ();
}

void csBulletSystem::InitDebugDraw ()
{
  if (!debugDraw)
  {
    debugDraw = new csBulletDebugDraw (inverseInternalScale);
    for (size_t i = 0; i < collSectors.GetSize (); i++)
      collSectors[i]->bulletWorld->setDebugDrawer (debugDraw);
  }
}

void csBulletSystem::DebugDraw (iView* rview)
{
  InitDebugDraw ();
  for (size_t i = 0; i < collSectors.GetSize (); i++)
    collSectors[i]->bulletWorld->debugDrawWorld ();
  debugDraw->DebugDraw (rview);
}

void csBulletSystem::SetDebugMode (CS::Physics::DebugMode mode)
{
  if (!debugDraw && mode)
    InitDebugDraw ();
  debugDraw->SetDebugMode (mode);
}

CS::Physics::DebugMode csBulletSystem::GetDebugMode ()
{
  if (!debugDraw)
    return CS::Physics::DEBUG_NOTHING;

  return debugDraw->GetDebugMode ();
}

void csBulletSystem::StartProfile ()
{
  CProfileManager::Start_Profile ("Crystal Space scene");
  InitDebugDraw ();
  debugDraw->StartProfile ();
}

void csBulletSystem::StopProfile ()
{
  CProfileManager::Stop_Profile ();
  debugDraw->StopProfile ();
}

void csBulletSystem::DumpProfile (bool resetProfile /* = true */)
{
  printf ("\n");
  printf ("==========================================================\n");
  printf ("====           Bullet dynamic scene profile           ====\n");
  printf ("==========================================================\n");
  CProfileManager::dumpAll ();
  printf ("==========================================================\n");
  printf ("\n");

  if (resetProfile)
    CProfileManager::Reset ();
}

btTriangleMesh* csBulletSystem::CreateBulletTriMesh (iTriangleMesh* triMesh)
{
  btTriangleMesh* btMesh = new btTriangleMesh ();
  
  size_t triangleCount = triMesh->GetTriangleCount ();
  //size_t vertexCount = triMesh->GetVertexCount ();

  size_t i;
  csTriangle *c_triangle = triMesh->GetTriangles ();
  csVector3 *c_vertex = triMesh->GetVertices ();
  for (i = 0; i < triangleCount; i++)
  {
    int index0 = c_triangle[i].a;
    int index1 = c_triangle[i].b;
    int index2 = c_triangle[i].c;

    btVector3 vertex0 (c_vertex[index0].x, c_vertex[index0].y, c_vertex[index0].z);
    btVector3 vertex1 (c_vertex[index1].x, c_vertex[index1].y, c_vertex[index1].z);
    btVector3 vertex2 (c_vertex[index2].x, c_vertex[index2].y, c_vertex[index2].z);

    vertex0 *= GetInternalScale ();
    vertex1 *= GetInternalScale ();
    vertex2 *= GetInternalScale ();

    btMesh->addTriangle (vertex0, vertex1, vertex2);
  }

  return btMesh;
}

void csBulletSystem::ReportWarning (const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csReportV (object_reg, CS_REPORTER_SEVERITY_WARNING,
	     "crystalspace.physics.bullet",
	     msg, arg);
  va_end (arg);
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
