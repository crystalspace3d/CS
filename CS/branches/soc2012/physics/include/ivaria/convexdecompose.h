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

namespace CS
{
namespace Collisions
{

/**
 *
 */
struct iConvexDecomposer : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iConvexDecomposer, 1, 0, 0);


};

}
}

#endif // __IVARIA_CONVEXDECOMPOSE__
