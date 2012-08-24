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

#ifndef __COLLISION2UTIL_H
#define __COLLISION2UTIL_H

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

class Collision2Helper
{
public:
  /// Creates and adds all collision objects of all meshes in the given engine to the collision system
  static void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iEngine* engine, 
    iConvexDecomposer* decomposer = nullptr, 
    iCollection* collection = nullptr);
  
  /// Creates and adds all collision objects of all meshes in the given sector to the collision system
  static void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iSector* sector, 
    iConvexDecomposer* decomposer = nullptr, 
    iCollection* collection = nullptr);

  /// Recursively creates and adds all collision objects of the mesh and it's children to the collision system
  static void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys, iSector* sector, 
    iMeshWrapper* mesh, 
    iConvexDecomposer* decomposer = nullptr);

  /// Perform convex decomposition on the given trimesh and compound the result meshes into a single collider
  static csPtr<CS::Collisions::iColliderCompound> PerformConvexDecomposition(
  CS::Collisions::iCollisionSystem* colSys,
  iConvexDecomposer* decomposer,
  iTriangleMesh* concaveMesh);
};


#endif