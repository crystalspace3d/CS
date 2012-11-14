/*
    Copyright (C) 2012 by Dominik Seifert

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

/**
 * Utility class for any kind of collision system
 */

#ifndef __CS_CSTOOL_COLLISIONHELPER_H
#define __CS_CSTOOL_COLLISIONHELPER_H

#include "csextern.h"

#include "csgeom/box.h"
#include "csutil/csobject.h"
#include "csutil/leakguard.h"
#include "csutil/array.h"
#include "csutil/scf_implementation.h"
#include "csutil/set.h"

#include "ivaria/physics.h"

struct iConvexDecomposer;
struct iTriangleMesh;

/// Callback-style result set that works around allocation issues and yields all partial colliders that make up a concave mesh
struct iConvexDecomposedColliderResult
{
  /// Yields the next convex-decomposed collider
  virtual void YieldCollider(CS::Collisions::iCollider* collider);
};

// TODO: move in the cstool lib
class CS_CRYSTALSPACE_EXPORT CollisionHelper 
{
  iObjectRegistry* objectRegistry;
  CS::Collisions::iCollisionSystem* collisionSystem;
  csStringID baseID;
  csStringID collisionID;

public:
  /// Initialize this collision helper
  // TODO: set convex decomposer as a param too
  void Initialize (iObjectRegistry* objectRegistry,
		   CS::Collisions::iCollisionSystem* collisionSystem);

  /// Creates and adds all collision objects of all meshes in the given engine to the collision system
  void InitializeCollisionObjects (iEngine* engine, 
				   iConvexDecomposer* decomposer = nullptr, 
				   iCollection* collection = nullptr);
  
  /// Creates and adds all collision objects of all meshes in the given sector to the collision system
  void InitializeCollisionObjects (iSector* sector, 
				   iConvexDecomposer* decomposer = nullptr, 
				   iCollection* collection = nullptr);

  /// Recursively creates and adds all collision objects of the mesh and it's children to the collision system
  void InitializeCollisionObjects (iSector* sector, 
				   iMeshWrapper* mesh, 
				   iConvexDecomposer* decomposer = nullptr);

  /// Tries to find and return the underlying collision iTriangleMesh that has any of this system's ids
  iTriangleMesh* FindCollisionMesh (iMeshWrapper* mesh);

  /// Perform convex decomposition on the given concave trimesh and compound the result meshes into a single collider
  // TODO: merge into the convex decomposition plugin
  csPtr<CS::Collisions::iColliderCompound> PerformConvexDecomposition (
    iConvexDecomposer* decomposer,
    iTriangleMesh* concaveMesh);
};

#endif // __CS_CSTOOL_COLLISIONHELPER_H
