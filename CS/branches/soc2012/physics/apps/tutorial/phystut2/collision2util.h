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

class Collision2Helper
{
public:
  void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iEngine* engine, iCollection* collection);

  void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys,
    iSector* sector, iCollection* collection);

  /// Recursively creates and adds all collision objects of the mesh and it's children to the collision system
  void InitializeCollisionObjects (CS::Collisions::iCollisionSystem* colsys, iMeshWrapper* mesh);
};


#endif