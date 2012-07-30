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

#ifndef __IVARIA_PHYSIH__
#define __IVARIA_PHYSIH__

/**\file
 * Physics interfaces
 */

#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "iutil/objreg.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "imesh/genmesh.h"
#include "csgeom/tri.h"
#include "cstool/primitives.h"
#include "ivaria/collisions.h"

#include "ivaria/physicalfactories.h"

namespace CS 
{
namespace Mesh 
{
struct iAnimatedMesh;
} 
}

namespace CS
{
namespace Collisions
{
struct iCollisionCallback;
struct iCollisionObject;
struct CollisionGroup;
struct iCollisionObject;
}
}

namespace CS 
{ 
namespace Physics 
{
struct iJoint;
struct iObject;
struct iRigidBody;
struct iSoftBody;
struct iDynamicActor;
struct iKinematicCallback;
struct iPhysicalSystem;
struct iPhysicalSector;

struct iVehicle;
struct iVehicleFactory;
struct iVehicleWheelFactory;

/**
 * The type of a rigid body state.
 */
enum RigidBodyState
{
  STATE_STATIC = 0,
  STATE_DYNAMIC,
  STATE_KINEMATIC
};

/**
 * The type of debug mode.
 */
enum DebugMode
{
  DEBUG_NOTHING = 0,     /*!< Nothing will be displayed. */
  DEBUG_COLLIDERS = 1,   /*!< Display the colliders of the bodies. */
  DEBUG_AABB = 2,        /*!< Display the axis aligned bounding boxes of the bodies. */
  DEBUG_JOINTS = 4       /*!< Display the joint positions and limits. */
};


/**
 * A base interface of physical bodies. 
 * iRigidBody and iSoftBody will be derived from this one.
 */
struct iPhysicalBody : public virtual CS::Collisions::iCollisionObject
{
  SCF_INTERFACE (CS::Physics::iPhysicalBody, 1, 0, 1);

  /// Whether this is a rigid or soft body object
  virtual bool IsPhysicalObject() const { return true; }

  /// Get the type of this physical body.
  virtual PhysicalObjectType GetPhysicalObjectType () const = 0;

  /**
   * Query the iRigidBody interface of this body. It returns NULL if the
   * interface is not valid, ie GetType() is not CS::Physics::PHYSICAL_OBJECT_RIGIDBODY.
   */
  virtual iRigidBody* QueryRigidBody () = 0;

  /**
   * Query the iSoftBody interface of this body. It returns NULL if the
   * interface is not valid, ie GetType() is not CS::Physics::PHYSICAL_OBJECT_SOFTYBODY.
   */
  virtual iSoftBody* QuerySoftBody () = 0;

  /// Disable this collision object.
  virtual bool Disable () = 0;

  /// Enable this collision object.
  virtual bool Enable () = 0;

  /// Check if the collision object is enabled.
  virtual bool IsEnabled () = 0;

  /// Set the total mass of this body.
  virtual void SetMass (float mass) = 0;

  /// Get the mass of this body.
  virtual float GetMass () const = 0;

  /// Get the density of the body.
  virtual float GetDensity () const = 0;

  /**
   * Set the density of this collider. If the mass of the body was not defined
   * then it will be computed from this. But iSoftBody must use SetMass instead. 
   * 
   * You should be really careful when using densities because most of the
   * game physics libraries do not work well when objects with large mass
   * differences interact. It is safer to artificially keep the mass of moving
   * objects in a safe range (from 1 to 100 kilogram for example).
   */
  virtual void SetDensity (float density) = 0;

  /// Return the volume of this body.
  virtual float GetVolume () const = 0;
  
  /// Add a force to the whole body.
  virtual void AddForce (const csVector3& force) = 0;
  
  /// Get the linear velocity (translational velocity component).
  virtual csVector3 GetLinearVelocity (size_t index = 0) const = 0;
  /// Set the linear velocity (translational velocity component).
  virtual void SetLinearVelocity (const csVector3& vel) = 0;

  /**
   * Get whether this object is dynamic.
   * Dynamic objects are moved by dynamics simulation.
   */
  virtual bool IsDynamic() const = 0;

  /**
   * Set the friction of this body.
   * [0,1] for soft body.
   */
  virtual void SetFriction (float friction) = 0;

  /// Get the friction of this body.
  virtual float GetFriction () const = 0;
};

/**
 * This is the interface for a rigid body.
 * It keeps all properties for the body.
 * It can also be attached to a movable or a bone,
 * to automatically update it.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateRigidBody()
 * 
 *  Main ways to get pointers to this interface:
 * - iPhysicalSector::GetRigidBody()
 * - iPhysicalSector::FindRigidBody()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 *
 * \sa CS::Physics::iSoftBody CS::Physics::iSoftBody
 */
struct iRigidBody : public virtual iPhysicalBody
{
  SCF_INTERFACE (CS::Physics::iRigidBody, 1, 0, 0);

  /// Get the current state of the body.
  virtual RigidBodyState GetState () const = 0;
  
  /// Set the current state of the body.
  virtual bool SetState (RigidBodyState state) = 0;
  
