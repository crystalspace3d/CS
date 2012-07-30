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

#ifndef __CS_IVARIA_PHYSICSCOMMON_H__
#define __CS_IVARIA_PHYSICSCOMMON_H__

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "iutil/object.h"
#include "colliders.h"

#include "ivaria/collisionscommon.h"

namespace CS
{
  namespace Collisions
  {
    struct iCollisionObject;
  }
}

namespace CS
{
  namespace Physics
  {
    struct iPhysicalSector;

    /**
    * Custom code to be called upon every step
    */
    struct iUpdatable : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iUpdatable, 1, 0, 0);

      /**
       * Do something every step, given "dt" seconds have passed since the beginning of the last step.
       */
      virtual void DoStep(csScalar dt) = 0;

      /// The collision object associated with this updatable (if any)
      virtual CS::Collisions::iCollisionObject* GetCollisionObject() = 0;

      /// Called when updatable is added to the given sector
      virtual void OnAdded(iPhysicalSector* sector) = 0;

      /// Called when updatable is removed from the given sector
      virtual void OnRemoved(iPhysicalSector* sector) = 0;
    };
  }
}
#endif