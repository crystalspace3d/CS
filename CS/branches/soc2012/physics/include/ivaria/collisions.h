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

#ifndef __CS_IVARIA_COLLISION2_H__
#define __CS_IVARIA_COLLISION2_H__

/**\file
 * Collision interfaces
 */

#include "csutil/scf.h"
#include "iutil/strset.h"
#include "ivaria/colliders.h"
#include "ivaria/collisionfactories.h"

struct iTerrainSystem;
struct iSector;
struct iMeshWrapper;
struct iMovable;
struct iSceneNode;
struct iPortal;
struct iCamera;
struct iView;
struct iTriangleMesh;

namespace CS {
namespace Physics {

struct iPhysicalBody;
struct iPhysicalSector;
struct iPhysicalSystem;

}
}

namespace CS {
namespace Collisions {

struct iActor;
struct iCollisionActor;
struct iCollisionObject;
struct iCollisionSector;
struct iCollisionSystem;

/**
 * The type of a collision object.
 */
enum CollisionObjectType
{
  COLLISION_OBJECT_SIMPLE = 0,     /*!< The collision object is a simple static collision object.
				     It can never be upcast to a iPhysicalObject.*/
  COLLISION_OBJECT_PHYSICAL,       /*!< The collision object is a physical object and can be
				     upcast to a iPhysicalObject. */
  COLLISION_OBJECT_GHOST,          /*!< The collision object is a ghost. */
  COLLISION_OBJECT_ACTOR           /*!< The collision object is an actor. */
  // TODO: a dynamic actor is both ACTOR and PHYSICAL
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
  // TODO: objectA/B redundant with the parameters of iCollisionCallback::OnCollision()?
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
// TODO: rename iCollisionListener
struct iCollisionCallback : public virtual iBase
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
 * Collision groups allow to filter the collisions occuring between the objects in
 * the system. Each iCollisionObject is associated with a collision group, and the
 * user can define whether or not the objects from one group will collide with the
 * objects of another.
 *
 * There is a maximum of 16 collision groups that can be created in total, and those
 * groups cannot (currently) be removed once created. You should therefore be careful
 * when defining and managing your set of collision groups.
 *
 * The collision system will always create one default collision group named "Default".
 * This collision group is associated by default to all collision objects without any
 * valid group.
 * 
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionGroup()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::FindCollisionGroup()
 * 
 * Main users of this interface:
 * - iCollisionSystem, iCollisionObject
 */
struct iCollisionGroup : public virtual iBase
{
  /// Get the name of this collision group
  virtual const char* GetName () const = 0;

  /**
   * Set whether or not the objects from this group will collide with the objects of
   * the group \a other. By default, all groups will collide with all others.
   *
   * Note that it is valid to use the same group as the parameter and the one being
   * called. This defines whether or not the objects of the group will collide together.
   *
   * \param other The other group that will or not collide with this one.
   * \param enabled Whether or not the collisions are enabled between the two groups.
   */
  virtual void SetCollisionEnabled (iCollisionGroup* other, bool enabled) = 0;

  /**
   * Get whether or not the objects from this group will collide with the objects of
   * the group \a other.
   */
  virtual bool GetCollisionEnabled (iCollisionGroup* other) = 0;
};

/**
 * \todo Document me
 */
struct iCollisionObjectFactory : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionObjectFactory, 1, 0, 0);

  /// Return the underlying object
  virtual iObject *QueryObject () = 0;

  /// Get the system of this factory
  // TODO: remove?
  virtual iCollisionSystem* GetSystem () const = 0;

  /// Create an instance
  virtual csPtr<iCollisionObject> CreateCollisionObject () = 0;

  /// Set the collider of this factory
  virtual void SetCollider (iCollider* value,
			    const csOrthoTransform& transform = csOrthoTransform ())  = 0;
  /// Get the collider of this factory
  virtual iCollider* GetCollider () const = 0;

