/*
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

#ifndef __IVARIA_PHYSIPROPERTIESH__
#define __IVARIA_PHYSIPROPERTIESH__

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
struct iKinematicCallback;
struct iPhysicalSystem;
struct iPhysicalSector;

struct iPhysicsProperties : virtual iBase
{

};

}
}
#endif
