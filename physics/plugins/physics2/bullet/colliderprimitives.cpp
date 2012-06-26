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

#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "csutil/stringquote.h"
#include "igeom/trimesh.h"
#include "imesh/objmodel.h"

// Bullet includes
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btConeShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "colliderprimitives.h"
#include "collisionobject2.h"
#include "bulletsystem.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

csRef<iTriangleMesh> FindColdetTriangleMesh (iMeshWrapper* mesh,
                                             csStringID baseID, csStringID colldetID)
{
  iObjectModel* objModel = mesh->GetMeshObject ()->GetObjectModel ();
  csRef<iTriangleMesh> triMesh;
  bool use_trimesh = objModel->IsTriangleDataSet (baseID);
  if (use_trimesh)
  {
    if  (objModel->GetTriangleData (colldetID))
       triMesh = objModel->GetTriangleData (colldetID);
    else
      triMesh = objModel->GetTriangleData (baseID);
  }

  if (!triMesh || triMesh->GetVertexCount () == 0
      || triMesh->GetTriangleCount () == 0)
  {
    csFPrintf (stderr, "iCollider: No collision polygons, triangles or vertices on mesh factory %s\n",
      CS::Quote::Single (mesh->QueryObject ()->GetName ()));

    return csRef<iTriangleMesh> (nullptr);
  }
  return triMesh;
}

#include "csutil/custom_new_disable.h"

btTriangleMesh* GenerateTriMeshData (iMeshWrapper* mesh, csStringID baseID, 
                                     csStringID colldetID, float interScale)
{
  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh, baseID, colldetID);
  if (!triMesh)
    return nullptr;
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

    vertex0 *= interScale;
    vertex1 *= interScale;
    vertex2 *= interScale;

    btMesh->addTriangle (vertex0,vertex1,vertex2);
  }
  return btMesh;
}

#include "csutil/custom_new_enable.h"

csBulletCollider::csBulletCollider ()
  : scfImplementationType (this), scale (1,1,1), shape (nullptr), margin (0.0), volume (0.0), collSystem (nullptr), 
    children(nullptr), usedShape(nullptr), dirty(true), localInertia(0, 0, 0)
{
  principalAxisTransform.setIdentity();
}

csBulletCollider::~csBulletCollider ()
{
  if (children)
  {
    delete children;
  }
}

bool csBulletCollider::IsDirty() const
{
  // recursively check if the collider tree has been changed
  bool isDirty = dirty;
  if (children && !dirty)
  {
    for (size_t i = 0; i < children->colliders.GetSize(); i++)
    {
      iCollider* icoll = children->colliders[i];
      csBulletCollider* coll = dynamic_cast<csBulletCollider*>(icoll);
      isDirty |= coll->IsDirty();
    }
  }
  return isDirty;
}

void csBulletCollider::SetLocalScale (const csVector3& scale)
{
  // TODO: Fix this
  this->scale = scale;
  shape->setLocalScaling (btVector3(scale.x, scale.y, scale.z));
  //volume *= scale.x * scale.y * scale.z;
}

void csBulletCollider::SetMargin (float margin)
{
  if (margin > 0.0f)
  {
    // TODO: Fix this
    this->margin = margin;
    shape->setMargin (margin * collSystem->getInternalScale ());
  }
}

float csBulletCollider::GetMargin () const
{
  return margin;
}

