/*
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

#ifndef __CS_IVARIA_COLLIDERS_H__
#define __CS_IVARIA_COLLIDERS_H__

/**\file
 * Collision primitives interfaces
 */

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "iutil/object.h"

struct iCamera;
struct iMeshWrapper;
struct iMovable;
struct iPortal;
struct iSector;
struct iTerrainCell;
struct iTerrainSystem;
struct iTriangleMesh;
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
COLLIDER_INVALID = 0,              /*!< Undefined collider type. */
COLLIDER_BOX,                      /*!< The collider is a box. */
COLLIDER_SPHERE,                   /*!< The collider is a sphere. */
COLLIDER_CYLINDER,                 /*!< The collider is a cylinder. */
COLLIDER_CAPSULE,                  /*!< The collider is a capsule. */
COLLIDER_CONE,                     /*!< The collider is a cone. */
COLLIDER_PLANE,                    /*!< The collider is a plane. */
COLLIDER_CONVEX_MESH,              /*!< The collider is a convex mesh. */
COLLIDER_CONCAVE_MESH,             /*!< The collider is a concave mesh. */
COLLIDER_CONCAVE_MESH_SCALED,      /*!< The collider is a scaled concave mesh. */
COLLIDER_TERRAIN,                  /*!< The collider is a terrain. */
COLLIDER_COMPOUND                  /*!< The collider is a compound collider. TODO: to be removed? */
};

/**
 * A base interface for colliders. 
 * Other colliders will be derived from this one.
 */
struct iCollider : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollider, 1, 0, 0);

  /// Get the type of this collider. 
  virtual ColliderType GetColliderType () const = 0;

  /// Set the scale of the collider shape on X/Y/Z axis.
  virtual void SetLocalScale (const csVector3& scale) = 0;

  /// Get the scale along the X/Y/Z axes.
  virtual csVector3 GetLocalScale () const = 0;
  
  /// Set the margin of this collider
  virtual void SetMargin (float margin) = 0;

  /// Get the margin of this collider
  virtual float GetMargin () const = 0; 

  /// Get the volume of this collider
  virtual float GetVolume () const = 0;

  /// Whether this collider (and all its children) can be used for dynamic simulation
  // TODO: remove?
  virtual bool IsDynamic () const = 0;

  /// Get the frame of reference
  // TODO: remove!
  virtual csOrthoTransform GetPrincipalAxisTransform () const = 0;
  /// Set the frame of reference
  virtual void SetPrincipalAxisTransform (const csOrthoTransform& trans) = 0;

  /// Add a child collider, fixed relative to this collider.
  // TODO: Add/RemoveChild?
  virtual void AddCollider (iCollider* collider, const csOrthoTransform& transform = csOrthoTransform ()) = 0;

  /// Remove the given collider from this collider.
  virtual void RemoveCollider (iCollider* collider) = 0;

  /// Remove the collider with the given index from this collider.
  virtual void RemoveCollider (size_t index) = 0;

  /// Get the collider with the given index.
  virtual iCollider* GetCollider (size_t index) = 0; 

  /// Get the collider and it's relative transformation at the given index.
  virtual void GetCollider (size_t index, iCollider*& collider, csOrthoTransform& transform) = 0;

  /// Get the count of colliders in this collider (including self).
  // TODO: const
  virtual size_t GetColliderCount () = 0;

  /// Returns the AABB of this shape, centered at it's center of mass (assuming uniform density)
  // TODO: remove? or use csBox at least
  virtual void GetAABB (csVector3& aabbMin, csVector3& aabbMax) const = 0;
};


/**
 * A compound collider. Does not have a root shape, but only children.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCompound()
 * 
 * Main ways to get pointers to this interface:
 * - iCollider::GetCollider()
 * 
 * Main users of this interface:
 * - iCollider
 * - iCollisionObject
 */
// TODO: remove?
struct iColliderCompound : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCompound, 1, 0, 0);
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
  virtual const csVector3& GetBoxGeometry () const = 0;
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
  virtual float GetSphereGeometry () const = 0;
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
  virtual void GetCylinderGeometry (float& length, float& radius) const = 0;
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
  virtual void GetCapsuleGeometry (float& length, float& radius) const = 0;
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
  virtual void GetConeGeometry (float& length, float& radius) const = 0;
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
  virtual const csPlane3& GetPlaneGeometry () const = 0;
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

  /// Get the triangle mesh of this collider.
  virtual iTriangleMesh* GetMesh () const = 0;
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
  virtual iTriangleMesh* GetMesh () const = 0;
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
  virtual iColliderConcaveMesh* GetCollider () const = 0;

  /// Get the scale of this collider.
  virtual const csVector3& GetScale () const = 0;
};

/**
 * A terrain collider.
 */
struct iColliderTerrain : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderTerrain, 1, 0, 0);

  /// Returns the terrain cell, represented by this collider
  virtual iTerrainCell* GetCell () const = 0;
};

}
}

#endif
