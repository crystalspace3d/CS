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

#include "cssysdef.h"

#include "imesh/animesh.h"
#include "iengine/scenenode.h"
#include "iengine/movable.h"
#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "csutil/sysfunc.h"
#include "iutil/objreg.h"
#include "ivaria/view.h"
#include "ivaria/collisions.h"
#include "igeom/trimesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "csutil/custom_new_enable.h"

#include "common2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "dynamicactor.h"
#include "collisionactor.h"
#include "joint2.h"

#include "physicsfactories.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  BulletCollisionObjectFactory::BulletCollisionObjectFactory
    (csBulletSystem* system, CS::Collisions::iCollider* collider)
    : scfImplementationType (this), system (system), collider (collider),
    group (system->GetDefaultGroup ())
  {}

  void BulletCollisionObjectFactory::SetCollisionGroup (CS::Collisions::iCollisionGroup* group)
  {
    this->group = dynamic_cast<CollisionGroup*> (group);
    if (!this->group) this->group = system->GetDefaultGroup ();
  }

  CS::Collisions::iCollisionGroup* BulletCollisionObjectFactory::GetCollisionGroup () const
  {
    return group;
  }

  void BulletCollisionObjectFactory::SetColliderTransform (const csOrthoTransform& transform)
  {
    this->transform = transform;
  }

  const csOrthoTransform& BulletCollisionObjectFactory::GetColliderTransform () const
  {
    return transform;
  }

  csPtr<CS::Collisions::iCollisionObject> BulletRigidBodyFactory::CreateCollisionObject () 
  { 
    return DowncastPtr<CS::Collisions::iCollisionObject, CS::Physics::iRigidBody> (CreateRigidBody ()); 
  }

  csPtr<CS::Physics::iRigidBody> BulletRigidBodyFactory::CreateRigidBody ()
  {
    csRef<csBulletRigidBody> body = csPtr<csBulletRigidBody>(new csBulletRigidBody (system));
    body->CreateRigidBodyObject (this);
    return csPtr<CS::Physics::iRigidBody>(body);
  }

  csPtr<CS::Collisions::iActor> BulletDynamicActorFactory::CreateActor ()
  {
    csRef<CS::Physics::iDynamicActor> actor = CreateDynamicActor ();
    return csPtr<iActor> (actor);
  }

  csPtr<CS::Physics::iDynamicActor> BulletDynamicActorFactory::CreateDynamicActor ()
  {
    csRef<csBulletDynamicActor> body = csPtr<csBulletDynamicActor> (new csBulletDynamicActor (system));
    body->CreateDynamicActor (this);
    return csPtr<CS::Physics::iDynamicActor> (body);
  }

  csPtr<CS::Physics::iRigidBody> BulletDynamicActorFactory::CreateRigidBody ()
  { 
    return DowncastPtr<CS::Physics::iRigidBody, CS::Physics::iDynamicActor>(CreateDynamicActor ()); 
  }

  csPtr<CS::Collisions::iCollisionObject> BulletDynamicActorFactory::CreateCollisionObject () 
  { 
    return DowncastPtr<CS::Collisions::iCollisionObject, CS::Physics::iDynamicActor>(CreateDynamicActor ()); 
  }

  csPtr<CS::Collisions::iCollisionObject> BulletSoftBodyFactory::CreateCollisionObject () 
  { 
    return DowncastPtr<CS::Collisions::iCollisionObject, CS::Physics::iSoftBody>(CreateSoftBody ()); 
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftRopeFactory::CreateSoftBody ()
  { 
    btSoftBody* body = btSoftBodyHelpers::CreateRope
      (*system->GetSoftBodyWorldInfo (), CSToBullet (GetStart (), system->GetInternalScale ()),
      CSToBullet (GetEnd (), system->GetInternalScale ()), int (GetNodeCount ()) - 1, 0);

    //hard-coded parameters for hair ropes
    body->m_cfg.kDP = 0.08f; // damping
    body->m_cfg.piterations = 16; // no white zone
    body->m_cfg.timescale = 2;

    csRef<csBulletSoftBody> csBody = csPtr<csBulletSoftBody>(new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody>(csBody);
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftClothFactory::CreateSoftBody ()
  {
    const csVector3* corners = GetCorners ();
    size_t segmentCount1, segmentCount2;
    GetSegmentCounts (segmentCount1, segmentCount2);

    btSoftBody* body = btSoftBodyHelpers::CreatePatch
      (*system->GetSoftBodyWorldInfo (), CSToBullet (corners[0], system->GetInternalScale ()),
      CSToBullet (corners[1], system->GetInternalScale ()),
      CSToBullet (corners[2], system->GetInternalScale ()),
      CSToBullet (corners[3], system->GetInternalScale ()), 
      int (segmentCount1), 
      int (segmentCount2), 0,
      GetWithDiagonals ());
    body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;


    csRef<csBulletSoftBody> csBody = csPtr<csBulletSoftBody>(new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody>(csBody);
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftMeshFactory::CreateSoftBody ()
  {
    iGeneralFactoryState*  genmeshFactory = GetGenmeshFactory ();
    btScalar* vertices = new btScalar[genmeshFactory->GetVertexCount () * 3];
    for (int i = 0; i < genmeshFactory->GetVertexCount (); i++)
    {
      csVector3 vertex = genmeshFactory->GetVertices ()[i] * system->GetInternalScale ();
      vertices[i * 3] = vertex[0];
      vertices[i * 3 + 1] = vertex[1];
      vertices[i * 3 + 2] = vertex[2];
    }

    int* triangles = new int[genmeshFactory->GetTriangleCount () * 3];
    for (int i = 0; i < genmeshFactory->GetTriangleCount (); i++)
    {
      csTriangle& triangle = genmeshFactory->GetTriangles ()[i];
      triangles[i * 3] = triangle.a;
      triangles[i * 3 + 1] = triangle.b;
      triangles[i * 3 + 2] = triangle.c;
    }

    btSoftBody* body = btSoftBodyHelpers::CreateFromTriMesh
      (*system->GetSoftBodyWorldInfo (), vertices, triangles, genmeshFactory->GetTriangleCount (),
      false);

    // TODO: Make this stuff customizable, too
    body->m_cfg.piterations = 10;
    body->m_cfg.collisions |=	btSoftBody::fCollision::SDF_RS;
    body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;
    body->m_materials[0]->m_kLST = 1;


    csRef<csBulletSoftBody> csBody = csPtr<csBulletSoftBody>(new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody>(csBody);
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