  /// Get the elasticity of this rigid body.
  virtual float GetElasticity () = 0;
  /// Set the elasticity of this rigid body.
  virtual void SetElasticity (float elasticity) = 0;
  
  /// Get the angular velocity (rotation)
  virtual csVector3 GetAngularVelocity () const = 0;
  /// Set the angular velocity (rotation).
  virtual void SetAngularVelocity (const csVector3& vel) = 0;

  /// Add a torque (world space) (active for one timestep).
  virtual void AddTorque (const csVector3& torque) = 0;

  /// Add a force (local space) (active for one timestep).
  virtual void AddRelForce (const csVector3& force) = 0;

  /// Add a torque (local space) (active for one timestep).
  virtual void AddRelTorque (const csVector3& torque) = 0;

  /**
   * Add a force (world space) at a specific position (world space)
   * (active for one timestep)
   */
  virtual void AddForceAtPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (world space) at a specific position (local space)
   * (active for one timestep)
   */
  virtual void AddForceAtRelPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (local space) at a specific position (world space)
   * (active for one timestep)
   */
  virtual void AddRelForceAtPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (local space) at a specific position (local space)
   * (active for one timestep)
   */
  virtual void AddRelForceAtRelPos (const csVector3& force,
      const csVector3& pos) = 0;

  /// Get total force (world space).
  virtual csVector3 GetForce () const = 0;

  /// Get total torque (world space).
  virtual csVector3 GetTorque () const = 0;

  /**
   * Set the callback to be used to update the transform of the kinematic body.
   * If no callback are provided then the dynamic system will use a default one.
   */
  virtual void SetKinematicCallback (iKinematicCallback* cb) = 0;

  /// Get the callback used to update the transform of the kinematic body.
  virtual iKinematicCallback* GetKinematicCallback () = 0;

  /**
   * Set the linear Damping for this rigid body. The dampening correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   * \sa iDynamicSystem::SetLinearDamping()
   */
  virtual void SetLinearDamping (float d) = 0;

  /// Get the linear Damping for this rigid body.
  virtual float GetLinearDamping () = 0;

  /**
   * Set the angular Damping for this rigid body. The dampening correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   */
  virtual void SetAngularDamping (float d) = 0;

  /// Get the angular Damping for this rigid body.
  virtual float GetAngularDamping () = 0;
  
  virtual csVector3 GetAngularFactor() const = 0;
  virtual void SetAngularFactor(const csVector3& f) = 0;
};

/**
 * This class can be implemented in order to update the position of an anchor of a
 * CS::Physics::iSoftBody. This can be used to try to control manually the
 * position of a vertex of a soft body.
 *
 * \warning This feature uses a hack around the physical simulation of soft bodies
 * and may not always be stable. Use it at your own risk.
 * \sa CS::Physics::iSoftBody::AnchorVertex(size_t,iAnchorAnimationControl)
 */
struct iAnchorAnimationControl : public virtual iBase
{
  SCF_INTERFACE(CS::Physics::iAnchorAnimationControl, 1, 0, 0);

  /**
   * Return the new position of the anchor, in world coordinates.
   */
  virtual csVector3 GetAnchorPosition () const = 0;
};

/**
 * A soft body is a physical body that can be deformed by the physical
 * simulation. It can be used to simulate eg ropes, clothes or any soft
 * volumetric object.
 *
 * A soft body does not have a positional transform by itself, but the
 * position of every vertex of the body can be queried through GetVertexPosition().
 *
 * A soft body can neither be static or kinematic, it is always dynamic.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionObject()
 * 
 * Main ways to get pointers to this interface: 
 * - iPhysicalSector::GetSoftBody()
 * - iPhysicalSector::FindSoftBody()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 *
 * \sa CS::Physics::iRigidBody CS::Physics::iSoftBody
 */
struct iSoftBody : public virtual iPhysicalBody
{
  SCF_INTERFACE (CS::Physics::iSoftBody, 1, 0, 0);

  /// Set the mass of a node by index.
  virtual void SetVertexMass (float mass, size_t index) = 0;

  /// Get the mass of a node by index.
  virtual float GetVertexMass (size_t index) = 0;

  /// Return the count of vertices of this soft body.
  virtual size_t GetVertexCount () = 0;

  /// Return the position in world coordinates of the given vertex.
  virtual csVector3 GetVertexPosition (size_t index) const = 0;

  /// Anchor the given vertex to its current position. This vertex will no more move.
  virtual void AnchorVertex (size_t vertexIndex) = 0;

  /**
   * Anchor the given vertex to the given rigid body. The relative position of the
   * vertex and the body will remain constant.
   */
  virtual void AnchorVertex (size_t vertexIndex,
      iRigidBody* body) = 0;

  /**
   * Anchor the given vertex to the given controller. The relative position of the
   * vertex and the controller will remain constant.
   */
  virtual void AnchorVertex (size_t vertexIndex,
      iAnchorAnimationControl* controller) = 0;

