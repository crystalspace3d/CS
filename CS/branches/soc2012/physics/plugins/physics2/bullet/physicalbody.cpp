/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "physicalbody.h"

using namespace CS::Collisions;
using namespace CS::Physics;


CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csPhysicalBody::CreatePhysicalBodyObject(iPhysicalObjectFactory* props)
  {
    CreateCollisionObject(props);

    if (props->GetDensity())
    {
      SetDensity(props->GetDensity());
    }
    else
    {
      SetMass(props->GetMass());
    }
    SetFriction(props->GetFriction());
  }

csPhysicalBody::csPhysicalBody (csBulletSystem* phySys)
: scfImplementationType (this, phySys)
{
}

csPhysicalBody::~csPhysicalBody ()
{
}

bool csPhysicalBody::Disable ()
{
  CS_ASSERT (btObject);
  SetLinearVelocity (csVector3 (0.0f));
  SetAngularVelocity (csVector3 (0.0f));
  btObject->setInterpolationWorldTransform (btObject->getWorldTransform());
  btObject->setActivationState (ISLAND_SLEEPING);
  return true;
}

bool csPhysicalBody::Enable ()
{
  CS_ASSERT (btObject);
  btObject->activate (true);
  return true;
}

bool csPhysicalBody::IsEnabled ()
{
 CS_ASSERT (btObject);
 return btObject->isActive ();
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)