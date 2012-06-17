/*
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

#ifndef __CS_IVARIA_COLLIDERS_H__
#define __CS_IVARIA_COLLIDERS_H__

/**\file
 * Collision interfaces
 */

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "iutil/object.h"

struct iTerrainSystem;
struct iSector;
struct iMeshWrapper;
struct iMovable;
struct iPortal;
struct iCamera;
struct iView;

namespace CS
{
namespace Physics
{
struct iPhysicalBody;
}
}

namespace CS
{
namespace Collisions
{
  
/**
 * The type of a collider.
 */
enum ColliderType
{
COLLIDER_INVALID = 0,
COLLIDER_BOX,
COLLIDER_SPHERE,
COLLIDER_CYLINDER,
COLLIDER_CAPSULE,
COLLIDER_CONE,
COLLIDER_PLANE,
COLLIDER_CONVEX_MESH,
COLLIDER_CONCAVE_MESH,
COLLIDER_CONCAVE_MESH_SCALED,
COLLIDER_TERRAIN
};

/**
 * A base interface for colliders. 
 * Other colliders will be derived from this one.
 */
struct iCollider : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollider, 1, 0, 0);

  /// Get the type of this collider. 
  virtual ColliderType GetType () const = 0;

  /// Set the scale of the collider shape on X/Y/Z axis.
  virtual void SetLocalScale (const csVector3& scale) = 0;

  /// Get the scale on X/Y/Z axis.
  virtual const csVector3& GetLocalScale () const = 0;
  
  /// Set the margin of collision shape.
  virtual void SetMargin (float margin) = 0;

  /// Get the margin of collision shape.
  virtual float GetMargin () const = 0; 
};

/**
 * A box collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderBox()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderBox : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderBox, 1, 0, 0);

  /// Get the box geometry of this collider.
  virtual csVector3 GetBoxGeometry ()  = 0;
};

/**
 * A sphere collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderSphere()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderSphere : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderSphere, 1, 0, 0);

  /// Get the sphere geometry of this collider.
  virtual float GetSphereGeometry () = 0;
};

/**
 * A cylinder collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCylinder()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCylinder : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCylinder, 1, 0, 0);

  /// Get the cylinder geometry of this collider.
  virtual void GetCylinderGeometry (float& length, float& radius) = 0;
};

/**
 * A capsule collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCapsule()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCapsule : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCapsule, 1, 0, 0);

  /// Get the capsule geometry of this collider.
  virtual void GetCapsuleGeometry (float& length, float& radius) = 0;
};

/**
 * A cone collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCone()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCone : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCone, 1, 0, 0);

  /// Get the cone geometry of this collider.
  virtual void GetConeGeometry (float& length, float& radius) = 0;
};

/**
 * A static plane collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderPlane()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderPlane : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderPlane, 1, 0, 0);

  /// Get the plane geometry of this collider.
  virtual csPlane3 GetPlaneGeometry () = 0;
};

/**
 * A convex mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConvexMesh()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConvexMesh : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConvexMesh, 1, 0, 0);

  /// Get the mesh factory of this collider.
  virtual iMeshWrapper* GetMesh () = 0;
};

/**
 * A static concave mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConcaveMesh()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConcaveMesh : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConcaveMesh, 1, 0, 0);

  /// Get the mesh factory of this collider.
  virtual iMeshWrapper* GetMesh () = 0;
};

/**
 * A scaled static concave mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConcaveMeshScaled()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConcaveMeshScaled : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConcaveMeshScaled, 1, 0, 0);

  /// Get the concave collider scaled by this collider.
  virtual iColliderConcaveMesh* GetCollider () = 0;
};

/**
 * A terrain collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderTerrain()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderTerrain : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderTerrain, 1, 0, 0);

  /// Get the terrain system.
  virtual iTerrainSystem* GetTerrain () const = 0;
};

}
}

#endif