  /**
   * Update the position of the anchor of the given vertex relatively to the anchored
   * rigid body. This can be used to have a finer control of the anchor position
   * relatively to the rigid body.
   *
   * This would work only if you called AnchorVertex(size_t,iRigidBody*) before.
   * The position to be provided is in world coordinates.
   *
   * \warning The stability of the simulation can be lost if you move the position too far
   * from the previous position.
   * \sa CS::Animation::iSoftBodyAnimationControl::CreateAnimatedMeshAnchor()
   */
  virtual void UpdateAnchor (size_t vertexIndex,
      csVector3& position) = 0;

  /**
   * Remove the given anchor. This won't work if you anchored the vertex to a rigid body, due
   * to a limitation in the Bullet library.
   */
  virtual void RemoveAnchor (size_t vertexIndex) = 0;

  /**
   * Set the rigidity of this body. The value should be in the 0 to 1 range, with
   * 0 meaning soft and 1 meaning rigid.
   */
  virtual void SetRigidity (float rigidity) = 0;

  /// Get the rigidity of this body.
  virtual float GetRigidity () = 0;

  /// Set the linear velocity of the given vertex of the body.
  virtual void SetLinearVelocity (const csVector3& velocity,
      size_t vertexIndex) = 0;

  /**
   * Set the wind velocity of the whole body.
   */
  virtual void SetWindVelocity (const csVector3& velocity) = 0;

  /// Get the wind velocity of the whole body.
virtual const csVector3 GetWindVelocity () const = 0;

  /// Add a force at the given vertex of the body.
  virtual void AddForce (const csVector3& force, size_t vertexIndex) = 0;

  /// Return the count of triangles of this soft body.
  virtual size_t GetTriangleCount () = 0;

  /// Return the triangle with the given index.
  virtual csTriangle GetTriangle (size_t index) const = 0;

  /// Return the normal vector in world coordinates for the given vertex.
  virtual csVector3 GetVertexNormal (size_t index) const = 0;
  

  /**
   * Draw the debug informations of this soft body. This has to be called
   * at each frame, and will add 2D lines on top of the rendered scene.
   */
  virtual void DebugDraw (iView* rView) = 0;

  /// Set linear stiffness coefficient [0,1].
  virtual void SetLinearStiff (float stiff) = 0;

  /// Set area/angular stiffness coefficient [0,1].
  virtual void SetAngularStiff (float stiff) = 0;

  /// Set volume stiffness coefficient [0,1].
  virtual void SetVolumeStiff (float stiff) = 0;

  /// Reset the collision flag to 0.
  virtual void ResetCollisionFlag () = 0;

  /// Set true if use cluster vs convex handling for rigid vs soft collision detection.
  virtual void SetClusterCollisionRS (bool cluster) = 0;

  /// Get true if use cluster vs convex handling for rigid vs soft collision detection.
  virtual bool GetClusterCollisionRS () = 0;

  /// Set true if use cluster vs cluster handling for soft vs soft collision detection.
  virtual void SetClusterCollisionSS (bool cluster) = 0;

  /// Get true if use cluster vs cluster handling for soft vs soft collision detection.
  virtual bool GetClusterCollisionSS () = 0;

  /// Set soft vs rigid hardness [0,1] (cluster only).
  virtual void SetSRHardness (float hardness) = 0;

  /// Set soft vs kinetic hardness [0,1] (cluster only).
  virtual void SetSKHardness (float hardness) = 0;

  /// Set soft vs soft hardness [0,1] (cluster only).
  virtual void SetSSHardness (float hardness) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only).
  virtual void SetSRImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only).
  virtual void SetSKImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only).
  virtual void SetSSImpulse (float impulse) = 0;

  /// Set velocities correction factor (Baumgarte).
  virtual void SetVeloCorrectionFactor (float factor) = 0;

  /// Set damping coefficient [0,1].
  virtual void SetDamping (float damping) = 0;

  /// Set drag coefficient [0,+inf].
  virtual void SetDrag (float drag) = 0;

  /// Set lift coefficient [0,+inf].
  virtual void SetLift (float lift) = 0;

  /// Set pressure coefficient [-inf,+inf].
  virtual void SetPressure (float pressure) = 0;

  /// Set volume conversation coefficient [0,+inf].
  virtual void SetVolumeConversationCoefficient (float conversation) = 0;

  /// Set pose matching coefficient [0,1].	
  virtual void SetShapeMatchThreshold (float matching) = 0;

  /// Set rigid contacts hardness [0,1].
  virtual void SetRContactsHardness (float hardness) = 0;

  /// Set kinetic contacts hardness [0,1].
  virtual void SetKContactsHardness (float hardness) = 0;

  /// Set soft contacts hardness [0,1].
  virtual void SetSContactsHardness (float hardness) = 0;

  /// Set anchors hardness [0,1].
  virtual void SetAnchorsHardness (float hardness) = 0;

  /// Set velocities solver iterations.
  virtual void SetVeloSolverIterations (int iter) = 0;

  /// Set positions solver iterations.
  virtual void SetPositionIterations (int iter) = 0;

  /// Set drift solver iterations.
  virtual void SetDriftIterations (int iter) = 0;

  /// Set cluster solver iterations.
  virtual void SetClusterIterations (int iter) = 0;

  /// Set true if use pose matching.
  virtual void SetShapeMatching (bool match) = 0;

  /// Set true if use bending constraint.
  virtual void SetBendingConstraint (bool bending) = 0;

  /// Generate cluster for the soft body.
  virtual void GenerateCluster (int iter) = 0;
};