btCollisionShape* csBulletCollider::GetOrCreateBulletShape()
{
  bool needsRebuild = IsDirty();
  if (!needsRebuild) return usedShape;

  dirty = false;
  
  if (children)
  {
    children->staticColliderCount = 0;
  }

  if (children && children->colliders.GetSize() > 0)
  {
    // create a new shape
    btCompoundShape& compound = children->compoundShape;

    volume = 0;
    int start = shape ? 1 : 0;
    int totalShapeCount = int(children->colliders.GetSize()) + start;

    // TODO: Add density ratio to colliders to allow for non-uniform density
    CS_ALLOC_STACK_ARRAY(float, masses, totalShapeCount);
    
    if (shape)
    {
      btTransform relaTrans;
      relaTrans.setIdentity();
      compound.addChildShape (relaTrans, shape);
      volume = ComputeShapeVolume();
      ++totalShapeCount;
      masses[0] = volume;
    }

    // create compound shape, find total staticColliderCount and compute volume
    for (int i = 0; i < children->colliders.GetSize(); i++)
    {
      iCollider* icoll = children->colliders[i];
      csBulletCollider* coll = dynamic_cast<csBulletCollider*>(icoll);
      btTransform relaTrans = CSToBullet (children->transforms[i], collSystem->getInternalScale ());
      compound.addChildShape (relaTrans, coll->GetOrCreateBulletShape());

      if (!coll->IsDynamic())
      {
        ++children->staticColliderCount;
      }
      masses[start + i] = coll->GetVolume();
      volume += coll->GetVolume();
    }
    
    btVector3 principalInertia;   // we don't care about this
    children->compoundShape.calculatePrincipalAxisTransform(masses, principalAxisTransform, principalInertia);
    usedShape = &children->compoundShape;
  }
  else
  {
    principalAxisTransform.setIdentity();
    usedShape = shape;
    volume = ComputeShapeVolume();
  }
    
  // TODO: Fix the principal axis transform
  principalAxisTransform.setIdentity();
  
  if (usedShape)
  {
    usedShape->calculateLocalInertia(1, localInertia);      // inertia is proportional to mass
  }
  return usedShape;
}

bool csBulletCollider::IsDynamic() const
{
  CS::Collisions::ColliderType type = GetColliderType ();
  if (type == CS::Collisions::COLLIDER_CONCAVE_MESH
    ||type == CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED
    ||type == CS::Collisions::COLLIDER_PLANE
    ||type == CS::Collisions::COLLIDER_TERRAIN)
  {
    return false;
  }
  return !children || children->staticColliderCount == 0;
}

void csBulletCollider::AddCollider (CS::Collisions::iCollider* iColl, const csOrthoTransform& relaTrans)
{
  csRef<csBulletCollider> coll (dynamic_cast<csBulletCollider*>(iColl));

  csColliderCollection* children = GetOrCreateChildren();
  
  dirty = true;
  children->colliders.Push (coll);
  children->transforms.Push (relaTrans);
}

void csBulletCollider::RemoveCollider (CS::Collisions::iCollider* collider)
{
  if (!children) return;

  for (size_t i =0; i < children->colliders.GetSize(); i++)
  {
    if (children->colliders[i] == collider)
    {
      RemoveCollider (i);
      return;
    }
  }
}

void csBulletCollider::RemoveCollider (size_t index)
{
  if (!children) return;

  index = index - (shape ? 1 : 0);
  if (index < children->colliders.GetSize ())
  {
    csRef<csBulletCollider> child = children->colliders[index];
    
    children->colliders.DeleteIndex (index);
    children->transforms.DeleteIndex (index);
  
    dirty = true;
  }
}

CS::Collisions::iCollider* csBulletCollider::GetCollider (size_t index)
{
  if (shape)
  {
    if (index == 0) return this;
  
    if (!children || index > children->colliders.GetSize()) return nullptr;
  
    return children->colliders[index-1];
  }
  else
  {
    if (!children || index >= children->colliders.GetSize()) return nullptr;
  
    return children->colliders[index];
  }
}


csBulletColliderCompound::csBulletColliderCompound (csBulletSystem* sys)
  : scfImplementationType (this)
{
  collSystem = sys;
}

