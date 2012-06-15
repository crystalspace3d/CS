/*
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

#ifndef __CS_IVARIA_COLLISION2_H__
#define __CS_IVARIA_COLLISION2_H__

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

#define	SQRT2				1.41421356237f	

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

struct csConvexResult;
struct iCollisionCallback;
struct iCollisionObject;
typedef short CollisionGroupMask;

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
 * The type of a collision object.
 */
enum CollisionObjectType
{
COLLISION_OBJECT_PHYSICAL_STATIC = 0,
COLLISION_OBJECT_PHYSICAL_DYNAMIC,
COLLISION_OBJECT_GHOST,
COLLISION_OBJECT_ACTOR
};

enum CollisionGroupType
{
  CollisionGroupTypeDefault =     0,
  CollisionGroupTypeStatic =      1,
  CollisionGroupTypeKinematic =   2,
  CollisionGroupTypePortal =      3,
  CollisionGroupTypeTerrain =     3,
  CollisionGroupTypePortalCopy =  4,
  CollisionGroupTypeActor =       5
};

enum CollisionGroupMaskValue
{
  CollisionGroupMaskValueDefault =      0x0001,
  CollisionGroupMaskValueStatic =       0x0002,
  CollisionGroupMaskValueKinematic =    0x0004,
  CollisionGroupMaskValuePortal =       0x0008,
  CollisionGroupMaskValueTerrain =      0x0008,
  CollisionGroupMaskValuePortalCopy =   0x0010,
  CollisionGroupMaskValueActor =        0x0020
};

/**
 * A structure of collision group. 
 * The objects in the group will not collide with each other.
 */
struct CollisionGroup
{
  /// The name of the group.
  csString name;

  /// The value of the group.
  CollisionGroupMask value;

  /// The mask of the group.
  CollisionGroupMask mask;

  CollisionGroup () {}

  CollisionGroup (const char* name)
    : name (name)
  {}
};

/**
 * A structure used to return the result of hit beam.
 */
struct HitBeamResult
{
  HitBeamResult ()
  : hasHit (false), object (0), isect (0.0f), normal (0.0f), vertexIndex (0)
  {}

  /// Whether the beam has hit a body or not.
  bool hasHit;

  /// The collision object that was hit, or \a nullptr if no object was hit.
  iCollisionObject* object;

  /// Intersection point in world space.
  csVector3 isect;

  /// Normal to the surface of the body at the intersection point.
  csVector3 normal;

  /**
   * The index of the closest vertex of the soft body to be hit. This is only valid
   * if it is a soft body which is hit.
   */
  size_t vertexIndex;
};

/**
 * A structure used to return the collision data between two objects.
 */
struct CollisionData
{
  /// Collision object A.
  iCollisionObject* objectA;

  /// Collision object B.
  iCollisionObject* objectB;

  /// The collision position of A in world space.
  csVector3 positionWorldOnA;

  /// The collision position of B in world space.
  csVector3 positionWorldOnB;

  /// The normal of hit position on B.
  csVector3 normalWorldOnB;

  /// The depth of penetration.
  float penetration; 
};

/**
 * This is the interface for attaching a collision callback to a collision object.
 *
 * Main ways to get pointers to this interface:
 * - application specific
 * 
 * Main users of this interface:
 * - iCollisionSystem
 */
struct iCollisionCallback: public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionCallback, 1, 0, 0);

  /**
   * A collision occurred.
   * \param thisbody The body that received a collision.
   * \param otherbody The body that collided with \a thisBody.
   * \param collisions The list of collisions between the two bodies.  
   */
  virtual void OnCollision (iCollisionObject *thisbody, iCollisionObject *otherbody, 
      const csArray<CollisionData>& collisions) = 0; 
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

/**
 * This is the interface of a collision object. 
 *It contains the collision information of the object.
 * 
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionObject()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::GetCollisionObject()
 * 
 * Main users of this interface:
 * - iCollisionSystem
 */