  /// Set the relative transform of the collider of this object
  virtual void SetColliderTransform (const csOrthoTransform& transform) = 0;
  /// Get the relative transform of the collider of this object
  virtual const csOrthoTransform& GetColliderTransform () const = 0;

  /// Set the collision group of this factory
  virtual void SetCollisionGroup (iCollisionGroup* group) = 0;
  /// Get the collision group of this factory
  virtual iCollisionGroup* GetCollisionGroup () const = 0;
};

/**
 * This is the interface of a collision object. 
 *It contains the collision information of the object.
 * 
 * Main creators of instances implementing this interface:
 * - iCollisionObjectFactory::CreateCollisionObject()
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

  /// Return the physical body pointer if it's a physical body, or nullptr otherwise.
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () = 0;

  /// Return the actor pointer if it's an actor, or nullptr.
  virtual iActor* QueryActor () = 0;

  /**
   * Put this object into the given collision sector. The object will now be
   * part of the physical simulation.
   * \note This is equivalent to a call to
   * CS::Collisions::iCollisionSector::AddCollisionObject().
   */
  virtual void SetSector (iCollisionSector* sector) = 0;

  /**
   * Return the collision sector containing this object, or nullptr if it is not
   * in a sector (therefore not currently part of the physical simulation).
   */
  virtual iCollisionSector* GetSector () const = 0;

  /// Return the type of the collision object.
  virtual CollisionObjectType GetObjectType () const = 0;

  /// Set the iSceneNode attached to this collision object. Its transform will always coincide with the object's transform
  virtual void SetAttachedSceneNode (iSceneNode* sceneNode) = 0;
  /// Get the iSceneNode attached to this collision object. Its transform will always coincide with the object's transform
  virtual iSceneNode* GetAttachedSceneNode () const = 0;

  /**
   * Set the camera attached to this collision object. Its position will be updated
   * automatically when this object is moved.
   */
  virtual void SetAttachedCamera (iCamera* camera) = 0;

  /// Get the camera attached to this collision object.
  virtual iCamera* GetAttachedCamera () const = 0;

  /// Set the collider that defines this object's shape
  virtual void SetCollider (iCollider* collider,
			    const csOrthoTransform& transform = csOrthoTransform ()) = 0;

  /// Get the collider that defines this object's shape
  virtual iCollider* GetCollider () const = 0;

  /// Set the relative transform of the collider of this object
  virtual void SetColliderTransform (const csOrthoTransform& transform) = 0;

  /// Get the relative transform of the collider of this object
  virtual const csOrthoTransform& GetColliderTransform () const = 0;

  /// Set the transform of this object.
  virtual void SetTransform (const csOrthoTransform& trans) = 0;

  /// Get the transform of this object.
  virtual csOrthoTransform GetTransform () const = 0;
  
  /**
   * Set the current rotation in angles around every axis and set to actor.
   * If a camera is used, set it to camera too.
   */
  virtual void SetRotation (const csMatrix3& rot) = 0;

  /// Rebuild this collision object.
  virtual void RebuildObject () = 0;

  /// Set the collision group of this object
  virtual void SetCollisionGroup (iCollisionGroup* group) = 0;

  /// Get the collision group of this object
  virtual iCollisionGroup* GetCollisionGroup () const = 0;

  /**
   * Set a callback to be executed when this body collides with another.
   * If 0, no callback is executed.
   * \todo This method is not implemented and no callback will be triggered
   * unless the test method Collide() is used.
   */
  virtual void SetCollisionCallback (iCollisionCallback* cb) = 0;

  /// Get the collision response callback.
  virtual iCollisionCallback* GetCollisionCallback () = 0;

  /// Test collision with another collision objects.
  // TODO: return explicitely the collision data
  // TODO: add a collision filter parameter
  virtual bool Collide (iCollisionObject* otherObject) = 0;

  /// Follow a beam from start to end and return whether this body was hit.
  // TODO: add a collision filter parameter
  virtual HitBeamResult HitBeam (
      const csVector3& start, const csVector3& end) = 0;