csBulletColliderBox::csBulletColliderBox (const csVector3& boxSize, csBulletSystem* sys)
  : scfImplementationType (this), boxSize (boxSize)
{
  collSystem = sys;
  shape = new btBoxShape (CSToBullet (boxSize*0.5f, collSystem->getInternalScale ()));
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderBox::~csBulletColliderBox ()
{
  delete shape;
}

float csBulletColliderBox::ComputeShapeVolume() const 
{
  return boxSize.x * boxSize.y * boxSize.z;
}

csBulletColliderSphere::csBulletColliderSphere (float radius, csBulletSystem* sys)
  : scfImplementationType (this), radius (radius)
{
  collSystem = sys;
  shape = new btSphereShape (radius * collSystem->getInternalScale ());
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderSphere::~csBulletColliderSphere ()
{
  delete shape;
}

float csBulletColliderSphere::ComputeShapeVolume() const 
{
  return (4/3.f) * PI * radius * radius * radius;
}

void csBulletColliderSphere::SetMargin (float margin)
{
  if (margin > 0.0f)
  {
    this->margin = margin;
    shape->setMargin ((margin + radius)* collSystem->getInternalScale ());
  }
}

// #######################################################################################################
// Cylinder

csBulletColliderCylinder::csBulletColliderCylinder (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  // Lulu: why Z?
  shape = new btCylinderShapeZ (btVector3 (radius * collSystem->getInternalScale (),
    radius * collSystem->getInternalScale (),
    length * collSystem->getInternalScale () * 0.5f));
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderCylinder::~csBulletColliderCylinder ()
{
  delete shape;
}

float csBulletColliderCylinder::ComputeShapeVolume() const 
{
  return PI * radius * radius * length;
}

void csBulletColliderCylinder::GetCylinderGeometry (float& length, float& radius)
{
  length = this->length;
  radius = this->radius;
}


// #######################################################################################################
// Capsule

csBulletColliderCapsule::csBulletColliderCapsule (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  shape = new btCapsuleShapeZ (radius * collSystem->getInternalScale (),
    length * collSystem->getInternalScale ());
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderCapsule::~csBulletColliderCapsule ()
{
  delete shape;
}

float csBulletColliderCapsule::ComputeShapeVolume() const 
{
  return PI * radius * radius * length 
    + 1.333333f * PI * radius * radius * radius;
}

void csBulletColliderCapsule::GetCapsuleGeometry (float& length, float& radius)
{
  length = this->length;
  radius = this->radius;
}



// #######################################################################################################
// Cone

csBulletColliderCone::csBulletColliderCone (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  shape = new btConeShapeZ (radius * collSystem->getInternalScale (),
    length * collSystem->getInternalScale ());
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderCone::~csBulletColliderCone ()
{
  delete shape;
}

float csBulletColliderCone::ComputeShapeVolume() const 
{
  return 0.333333f * PI * radius * radius * length;
}

void csBulletColliderCone::GetConeGeometry (float& length, float& radius)
{
  length = this->length;
  radius = this->radius;
}



// #######################################################################################################
// Plane

csBulletColliderPlane::csBulletColliderPlane (const csPlane3& plane, 
                                              csBulletSystem* sys)
  : scfImplementationType (this), plane (plane)
{
  collSystem = sys;
  csVector3 normal = plane.GetNormal ();
  shape = new btStaticPlaneShape (btVector3 (normal.x,normal.y,normal.z),
    plane.D () * collSystem->getInternalScale ());
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderPlane::~csBulletColliderPlane ()
{
  delete shape;
}

float csBulletColliderPlane::ComputeShapeVolume() const 
{
  return 0;
}



// #######################################################################################################
// Convex Mesh

csBulletColliderConvexMesh::csBulletColliderConvexMesh (iMeshWrapper* mesh, csBulletSystem* sys, bool simplify)
  : scfImplementationType (this), mesh (mesh)
{
  collSystem = sys;
  
  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh, 
    collSystem->baseID, collSystem->colldetID);

  if (!triMesh)
    return;

  size_t vertexCount = triMesh->GetVertexCount ();
  if (vertexCount == 0)
    return;

  btConvexHullShape* convexShape = new btConvexHullShape ();
  
  btTriangleMesh* btTriMesh = GenerateTriMeshData (mesh, collSystem->baseID, collSystem->colldetID, collSystem->getInternalScale ());
  if (! btTriMesh)
    return;
  
  csVector3 *c_vertex = triMesh->GetVertices ();
  if (simplify)
  {
    btConvexShape* tmpConvexShape = new btConvexTriangleMeshShape (btTriMesh);
    btShapeHull* hull = new btShapeHull (tmpConvexShape);
    btScalar marg = tmpConvexShape->getMargin ();
    hull->buildHull (marg);
    tmpConvexShape->setUserPointer (hull);

    for  (int i=0 ; i < hull->numVertices ();i++)
    {
      convexShape->addPoint (hull->getVertexPointer ()[i]);	
    }

    delete tmpConvexShape;
    delete hull;
  }
  else
    for (size_t i = 0; i < vertexCount; i++)
    {
      convexShape->addPoint (CSToBullet (c_vertex[i], collSystem->getInternalScale ()));
    }
  shape = convexShape;
  margin = 0.04 * collSystem->getInverseInternalScale ();

  delete btTriMesh;
}

csBulletColliderConvexMesh::~csBulletColliderConvexMesh ()
{
  delete shape;
}

float csBulletColliderConvexMesh::ComputeShapeVolume() const 
{
  csRef<iTriangleMesh> triMesh = FindColdetTriangleMesh (mesh, 
    collSystem->baseID, collSystem->colldetID);

  if (!triMesh)
    return 0;

  size_t triangleCount = triMesh->GetTriangleCount ();
  size_t vertexCount = triMesh->GetVertexCount ();

  csTriangle *c_triangle = triMesh->GetTriangles ();
  csVector3 *c_vertex = triMesh->GetVertices ();
  csVector3 origin = c_vertex[c_triangle[0].a];

  float vol = 0.0f;
  for (size_t i = 1; i < triangleCount; i++)
  {
    csVector3 a = c_vertex[c_triangle[i].a] - origin;
    csVector3 b = c_vertex[c_triangle[i].b] - origin;
    csVector3 c = c_vertex[c_triangle[i].c] - origin;
    csVector3 d;
    d.Cross (b, c);
    vol += btFabs (a * d);
  }

  vol /= 6.0f;
  return vol;
}



// #######################################################################################################
// Concave Mesh

csBulletColliderConcaveMesh::csBulletColliderConcaveMesh (iMeshWrapper* mesh, csBulletSystem* sys)
  : scfImplementationType (this), mesh (mesh)
{
  btTransform tran;
  btVector3 aabbmin (-1000, -1000, -1000);
  btVector3 aabbmax (1000, 1000, 1000);

  collSystem = sys;
  triMesh = GenerateTriMeshData (mesh, collSystem->baseID, collSystem->colldetID, collSystem->getInternalScale ());
  btBvhTriangleMeshShape* treeShape = new btBvhTriangleMeshShape (triMesh,true);
  treeShape->recalcLocalAabb();
  shape = treeShape;
  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderConcaveMesh::~csBulletColliderConcaveMesh ()
{
  delete shape;
  delete triMesh;
}

float csBulletColliderConcaveMesh::ComputeShapeVolume() const 
{
  btTransform tran;
  btVector3 aabbmin (-1000, -1000, -1000);
  btVector3 aabbmax (1000, 1000, 1000);

  shape->getAabb (tran, aabbmin, aabbmax);

  btVector3 aabbExtents = aabbmax - aabbmin;
  aabbExtents *= collSystem->getInverseInternalScale ();

  return aabbExtents.x() * aabbExtents.y() * aabbExtents.z();
}

csBulletColliderConcaveMeshScaled::csBulletColliderConcaveMeshScaled (CS::Collisions::iColliderConcaveMesh* collider,
                                                                      csVector3 scale, csBulletSystem* sys)
  : scfImplementationType (this)
{
  collSystem = sys;
  this->scale = scale;
  originalCollider = dynamic_cast<csBulletColliderConcaveMesh*> (collider);
  if (originalCollider->shape->getShapeType () == TRIANGLE_MESH_SHAPE_PROXYTYPE)
  {
    btBvhTriangleMeshShape* meshShape = dynamic_cast<btBvhTriangleMeshShape*> (originalCollider->shape);
    shape = new btScaledBvhTriangleMeshShape (meshShape, 
      btVector3 (scale[0], scale[1] , scale[2]));
  }
  else
    csFPrintf  (stderr, "csBulletColliderConcaveMeshScaled: Original collider is not concave mesh.\n");

  margin = 0.04 * collSystem->getInverseInternalScale ();
}

csBulletColliderConcaveMeshScaled::~csBulletColliderConcaveMeshScaled ()
{
  delete shape;
}

float csBulletColliderConcaveMeshScaled::ComputeShapeVolume() const 
{
  // TODO: This is very wrong
  return originalCollider->GetVolume () * scale.x * scale.y * scale.z;
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
