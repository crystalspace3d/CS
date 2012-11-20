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

#include "csutil/scf_implementation.h"
#include "iutil/strset.h"

struct iCollection;
struct iEngine;
struct iMeshWrapper;
struct iSector;
struct iTriangleMesh;

namespace CS {
namespace Collisions {

struct iCollider;
struct iCollisionSystem;
struct iConvexDecomposer;

// TODO: move in the cstool lib
class CS_CRYSTALSPACE_EXPORT CollisionHelper 
{
  iObjectRegistry* objectRegistry;
  csRef<CS::Collisions::iCollisionSystem> collisionSystem;
  csRef<CS::Collisions::iConvexDecomposer> decomposer;
  csStringID baseID;
  csStringID collisionID;

  void ReportError (const char* msg, ...);

public:
  /// Initialize this collision helper
  bool Initialize (iObjectRegistry* objectRegistry,
		   CS::Collisions::iCollisionSystem* collisionSystem = nullptr,
		   CS::Collisions::iConvexDecomposer* decomposer = nullptr);

  /// Creates and adds all collision objects of all meshes in the given engine to the collision system
  void InitializeCollisionObjects (iEngine* engine, 
				   iCollection* collection = nullptr) const;
  
  /// Creates and adds all collision objects of all meshes in the given sector to the collision system
  void InitializeCollisionObjects (iSector* sector, 
				   iCollection* collection = nullptr) const;

  /// Recursively creates and adds all collision objects of the mesh and it's children to the collision system
  void InitializeCollisionObjects (iSector* sector, 
				   iMeshWrapper* mesh) const;

  /// Tries to find and return the underlying collision iTriangleMesh that has any of this system's ids
  iTriangleMesh* FindCollisionMesh (iMeshWrapper* mesh) const;

  /**
   * Perform a convex decomposition on the given concave triangle mesh and compound the
   * resulting convex parts into the given collider.
   */
  void DecomposeConcaveMesh (iTriangleMesh* mesh, CS::Collisions::iCollider* collider,
			     CS::Collisions::iConvexDecomposer* decomposer) const;

  /**
   * Perform a convex decomposition on the given concave mesh wrapper and compound the
   * resulting convex parts into the given collider. This is equivalent to a call to
   * FindCollisionMesh() followed by a call to DecomposeConcaveMesh(iTriangleMesh*,CS::Collisions::iCollider*).
   */
  void DecomposeConcaveMesh (iMeshWrapper* mesh, CS::Collisions::iCollider* collider,
			     CS::Collisions::iConvexDecomposer* decomposer) const;
};

} // namespace Collisions
} // namespace CS

#endif // __CS_CSTOOL_COLLISIONHELPER_H