  /// Get the count of collision objects contacted with this object.
  virtual size_t GetContactObjectsCount () = 0;

  /// Get the collision object contacted with this object by index.
  virtual iCollisionObject* GetContactObject (size_t index) = 0;
  
  /// Whether this object may be excluded from deactivation.
  virtual void SetDeactivable (bool d) = 0;
  /// Whether this object may be excluded from deactivation.
  virtual bool GetDeactivable () const = 0;

  /// Creates a new object that has all the properties of this one, except for transformation and movable and camera
  // TODO: remove? factories should be used instead
  //virtual csPtr<iCollisionObject> CloneObject () = 0;
  
  /**
   * Passive objects, such as portal clones, are not supposed to be tempered with 
   * and should not be acted upon by game logic.
   * \todo This should be removed, passive objects should simply not be visible from
   * outside the physics plugin.
   */
  virtual bool IsPassive () const = 0;
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
 * \todo This is redundant with the terrain collider, hence to be removed
 */
struct iCollisionTerrain : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionTerrain, 1, 0, 0);

  /// Get the terrain system.
  virtual iTerrainSystem* GetTerrain () const = 0;

  // TODO: Methods to iterate over the terrain objects etc
};

/**
 * \todo Document me + all actor classes should be merged around a common abstract interface
 * \todo Put back the API closer to the one of csColliderActor?
 */
struct iActor : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iActor, 1, 0, 0);

  // TODO: remove?
  virtual iCollisionObject* QueryCollisionObject () = 0;

  /// Take care of actor-specific stuff, before the simulation step
  // TODO: remove
  virtual void UpdatePreStep (float delta) = 0;
  
  /// Take care of actor-specific stuff, after the simulation step
  virtual void UpdatePostStep (float delta) = 0;

  /**
   * Start walking in the given direction with walk speed. 
   * Sets linear velocity. 
   * Takes air control into consideration.
   * Adds the current vertical velocity to the given vertical velocity.
   */
  virtual void Walk (csVector3 dir) = 0;
  
  /**
   * Start walking in the given horizontal direction with walk speed. 
   * Sets linear velocity. 
   * Takes air control into consideration.
   * Does not influence vertical movement.
   */
  virtual void WalkHorizontal (csVector2 dir) = 0;

  /// Applies an upward impulse to this actor, and an inverse impulse to objects beneath
  virtual void Jump () = 0;

  /// Stops any player-controlled movement
  virtual void StopMoving () = 0;
  
  /// Whether the actor is not on ground and gravity applies
  virtual bool IsFreeFalling () const = 0;

  /// Whether this actor touches the ground
  virtual bool IsOnGround () const = 0;

  /// Get the max vertical threshold that this actor can step over
  virtual float GetStepHeight () const = 0;
  /// Set the max vertical threshold that this actor can step over
  virtual void SetStepHeight (float h) = 0;

  /// Get the walk speed
  virtual float GetWalkSpeed () const = 0;
  /// Set the walk speed
  virtual void SetWalkSpeed (float s) = 0;
  
  /// Get the jump speed
  virtual float GetJumpSpeed () const = 0;
  /// Set the jump speed
  virtual void SetJumpSpeed (float s) = 0;
  
  /// Determines how much the actor can control movement when free falling
  virtual float GetAirControlFactor () const = 0;
  /// Determines how much the actor can control movement when free falling
  virtual void SetAirControlFactor (float f) = 0;

  /// Whether this object is subject to the constant gravitational forces of its sector
  // TODO: working?
  virtual bool GetGravityEnabled () const = 0;
  /// Whether this object is subject to the constant gravitational forces of its sector
  virtual void SetGravityEnabled (bool g) = 0;
};

/**
 * \todo Document me
 */
struct iCollisionActorFactory : public virtual iCollisionObjectFactory
{
  SCF_INTERFACE (CS::Collisions::iCollisionActorFactory, 1, 0, 0);

  /// Create an instance
  virtual csPtr<iCollisionActor> CreateCollisionActor () = 0;

