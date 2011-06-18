#include "cssysdef.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/quaternion.h"
#include "csgeom/vector3.h"
#include "iutil/strset.h"

// Bullet includes.
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "softbody2.h"
#include "rigidbody2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
csBulletSoftBody::csBulletSoftBody (csBulletSystem* phySys, btSoftBody* body)
  :scfImplementationType (this, phySys), btBody (body)
{
  btObject = body;
  btBody->setUserPointer (dynamic_cast<iPhysicalBody*> (this));
  this->type = CS::Collision::COLLISION_OBJECT_PHYSICAL;
}

csBulletSoftBody::~csBulletSoftBody ()
{
  RemoveBulletObject ();
  (sector->anchoredSoftBodies).Delete (this);
}

void csBulletSoftBody::SetTransform (const csOrthoTransform& trans)
{
  transform = CSToBullet (trans, system->getInternalScale ());
  btBody->transform (transform);
}

csOrthoTransform csBulletSoftBody::GetTransform ()
{
  return BulletToCS (transform, system->getInverseInternalScale ());
}

void csBulletSoftBody::RebuildObject ()
{
  btBody->randomizeConstraints();
  btBody->setCollisionFlags(0);
}

HitBeamResult csBulletSoftBody::HitBeam (const csVector3& start, const csVector3& end)
{
  CS::Collision::HitBeamResult result;
  btVector3 rayFrom = CSToBullet (start, system->getInternalScale ());
  btVector3 rayTo = CSToBullet (end, system->getInternalScale ());
  btSoftBody::sRayCast ray;

  btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);
  if (btBody->rayTest (rayFrom, rayTo, ray))
  {
    result.hasHit = true;
    result.object = QueryCollisionObject ();
    result.isect = BulletToCS (rayCallback.m_hitPointWorld,
      system->getInverseInternalScale ());
    result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
      system->getInverseInternalScale ());	
    
    btVector3 impact = rayFrom + (rayTo - rayFrom) * ray.fraction;
    switch (ray.feature)
    {
    case btSoftBody::eFeature::Face:
      {
        btSoftBody::Face& face = btBody->m_faces[ray.index];
        btSoftBody::Node* node = face.m_n[0];
        float distance = (node->m_x - impact).length2 ();

        for (int i = 1; i < 3; i++)
        {
          float nodeDistance = (face.m_n[i]->m_x - impact).length2 ();
          if (nodeDistance < distance)
          {
            node = face.m_n[i];
            distance = nodeDistance;
          }
        }
        result.vertexIndex = (size_t) (node - &btBody->m_nodes[0]);
        break;
      }
    default:
      break;
    }
  }
  return result;
}

void csBulletSoftBody::RemoveBulletObject ()
{
  if (insideWorld)
  {
    btSoftRigidDynamicsWorld* softWorld =
      static_cast<btSoftRigidDynamicsWorld*> (sector->bulletWorld);
    softWorld->removeSoftBody (btBody);
    insideWorld = false;
  }
}

void csBulletSoftBody::AddBulletObject ()
{
  if (!insideWorld)
  {
    btBody->m_worldInfo = sector->softWorldInfo;
    btSoftRigidDynamicsWorld* softWorld =
      static_cast<btSoftRigidDynamicsWorld*> (sector->bulletWorld);
    softWorld->addSoftBody (btBody);
    insideWorld = true;
  }
}

bool csBulletSoftBody::Disable ()
{
  SetLinearVelocity (csVector3 (0.0f));
  if (btBody)
  {
    btBody->setActivationState (ISLAND_SLEEPING);
    return true;
  }
  else
    return false;
}

bool csBulletSoftBody::Enable ()
{
  if (btBody)
  {
    btBody->setActivationState (ACTIVE_TAG);
    return true;
  }
  else
    return false;
}

bool csBulletSoftBody::IsEnabled ()
{
 if (btBody)
   return btBody->isActive ();
 else
   return false;
}

float csBulletSoftBody::GetMass ()
{
  if (btBody)
    return btBody->getTotalMass ();
  else
    return 0.0f;
}

void csBulletSoftBody::SetDensity (float density)
{
  this->density = density;
  if (btBody)
    btBody->setTotalDensity (density);
}

