/*
    Dynamics/Kinematics modeling and simulation library.
    Copyright (C) 1999 by Michael Alexander Ewert and Noah Gibbs

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

#ifndef QUATRIGIDBODYCONNECTOR_H
#define QUATRIGIDBODYCONNECTOR_H

#include "csphyzik/phyztype.h"
#include "csphyzik/entity.h"
#include "csphyzik/ctquat.h"
#include "csgeom/quaterni.h"
#include "csphyzik/point.h"

class ctQuatRigidBody;

class ctQuatRigidBodyConnector : public ctPointObj {
  ctQuatRigidBody *rigid;
  ctVector3        r;     // Offset from center of mass
 public:
  ctQuatRigidBodyConnector(ctQuatRigidBody *rb, ctVector3 offset);
  ~ctQuatRigidBodyConnector() {}

  ctVector3 pos();
  ctVector3 vel();

  void apply_force(ctVector3 F);
};

#endif // QUATRIGIDBODYCONNECTOR_H