/**
 * General helper class for CS::Physics::Bullet::iSoftBody.
 */
struct SoftBodyHelper
{
  /**
   * Create a genmesh from the given cloth soft body.
   * The genmesh will be double-sided, in order to have correct normals on both
   * sides of the cloth (ie the vertices of the soft body will be duplicated for the
   * genmesh).
   * \warning Don't forget to use doubleSided = true in
   * CS::Animation::iSoftBodyAnimationControl::SetSoftBody()
   */
  static csPtr<iMeshFactoryWrapper> CreateClothGenMeshFactory
  (iObjectRegistry* object_reg, const char* factoryName, iSoftBody* cloth)
  {
    csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);

    // Create the cloth mesh factory.
    csRef<iMeshFactoryWrapper> clothFact = engine->CreateMeshFactory
      ("crystalspace.mesh.object.genmesh", factoryName);
    if (!clothFact)
      return 0;

    csRef<iGeneralFactoryState> gmstate = scfQueryInterface<iGeneralFactoryState>
      (clothFact->GetMeshObjectFactory ());

    // Create the vertices of the genmesh
    size_t vertexCount = cloth->GetVertexCount ();
    gmstate->SetVertexCount (int (vertexCount * 2));
    csVector3* vertices = gmstate->GetVertices ();
    for (size_t i = 0; i < vertexCount; i++)
    {
      vertices[i] = cloth->GetVertexPosition (i);
      vertices[i + vertexCount] = cloth->GetVertexPosition (i);
    }

    // Create the triangles of the genmesh
    gmstate->SetTriangleCount (int (cloth->GetTriangleCount ()) * 2);
    csTriangle* triangles = gmstate->GetTriangles ();
    for (size_t i = 0; i < cloth->GetTriangleCount (); i++)
    {
      csTriangle triangle = cloth->GetTriangle (i);
      triangles[i * 2] = triangle;
      triangles[i * 2 + 1] = csTriangle (int (triangle[2] + vertexCount),
					 int (triangle[1] + vertexCount),
					 int (triangle[0] + vertexCount));
    }

    gmstate->CalculateNormals ();

    // Set up the texels of the genmesh
    csVector2* texels = gmstate->GetTexels ();
    csVector3* normals = gmstate->GetNormals ();
    CS::Geometry::TextureMapper* mapper = new CS::Geometry::DensityTextureMapper (1.0f);
    for (size_t i = 0; i < vertexCount * 2; i++)
      texels[i] = mapper->Map (vertices[i], normals[i], i);

    gmstate->Invalidate ();

    return csPtr<iMeshFactoryWrapper> (clothFact);
  }
};

/**
 * A joint that can constrain the relative motion between two iPhysicalBody.
 * For instance if all motion in along the local X axis is constrained
 * then the bodies will stay motionless relative to each other
 * along an x axis rotated and positioned by the joint's transform.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateJoint()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 */
