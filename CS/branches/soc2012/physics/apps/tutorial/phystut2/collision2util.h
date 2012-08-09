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