float csBulletSoftBody::GetVolume ()
{
  if (btBody)
    return btBody->getVolume ();
  else
    return 0;
}

void csBulletSoftBody::AddForce (const csVector3& force)
{
  CS_ASSERT (btBody);
  btBody->addForce (CSToBullet (force, system->getInternalScale ()));
}

void csBulletSoftBody::SetLinearVelocity (const csVector3& vel)
{
  CS_ASSERT (btBody);
  btBody->setVelocity (CSToBullet (vel, system->getInternalScale ()));
}

csVector3 csBulletSoftBody::GetLinearVelocity (size_t index /* = 0 */) const
{
  CS_ASSERT ( btBody && index < (size_t) btBody->m_nodes.size ());
  return BulletToCS (btBody->m_nodes[index].m_v, system->getInverseInternalScale ());
}

void csBulletSoftBody::SetFriction (float friction)
{
  this->friction = friction;
  if (friction >= 0.0f && friction <= 1.0f)
    btBody->m_cfg.kDF = friction;
}

void csBulletSoftBody::SetVertexMass (float mass, size_t index)
{
  if (btBody)
    btBody->setMass (index, mass);
}

float csBulletSoftBody::GetVertexMass (size_t index)
{
  if (btBody)
    return btBody->getMass (index);
  else
    return 0.0f;
}

size_t csBulletSoftBody::GetVertexCount ()
{
  if (btBody)
    return btBody->m_nodes.size ();
  else
    return 0;
}

csVector3 csBulletSoftBody::GetVertexPosition (size_t index) const
{
  CS_ASSERT(btBody && index < (size_t) btBody->m_nodes.size ());
  return BulletToCS (btBody->m_nodes[index].m_x, system->getInverseInternalScale ());
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex)
{
  CS_ASSERT(vertexIndex < (size_t) btBody->m_nodes.size ());
  btBody->setMass (vertexIndex, 0.0f);
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex, iRigidBody* body)
{
  csBulletRigidBody* rigidBody = static_cast<csBulletRigidBody*> (body);
  CS_ASSERT(rigidBody
    && vertexIndex < (size_t) this->btBody->m_nodes.size ()
    && rigidBody->btBody);
  this->btBody->appendAnchor (vertexIndex, rigidBody->btBody);
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex,
                                     CS::Physics::iAnchorAnimationControl* controller)
{
  AnimatedAnchor anchor (vertexIndex, controller);
  animatedAnchors.Push (anchor);
  (sector->anchoredSoftBodies).Push (this);
}

void csBulletSoftBody::UpdateAnchor (size_t vertexIndex, csVector3& position)
{
  CS_ASSERT(vertexIndex < (size_t) btBody->m_nodes.size ());

  // Update the local position of the anchor
  for (int i = 0; i < this->btBody->m_anchors.size (); i++)
    if (this->btBody->m_anchors[i].m_node == &this->btBody->m_nodes[vertexIndex])
    {
      this->btBody->m_anchors[i].m_local =
        this->btBody->m_anchors[i].m_body->getInterpolationWorldTransform ().inverse ()
        * CSToBullet (position, system->getInternalScale ());
      return;
    }
}

void csBulletSoftBody::RemoveAnchor (size_t vertexIndex)
{
  CS_ASSERT(vertexIndex < (size_t) btBody->m_nodes.size ());

  // Check if it is a fixed anchor
  if (btBody->getMass (vertexIndex) < SMALL_EPSILON)
  {
    btBody->setMass (vertexIndex, btBody->getTotalMass () / btBody->m_nodes.size ());
    return;
  }

  // Check if it is an animated anchor
  size_t index = 0;
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); index++)
  {
    AnimatedAnchor& anchor = it.Next ();
    if (anchor.vertexIndex == vertexIndex)
    {
      animatedAnchors.DeleteIndex (index);
      (sector->anchoredSoftBodies).Delete (this);
      return;
    }
  }

  // Check if it is a simple 'rigid body' anchor
  for (int i = 0; i < this->btBody->m_anchors.size (); i++)
    if (this->btBody->m_anchors[i].m_node == &this->btBody->m_nodes[vertexIndex])
    {
      // TODO: this is not possible within Bullet
      //btSoftBody::Anchor* anchor = this->body->m_anchors[i];
      //this->body->m_anchors.remove (i);
      return;
    }
}