struct iJoint : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iJoint, 1, 0, 0);

  /**
   * Set the rigid bodies that will be affected by this joint. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void Attach (iPhysicalBody* body1, iPhysicalBody* body2,
      bool forceUpdate = true) = 0;

  /// Get the attached body with the given index (valid values for body are 0 and 1).
  virtual iPhysicalBody* GetAttachedBody (int index) = 0;

  /**
   * Set the world transformation of the joint.
   * Set force_update to true if you want to apply the changes right away.
   */
  virtual void SetTransform (const csOrthoTransform& trans,
      bool forceUpdate = false) = 0;

  /// Get the world transformation of the joint.
  virtual csOrthoTransform GetTransform () const = 0;

  /// Set the new position of the joint, in world coordinates.
  virtual void SetPosition (const csVector3& position,
    bool forceUpdate = false) = 0;

  /// Get the current position of the joint, in world coordinates.
  virtual csVector3 GetPosition () const = 0;

  /**
   * Set the translation constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all motion along
   * that axis (ie no motion will be allowed). If false is passed in then all motion along that
   * axis is free, but bounded by the minimum and maximum distance
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetTransConstraints (bool X, 
      bool Y, bool Z, 
      bool forceUpdate = false) = 0;

  /// True if this axis' translation is constrained.
  virtual bool IsXTransConstrained () = 0;

  /// True if this axis' translation is constrained.
  virtual bool IsYTransConstrained () = 0;

  /// True if this axis' translation is constrained.
  virtual bool IsZTransConstrained () = 0;

  /**
   * Set the minimum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumDistance (const csVector3& dist,
      bool forceUpdate = false) = 0;

  /// Get the minimum allowed distance between the two bodies.
  virtual csVector3 GetMinimumDistance () const = 0;

  /**
   * Set the maximum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumDistance (const csVector3& dist,
      bool forceUpdate = false) = 0;

  /// Get the maximum allowed distance between the two bodies.
  virtual csVector3 GetMaximumDistance () const = 0;

  /**
   * Set the rotational constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all rotation around
   * that axis (ie no motion will be allowed). If false is passed in then all rotation around that
   * axis is free, but bounded by the minimum and maximum angle
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetRotConstraints (bool X, 
      bool Y, bool Z, 
      bool forceUpdate = false) = 0;

  /// True if this axis' rotation is constrained.
  virtual bool IsXRotConstrained () = 0;

  /// True if this axis' rotation is constrained.
  virtual bool IsYRotConstrained () = 0;

  /// True if this axis' rotation is constrained.
  virtual bool IsZRotConstrained () = 0;

  /**
   * Set the minimum allowed angle between the two bodies, in radian. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumAngle (const csVector3& angle,
      bool forceUpdate = false) = 0;

  /// Get the minimum allowed angle between the two bodies (in radian).
  virtual csVector3 GetMinimumAngle () const = 0;

  /**
   * Set the maximum allowed angle between the two bodies (in radian). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumAngle (const csVector3& angle,
      bool forceUpdate = false) = 0;

  /// Get the maximum allowed angle between the two bodies (in radian).
  virtual csVector3 GetMaximumAngle () const = 0;

  /** 
   * Set the restitution of the joint's stop point (this is the 
   * elasticity of the joint when say throwing open a door how 
   * much it will bounce the door back closed when it hits).
   */
  virtual void SetBounce (const csVector3& bounce,
      bool forceUpdate = false) = 0;

  /// Get the joint restitution.
  virtual csVector3 GetBounce () const = 0;

  /**
   * Apply a motor velocity to joint (for instance on wheels). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetDesiredVelocity (const csVector3& velo,
      bool forceUpdate = false) = 0;

  /// Get the desired velocity of the joint motor.
  virtual csVector3 GetDesiredVelocity () const = 0;

  /**
   * Set the maximum force that can be applied by the joint motor to reach the desired velocity.
   * Set force_update to true if  you want to apply the changes right away.
   */
  virtual void SetMaxForce (const csVector3& force,
      bool forceUpdate = false) = 0;

  /// Get the maximum force that can be applied by the joint motor to reach the desired velocity.
  virtual csVector3 GetMaxForce () const = 0;

  /**
   * Rebuild the joint using the current setup. Return true if the rebuilding operation was successful
   * (otherwise the joint won't be active).
   */
  virtual bool RebuildJoint () = 0;

  /// Set this joint to a spring joint.
  virtual void SetSpring(bool isSpring, bool forceUpdate = false) = 0;

  /// Set the linear stiffness of the spring.
  virtual void SetLinearStiffness (csVector3 stiff, bool forceUpdate = false) = 0;

  /// Get the linear stiffness of the spring.
  virtual csVector3 GetLinearStiffness () const = 0;

  /// Set the angular stiffness of the spring.
  virtual void SetAngularStiffness (csVector3 stiff, bool forceUpdate = false) = 0;

  /// Get the angular stiffness of the spring.
  virtual csVector3 GetAngularStiffness () const = 0;

  /// Set the linear damping of the spring.
  virtual void SetLinearDamping (csVector3 damp, bool forceUpdate = false) = 0;

  /// Get the linear damping of the spring.
  virtual csVector3 GetLinearDamping () const = 0;

  /// Set the angular damping of the spring.
  virtual void SetAngularDamping (csVector3 damp, bool forceUpdate = false) = 0;

  /// Get the angular damping of the spring.
  virtual csVector3 GetAngularDamping () const = 0;
  
  /// Set the value to an equilibrium point for translation.
  virtual void SetLinearEquilibriumPoint (csVector3 point, bool forceUpdate = false) = 0;

  /// Set the value to an equilibrium point for rotation.
  virtual void SetAngularEquilibriumPoint (csVector3 point, bool forceUpdate = false) = 0;

  /// Set the threshold of a breaking impulse.
  virtual void SetBreakingImpulseThreshold (float threshold, bool forceUpdate = false) = 0;

  /// Get the threshold of a breaking impulse.
  virtual float GetBreakingImpulseThreshold () = 0;
};

/**
 * A callback to be implemented when you are using kinematic bodies. If no
 * callback are provided then the dynamic system will use a default one which
 * will update the transform of the body from the position of the attached
 * movable (see iCollisionObject::SetAttachedMovable()).
 * \sa CS::Physics::iRigidBody::SetKinematicCallback()
 */
struct iKinematicCallback : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iKinematicCallback, 1, 0, 0);

  /**
   * Update the new transform of the rigid body.
   */
  virtual void GetBodyTransform (iRigidBody* body,
				 csOrthoTransform& transform) const = 0;
};

/**
 * This is the interface for the actual plugin.
 * It is responsible for creating iPhysicalSector.
 *
 * Main creators of instances implementing this interface:
 * - Bullet plugin (crystalspace.physics.bullet2)
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 * 
 * Main users of this interface:
 * - Dynamics loader plugin (crystalspace.dynamics.loader)
 *
 * \sa CS::Collisions::iCollisionSystem
 */
