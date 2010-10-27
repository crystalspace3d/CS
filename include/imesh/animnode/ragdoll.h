/*
  Copyright (C) 2009 Christian Van Brussel, Communications and Remote
      Sensing Laboratory of the School of Engineering at the 
      Universite catholique de Louvain, Belgium
      http://www.tele.ucl.ac.be

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
#ifndef __CS_IMESH_ANIMNODE_RAGDOLL_H__
#define __CS_IMESH_ANIMNODE_RAGDOLL_H__

/**\file
 * Ragdoll animation node for an animated mesh.
 */

#include "csutil/scf_interface.h"

#include "imesh/bodymesh.h"

/**\addtogroup meshplugins
 * @{ */

namespace CS
{
namespace Mesh
{

struct iAnimatedMesh;

} // namespace Mesh
} // namespace CS

namespace CS
{
namespace Animation
{

struct iSkeletonRagdollNodeFactory;

/**
 * A class to manage the creation and deletion of ragdoll animation 
 * node factories.
 */
struct iSkeletonRagdollNodeManager : public virtual iBase
{
  SCF_INTERFACE(CS::Animation::iSkeletonRagdollNodeManager, 1, 0, 0);

  /**
   * Create a new ragdoll animation node factory.
   */
  virtual iSkeletonRagdollNodeFactory* CreateAnimNodeFactory (const char *name,
		  iBodySkeleton* skeleton, iDynamicSystem* dynSys) = 0;

  /**
   * Find the ragdoll animation node factory with the given name.
   */
  virtual iSkeletonRagdollNodeFactory* FindAnimNodeFactory
    (const char* name) const = 0;

  /**
   * Delete all ragdoll animation node factories.
   */
  virtual void ClearAnimNodeFactories () = 0;
};

/**
 * The physical state of a body chain.
 */
enum RagdollState
{
  STATE_INACTIVE = 0,   /*!< The chain is physically inactive. */
  STATE_DYNAMIC,        /*!< The chain is dynamic, ie the motion of 
			  the chain is controlled by the dynamic simulation. */
  STATE_KINEMATIC       /*!< The chain is kinematic, ie the motion 
			  of the chain is controlled by the animation system,
			  but its bones do interact with the dynamic simulation. */
};

/**
 * Factory for the ragdoll animation node.
 */
struct iSkeletonRagdollNodeFactory : public iSkeletonAnimNodeFactory
{
  SCF_INTERFACE(CS::Animation::iSkeletonRagdollNodeFactory, 1, 0, 2);

  /**
   * Add a new body chain to the ragdoll animation node. Adding more than 
   * one body chain is not yet supported.
   * \param state The initial state of the body chain.
   */
  virtual void AddBodyChain
    (iBodyChain* chain,
     RagdollState state = STATE_INACTIVE) = 0;

  /**
   * Remove the chain from the ragdoll animation node.
   */
  virtual void RemoveBodyChain (iBodyChain* chain) = 0;

  /**
   * Set the child animation node of this node. The ragdoll controller will
   * add its control on top of the animation of the child node. This child
   * node is not mandatory.
   *
   * The orientation/position values of the bones that are in state
   * CS::Animation::STATE_INACTIVE or CS::Animation::STATE_KINEMATIC
   * will be read from the child node, while the bones in state
   * CS::Animation::STATE_DYNAMIC will be overwriten by this node.
   */
  virtual void SetChildNode (iSkeletonAnimNodeFactory* node) = 0;

  /**
   * Return the child animation node of this node.
   */
  virtual iSkeletonAnimNodeFactory* GetChildNode () = 0;

  /**
   * Clear the child animation node of this node.
   */
  virtual void ClearChildNode () = 0;

  /**
   * Get the dynamic system where the ragdoll bodies are evolving
   */
  virtual iDynamicSystem* GetDynamicSystem () const = 0;
};

/**
 * An animation node that interacts with the dynamic simulation in order to
 * control the animation of the animated mesh, and/or in order to make the mesh
 * collide with the rigid bodies of the simulation.
 */
struct iSkeletonRagdollNode : public iSkeletonAnimNode
{
  SCF_INTERFACE(CS::Animation::iSkeletonRagdollNode, 1, 0, 1);

  /**
   * Set the body chain in the specified physical state.
   */
  virtual void SetBodyChainState (iBodyChain* chain,
				  RagdollState state) = 0;

  /**
   * Get the physical state of the body chain specified.
   */
  virtual RagdollState GetBodyChainState (iBodyChain* chain) = 0;

  /**
   * Get the rigid body of the specified bone.
   */
  virtual iRigidBody* GetBoneRigidBody (BoneID bone) = 0;

  /**
   * Get the joint of the specified bone.
   */
  virtual iJoint* GetBoneJoint (const BoneID bone) = 0;

  /**
   * Get the count of bones in the specified physical state.
   */
  virtual uint GetBoneCount (RagdollState state) const = 0;

  /**
   * Get a bone from its index.
   */
  virtual BoneID GetBone (RagdollState state, uint index) const = 0;

  /**
   * Reset the transform of each rigid body of the chain to the initial 'bind'
   * transform. This can be used only on chains that are in a dynamic state.
   *
   * Use this when the bone where the chain is attached is moved abruptly (ie
   * when the whole mesh is moved sharply, or when the transition of the
   * animation is not continuous). Otherwise, letting the dynamic simulation
   * handle such a radical teleportation might lead to an unstable and unwanted
   * behavior.
   */
  virtual void ResetChainTransform (iBodyChain* chain) = 0;
};

} // namespace Animation
} // namespace CS

/** @} */

#endif //__CS_IMESH_ANIMNODE_RAGDOLL_H__