float csBulletSoftBody::GetRidigity ()
{
  return this->btBody->m_materials[0]->m_kLST;
}

void csBulletSoftBody::SetRigidity (float rigidity)
{
  CS_ASSERT(rigidity >= 0.0f && rigidity <= 1.0f);

  btBody->m_materials[0]->m_kLST = rigidity;
}

void csBulletSoftBody::SetLinearVelocity (const csVector3& velocity, size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());
  btBody->addVelocity (CSToBullet (velocity, system->getInternalScale ())
    - btBody->m_nodes[vertexIndex].m_v, vertexIndex);
}

void csBulletSoftBody::SetWindVelocity (const csVector3& velocity)
{
  btVector3 velo = CSToBullet (velocity, system->getInternalScale ());
  btBody->setWindVelocity (velo);
}

const csVector3 csBulletSoftBody::GetWindVelocity () const
{
  csVector3 velo = BulletToCS (btBody->getWindVelocity (), system->getInternalScale ());
  return velo;
}

void csBulletSoftBody::AddForce (const csVector3& force, size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size());
  //TODO: in softbodies.cpp the force was multiplied by 100, why?
  btBody->addForce (CSToBullet (force, system->getInternalScale () * system->getInternalScale ()), vertexIndex);
}

size_t csBulletSoftBody::GetTriangleCount ()
{
  return btBody->m_faces.size ();
}

csTriangle csBulletSoftBody::GetTriangle (size_t index) const
{
  CS_ASSERT(index < (size_t) btBody->m_faces.size ());
  btSoftBody::Face& face = btBody->m_faces[index];
  return csTriangle (face.m_n[0] - &btBody->m_nodes[0],
    face.m_n[1] - &btBody->m_nodes[0],
    face.m_n[2] - &btBody->m_nodes[0]);
}

csVector3 csBulletSoftBody::GetVertexNormal (size_t index) const
{
  CS_ASSERT(index < (size_t) btBody->m_nodes.size ());
  csVector3 normal (btBody->m_nodes[index].m_n.getX (),
    btBody->m_nodes[index].m_n.getY (),
    btBody->m_nodes[index].m_n.getZ ());
  normal.Normalize ();
  return normal;
}

void csBulletSoftBody::DebugDraw (iView* rView)
{
  if (!sector->debugDraw)
  {
    sector->debugDraw = new csBulletDebugDraw (system->getInverseInternalScale ());
    sector->bulletWorld->setDebugDrawer (sector->debugDraw);
  }

  btSoftBodyHelpers::Draw (btBody, sector->debugDraw);
  sector->debugDraw->DebugDraw (rView);
}

void csBulletSoftBody::SetLinearStiff (float stiff)
{
  if (stiff >= 0.0f && stiff <= 1.0f)
  {
    btSoftBody::Material*	pm=btBody->m_materials[0];
    pm->m_kLST = stiff;
  }
}

void csBulletSoftBody::SetAngularStiff (float stiff)
{
  if (stiff >= 0.0f && stiff <= 1.0f)
  {
    btSoftBody::Material*	pm=btBody->m_materials[0];
    pm->m_kAST = stiff;
  }
}

void csBulletSoftBody::SetVolumeStiff (float stiff)
{
  if (stiff >= 0.0f && stiff <= 1.0f)
  {
    btSoftBody::Material*	pm=btBody->m_materials[0];
    pm->m_kVST = stiff;
  }
}

void csBulletSoftBody::ResetCollisionFlag ()
{
  btBody->m_cfg.collisions = 0;
}

void csBulletSoftBody::SetClusterCollisionRS (bool cluster)
{
  if (cluster)
    btBody->m_cfg.collisions	+=	btSoftBody::fCollision::CL_RS;
  else
    btBody->m_cfg.collisions	+=	btSoftBody::fCollision::SDF_RS;
}

void csBulletSoftBody::SetClusterCollisionSS (bool cluster)
{
  if (cluster)
    btBody->m_cfg.collisions += btSoftBody::fCollision::CL_SS;
  else
    btBody->m_cfg.collisions += btSoftBody::fCollision::VF_SS;
}