struct iCollisionObject : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionObject, 1, 0, 1);

  /// Return the underlying object
  virtual iObject *QueryObject (void) = 0;

  /// Return the collision object pointer.
  virtual iCollisionObject* QueryCollisionObject () = 0;

  /// Return the physical body pointer if it's a physical body, or NULL.
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () = 0;

  /// Whether this is a rigid or soft body object
  virtual bool IsPhysicalObject() const = 0;

  /// Return the type of the collision object.
  virtual CollisionObjectType GetObjectType () const = 0;

  /**
   * Set the movable attached to this collision object. Its position will be updated
   * automatically when this object is moved.
   */
  virtual void SetAttachedMovable (iMovable* movable) = 0;

  /// Get the movable attached to this collision object.
  virtual iMovable* GetAttachedMovable () = 0;

  /**
   * Set the camera attached to this collision object. Its position will be updated
   * automatically when this object is moved.
   */
  virtual void SetAttachedCamera (iCamera* camera) = 0;

  /// Get the camera attached to this collision object.
  virtual iCamera* GetAttachedCamera () const = 0;
  
  /// Set the transform.
  virtual void SetTransform (const csOrthoTransform& trans) = 0;

  /// Get the transform.
  virtual csOrthoTransform GetTransform () const = 0;

  /**
   * Set current rotation in angles around every axis and set to actor.
   * If a camera is used, set it to camera too.
   */
  virtual void SetRotation (const csMatrix3& rot) = 0;

  /// Rotate the collision actor.
  virtual void Rotate (const csVector3& v, float angle) = 0;

  /// Increases pitch angle by the given value in radians
  virtual void IncreasePitch(float yawDelta) = 0;

  /// Increases yaw angle by the given value in radians
  virtual void IncreaseYaw(float yawDelta) = 0;

  /// Add a collider to this collision body.
  virtual void AddCollider (iCollider* collider, const csOrthoTransform& relaTrans
    = csOrthoTransform (csMatrix3 (), csVector3 (0))) = 0;

  /// Remove the given collider from this collision object.
  virtual void RemoveCollider (iCollider* collider) = 0;

  /// Remove the collider with the given index from this collision object.
  virtual void RemoveCollider (size_t index) = 0;

  /// Get the collider with the given index.
  virtual iCollider* GetCollider (size_t index) = 0;

  /// Get the count of colliders in this collision object.
  virtual size_t GetColliderCount () = 0;

  /// Rebuild this collision object.
  virtual void RebuildObject () = 0;

  /// Set the collision group this object belongs to by name.
  virtual void SetCollisionGroup (const char* name) = 0;

  /// Get the collision group this object belongs to.
  virtual const char* GetCollisionGroup () const = 0;

  /**
   * Set a callback to be executed when this body collides with another.
   * If 0, no callback is executed.
   */
  virtual void SetCollisionCallback (iCollisionCallback* cb) = 0;

  /// Get the collision response callback.
  virtual iCollisionCallback* GetCollisionCallback () = 0;

  /// Test collision with another collision objects.
  virtual bool Collide (iCollisionObject* otherObject) = 0;

  /// Follow a beam from start to end and return whether this body was hit.
  virtual HitBeamResult HitBeam (
      const csVector3& start, const csVector3& end) = 0;

  /// Get the count of collision objects contacted with this object.
  virtual size_t GetContactObjectsCount () = 0;

  /// Get the collision object contacted with this object by index.
  virtual iCollisionObject* GetContactObject (size_t index) = 0;
};

/**
 * This is the interface for ghost-type collision objects.
 * Ghost objects can be set to not interact with the physical world while still collecting collision information.
 * This can be used as a test for collisions, or to implement any sort of object that does not entirely play by the laws 
 * of ridig body or soft body dynamics.
 */
struct iCollisionGhostObject : public virtual iCollisionObject
{

};

/**
 * A iCollisionActor is a kinematic collision object. It has a faster collision detection and response.
 * You can use it to create a player or character model with gravity handling.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionActor()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::GetCollisionActor()
 * 
 * Main users of this interface:
 * - iCollisionSystem
 * \remark The collider of iCollisionActor must be a convex shape. For example, box, convex mesh.
 */
// kickvb: most of this would have to be redesigned, let's do it later
struct iCollisionActor : public virtual iCollisionGhostObject
{
  SCF_INTERFACE (CS::Collisions::iCollisionActor, 1, 0, 0);