struct iPhysicalSystem : public virtual CS::Collisions::iCollisionSystem
 //public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iPhysicalSystem, 1, 0, 0);
  
  /**
   * Create a new physical sector
   */
  virtual csPtr<iPhysicalSector> CreatePhysicalSector () = 0;

  /// Create a general 6DOF joint.
  virtual csPtr<iJoint> CreateJoint () = 0;

  /*
   * Create a P2P joint for rigid bodies by setting the position in 
   * world space.
   */
  virtual csPtr<iJoint> CreateRigidP2PJoint (const csVector3 position) = 0;
  
  /* 
   * Create a slide joint for rigid bodies.
   * \param trans The transform of the joint in world space.
   * \param minDist The min distance the body can move along the axis.
   * \param maxDist The max distance the body can move along the axis.
   * \param minAngle The min angle the body can rotate around the axis.
   * \param maxAngle The max angle the body can rotate around the axis.
   * \param axis The slide axis, can only be 0, 1, 2.
   */
  virtual csPtr<iJoint> CreateRigidSlideJoint (const csOrthoTransform trans,
    float minDist, float maxDist, float minAngle, float maxAngle, int axis) = 0;

  /* 
   * Create a hinge joint for rigid bodies.
   * \param position The position of the joint in world space.
   * \param minAngle The min angle the body can rotate around the axis.
   * \param maxAngle The max angle the body can rotate around the axis.
   * \param axis The axis of the hinge, can only be 0, 1, 2.
   */
  virtual csPtr<iJoint> CreateRigidHingeJoint (const csVector3 position,
    float minAngle, float maxAngle, int axis) = 0;

  /* 
   * Create a cone twist joint for rigid bodies.
   * \param trans The transform of the joint in world space.
   * \param swingSpan1 The swing span the body can rotate around the local Z axis of joint.
   * \param swingSpan2 The swing span the body can rotate around the local Y axis of joint.
   * \param twistSpan The twist span the body can rotate around the local X axis of joint.
   */
  virtual csPtr<iJoint> CreateRigidConeTwistJoint (const csOrthoTransform trans,
    float swingSpan1,float swingSpan2,float twistSpan) = 0;

  /* 
   * Create a linear joint for soft body by setting the position in 
   * world space.
   */
  virtual csPtr<iJoint> CreateSoftLinearJoint (const csVector3 position) = 0;

  /* 
   * Create a angular joint for soft body by setting the rotation axis.
   * The axis can only be 0, 1, 2.
   */
  virtual csPtr<iJoint> CreateSoftAngularJoint (int axis) = 0;

  /*
   * Create a pivot joint to attach to a rigid body to a position in world space
   * in order to manipulate it.
   */
  virtual csPtr<iJoint> CreateRigidPivotJoint (iRigidBody* body, const csVector3 position) = 0;
  

  // Factories
  
  virtual csPtr<iRigidBodyFactory> CreateRigidBodyFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "") = 0;

  virtual csPtr<iDynamicActorFactory> CreateDynamicActorFactory (CS::Collisions::iCollider* collider = nullptr, const csString& name = "DynamicActor") = 0;

  virtual csPtr<iSoftRopeFactory> CreateSoftRopeFactory () = 0;
  virtual csPtr<iSoftClothFactory> CreateSoftClothFactory () = 0;
  virtual csPtr<iSoftMeshFactory> CreateSoftMeshFactory () = 0;
  

  // Vehicles

  /// Creates a new factory to produce vehicles
  virtual csPtr<iVehicleFactory> CreateVehicleFactory () = 0;
  
  /// Creates a new factory to produce vehicle wheels
  virtual csPtr<iVehicleWheelFactory> CreateVehicleWheelFactory () = 0;

  /// Returns the vehicle that the given object is a part of, or nullptr
  virtual iVehicle* GetVehicle (CS::Collisions::iCollisionObject* obj) = 0;
};

/**
 * This is the interface for the physical sector.
 * It manage all physical bodies.
 *
 * \sa CS::Collisions::iCollisionSector CS::Physics::iPhysicalSector
 */
struct iPhysicalSector : public virtual CS::Collisions::iCollisionSector
{
  SCF_INTERFACE (CS::Physics::iPhysicalSector, 1, 0, 0);

  /**
   * Set the simulation speed. A value of 0 means that the simulation is not made
   * automatically (but it can still be made manually through Step())
   */
  virtual void SetSimulationSpeed (float speed) = 0;

  /**
   * Set the parameters of the constraint solver. Use this if you want to find a
   * compromise between accuracy of the simulation and performance cost.
   * \param timeStep The internal, constant, time step of the simulation, in seconds.
   * A smaller value gives better accuracy. Default value is 1/60 s (ie 0.0166 s).
   * \param maxSteps Maximum number of steps that Bullet is allowed to take each
   * time you call iPhysicalSector::Step(). If you pass a very small time step as
   * the first parameter, then you must increase the number of maxSteps to
   * compensate for this, otherwise your simulation is 'losing' time. Default value
   * is 1. If you pass maxSteps=0 to the function, then it will assume a variable
   * tick rate. Don't do it.
   * \param iterations Number of iterations of the constraint solver. A reasonable
   * range of iterations is from 4 (low quality, good performance) to 20 (good
   * quality, less but still reasonable performance). Default value is 10. 
   */
  virtual void SetStepParameters (csScalar timeStep, size_t maxSteps,
    size_t iterations) = 0;  

