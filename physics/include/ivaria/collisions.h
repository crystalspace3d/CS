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
#include "colliders.h"
#include "collisionproperties.h"

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
struct iCollisionSector;
struct iCollisionSystem;

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

  /// Returns the sector to which is this object has been added or nullptr, if not in world
  virtual iCollisionSector* GetSector () const = 0;

  /// Whether this object is currently added to the world
  virtual bool IsInWorld () const = 0;

  /// Whether this is a rigid or soft body object
  virtual bool IsPhysicalObject () const = 0;

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

  /// Get the collider that defines this object's shape
  virtual iCollider* GetCollider () const = 0;

  /// Set the collider that defines this object's shape
  virtual void SetCollider (iCollider* collider) = 0;
  
  /// Set the transform.
  virtual void SetTransform (const csOrthoTransform& trans) = 0;

  /// Get the transform.
  virtual csOrthoTransform GetTransform () const = 0;

  /**
   * Set current rotation in angles around every axis and set to actor.
   * If a camera is used, set it to camera too.
   */
  virtual void SetRotation (const csMatrix3& rot) = 0;

  /// Rotate the collision actor by the given angle about the given axis
  virtual void Rotate (const csVector3& v, float angle) = 0;

  /// Increases pitch angle by the given value in radians
  virtual void IncreasePitch(float yawDelta) = 0;

  /// Increases yaw angle by the given value in radians
  virtual void IncreaseYaw(float yawDelta) = 0;

  /// Rebuild this collision object.
  virtual void RebuildObject () = 0;

  /// Set the collision group this object belongs to by name
  virtual void SetCollisionGroup (const char* name) = 0;
  
  /// Set the collision group of this object
  virtual void SetCollisionGroup (const CollisionGroup& group) = 0;

  /// Get the collision group of this object
  virtual const CollisionGroup& GetCollisionGroup () const = 0;

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
struct iGhostCollisionObject : public virtual iCollisionObject
{

};


/**
 * A collision terrain consists of multiple cells.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionTerrain()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - 
 */
struct iCollisionTerrain : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionTerrain, 1, 0, 0);

  /// Get the terrain system.
  virtual iTerrainSystem* GetTerrain () const = 0;

  // TODO: Methods to iterate over the terrain objects etc
};

/**
 * A iCollisionActor is a kinematic collision object. It has a faster collision detection and response.
 * You can use it to create a player or character model with gravity handling.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionActor()
 * 
 * Main users of this interface:
 * - iCollisionSystem
 * \remark The collider of iCollisionActor must be a convex shape. For example, box, convex mesh.
 */
// kickvb: most of this would have to be redesigned, let's do it later
struct iCollisionActor : public virtual iGhostCollisionObject
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

  /// Return the system that this sector belongs to
  virtual CS::Collisions::iCollisionSystem* GetSystem() = 0;

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

  /// Adds the given terrain to this sector
  virtual void AddCollisionTerrain(iCollisionTerrain* terrain) = 0;

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
  SCF_INTERFACE (CS::Collisions::iCollisionSystem, 2, 0, 0);

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

  /// Creates an empty compound collider (does not have a root shape, but only children)
  virtual csPtr<iColliderCompound> CreateColliderCompound () = 0;

  /// Create a convex mesh collider.
  virtual csPtr<iColliderConvexMesh> CreateColliderConvexMesh (
    iMeshWrapper* mesh, bool simplify = false) = 0;

  /// Create a static concave mesh collider.
  virtual csPtr<iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh) = 0;

  /// Create a scaled concave mesh collider.
  virtual csPtr<iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled (
    iColliderConcaveMesh* collider, csVector3 scale) = 0;

  /// Create a CollisionObject
  virtual csPtr<iCollisionObject> CreateCollisionObject(CollisionObjectProperties* props) = 0;

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
  virtual csPtr<iCollisionTerrain> CreateCollisionTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0) = 0;

  /**
   * Create a ghost collision object
   */
  virtual csPtr<iGhostCollisionObject> CreateGhostCollisionObject (GhostCollisionObjectProperties* props) = 0;

  /**
   * Create a collision actor.
   * Need to call iCollisionObject::RebuildObject.
   */
  virtual csPtr<iCollisionActor> CreateCollisionActor (CollisionActorProperties* props) = 0;
  
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
  virtual void DecomposeConcaveMesh (iCollider* object, 
    iMeshWrapper* mesh, bool simplify = false) = 0;

  /// Create a collision group.
  virtual CollisionGroup& CreateCollisionGroup (const char* name) = 0;

  /// Find a collision group by name.
  virtual CollisionGroup& FindCollisionGroup (const char* name) = 0;

  /// Set whether the two groups collide with each other.
  virtual void SetGroupCollision (const char* name1,
    const char* name2, bool collide) = 0;

  /// Get true if the two groups collide with each other.
  virtual bool GetGroupCollision (const char* name1, const char* name2) = 0;
};

} }

#endif
