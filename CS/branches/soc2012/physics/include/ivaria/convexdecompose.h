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
#ifndef __IVARIA_CONVEXDECOMPOSE__
#define __IVARIA_CONVEXDECOMPOSE__

/**\file
* Convex decomposition tools
*/

#include "csutil/scf.h"
#include "csutil/scf_interface.h"

struct iTriangleMesh;

// This is not actually related to collisions - Convex decomposition is a mesh operation
//namespace CS
//{
//  namespace Collisions
//  {
//
    struct iConvexDecomposedMeshResult
    {
      /// Yields the next convex part. The given mesh is ensured to be convex.
      virtual void YieldMesh(iTriangleMesh* convexMeshPart) = 0;
    };

    /**
    * Performs flavored convex decomposition on a concave mesh.
    * This class is needed because concave meshes are a lot harder to deal with.
    */
    struct iConvexDecomposer : public virtual iBase
    {
      SCF_INTERFACE (iConvexDecomposer, 1, 0, 0);

      /// Performs convex decomposition on a (possibly) concave mesh.
      virtual void Decompose(iTriangleMesh* triMesh, iConvexDecomposedMeshResult* results) = 0;
    };
//
//  }
//}

#endif // __IVARIA_CONVEXDECOMPOSE__