  /// Step the simulation forward by the given duration, in second
  virtual void Step (csScalar duration) = 0;

  /**
   * Set the global linear Damping. The dampening correspond to how
   * much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   * \sa CS::Physics::iRigidBody::SetLinearDamping()
   */
  virtual void SetLinearDamping (csScalar d) = 0;

  /**
   * Get the global linear Damping setting.
   */
  virtual csScalar GetLinearDamping () const = 0;

  /**
   * Set the global angular Damping. The dampening correspond to how
   * much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   * \sa CS::Physics::iRigidBody::SetAngularDamping()
   */
  virtual void SetAngularDamping (csScalar d) = 0;

  /// Get the global angular damping value
  virtual csScalar GetAngularDamping () const = 0;
  
  /**
   * Set the parameters for AutoDisable.
   * \param linear Maximum linear movement to disable a body. Default value is 0.8.
   * \param angular Maximum angular movement to disable a body. Default value is 1.0.
   * \param steps Minimum number of steps the body meets linear and angular
   * requirements before it is disabled. Default value is 0.
   * \param time Minimum time the body needs to meet linear and angular
   * movement requirements before it is disabled. Default value is 0.0.
   * \remark With the Bullet plugin, the 'steps' parameter is ignored.
   * \remark With the Bullet plugin, calling this method will not affect bodies already
   * created.
   */
  virtual void SetAutoDisableParams (float linear, float angular, float time) = 0;

  /// Get the count of rigid bodies.
  virtual size_t GetRigidBodyCount () = 0;

  /// Get the rigid body by index.
  virtual iRigidBody* GetRigidBody (size_t index) = 0;

  /// Find the rigid body in this sector.
  virtual iRigidBody* FindRigidBody (const char* name) = 0;


  /// Get the count of soft bodies.
  virtual size_t GetSoftBodyCount () = 0;

  /// Get the soft body by index.
  virtual iSoftBody* GetSoftBody (size_t index) = 0;

  /// Find  the soft body in this setor.
  virtual iSoftBody* FindSoftBody (const char* name) = 0;


  /// Add a joint to the sector. The joint must have attached two physical bodies.
  virtual void AddJoint (iJoint* joint) = 0;

  /// Remove a joint by pointer.
  virtual void RemoveJoint (iJoint* joint) = 0;

  /**
   * Set whether this dynamic world can handle soft bodies or not.
   * \warning You have to call this method before adding any objects in the
   * dynamic world.
   */
  virtual void SetSoftBodyEnabled (bool enabled) = 0; 

  /**
   * Return whether this dynamic world can handle soft bodies or not.
   */
  virtual bool GetSoftBodyEnabled () = 0;

  /**
   * Save the current state of the dynamic world in a file.
   * \return True if the operation succeeds, false otherwise.
   */
  virtual bool SaveWorld (const char* filename) = 0;

  /**
   * Draw the debug informations of the dynamic system. This has to be called
   * at each frame, and will add 2D lines on top of the rendered scene. The
   * objects to be displayed are defined by SetDebugMode().
   */
  virtual void DebugDraw (iView* rview) = 0;

  /**
   * Set the mode to be used when displaying debug informations. The default value
   * is 'CS::Physics::DEBUG_COLLIDERS | CS::Physics::DEBUG_JOINTS'.
   * \remark Don't forget to call DebugDraw() at each frame to effectively display
   * the debug informations.
   */
  virtual void SetDebugMode (DebugMode mode) = 0;

  /// Get the current mode used when displaying debug informations.
  virtual DebugMode GetDebugMode () = 0;  

  /**
   * Start the profiling of the simulation. This would add an overhead to the
   * computations, but allows to display meaningful information on the behavior
   * of the simulation.
   */ 
  virtual void StartProfile () = 0;

  /**
   * Stop the profiling of the simulation. This would add an overhead to the
   */
  virtual void StopProfile () = 0;

  /**
   * Dump the profile information on the standard output. StartProfile() must
   * have been called before.
   * \param resetProfile Whether or not the profile data must be reset after
   * the dumping.
   */
  virtual void DumpProfile (bool resetProfile = true) = 0;

  /**
   * Will cause the step function to be called on this updatable every step
   */
  virtual void AddUpdatable(iUpdatable* u) = 0;
  
  /**
   * Removes the given updatable
   */
  virtual void RemoveUpdatable(iUpdatable* u) = 0;
};

/**
 * Animation control type for a genmesh animated by a CS::Physics::iSoftBody.
 *
 * Main ways to get pointers to this interface:
 * - csQueryPluginClass()
 * - csLoadPlugin()
 *
 * Main users of this interface:
 * - Genmesh plugin (crystalspace.mesh.object.genmesh) 
 */
struct iSoftBodyAnimationControlType : public iGenMeshAnimationControlType
{
  SCF_INTERFACE (CS::Physics::iSoftBodyAnimationControlType, 1, 0, 0);
};