void csBulletSoftBody::SetSRHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSRHR_CL = hardness;
}

void csBulletSoftBody::SetSKHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSKHR_CL = hardness;
}

void csBulletSoftBody::SetSSHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSSHR_CL = hardness;
}

void csBulletSoftBody::SetSRImpulse (float impulse)
{
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSR_SPLT_CL = impulse;
}

void csBulletSoftBody::SetSKImpulse (float impulse)
{
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSK_SPLT_CL = impulse;
}

void csBulletSoftBody::SetSSImpulse (float impulse)
{
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSS_SPLT_CL = impulse;
}

void csBulletSoftBody::SetVeloCorrectionFactor (float factor)
{
  btBody->m_cfg.kVCF = factor;
}

void csBulletSoftBody::SetDamping (float damping)
{
  if (damping >= 0.0f && damping <= 1.0f)
    btBody->m_cfg.kDP = damping;
}

void csBulletSoftBody::SetDrag (float drag)
{
  if (drag >= 0.0f)
    btBody->m_cfg.kDG = drag;
}

void csBulletSoftBody::SetLift (float lift)
{
  if (lift >= 0.0f)
    btBody->m_cfg.kLF = lift;
}

void csBulletSoftBody::SetPressure (float pressure)
{
  if (pressure >= 0.0f && pressure <= 1.0f)
    btBody->m_cfg.kPR = pressure;
}

void csBulletSoftBody::SetVolumeConversationCoefficient (float conversation)
{
  btBody->m_cfg.kVC = conversation;
}

void csBulletSoftBody::SetShapeMatchThreshold (float matching)
{
  if (matching >= 0.0f && matching <= 1.0f)
    btBody->m_cfg.kMT = matching;
}

void csBulletSoftBody::SetRContactsHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kCHR = hardness;
}

void csBulletSoftBody::SetKContactsHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kKHR = hardness;
}

void csBulletSoftBody::SetSContactsHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSHR = hardness;
}

void csBulletSoftBody::SetAnchorsHardness (float hardness)
{
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kAHR = hardness;
}

void csBulletSoftBody::SetVeloSolverIterations (int iter)
{
  btBody->m_cfg.piterations = iter;
}

void csBulletSoftBody::SetPositionIterations (int iter)
{
  btBody->m_cfg.viterations = iter;
}

void csBulletSoftBody::SetDriftIterations (int iter)
{
  btBody->m_cfg.diterations = iter;
}

void csBulletSoftBody::SetClusterIterations (int iter)
{
  btBody->m_cfg.citerations = iter;
}

void csBulletSoftBody::SetShapeMatching (bool match)
{
  if (match)
    btBody->setPose (false,true);
  else
    btBody->setPose (true,false);
}

void csBulletSoftBody::SetBendingConstraint (bool bending)
{
  if (bending)
  {
    btSoftBody::Material*	pm=btBody->m_materials[0];
    btBody->generateBendingConstraints (2,pm);
  }
}

void csBulletSoftBody::GenerateCluster (int iter)
{
  if (btBody->m_cfg.collisions & (btSoftBody::fCollision::CL_RS + btSoftBody::fCollision::CL_SS))
    btBody->generateClusters(iter);
}

void csBulletSoftBody::UpdateAnchorPositions ()
{
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); )
  {
    AnimatedAnchor& anchor = it.Next ();
    anchor.position = CSToBullet (anchor.controller->GetAnchorPosition (), system->getInternalScale ());
  }
}

void csBulletSoftBody::UpdateAnchorInternalTick (btScalar timeStep)
{
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); )
  {
    AnimatedAnchor& anchor = it.Next ();

    btVector3 delta = anchor.position - btBody->m_nodes[anchor.vertexIndex].m_x;
    static const btScalar maxdrag = 10;
    if (delta.length2 () > maxdrag * maxdrag)
      delta = delta.normalized() * maxdrag;
    btBody->m_nodes[anchor.vertexIndex].m_v += delta / timeStep;
  }  
}
}
CS_PLUGIN_NAMESPACE_END (Bullet2)