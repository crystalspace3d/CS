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
#include "convexdecompose/ConvexBuilder.h"

#include "csutil/custom_new_enable.h"

#include "common2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "collisionactor.h"
#include "joint2.h"

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

  CS::Collisions::CollisionGroup terrainGroup ("Terrain");
  terrainGroup.value = CollisionGroupMaskValueTerrain;
  terrainGroup.mask = allFilter ^ CollisionGroupMaskValueTerrain;
  collGroups.Push (terrainGroup);

  CS::Collisions::CollisionGroup portalGroup ("Portal");
  portalGroup.value = CollisionGroupMaskValuePortal;
  portalGroup.mask = allFilter ^ CollisionGroupMaskValuePortal;
  collGroups.Push (portalGroup);

  CS::Collisions::CollisionGroup copyGroup ("PortalCopy");
  copyGroup.value = CollisionGroupMaskValuePortalCopy;
  copyGroup.mask = allFilter ^ CollisionGroupMaskValuePortalCopy;
  collGroups.Push (copyGroup);

  CS::Collisions::CollisionGroup characterGroup ("Character");
  characterGroup.value = CollisionGroupMaskValueActor;
  characterGroup.mask = allFilter ^ CollisionGroupMaskValueActor;
  collGroups.Push (characterGroup);
  
  SetGroupCollision ("PortalCopy", "Terrain", false);
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
  csRef<csBulletColliderConvexMesh> collider = csPtr<csBulletColliderConvexMesh>(new csBulletColliderConvexMesh (mesh, this, simplify));

  //colliders.Push (collider);
  return csPtr<iColliderConvexMesh>(collider);
}