  /// Whether the actor is currently not bound by gravity
  virtual bool IsFlying () const = 0;

  /// Set whether the actor is currently not bound by gravity
  virtual void SetFlying (bool flying) = 0;

  /// Check if we are on the ground.
  virtual bool IsOnGround () const = 0;

  virtual float GetPitch () const = 0;

  /// Set the onground status.
  //virtual void SetOnGround (bool og) = 0;

  /// Move the actor.
  virtual void UpdateAction (float delta) = 0;

  /// Set the walking velocity of the actor in the forward and right direction for the given timeInterval.
  virtual void SetPlanarVelocity (const csVector2& vel, float timeInterval = float(INT_MAX)) = 0;

  /// Set the walking velocity of the actor in the forward and right direction for the given timeInterval.
  virtual void SetVelocity (const csVector3& vel, float timeInterval = float(INT_MAX)) = 0;


  /// Set the falling speed.
  virtual void SetFallSpeed (float fallSpeed) = 0;

  /// Set the jumping speed.
  virtual void SetJumpSpeed (float jumpSpeed) = 0;

  /// Set the max jump height an actor can have.
  virtual void SetMaxJumpHeight (float maxJumpHeight) = 0;

  /// Set the step up height an actor can have.
  virtual void StepHeight (float stepHeight) = 0;

  /// Let the actor jump.
  virtual void Jump () = 0;

  /**
   * The max slope determines the maximum angle that the actor can walk up.
   * The slope angle is measured in radians.
   */
  virtual void SetMaxSlope (float slopeRadians) = 0;
  
  /// Get the max slope.
  virtual float GetMaxSlope () const = 0;
};

/**
 * This is the interface for the collision sector.
 * It handles all collision detection of collision objects.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionSector()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::FindCollisionSector()
 *
 * \sa CS::Physics::iPhysicalSector CS::Physics::Bullet2::iPhysicalSector
 */
struct iCollisionSector : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionSector, 1, 0, 0);

  /// Return the underlying object
  virtual iObject *QueryObject (void) = 0;

  /// Set the global gravity.
  virtual void SetGravity (const csVector3& v) = 0;

  /// Get the global gravity.
  virtual csVector3 GetGravity () const = 0;

  /**
   * Add a collision object into the sector.
   * The collision object has to be initialized.
   */
  virtual void AddCollisionObject (iCollisionObject* object) = 0;

  /// Remove a collision object by pointer.
  virtual void RemoveCollisionObject (iCollisionObject* object) = 0;

  /// Get the count of collision objects.
  virtual size_t GetCollisionObjectCount () = 0;

  /// Get the collision object by index.
  virtual iCollisionObject* GetCollisionObject (size_t index) = 0;

  /// Find a collision object within a sector.
  virtual iCollisionObject* FindCollisionObject (const char* name) = 0;

  /// Add a portal into the sector. Collision objects crossing a portal will be switched from iCollisionSector's.
  virtual void AddPortal (iPortal* portal, const csOrthoTransform& meshTrans) = 0;

  /// Remove the given portal from this sector.
  virtual void RemovePortal (iPortal* portal) = 0;

  /**
   * Set the engine iSector related to this collision sector. The iMovable that are 
   * attached to a iCollisionObject present in this collision sector will be put automatically in the given engine sector.
   * The portals in iSector will be added to this collision sector.
   */
  virtual void SetSector (iSector* sector) = 0;

  /// Get the engine iSector related to this collision sector.
  virtual iSector* GetSector () = 0;

  /// Follow a beam from start to end and return the first body that is hit.
  virtual HitBeamResult HitBeam (
      const csVector3& start, const csVector3& end) = 0;

  /**
   * Follow a beam from start to end and return the first body that is hit.
   */
  virtual HitBeamResult HitBeamPortal (
      const csVector3& start, const csVector3& end) = 0;

  /**
   * Performs a discrete collision test against all objects in this iCollisionSector.
   * it reports one or more contact points for every overlapping object
   */
  virtual bool CollisionTest (iCollisionObject* object, csArray<CollisionData>& collisions) = 0;

  /// Create a collision group.
  virtual CollisionGroup& CreateCollisionGroup (const char* name) = 0;

  /// Find a collision group by name.
  virtual CollisionGroup& FindCollisionGroup (const char* name) = 0;

  /// Set whether the two groups collide with each other.
  virtual void SetGroupCollision (const char* name1,
    const char* name2, bool collide) = 0;

  /// Get true if the two groups collide with each other.
  virtual bool GetGroupCollision (const char* name1, const char* name2) = 0;

  /**
   * Add a collision actor into the sector.
   * The collision actor has to be initialized.
   */
  virtual void AddCollisionActor (iCollisionActor* actor) = 0;

  /// Remove a collision object by pointer.
  virtual void RemoveCollisionActor () = 0;

  /// Get the collision object by index.
  virtual iCollisionActor* GetCollisionActor () = 0;
};