  /// Get the max vertical threshold that this actor can step over
  virtual float GetStepHeight () const = 0;
  /// Set the max vertical threshold that this actor can step over
  virtual void SetStepHeight (float h) = 0;

  /// Get the walk speed
  virtual float GetWalkSpeed () const = 0;
  /// Set the walk speed
  virtual void SetWalkSpeed (float s) = 0;

  /// Get the jump speed
  virtual float GetJumpSpeed () const = 0;
  /// Set the jump speed
  virtual void SetJumpSpeed (float s)  = 0;

  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  virtual float GetAirControlFactor () const = 0;
  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  virtual void SetAirControlFactor (float f) = 0;
};

/**
 * A iCollisionActor is a kinematic collision object. It has a faster collision detection
 * and response. You can use it to create a player or character model with gravity handling.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionActorFactory::CreateCollisionObject
 * 
 * Main users of this interface:
 * - iCollisionSystem
 * \remark The collider of iCollisionActor must be a convex shape. For example, box, convex
 * mesh.
 * \todo All actor classes should be merged around a common abstract interface
 */
struct iCollisionActor : public virtual iCollisionObject, public virtual iActor
{
  SCF_INTERFACE (CS::Collisions::iCollisionActor, 1, 0, 0);

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
 * \sa CS::Physics::iPhysicalSector CS::Physics::iPhysicalSector
 */
struct iCollisionSector : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionSector, 1, 0, 0);

  /// Return the system that this sector belongs to
  virtual CS::Collisions::iCollisionSystem* GetSystem () const = 0;

  /// Return the underlying object
  virtual iObject *QueryObject (void) const = 0;

  /**
   * Return the type of this sector, that is either CS::Collisions::COLLISION_OBJECT_PHYSICAL
   * For sectors that can be upcast to a iPhysicalSector, or CS::Collisions::COLLISION_OBJECT_SIMPLE
   * for sectors that cannot be upcast to a iPhysicalSector.
   */
  virtual CollisionObjectType GetSectorType () const = 0;

  /**
   * Return a reference to the physical interface of this sector if its type is
   * CS::Collisions::COLLISION_OBJECT_PHYSICAL, or nullptr otherwise.
   */
  virtual CS::Physics::iPhysicalSector* QueryPhysicalSector () const = 0;

  /// Set the global gravity.
  virtual void SetGravity (const csVector3& v) = 0;

  /// Get the global gravity.
  virtual csVector3 GetGravity () const = 0;

  /**
   * Add a collision object into the sector.
   * TODO: remove: The collision object has to be initialized.
   * TODO: rename all AddXXX() methods in Add()
   */
  virtual void AddCollisionObject (iCollisionObject* object) = 0;

  /// Remove the given collision object from this sector
  virtual void RemoveCollisionObject (iCollisionObject* object) = 0;

  /// Get the count of collision objects.
  virtual size_t GetCollisionObjectCount () = 0;

  /// Get the collision object by index.
  virtual iCollisionObject* GetCollisionObject (size_t index) = 0;

  //  Terrain

  /// Adds the given terrain to this sector
  virtual void AddCollisionTerrain (iCollisionTerrain* terrain) = 0;

  /// Remove the given collision terrain from this sector
  virtual void RemoveCollisionTerrain (iCollisionTerrain* terrain) = 0;

  /// Total amount if iCollisionTerrain objects in this sector
  virtual size_t GetCollisionTerrainCount () const = 0;

  /// Get the index'th iCollisionTerrain object
  virtual iCollisionTerrain* GetCollisionTerrain (size_t index) const = 0;

  /// Retreive the CollisionTerrain that wraps the given TerrainSystem
  virtual iCollisionTerrain* GetCollisionTerrain (iTerrainSystem* terrain) = 0;

  // Portals

  /// Add a portal into the sector. Collision objects crossing a portal will be switched from iCollisionSector's.
  virtual void AddPortal (iPortal* portal, const csOrthoTransform& meshTrans) = 0;

  /// Remove the given portal from this sector.
  virtual void RemovePortal (iPortal* portal) = 0;

  // Other stuff

  /**
   * Set the engine iSector related to this collision sector. The iMovable that are 
   * attached to a iCollisionObject present in this collision sector will be put
   * automatically in the given engine sector. The portals in iSector will be added
   * to this collision sector.
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

  /// Delete all objects in this collision sector.
  // TODO: mask for selecting the type/state/collgroup of the objects to be removed?
  // TODO: flag indicating whether the attached iSceneNode should be removed from the engine?
  virtual void DeleteAll () = 0;
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
   * Return the physical system pointer if this interface is also implemented by
   * the system, or nullptr otherwise.
   */
  virtual CS::Physics::iPhysicalSystem* QueryPhysicalSystem () = 0;

  /**
   * Set the speed of the simulation, that is the time ratio that is applied
   * when updating the motion of the objects. The default value is 1.0f.
   */
  virtual void SetSimulationSpeed (float speed) = 0;

  /**
   * Get the speed of the simulation, that is the time ratio that is applied
   * when updating the motion of the objects.
   */
  virtual float GetSimulationSpeed () const = 0;

  /**
   * Create an empty collider (it does not have a root shape, but only potentially
   * children). The collider type of this object is CS::Collisions::COLLIDER_COMPOUND.
   */
  virtual csPtr<iCollider> CreateCollider () = 0;

  /// Create a convex mesh collider.
  // TODO: what is simplify?
  virtual csPtr<iColliderConvexMesh> CreateColliderConvexMesh (iTriangleMesh* triMesh, bool simplify = false) = 0;

  /// Create a static concave mesh collider.
  virtual csPtr<iColliderConcaveMesh> CreateColliderConcaveMesh (iTriangleMesh* mesh) = 0;

  /// Create a static, scaled concave mesh collider.
  virtual csPtr<iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled (
    iColliderConcaveMesh* collider, const csVector3& scale) = 0;

  /// Create a cylinder collider, oriented along the y-axis
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

  /// Creates a new collision sector and adds it to the system's set
  virtual iCollisionSector* CreateCollisionSector (iSector* sector = nullptr) = 0;
  
  /// Remove the given collision sector
  virtual void RemoveCollisionSector (iCollisionSector* sector) = 0;
  
  /// Return the amount of sectors in this system
  virtual size_t GetCollisionSectorCount () const = 0;

  /// Get a collision sector by index
  virtual iCollisionSector* GetCollisionSector (size_t index) = 0; 
  
  /// Find a collision sector by its associated iSector, or nullptr if it has not been found
  virtual iCollisionSector* FindCollisionSector (const iSector* sceneSector) = 0;

  /**
   * Create a collision group of the given name. Return nullptr if the group could
   * not be created. If a group with the given name already exists, then return a
   * reference to this group.
   * \warning You cannot create more than 16 collision groups in total.
   * \warning Collision groups cannot be removed once created.
   */
  virtual iCollisionGroup* CreateCollisionGroup (const char* name) = 0;

  /// Find the collision group of the given name, or return nullptr if it has not been found.
  virtual iCollisionGroup* FindCollisionGroup (const char* name) const = 0;

  /// Get the count of collision groups in this system
  virtual size_t GetCollisionGroupCount () const = 0;

  /// Get a collision group by its index
  virtual iCollisionGroup* GetCollisionGroup (size_t index) const = 0;

  // Factory

  /// Create a iCollisionObjectFactory
  virtual csPtr<iCollisionObjectFactory> CreateCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a iCollisionObjectFactory of type CS::Collisions::COLLISION_OBJECT_GHOST
  virtual csPtr<iCollisionObjectFactory> CreateGhostCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a iCollisionActorFactory
  virtual csPtr<iCollisionActorFactory> CreateCollisionActorFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Reset the entire system and delete all sectors
  virtual void DeleteAll () = 0;
};

} }

#endif
