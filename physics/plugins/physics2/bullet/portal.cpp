#include "cssysdef.h"

#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"
#include "csgeom/sphere.h"

#include "ivaria/collisions.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

//#include "common2.h"
#include "portal.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "joint2.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

  csBulletCollisionPortal::csBulletCollisionPortal
    (iPortal* portal, const csOrthoTransform& meshTrans, csBulletSector* sourceSector)
    : portal (portal), sourceSector (sourceSector)
  {
    // set sector
    targetSector = dynamic_cast<csBulletSector*>(sourceSector->sys->GetCollisionSector(portal->GetSector()));

    // Create a ghost collisder for the portal
    ghostPortal = new btGhostObject ();

    // compute the size of the portal
    // WARNING: GetWorldVertices() initializes the portal plane - Always call this before GetObjectPlane()!
    const csVector3* vert = portal->GetWorldVertices ();
    csVector3 portalPos(0);
    csBox3 box;
    for (int i = 0; i < portal->GetVertexIndicesCount (); i++)
    {
      csVector3 v(vert[portal->GetVertexIndices ()[i]]);
      portalPos += v;
      box.AddBoundingVertex (v);
    }

    portalPos /= portal->GetVertexIndicesCount();

    // Compute the warp transform of the portal
    warpTrans = portal->GetWarp ().GetInverse ();
    warpTrans = meshTrans.GetInverse () * warpTrans * meshTrans;
    csVector3 normal = portal->GetObjectPlane ().GetNormal ();
    normal = meshTrans.GetT2O() * normal;              // rotate normal

    float maxEdge = csMax (box.GetSize ().x, box.GetSize ().y);
    maxEdge = csMax (maxEdge, box.GetSize ().z);
    float thresh = maxEdge * 0.05f;
    
    // place the portal at the center of the box that represents it
    csVector3 size = 0.499f * box.GetSize ();           // compute "flat" box size
    size = meshTrans.GetT2O() * size;                   // rotate box
    size += normal * thresh;                            // add thickness to box
    csVector3 centerDist = (size * normal) * normal;    // move origin into the center of the box
    
    csOrthoTransform realTrans;                         // no rotation necessary, since we already rotated everything
    realTrans.SetOrigin(portalPos + meshTrans.GetOrigin() + centerDist);
    ghostPortal->setWorldTransform (CSToBullet (realTrans, sourceSector->sys->getInternalScale ()));

    // give the portal it's shape and add it to the world
    btCollisionShape* shape = new btBoxShape (CSToBullet (size, sourceSector->sys->getInternalScale ()));

    ghostPortal->setCollisionShape (shape);
    ghostPortal->setCollisionFlags (ghostPortal->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    
    sourceSector->bulletWorld->addCollisionObject (ghostPortal, sourceSector->sys->collGroups[CollisionGroupTypePortal].value, sourceSector->sys->collGroups[CollisionGroupTypePortal].mask);
  }

  csBulletCollisionPortal::~csBulletCollisionPortal ()
  {
    if (ghostPortal) 
    {
      delete ghostPortal->getCollisionShape ();
      delete ghostPortal;
    }
  }

  bool csBulletCollisionPortal::CanTraverse(csBulletCollisionObject* obj)
  {
    // Only actors and dynamic objects can traverse portals
    return obj->QueryActor() || (obj->GetObjectType () == COLLISION_OBJECT_PHYSICAL && obj->QueryPhysicalBody()->IsDynamic());
  }

  void csBulletCollisionPortal::UpdateCollisions (csBulletSector* sector)
  {
    if (!targetSector)
    {
      targetSector = dynamic_cast<csBulletSector*>(sourceSector->sys->GetCollisionSector(portal->GetSector()));
    }

    csRefArray<csBulletCollisionObject> oldObjects (objects);
    csArray<csOrthoTransform> oldTrans (transforms);
    objects.Empty ();
    transforms.Empty ();
    btVector3 aabbMin2, aabbMax2;

    ghostPortal->getCollisionShape()->getAabb(ghostPortal->getWorldTransform (),aabbMin2,aabbMax2);

    for (int j = 0; j < ghostPortal->getNumOverlappingObjects (); j++)
    {
      //btTransform tran = ghostPortal->getWorldTransform ();
      btCollisionObject* btObj = ghostPortal->getOverlappingObject (j);
      iCollisionObject* iObj = static_cast<iCollisionObject*> (btObj->getUserPointer ());
      csBulletCollisionObject* obj = dynamic_cast<csBulletCollisionObject*> (iObj);

      //for (size_t k = 0; k < sector->GetCollisionObjectCount(); ++k)
      //{
      //  iCollisionObject* ooo = sector->GetCollisionObject(k);
      //  btCollisionObject* btooo = dynamic_cast<csBulletCollisionObject*>(ooo)->GetBulletCollisionPointer();
      //  //bool collliides = ghostPortal->col(btooo);
      //  bool aaactive = btooo->isActive();
      //  csString nnn = ooo->QueryObject()->GetName();
      //  nnn = obj->QueryObject()->GetName();
      //}

      if (!obj || !CanTraverse(obj)) continue;
      
      csBulletCollisionObject* newObject;
      
      // Phsical objects can traverse portals
      if (obj->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
      {
        iPhysicalBody* pb = obj->QueryPhysicalBody ();
        if (pb->GetPhysicalObjectType () == PHYSICAL_OBJECT_SOFTYBODY)
        {
          //use AABB
          btVector3 aabbMin, aabbMax;
          csBulletSoftBody* sb = dynamic_cast<csBulletSoftBody*> (pb);
          if (sb->anchorCount != 0)
            continue;
          
          if (sb->joints.GetSize () != 0)
          {
            // TODO: Softbodies with joints can't traverse portals
            sb->SetLinearVelocity (csVector3 (0.0f));
            continue;
          }
          
          sb->btBody->getAabb (aabbMin, aabbMax);
          
          btVector3 bodyCenter = (aabbMin + aabbMax)/2.0f;
          btVector3 dis = ghostPortal->getWorldTransform ().getOrigin () - bodyCenter;

          //csVector3 center = BulletToCS (dis * 2.0f, sys->getInverseInternalScale ());
          // Do not use SetTransform...Use transform in btSoftBody.
          btVector3 norm = CSToBullet (portal->GetObjectPlane ().GetNormal (), 1.0f);

          // TODO: Transform normal
          float length = dis.dot(norm);
          
          btTransform tr;
          tr.setIdentity ();
          tr.setOrigin (dis + norm * length * 1.2f);

          if (portal->GetSector () != sector->GetSector())
          {
            // Move the body to the new sector.
            sector->RemoveCollisionObject (sb);
            targetSector->AddCollisionObject (sb);
          }
          sb->SetLinearVelocity (warpTrans.GetT2O () * sb->GetLinearVelocity ());
          tr = CSToBullet (warpTrans, sector->sys->getInternalScale ()) * tr;
          sb->btBody->transform (tr);
          continue;
        }
        else
        {
          //Check if this is a joint chain.
          csBulletRigidBody* firstBody = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());
          if (firstBody->anchorCount != 0)
            continue;
          if (firstBody->joints.GetSize () != 0)
          {
            btVector3 aabbMin, aabbMax;
            firstBody->btBody->getAabb (aabbMin, aabbMax);
            if (!TestAabbAgainstAabb2 (aabbMin, aabbMax, aabbMin2, aabbMax2))
              continue;
            csArray<iPhysicalBody*> rbs;
            csArray<csBulletJoint*> jnts;
            bool doTrans = true;
            size_t head, end;
            head = 0;
            end = 1;
            rbs.Push ((csPhysicalBody*)firstBody);

            csBox3 box;
            while (head < end)
            {
              if (rbs[head] == nullptr)
              {
                // Only attached one body on this joint. This should not be transmitted.
                doTrans = false;
                break;
              }
              else if (rbs[head]->GetPhysicalObjectType () == PHYSICAL_OBJECT_SOFTYBODY)
              {
                // Soft joint should not be transmitted.
                doTrans = false;
                firstBody->SetLinearVelocity (csVector3 (0.0f));
                firstBody->SetAngularVelocity (csVector3 (0.0f));
                break;
              }
              else if (rbs[head]->QueryRigidBody ()->GetState () == STATE_STATIC)
              {
                // If there's a static body in the chain, the chain should not be transmited.
                // Just ignore the motion.
                doTrans = false;
                break;
              }
              csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*>(rbs[head]->QueryRigidBody ());
              rb->btBody->getAabb (aabbMin, aabbMax);
              box.AddBoundingVertex (csVector3 (aabbMax.x(), aabbMax.y(), aabbMax.z()));
              box.AddBoundingVertex (csVector3 (aabbMin.x(), aabbMin.y(), aabbMin.z()));

              for (size_t k = 0; k < rb->joints.GetSize (); k ++)
              {
                csBulletJoint* jnt = dynamic_cast<csBulletJoint*> (rb->joints[k]);
                size_t index = jnts.Find (jnt);
                if (index != csArrayItemNotFound)
                  continue;

                jnts.Push (jnt);
                iPhysicalBody* otherBody;
                if (jnt->bodies[0] != rb->QueryPhysicalBody ())
                  otherBody = jnt->bodies[0];
                else
                  otherBody = jnt->bodies[1];

                rbs.Push (otherBody);
                end++;
              }
              head ++;
            } // (While): Finish a body chain.

            if (doTrans)
            {
              btVector3 center = btVector3 (box.GetCenter ().x, box.GetCenter ().y, box.GetCenter ().z);

              btTransform tr = ghostPortal->getWorldTransform ();
              btVector3 dis = tr.getOrigin () - center;

              btVector3 norm = CSToBullet (portal->GetObjectPlane ().GetNormal (), 1.0f);
              float length = dis.dot(norm);
              dis += norm * length * 1.2f;

              //TODO: Remove this to test transmit to a different sector.
              if (portal->GetSector () != sector->GetSector())
              {
                // Move the body to the new sector.
                for (size_t l = 0; l < jnts.GetSize (); l++)
                  sector->RemoveJoint (jnts[l]);
                for (size_t l = 0; l < rbs.GetSize (); l++)
                  sector->RemoveCollisionObject (rbs[l]->QueryRigidBody ());
                for (size_t l = 0; l < rbs.GetSize (); l++)
                  targetSector->AddCollisionObject (rbs[l]->QueryRigidBody ());
                for (size_t l = 0; l < jnts.GetSize (); l++)
                {
                  jnts[l]->RebuildJoint ();
                  targetSector->AddJoint (jnts[l]);
                }
              }

              csVector3 offset = BulletToCS (dis, sector->sys->getInverseInternalScale ());
              for (size_t l = 0; l < rbs.GetSize (); l++)
              {
                csOrthoTransform trans = rbs[l]->GetTransform ();
                trans.SetOrigin (trans.GetOrigin () + offset);
                trans *= warpTrans;
                rbs[l]->SetTransform (trans);
                rbs[l]->QueryRigidBody ()->SetLinearVelocity (
                  warpTrans.GetT2O () * rbs[l]->GetLinearVelocity ());
                rbs[l]->QueryRigidBody ()->SetAngularVelocity (
                  warpTrans.GetT2O () * rbs[l]->QueryRigidBody ()->GetAngularVelocity ());
              }
            }
            continue;
          } // Has joint.
        } // Rigid body.
      } // Physical body.

      // Single object.
      size_t index = oldObjects.Find (obj);
      if (index != csArrayItemNotFound)
      {
        // Check if it has traversed the portal
        csOrthoTransform transform = obj->GetTransform ();
        csVector3 newPosition = transform.GetOrigin ();
        //csVector3 oldPosition = oldTrans[index].GetOrigin ();
        csVector3 orien1 = newPosition - portal->GetWorldSphere ().GetCenter ();
        //csVector3 orien2 = oldPosition - portal->GetWorldSphere ().GetCenter ();
        float a = orien1 * portal->GetWorldPlane ().GetNormal ();
        //float b = orien2 * portal->GetWorldPlane ().GetNormal ();
        if ( a >= 0)
        {
          transform = transform * warpTrans;
          if (portal->GetSector () != sector->GetSector())
          {
            // Move the body to the new sector.
            targetSector->RemoveCollisionObject (obj->objectCopy);
            sector->RemoveCollisionObject (obj);
            targetSector->AddCollisionObject (obj);
          }
          if (obj->QueryPhysicalBody() && obj->QueryPhysicalBody()->QueryRigidBody())
          {
            iPhysicalBody* pb = obj->QueryPhysicalBody ();
            csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());
            rb->SetLinearVelocity (warpTrans.GetT2O () * rb->GetLinearVelocity ());
            rb->SetAngularVelocity (warpTrans.GetT2O () * rb->GetAngularVelocity ()); 
            /*rb->SetLinearDamping (rb->GetLinearDamping ());
            rb->SetAngularDamping (rb->GetAngularDamping ());*/
          }
          obj->SetTransform (transform);

          continue;
        }
        else
        {
          AddObject (obj);
          newObject = obj->objectCopy;
        }

      }
      // Create a new copy.
      else
      {
        AddObject (obj);
        if (obj->IsPhysicalObject())
        {
          btVector3 localInertia (0.0f, 0.0f, 0.0f);
          iPhysicalBody* pb = obj->QueryPhysicalBody ();
          if (pb->QueryRigidBody())
          {
            csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());

            csRef<iRigidBodyFactory> factory = sector->sys->CreateRigidBodyFactory(rb->GetCollider());
            
            factory->SetDensity (rb->GetDensity());
            factory->SetFriction (rb->GetFriction());
            factory->SetElasticity (rb->GetElasticity());
            factory->SetLinearDamping (0.5f);
            factory->SetAngularDamping (0.5f);

            factory->SetCollisionGroup (sector->GetSystem()->FindCollisionGroup("Portal"));

            csRef<iRigidBody> inb = factory->CreateRigidBody();

            csBulletRigidBody* newBody = dynamic_cast<csBulletRigidBody*> ((iRigidBody*)inb);

            // Eliminate gravity and terrain influences, since we assume that the object is still "standing" on "this side" of the portal
            newBody->btBody->setGravity (btVector3 (0.0,0.0,0.0));
            newObject = newBody;
            
            targetSector->AddCollisionObject (newBody);

            CS_ASSERT (!rb->objectCopy);
            rb->objectCopy = newBody;
            newBody->objectOrigin = rb;
          }
          else
          {
            //TODO Soft Body
          }
        }
        else if (obj->GetObjectType () == COLLISION_OBJECT_GHOST || obj->GetObjectType () == COLLISION_OBJECT_ACTOR)
        {
          csRef<iGhostCollisionObjectFactory> factory = 
            sector->sys->CreateGhostCollisionObjectFactory(obj->GetCollider(), "ghost copy");

          factory->SetCollisionGroup (sector->sys->FindCollisionGroup("Portal"));
          
          csRef<iGhostCollisionObject> co = factory->CreateGhostCollisionObject();
          
          newObject = dynamic_cast<csBulletCollisionObject*> ((iCollisionObject*)co);


          // TODO: When traversing forth and back, it might still have a copy that is saved in the other portal
          CS_ASSERT (!obj->objectCopy);
          obj->objectCopy = newObject;
          newObject->objectOrigin = obj;

          targetSector->AddCollisionObject (co);
        }
        CSToBullet (warpTrans.GetO2T ()).getRotation (newObject->portalWarp);
      }

      // And set the transform and record old transforms.
      SetInformationToCopy (obj, newObject, warpTrans);
      if (obj->movable)
      {
        obj->movable->GetSceneNode ()->QueryMesh ()->PlaceMesh ();
      }
      transforms.Push (obj->GetTransform ());
    }

    // Remove the rest objects from portal list. 
    for (size_t j = 0; j < oldObjects.GetSize (); j ++)
    {
      size_t index = objects.Find (oldObjects[j]);
      if (index == csArrayItemNotFound)
        targetSector->RemoveCollisionObject (oldObjects[j]->objectCopy );
    }
  }

  void csBulletCollisionPortal::SetInformationToCopy (csBulletCollisionObject* obj, 
    csBulletCollisionObject* cpy, 
    const csOrthoTransform& warpTrans)
  {
    // TODO warp the transform.
    if (!obj || !cpy )
      return;

    csOrthoTransform trans = obj->GetTransform () * warpTrans;

    btTransform btTrans = CSToBullet (trans.GetInverse (), sourceSector->sys->getInternalScale ());

    if (obj->IsPhysicalObject())
    {
      iPhysicalBody* pb = obj->QueryPhysicalBody ();
      if (pb->QueryRigidBody())
      {
        csBulletRigidBody* btCopy = dynamic_cast<csBulletRigidBody*> (cpy->QueryPhysicalBody ()->QueryRigidBody ());
        csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());

        if (rb->GetState () == STATE_DYNAMIC)
        {
          btQuaternion rotate;
          CSToBullet (warpTrans.GetT2O ()).getRotation (rotate);
          btCopy->btBody->setLinearVelocity (quatRotate (rotate, rb->btBody->getLinearVelocity ()));
          btCopy->btBody->setAngularVelocity (quatRotate (rotate, rb->btBody->getAngularVelocity ()));
        }
        btCopy->SetTransform (trans);
      }
      else
      {
        //TODO Soft Body
      }
    }
    else
    {
      cpy->btObject->setWorldTransform (btTrans);
    }
  }

  void csBulletCollisionPortal::GetInformationFromCopy (csBulletCollisionObject* obj, 
    csBulletCollisionObject* cpy, 
    float duration)
  {
    // TODO Warp the velocity.
    if (obj->IsPhysicalObject())
    {
      iPhysicalBody* pb = obj->QueryPhysicalBody ();
      if (pb->QueryRigidBody())
      {
        csBulletRigidBody* btCopy = dynamic_cast<csBulletRigidBody*> (cpy->QueryPhysicalBody ()->QueryRigidBody ());
        csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());
        if (rb->GetState () == STATE_DYNAMIC)
        {
          btQuaternion qua = cpy->portalWarp;

          rb->btBody->internalGetDeltaLinearVelocity () = quatRotate (qua, btCopy->btBody->internalGetDeltaLinearVelocity ())
            - rb->btBody->internalGetDeltaLinearVelocity ();
          rb->btBody->internalGetDeltaAngularVelocity () = quatRotate (qua, btCopy->btBody->internalGetDeltaAngularVelocity ())
            - rb->btBody->internalGetDeltaAngularVelocity ();
          rb->btBody->internalGetPushVelocity ()= quatRotate (qua, btCopy->btBody->internalGetPushVelocity ())
            - rb->btBody->internalGetPushVelocity ();
          rb->btBody->internalGetTurnVelocity ()= quatRotate (qua, btCopy->btBody->internalGetTurnVelocity ())
            - rb->btBody->internalGetTurnVelocity ();
          // I don't know if there are any other parameters exist.
          rb->btBody->internalWritebackVelocity (duration);
          //rb->btBody->getMotionState ()->setWorldTransform (rb->btBody->getWorldTransform ());

          //csOrthoTransform trans = obj->GetTransform () * warpTrans;
          //cpy->SetTransform (trans);
        }
        else if (rb->GetState () == STATE_KINEMATIC)
        {
          //Nothing to do?
        }
      }
      else
      {
        //TODO Soft Body
      }
    }
    else
    {
      //btPairCachingGhostObject* ghostCopy = btPairCachingGhostObject::upcast (cpy);
      //btPairCachingGhostObject* ghostObject = btPairCachingGhostObject::upcast (obj->btObject);

      // Need to think about the implementation of actor.
    }
  }

}
CS_PLUGIN_NAMESPACE_END(Bullet2)