/**
 * This is the Collision plug-in. This plugin is a factory for creating
 * iCollider, iCollisionObject, iCollisionSector and iCollisionActor
 * entities. 
 *
 * Main creators of instances implementing this interface:
 * - Opcode plugin (crystalspace.collision.opcode2)
 * - Bullet plugin (crystalspace.physics.bullet2)
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 * 
 * \sa CS::Physics::iPhysicalSystem
 */
struct iCollisionSystem : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionSystem, 1, 0, 1);

  /**
   * Set the internal scale to be applied to the whole dynamic world. Use this
   *to put back the range of dimensions you use for your objects to the one
   * Bullet was designed for.
   * 
   * Bullet does not work well if the dimensions of your objects are smaller
   * than 0.1 to 1.0 units or bigger than 10 to 100 units. Use this method to
   * fix the problem.
   * 
   * \warning You have to call this method before adding any objects in the
   * world, otherwise the objects won't have the same scale.
   */
  virtual void SetInternalScale (float scale) = 0;

  /// Create a convex mesh collider.
  virtual csPtr<iColliderConvexMesh> CreateColliderConvexMesh (
    iMeshWrapper* mesh, bool simplify = false) = 0;

  /// Create a static concave mesh collider.
  virtual csPtr<iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh) = 0;

  /// Create a scaled concave mesh collider.
  virtual csPtr<iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled (
    iColliderConcaveMesh* collider, csVector3 scale) = 0;

  virtual csPtr<iCollisionObject> CreateCollisionObject() = 0;

  /// Create a cylinder collider.
  virtual csPtr<iColliderCylinder> CreateColliderCylinder (float length, float radius) = 0;

  /// Create a box collider.
  virtual csPtr<iColliderBox> CreateColliderBox (const csVector3& size) = 0;

  /// Create a sphere collider.
  virtual csPtr<iColliderSphere> CreateColliderSphere (float radius) = 0;

  /// Create a capsule collider.
  virtual csPtr<iColliderCapsule> CreateColliderCapsule (float length, float radius) = 0;

  /// Create a cone collider.
  virtual csPtr<iColliderCone> CreateColliderCone (float length, float radius) = 0;

  /// Create a static plane collider.
  virtual csPtr<iColliderPlane> CreateColliderPlane (const csPlane3& plane) = 0;

  /// Create a terrain collider.
  virtual csPtr<iColliderTerrain> CreateColliderTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0) = 0;

  /**
   * Create a ghost collision object
   */
  virtual csPtr<iCollisionGhostObject> CreateGhostCollisionObject () = 0;

  /**
   * Create a collision actor.
   * Need to call iCollisionObject::RebuildObject.
   */
  virtual csPtr<iCollisionActor> CreateCollisionActor (iCollider* collider) = 0;
  
  /// Create a collision sector.
  virtual csPtr<iCollisionSector> CreateCollisionSector () = 0;

  /// Find a collision sector by name.
  virtual iCollisionSector* FindCollisionSector (const char* name) = 0; 

  /// Find a collision sector by name.
  virtual iCollisionSector* GetCollisionSector (const iSector* sceneSector) = 0; 

  /**
   * Decompose a concave mesh in convex parts. Each convex part will be added to
   * the collision object as a separate iColliderConvexMesh. By this way you can
   * get a dynamic concave mesh collider.
   */
  virtual void DecomposeConcaveMesh (iCollisionObject* object, 
    iMeshWrapper* mesh, bool simplify = false) = 0;
};
}
}

#endif