/**
 * Animation control factory for a genmesh animated by a CS::Physics::iSoftBody.
 *
 * Main creators of instances implementing this interface:
 * - CS::Physics::iSoftBodyAnimationControlType::CreateAnimationControlFactory()
 *
 * Main ways to get pointers to this interface:
 * - iGeneralFactoryState::GetAnimationControlFactory()
 *
 * Main users of this interface:
 * - Genmesh plugin (crystalspace.mesh.object.genmesh) 
 */
struct iSoftBodyAnimationControlFactory : public iGenMeshAnimationControlFactory
{
  SCF_INTERFACE (CS::Physics::iSoftBodyAnimationControlFactory, 1, 0, 0);
};

/**
 * Animation control for a genmesh animated by a CS::Physics::iSoftBody. This class will
 * animate the vertices of the genmesh depending on the physical simulation of the
 * soft body. It will also update automatically the position of the genmesh.
 *
 * The soft body controlling the animation of the genmesh can also be attached precisely to a
 * given vertex of an animesh. This allows to have the soft body following precisely the vertices
 * of the animesh, even when it is deformed by the skinning and morphing processes.
 *
 * Main creators of instances implementing this interface:
 * - CS::Physics::iSoftBodyAnimationControlFactory::CreateAnimationControl()
 *
 * Main ways to get pointers to this interface:
 * - iGeneralMeshState::GetAnimationControl()
 *
 * Main users of this interface:
 * - Genmesh plugin (crystalspace.mesh.object.genmesh) 
 */
struct iSoftBodyAnimationControl : public iGenMeshAnimationControl
{
  SCF_INTERFACE (CS::Physics::iSoftBodyAnimationControl, 1, 0, 0);

  /**
   * Set the soft body to be used to animate the genmesh. You can switch this soft body
   * at any time, the animation of the genmesh will just be adapted to the new soft body.
   * \param body The soft body that will be used to animate this genmesh.
   * \param doubleSided True if the genmesh is double-sided (ie this is a cloth
   * soft body), false otherwise. If the genmesh is double-sided, then the duplicated
   * vertices must be added at the end of the vertex array, so that a vertex of index
   * 'i' is duplicated at index 'i + body->GetVertexCount ()'.
   */
  virtual void SetSoftBody (iSoftBody* body, bool doubleSided = false) = 0;

  /**
   * Get the soft body used to animate the genmesh.
   */
  virtual iSoftBody* GetSoftBody () = 0;

  /**
   * Create an anchor between the soft body and an animesh. The position of the anchor
   * will be updated accordingly when the vertex is moved by the skinning and morphing
   * processes of the animesh.
   *
   * This anchor is only effective if the vertex of the animesh is influenced by more
   * than one bone or by some morph targets. If it is not the case then it is more
   * efficient to simply use CS::Physics::iSoftBody::AnchorVertex(size_t,iRigidBody*).
   *
   * You have to provide a rigid body attached to the animesh as a main physical anchor
   * point. The main way to do that is to use a CS::Animation::iSkeletonRagdollNode
   * animation node.
   *
   * Note also that you may anchor a same soft body to different animeshes, for example
   * to create a cloth hold by several avatars.
   *
   * \param animesh The CS::Mesh::iAnimatedMesh to attach the soft body to.
   * \param body The rigid body used as the main physical anchor point.
   * \param bodyVertexIndex The index of the vertex on the soft body which will be anchored.
   * \param animeshVertexIndex The index of the vertex on the animesh which will be anchored.
   * If no values are provided then the system will compute the vertex on the animesh which is
   * the closest to the given vertex of the soft body. This vertex can be queried afterwards
   * through GetAnimatedMeshAnchorVertex().
   */
  virtual void CreateAnimatedMeshAnchor (CS::Mesh::iAnimatedMesh* animesh,
    iRigidBody* body,
		size_t bodyVertexIndex,
		size_t animeshVertexIndex = (size_t) ~0) = 0;

  /**
   * Get the vertex of the animesh which is anchored to the given vertex of the soft body.
   */
  virtual size_t GetAnimatedMeshAnchorVertex (size_t bodyVertexIndex) = 0;

  /**
   * Remove the given anchor.
   * \warning This won't actually work, due to a limitation inside the Bullet library...
   */
  virtual void RemoveAnimatedMeshAnchor (size_t bodyVertexIndex) = 0;
};


/**
 * TODO: This class should have a common base interface with iCollisionActor
 * 
 * This is the interface for a Dynamic Actor.
 * It allows the user to easily navigate a physical object on ground.
 * The actual RigidBody that represents the actor always floats <step height> above the ground to be able to
 * move smoothly over terrain and small obstacles.
 * The air control factor determines whether and how well the actor can be controlled while not touching the ground.
 * Air control is always 100% when gravity is off.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateDynamicActor()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 *
 */
struct iDynamicActor : public virtual iRigidBody, public virtual CS::Collisions::iActor
{
  SCF_INTERFACE (CS::Physics::iDynamicActor, 1, 0, 0);

  /// Get whether to use a kinematic method for smooth steps
  virtual bool GetUseKinematicSteps() const = 0;
  /// Set whether to use a kinematic method for smooth steps
  virtual void SetUseKinematicSteps(bool u) = 0;
};
}
}
#endif