csPtr<CS::Collisions::iColliderConcaveMesh> csBulletSystem::CreateColliderConcaveMesh (iMeshWrapper* mesh)
{
  csRef<csBulletColliderConcaveMesh> collider = csPtr<csBulletColliderConcaveMesh>(new csBulletColliderConcaveMesh (mesh,this));

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

csPtr<CS::Collisions::iGhostCollisionObject> csBulletSystem::CreateGhostCollisionObject (CS::Collisions::GhostCollisionObjectProperties* props)
{
  csRef<csBulletGhostCollisionObject> collObject = csPtr<csBulletGhostCollisionObject>(new csBulletGhostCollisionObject (this));

  collObject->CreateCollisionObject(props);
  collObject->RebuildObject ();

  //objects.Push (collObject);
  return csPtr<iGhostCollisionObject>(collObject);
}

csPtr<CS::Collisions::iCollisionObject> csBulletSystem::CreateCollisionObject (CS::Collisions::CollisionObjectProperties* props)
{
  return nullptr;
}

csPtr<CS::Collisions::iCollisionActor> csBulletSystem::CreateCollisionActor (CS::Collisions::CollisionActorProperties* props)
{
  csBulletCollisionActor* actor = new csBulletCollisionActor (this);

  actor->CreateCollisionActor(props);
  actor->RebuildObject ();

  csRef<CS::Collisions::iCollisionActor> iactor = csPtr<CS::Collisions::iCollisionActor>(actor);
  return csPtr<iCollisionActor>(iactor);
}

csPtr<CS::Collisions::iCollisionSector> csBulletSystem::CreateCollisionSector ()
{
  csRef<csBulletSector> collSector = csPtr<csBulletSector>(new csBulletSector (this));

  collSectors.Push (collSector);
  return csPtr<iCollisionSector>(collSector);
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const char* name)
{
  return this->collSectors.FindByName (name);
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

void csBulletSystem::DecomposeConcaveMesh (CS::Collisions::iCollider* root, iMeshWrapper* mesh, bool simplify)
{
  class MyConvexDecomposition : public ConvexDecomposition::ConvexDecompInterface
  {
    int mHullCount;
    int mBaseCount;
    float scale;
    btVector3 centroid;
    bool simp;
  public:
    btAlignedObjectArray<btConvexHullShape*> m_convexShapes;
    btAlignedObjectArray<btVector3> m_convexCentroids;
    btAlignedObjectArray<float> m_convexVolume;

    MyConvexDecomposition (float scale, bool simplify)
      : mHullCount (0), mBaseCount (0), scale (scale), simp (simplify)
    {
    }

    virtual void ConvexDecompResult(ConvexDecomposition::ConvexResult &result)
    {
      btVector3 localScaling(scale, scale, scale);

      //calc centroid, to shift vertices around center of mass
      centroid.setValue(0,0,0);

      btAlignedObjectArray<btVector3> vertices;
      //const unsigned int *src = result.mHullIndices;
      for (unsigned int i=0; i<result.mHullVcount; i++)
      {
        btVector3 vertex(result.mHullVertices[i*3],result.mHullVertices[i*3+1],result.mHullVertices[i*3+2]);
        vertex *= localScaling;
        centroid += vertex;

      }

      centroid *= 1.f/(float(result.mHullVcount) );

      if (simp)
        for (unsigned int i=0; i<result.mHullVcount; i++)
        {
          btVector3 vertex(result.mHullVertices[i*3],result.mHullVertices[i*3+1],result.mHullVertices[i*3+2]);
          vertex *= localScaling;
          vertex -= centroid;
          vertices.push_back(vertex);
        }
      else
      {
        const unsigned int *src = result.mHullIndices;
        for (unsigned int i=0; i<result.mHullTcount; i++)
        {
          unsigned int index0 = *src++;
          unsigned int index1 = *src++;
          unsigned int index2 = *src++;

          btVector3 vertex0(result.mHullVertices[index0*3], result.mHullVertices[index0*3+1],result.mHullVertices[index0*3+2]);
          btVector3 vertex1(result.mHullVertices[index1*3], result.mHullVertices[index1*3+1],result.mHullVertices[index1*3+2]);
          btVector3 vertex2(result.mHullVertices[index2*3], result.mHullVertices[index2*3+1],result.mHullVertices[index2*3+2]);
          vertex0 *= localScaling;
          vertex1 *= localScaling;
          vertex2 *= localScaling;

          vertex0 -= centroid;
          vertex1 -= centroid;
          vertex2 -= centroid;

          //TODO this will add duplicate vertices to convex shape. But the debug draw result is right now.
          vertices.push_back(vertex0);
          vertices.push_back(vertex1);
          vertices.push_back(vertex2);

          index0+=mBaseCount;
          index1+=mBaseCount;
          index2+=mBaseCount;
        }
      }

      btConvexHullShape* convexShape = new btConvexHullShape(&(vertices[0].getX()),vertices.size());
      convexShape->setMargin(0.01f);
      m_convexShapes.push_back(convexShape);
      m_convexCentroids.push_back(centroid);
      m_convexVolume.push_back(result.mHullVolume);
      mBaseCount+=result.mHullVcount; // advance the 'base index' counter.
    }
  };

  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh, baseID, colldetID);
  if (! triMesh)
    return;

  unsigned int depth = 5;
  float cpercent     = 5;
  float ppercent     = 15;
  unsigned int maxv  = 16;
  float skinWidth    = 0.0;

  csTriangle *c_triangle = triMesh->GetTriangles ();
  csVector3 *c_vertex = triMesh->GetVertices ();

  ConvexDecomposition::DecompDesc desc;
  desc.mVcount       = uint (triMesh->GetVertexCount ());
  desc.mVertices     = (float*)c_vertex;
  desc.mTcount       = uint (triMesh->GetTriangleCount ());
  desc.mIndices      = (unsigned int *)c_triangle;
  desc.mDepth        = depth;
  desc.mCpercent     = cpercent;
  desc.mPpercent     = ppercent;
  desc.mMaxVertices  = maxv;
  desc.mSkinWidth    = skinWidth;

  MyConvexDecomposition	convexDecomposition(internalScale, simplify);
  desc.mCallback = &convexDecomposition;

  ConvexBuilder cb(desc.mCallback);
  cb.process(desc);

  btTransform trans;
  trans.setIdentity();
  csOrthoTransform relaTransform;
  for (int i=0;i<convexDecomposition.m_convexShapes.size();i++)
  {

    btVector3 centroid = convexDecomposition.m_convexCentroids[i];
    trans.setOrigin(centroid);
    btConvexHullShape* convexShape = convexDecomposition.m_convexShapes[i];
    csRef<csBulletCollider> collider = csPtr<csBulletCollider>(new csBulletColliderConvexMesh (convexShape, convexDecomposition.m_convexVolume[i], this));
    //colliders.Push (collider);
    relaTransform = BulletToCS (trans, inverseInternalScale);
    root->AddCollider (collider, relaTransform);
  }
}

csPtr<CS::Physics::iRigidBody> csBulletSystem::CreateRigidBody (RigidBodyProperties* props)
{
  csRef<csBulletRigidBody> body = csPtr<csBulletRigidBody>(new csBulletRigidBody (this));

  body->CreateRigidBodyObject(props);
  
  //rigidBodies.Push (body);
  return csPtr<CS::Physics::iRigidBody>(body);
}

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

csPtr<CS::Physics::iSoftBody> csBulletSystem::CreateRope (csVector3 start,
                                             csVector3 end,
                                             size_t segmentCount)
{
  //Don't know the soft world info currently. So just set it to nullptr
  
  btSoftBody* body = btSoftBodyHelpers::CreateRope
    (*defaultInfo, CSToBullet (start, internalScale),
    CSToBullet (end, internalScale), int (segmentCount) - 1, 0);

  //hard-coded parameters for hair ropes
  body->m_cfg.kDP = 0.08f; // no elasticity
  body->m_cfg.piterations = 16; // no white zone
  body->m_cfg.timescale = 2;

  return csPtr<CS::Physics::iSoftBody>(new csBulletSoftBody (this, body));
}

csPtr<CS::Physics::iSoftBody> csBulletSystem::CreateRope (csVector3* vertices, size_t vertexCount)
{
  // Create the nodes
  CS_ALLOC_STACK_ARRAY(btVector3, nodes, vertexCount);
  CS_ALLOC_STACK_ARRAY(btScalar, materials, vertexCount);

  for (size_t i = 0; i < vertexCount; i++)
  {
    nodes[i] = CSToBullet (vertices[i], internalScale);
    materials[i] = 1;
  }

  btSoftBody* body = new btSoftBody(nullptr, int (vertexCount), &nodes[0], &materials[0]);

  // Create the links between the nodes
  for (size_t i = 1; i < vertexCount; i++)
    body->appendLink (int (i - 1), int (i));

  //hard-coded parameters for hair ropes
  body->m_cfg.kDP = 0.08f; // no elasticity
  body->m_cfg.piterations = 16; // no white zone
  body->m_cfg.timescale = 2;

  //softBodies.Push (csBody);
  return csPtr<CS::Physics::iSoftBody>(new csBulletSoftBody (this, body)); 
}

csPtr<CS::Physics::iSoftBody> csBulletSystem::CreateCloth (csVector3 corner1, csVector3 corner2,
                                              csVector3 corner3, csVector3 corner4,
                                              size_t segmentCount1, size_t segmentCount2,
                                              bool withDiagonals /* = false */)
{
  btSoftBody* body = btSoftBodyHelpers::CreatePatch
    (*defaultInfo, CSToBullet (corner1, internalScale),
    CSToBullet (corner2, internalScale), CSToBullet (corner3, internalScale),
    CSToBullet (corner4, internalScale), int (segmentCount1), int (segmentCount2), 0,
    withDiagonals);
  body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

  //softBodies.Push (csBody);
  return csPtr<CS::Physics::iSoftBody>(new csBulletSoftBody (this, body));
}

csPtr<CS::Physics::iSoftBody> csBulletSystem::CreateSoftBody (iGeneralFactoryState* genmeshFactory,
                                                 const csOrthoTransform& bodyTransform)
{
  btScalar* vertices = new btScalar[genmeshFactory->GetVertexCount () * 3];
  for (int i = 0; i < genmeshFactory->GetVertexCount (); i++)
  {
    csVector3 vertex = genmeshFactory->GetVertices ()[i] * internalScale;
    vertices[i * 3] = vertex[0];
    vertices[i * 3 + 1] = vertex[1];
    vertices[i * 3 + 2] = vertex[2];
  }

  int* triangles = new int[genmeshFactory->GetTriangleCount () * 3];
  for (int i = 0; i < genmeshFactory->GetTriangleCount (); i++)
  {
    csTriangle& triangle = genmeshFactory->GetTriangles ()[i];
    triangles[i * 3] = triangle.a;
    triangles[i * 3 + 1] = triangle.b;
    triangles[i * 3 + 2] = triangle.c;
  }

  btSoftBody* body = btSoftBodyHelpers::CreateFromTriMesh
    (*defaultInfo, vertices, triangles, genmeshFactory->GetTriangleCount (),
    false);

  body->m_cfg.piterations = 10;
  body->m_cfg.collisions |=	btSoftBody::fCollision::SDF_RS;
  body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;
  body->m_materials[0]->m_kLST = 1;

  csRef<csBulletSoftBody> csBody = csPtr<csBulletSoftBody>(new csBulletSoftBody (this, body));
  csBody->SetTransform (bodyTransform);

  //softBodies.Push (csBody);
  return csPtr<CS::Physics::iSoftBody>(csBody);
}

csPtr<CS::Physics::iSoftBody> csBulletSystem::CreateSoftBody (csVector3* vertices, size_t vertexCount,
                                                 csTriangle* triangles, size_t triangleCount,
                                                 const csOrthoTransform& bodyTransform)
{
  btScalar* btVertices = new btScalar[vertexCount * 3];
  for (size_t i = 0; i < vertexCount; i++)
  {
    csVector3 vertex = vertices[i] * internalScale;
    btVertices[i * 3] = vertex[0];
    btVertices[i * 3 + 1] = vertex[1];
    btVertices[i * 3 + 2] = vertex[2];
  }

  int* btTriangles = new int[triangleCount * 3];
  for (size_t i = 0; i < triangleCount; i++)
  {
    csTriangle& triangle = triangles[i];
    btTriangles[i * 3] = triangle.a;
    btTriangles[i * 3 + 1] = triangle.b;
    btTriangles[i * 3 + 2] = triangle.c;
  }

  btSoftBody* body = btSoftBodyHelpers::CreateFromTriMesh
    (*defaultInfo, btVertices, btTriangles, int (triangleCount), false);

  body->m_cfg.piterations = 10;
  body->m_cfg.collisions |=	btSoftBody::fCollision::SDF_RS;
  body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;
  body->m_materials[0]->m_kLST = 1;

  delete [] btVertices;
  delete [] btTriangles;

  csRef<csBulletSoftBody> csBody = csPtr<csBulletSoftBody>(new csBulletSoftBody (this, body));
  csBody->SetTransform (bodyTransform);

  //softBodies.Push (csBody);
  return csPtr<CS::Physics::iSoftBody>(csBody);
}

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

}
CS_PLUGIN_NAMESPACE_END(Bullet2)
