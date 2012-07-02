/**
 * Create and handle dynamic (interacting) objects
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"
#include "physdemo.h"

#include "ivaria/colliders.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

void PhysDemo::CreateGhostCylinder()
{
  // Create the cylinder mesh factory.
  csRef<iMeshFactoryWrapper> cylinderFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "cylinderFact");

  if (!cylinderFact)
  {
    ReportError ("Error creating mesh object factory!");
    return;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (cylinderFact->GetMeshObjectFactory());

  const float radius (1.5f);
  const float length (4.0f);
  gmstate->GenerateCylinder (length, radius, 10);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (cylinderFact, "cylinder"));

  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh
  csRef<CS::Collisions::iColliderCylinder> cylinder = physicalSystem->CreateColliderCylinder (length, radius);
  GhostCollisionObjectProperties props(cylinder);
  ghostObject = physicalSystem->CreateGhostCollisionObject(&props);
  
  csYRotMatrix3 m (PI/2.0);
  csOrthoTransform trans (m, csVector3 (0, -3, 5));
  if (this->environment == ENVIRONMENT_TERRAIN)
    trans.SetOrigin (csVector3 (0, 1.0, 5));
  ghostObject->SetTransform (trans);
  ghostObject->SetAttachedMovable (mesh->GetMovable());

  // It won't work for ghost and actor.
  ghostObject->QueryObject()->SetName("ghostObject");
  ghostObject->Rotate(csVector3(0, 1, 0), PI/2.0);
  //ghostObject->AddCollider (cylinder, trans)

  physicalSector->AddCollisionObject (ghostObject);
}

void PhysDemo::CreateBoxCollider (csRef<CS::Collisions::iColliderBox>& collider, csRef<iMeshWrapper>& mesh, const csVector3& extents)
{
  DensityTextureMapper mapper (0.3f);
  TesselatedBox tbox (-extents/2, extents/2);
  tbox.SetLevel (3);
  tbox.SetMapper (&mapper);

  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
  {
    ReportWarning ("Could not load texture %s", CS::Quote::Single ("stone"));
  }

  mesh = GeneralMeshBuilder::CreateFactoryAndMesh (engine, room, "walls", "walls_factory", &tbox);
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("stone");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create and attach a box collider
  collider = physicalSystem->CreateColliderBox (extents);

}

CS::Physics::iRigidBody* PhysDemo::SpawnBox (bool setVelocity /* = true */)
{
  csVector3 extents(0.4f, 0.8f, 0.4f);
  csVector3 pos = view->GetCamera()->GetTransform().GetOrigin() + GetCameraDirection();
  return SpawnBox(extents, pos, 15, setVelocity);
}

CS::Physics::iRigidBody* PhysDemo::SpawnBox (const csVector3& extents, const csVector3& pos, float mass, bool setVelocity /* = true */)
{
  static csRandomFloatGen randGen;

  // Create a box collider & mesh
  csRef<iColliderBox> box;
  csRef<iMeshWrapper> mesh;
  CreateBoxCollider(box, mesh, extents);

  return SpawnRigidBody(box, mesh, pos, mass, setVelocity);
}

CS::Physics::iRigidBody* PhysDemo::SpawnRigidBody (
  CS::Collisions::iCollider* collider,
  iMeshWrapper* mesh, 
  const csVector3& pos, 
  float mass, 
  bool setVelocity /* = true */)
{
  static csRandomFloatGen randGen;
  
  // Create a body
  RigidBodyProperties props(collider, mesh->QueryObject ()->GetName ());
  props.SetMass (mass);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);
  
  // Set transform, attach mesh and add to world
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();
  csOrthoTransform trans = tc;
  trans.RotateThis(UpVector, randGen.GetAngle());
  trans.SetOrigin (pos);
  rb->SetTransform (trans);

  rb->SetAttachedMovable (mesh->GetMovable());
  physicalSector->AddCollisionObject (rb);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnSphere (bool setVelocity /* = true */)
{
  return SpawnSphere(view->GetCamera()->GetTransform().GetOrigin() + GetCameraDirection(), rand()%5/10. + .2, setVelocity);
}

CS::Physics::iRigidBody* PhysDemo::SpawnSphere (const csVector3& pos, float radiusf, bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the ball mesh factory.
  csRef<iMeshFactoryWrapper> ballFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "ballFact");
  if (!ballFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (ballFact->GetMeshObjectFactory());

  csVector3 radius (radiusf, radiusf, radiusf);
  csEllipsoid ellips (csVector3 (0), radius);
  gmstate->GenerateSphere (ellips, 16);

  // We do a hardtransform here to make sure our sphere has an artificial
  // offset. That way we can test if the physics engine supports that.
  csMatrix3 m;
  csVector3 artificialOffset (0, 0, 0);
  csReversibleTransform t = csReversibleTransform (m, artificialOffset);
  ballFact->HardTransform (t);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (ballFact, "ball"));

  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh and attach a sphere collider.
  csRef<CS::Collisions::iColliderSphere> sphere = physicalSystem->CreateColliderSphere (1.0);
  sphere->SetLocalScale (radius);
  RigidBodyProperties props(sphere, "sphere");
  props.SetDensity (10.0f);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);
  
  rb->SetAttachedMovable (mesh->GetMovable());
  csOrthoTransform trans = tc;
  trans.SetOrigin (pos);
  rb->SetTransform (trans);
  
  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }

  physicalSector->AddCollisionObject (rb);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCone (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  const float radius (0.4f);
  const float length (0.8f);

  // We do a hardtransform here to make sure our cylinder has an artificial
  // offset. That way we can test if the physics engine supports that.
 
  // Create a body and attach the mesh and attach a cone collider.
  csRef<CS::Collisions::iColliderCone> cone = physicalSystem->CreateColliderCone (length, radius);
  cone->SetLocalScale (csVector3 (rand()%5/10. + .2, rand()%5/10. + .2, rand()%5/10. + .2));

  // Create object
  RigidBodyProperties props(cone, "cone");
  props.SetDensity (10.0f);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);

  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);

  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  rb->SetTransform (trans);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }

  // TODO: Attach a mesh
  physicalSector->AddCollisionObject (rb);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCylinder (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the cylinder mesh factory.
  csRef<iMeshFactoryWrapper> cylinderFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "cylinderFact");
  if (!cylinderFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (cylinderFact->GetMeshObjectFactory());
  const float radius (rand() % 10 / 50. + .2);
  const float length (rand() % 3 / 50. + .7);
  gmstate->GenerateCylinder (length, radius, 10);

  // We do a hardtransform here to make sure our cylinder has an artificial
  // offset. That way we can test if the physics engine supports that.
  //csVector3 artificialOffset (3, 3, 3);
  csVector3 artificialOffset (0, 0, 0);
  csReversibleTransform hardTransform (csYRotMatrix3 (PI/2.0), artificialOffset);
  cylinderFact->HardTransform (hardTransform);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    cylinderFact, "cylinder"));

  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh.
  csRef<CS::Collisions::iColliderCylinder> cylinder = physicalSystem->CreateColliderCylinder (length, radius);
  csMatrix3 m;

  RigidBodyProperties props(cylinder, "cylinder");
  props.SetDensity (10.0f);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);

  csOrthoTransform trans = tc;
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));
  rb->SetTransform (trans);
  rb->SetAttachedMovable (mesh->GetMovable());

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }
  
  rb->RebuildObject();
  physicalSector->AddCollisionObject (rb);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCapsule (float length, float radius, bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the capsule mesh factory.
  csRef<iMeshFactoryWrapper> capsuleFact = engine->CreateMeshFactory (
    "crystalspace.mesh.object.genmesh", "capsuleFact");
  if (!capsuleFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate =
    scfQueryInterface<iGeneralFactoryState> (capsuleFact->GetMeshObjectFactory());
  gmstate->GenerateCapsule (length, radius, 10);
  capsuleFact->HardTransform (
    csReversibleTransform (csYRotMatrix3 (PI/2), csVector3 (0)));

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    capsuleFact, "capsule"));
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create a body
  csRef<CS::Collisions::iColliderCapsule> capsule = physicalSystem->CreateColliderCapsule (length, radius);
  RigidBodyProperties props(capsule, "capsule");
  props.SetDensity (10.0f);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);

  // set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  rb->SetTransform (trans);

  // attach the mesh
  rb->SetAttachedMovable (mesh->GetMovable());

  // Add to world
  physicalSector->AddCollisionObject (rb);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }
  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Collisions::iCollisionObject* PhysDemo::SpawnConcaveMesh()
{
  // Find the 'star' mesh factory
  csRef<iMeshFactoryWrapper> starFact;
  starFact = engine->FindMeshFactory ("genstar");
  if (!starFact)
  {
    loader->Load ("/lib/std/star.xml");
    starFact = engine->FindMeshFactory ("genstar");
    if (!starFact)
    {
      ReportError ("Error loading %s!", CS::Quote::Single ("star.xml"));
      return nullptr;
    }
  }

  // Use the camera transform.
  csOrthoTransform tc = view->GetCamera()->GetTransform();
  tc.SetOrigin (tc.This2Other (csVector3 (0, 0, 3)));

  // Create the mesh.
  csRef<iMeshWrapper> star = engine->CreateMeshWrapper (starFact, "star");
  star->GetMovable()->SetTransform (tc);
  star->GetMovable()->UpdateMove();

  csRef<CS::Collisions::iCollider> starCollider;
  if (mainCollider == nullptr)
  {
    mainCollider = physicalSystem->CreateColliderConcaveMesh (star);
    starCollider = mainCollider;
  }
  else
  {
    starCollider = csRef<CS::Collisions::iColliderConcaveMeshScaled>(physicalSystem->CreateColliderConcaveMeshScaled (mainCollider, 1.0f));
  }  

  // create body
  RigidBodyProperties props(starCollider);
  props.SetName("star");
  csRef<CS::Physics::iRigidBody> co = physicalSystem->CreateRigidBody(&props);

  // set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 2));
  co->SetAttachedMovable (star->GetMovable());
  co->SetTransform (trans);
  
  physicalSector->AddCollisionObject (co);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return co;
}

CS::Physics::iRigidBody* PhysDemo::SpawnConvexMesh (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the mesh factory (a capsule in this example)
  csRef<iMeshFactoryWrapper> capsuleFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "capsuleFact");
  if (!capsuleFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (capsuleFact->GetMeshObjectFactory());
  const float radius (rand() % 10 / 50. + .2);
  const float length (rand() % 3 / 50. + .7);
  gmstate->GenerateCapsule (length, radius, 10);
  capsuleFact->HardTransform (
    csReversibleTransform (csYRotMatrix3 (PI/2), csVector3 (0)));

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    capsuleFact, "capsule"));
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh.
  csRef<CS::Collisions::iColliderConvexMesh> collider = physicalSystem->CreateColliderConvexMesh (mesh);
  RigidBodyProperties props(collider, "convexmesh");
  props.SetDensity (10.0f);
  props.SetElasticity (0.8f);
  props.SetFriction (10.0f);
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);

  // Set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));
  rb->SetTransform (trans);
  
  // Attach mesh
  rb->SetAttachedMovable (mesh->GetMovable());
  
  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }

  // Add to world
  physicalSector->AddCollisionObject (rb);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCompound (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (meshFact, "mesh"));

  csRef<CS::Collisions::iColliderCompound> rootCollider = physicalSystem->CreateColliderCompound();
  physicalSystem->DecomposeConcaveMesh (&*rootCollider, mesh, true);

  // Create a body
  RigidBodyProperties props(rootCollider, "compound");
  csRef<CS::Physics::iRigidBody> rb = physicalSystem->CreateRigidBody(&props);

  // Set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 2));
  rb->SetTransform (trans);
  rb->SetAttachedMovable (mesh->GetMovable());

  // Add to world
  physicalSector->AddCollisionObject (rb);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 0, 0));
  }

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();

  return rb;
}

CS::Physics::iJoint* PhysDemo::SpawnJointed()
{
#define SOFT_ANGULAR

#ifdef P2P
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.5f, 0.5f, 0.0f));
  rb1->SetTransform (trans);
  csVector3 jointPosition = trans.This2Other(csVector3(-0.2f, 0.4f, 0.2f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.0f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);
  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateRigidP2PJoint (jointPosition);
  joint->Attach (rb2, rb1);
#endif

#ifdef CONETWIST
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.0f, 1.0f, 0.0f));
  rb1->SetTransform (trans);
  csVector3 jointPosition = trans.This2Other(csVector3(-0.2f, 0.4f, 0.2f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform();
  csOrthoTransform jointTrans = trans;
  jointTrans.SetO2T (csZRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.0f, -1.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);
  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateRigidConeTwistJoint (jointTrans,
    PI * 0.15f, PI * 0.25f, PI * 0.8f);
  joint->Attach (rb1, rb2);
#endif

#ifdef HINGE
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + csVector3 (1.0f, 2.0f, 0.0f));
  rb1->SetTransform (trans);
  rb1->SetLinearVelocity (csVector3 (0.0f));
  rb1->SetAngularVelocity (csVector3 (0.0f));
  csVector3 jointPosition = trans.This2Other(csVector3(0.2f, -0.2f, 0.2f));
  // Create a joint and attach the two bodies to it.
  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateRigidHingeJoint (jointPosition,
  PI, -PI, 1);
  // Add a motor to the joint
  joint->SetDesiredVelocity (csVector3 (0.0f, -0.5f, 0.0f));
  joint->SetMaxForce (csVector3 (0.0f, 1.0f, 0.0f));
  joint->Attach (rb1, nullptr/* rb2, true*/);
#endif

#ifdef SLIDE
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform();
  csOrthoTransform jointTrans = trans;

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (-0.5f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);
  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateRigidSlideJoint (jointTrans, -1.f, 1.f, 1.f, -1.f, 0);
  joint->Attach (rb2, rb1);
#endif

#ifdef SPRING
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform jointTrans = rb1->GetTransform();
  jointTrans.SetOrigin (jointTrans.GetOrigin() + jointTrans.GetT2O() * csVector3 (0.0f, 0.0f, 0.0f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  csOrthoTransform trans = rb2->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (-1.0f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);

  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateJoint();
  joint->SetTransform (jointTrans);
  joint->SetSpring (true);
  joint->SetTransConstraints (true, true, true);
  joint->SetMinimumDistance (csVector3(-1.0f, 0.0f, 0.0f));
  joint->SetMaximumDistance (csVector3(1.0f, 0.0f, 0.0f));
  joint->SetLinearStiffness (csVector3(9.478f, 0.0f, 0.0f));
  joint->SetLinearDamping (csVector3(0.5f, 0.0f, 0.0f));
  joint->SetRotConstraints (true, true, true);
  joint->SetMinimumAngle (csVector3(-PI/2.0f, 0.0f, 0.0f));
  joint->SetMaximumAngle (csVector3(PI/2.0f, 0.0f, 0.0f));
  joint->SetAngularStiffness (csVector3(9.478f, 0.0f, 0.0f));
  joint->SetAngularDamping (csVector3(0.5f, 0.0f, 0.0f));
  joint->Attach (rb2, rb1);
#endif

#ifdef SOFT_LINEAR
  csRef<CS::Physics::iSoftBody> sb1 = SpawnSoftBody (false);
  csRef<CS::Physics::Bullet2::iSoftBody> bulletSoftBody = scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (sb1);
  bulletSoftBody->ResetCollisionFlag();
  // The soft body need to use cluster collision.
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (32);
  
  //CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  //csOrthoTransform trans = rb2->GetTransform();
  //trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.0f,2.0f,0.0f));
  //rb2->SetTransform (trans);
  //rb2->SetLinearDamping (0.8f);
  //rb2->SetAngularDamping (0.8f);

  //Or you can attach it with a softbody.
  CS::Physics::iSoftBody* rb2 = SpawnSoftBody (false);
  // SetTransform can only be called once in iSoftBody. And this is a local transform.
  csOrthoTransform trans = rb2->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + csVector3 (0.0f,1.0f,-0.5f));
  rb2->SetTransform (trans);
  bulletSoftBody = scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (rb2);
  bulletSoftBody->ResetCollisionFlag();
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (32);

  //Use local position when it's soft joint.
  csRef<CS::Physics::iJoint> joint = 
    physicalSystem->CreateSoftLinearJoint (rb2->GetVertexPosition (0));
  joint->Attach (sb1, rb2);
#endif

#ifdef SOFT_ANGULAR
  csRef<CS::Physics::iSoftBody> sb1 = SpawnSoftBody (false);
  csRef<CS::Physics::Bullet2::iSoftBody> bulletSoftBody = scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (sb1);
  bulletSoftBody->ResetCollisionFlag();
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (16);

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  csOrthoTransform trans = rb2->GetTransform();
  trans.SetOrigin (trans.GetOrigin() + trans.GetT2O() * csVector3 (0.0f,2.0f,0.0f));
  rb2->SetTransform (trans);
  rb2->SetLinearDamping (0.8f);
  rb2->SetAngularDamping (0.8f);

  csRef<CS::Physics::iJoint> joint = physicalSystem->CreateSoftAngularJoint (1);
  joint->Attach (sb1, rb2);
#endif

  physicalSector->AddJoint (joint);
  return joint;
}

CS::Physics::iRigidBody* PhysDemo::SpawnFilterBody (bool setVelocity)
{
  CS::Physics::iRigidBody* rb;
  if (rand() % 2)
  {
    rb = SpawnSphere (setVelocity);
    rb->SetCollisionGroup ("BoxFiltered");
  }
  else
  {
    rb = SpawnBox (setVelocity);
    // This is another way to avoid sphere collide with box.
    // We usually put these things in the same group.
    //rb->SetCollisionGroup ("BoxFiltered");
    rb->SetCollisionGroup ("Box");
  }

  return rb;
}

void ConstraintJoint (CS::Physics::iJoint* joint)
{
  // If the axis is constrained. You should set it with axis and pass min and max value to it.
  // Min = max means full constraint; min < max means move in the range. 
  // The translations are fully constrained.
  joint->SetTransConstraints (true, true, true);
  joint->SetMinimumDistance (csVector3 (0,0,0));
  joint->SetMaximumDistance (csVector3 (0,0,0));

  // The rotations are bounded
  joint->SetRotConstraints (true, true, true);
  joint->SetMinimumAngle (csVector3 (-PI/4.0, -PI/6.0, -PI/6.0));
  joint->SetMaximumAngle (csVector3 (PI/4.0, PI/6.0, PI/6.0));
}

void PhysDemo::SpawnChain()
{
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform();
  csVector3 initPos = trans.GetOrigin() + csVector3 (0.0f, 5.0f, 0.0f);
  trans.SetOrigin (initPos);
  rb1->SetTransform (trans);
  rb1->SetState (CS::Physics::STATE_STATIC);

  csVector3 offset (0.0f, 1.3f, 0.0f);

  CS::Physics::iRigidBody* rb2 = SpawnCapsule (0.4f, 0.3f, false);
  trans.SetO2T (csXRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (initPos - offset);
  rb2->SetTransform (trans);

  CS::Physics::iRigidBody* rb3 = SpawnBox (false);
  trans.Identity();
  trans.SetOrigin (initPos - 2.0f * offset);
  rb3->SetTransform (trans);

  CS::Physics::iRigidBody* rb4 = SpawnCapsule (0.4f, 0.3f, false);
  trans.SetO2T (csXRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (initPos - 3.0f * offset);
  rb4->SetTransform (trans);

  CS::Physics::iRigidBody* rb5 = SpawnBox (false);
  trans.Identity();
  trans.SetOrigin (initPos - 4.0f * offset);
  rb5->SetTransform (trans);

  // Create joints and attach bodies.
  csOrthoTransform jointTransform;
  csRef<CS::Physics::iJoint> joint;

  joint = physicalSystem->CreateJoint();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - csVector3 (0.0f, 0.6f, 0.0f));
  joint->SetTransform (jointTransform);
  joint->Attach (rb1, rb2, false);
  ConstraintJoint (joint);
  joint->RebuildJoint();
  physicalSector->AddJoint (joint);

  joint = physicalSystem->CreateJoint();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - csVector3 (0.0f, 0.6f, 0.0f) - offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb2, rb3, false);
  ConstraintJoint (joint);
  joint->RebuildJoint();
  physicalSector->AddJoint (joint);

  joint = physicalSystem->CreateJoint();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - csVector3 (0.0f, 0.6f, 0.0f) - 2.0f * offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb3, rb4, false);
  ConstraintJoint (joint);
  joint->RebuildJoint();
  physicalSector->AddJoint (joint);

  joint = physicalSystem->CreateJoint();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - csVector3 (0.0f, 0.6f, 0.0f) - 3.0f * offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb4, rb5, false);
  ConstraintJoint (joint);
  joint->RebuildJoint();
  physicalSector->AddJoint (joint);

  // Update the display of the dynamics debugger
  //dynamicsDebugger->UpdateDisplay();
}

void PhysDemo::LoadFrankieRagdoll()
{
  // Load animesh factory
  csLoadResult rc = loader->Load ("/lib/frankie/frankie.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Frankie!");
    return;
  }

  csRef<iMeshFactoryWrapper> meshfact = engine->FindMeshFactory ("franky_frankie");
  if (!meshfact)
    return;

  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory());
  if (!animeshFactory)
  {
    ReportError ("Can't find Frankie's animesh factory!");
    return;
  }

  // Load bodymesh (animesh's physical properties)
  rc = loader->Load ("/lib/frankie/skelfrankie_body");
  if (!rc.success)
  {
    ReportError ("Can't load Frankie's body description!");
    return;
  }

  csRef<CS::Animation::iBodyManager> bodyManager =
    csQueryRegistry<CS::Animation::iBodyManager> (GetObjectRegistry());
  CS::Animation::iBodySkeleton* bodySkeleton = bodyManager->FindBodySkeleton ("frankie_body");
  if (!bodySkeleton)
  {
    ReportError ("Can't find Frankie's body description!");
    return;
  }

  // Create bone chain
  CS::Animation::iBodyChain* chain = bodySkeleton->CreateBodyChain
    ("body_chain", animeshFactory->GetSkeletonFactory()->FindBone ("Frankie_Main"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("CTRL_Head"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("Tail_8"));

  // Create ragdoll animation node factory
  csRef<CS::Animation::iSkeletonRagdollNodeFactory2> ragdollFactory =
    ragdollManager->CreateAnimNodeFactory ("frankie_ragdoll");
  ragdollFactory->SetBodySkeleton (bodySkeleton);
  ragdollFactory->AddBodyChain (chain, CS::Animation::STATE_DYNAMIC);

  // Set the ragdoll anim node as the only node of the animation tree
  animeshFactory->GetSkeletonFactory()->GetAnimationPacket()
    ->SetAnimationRoot (ragdollFactory);
}

void PhysDemo::LoadKrystalRagdoll()
{
  // Load animesh factory
  csLoadResult rc = loader->Load ("/lib/krystal/krystal.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Krystal library file!");
    return;
  }

  csRef<iMeshFactoryWrapper> meshfact =
    engine->FindMeshFactory ("krystal");
  if (!meshfact)
  {
    ReportError ("Can't find Krystal's mesh factory!");
    return;
  }

  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory());
  if (!animeshFactory)
  {
    ReportError ("Can't find Krystal's animesh factory!");
    return;
  }

  // Load bodymesh (animesh's physical properties)
  rc = loader->Load ("/lib/krystal/skelkrystal_body");
  if (!rc.success)
  {
    ReportError ("Can't load Krystal's body mesh file!");
    return;
  }

  csRef<CS::Animation::iBodyManager> bodyManager =
    csQueryRegistry<CS::Animation::iBodyManager> (GetObjectRegistry());
  csRef<CS::Animation::iBodySkeleton> bodySkeleton = bodyManager->FindBodySkeleton ("krystal_body");
  if (!bodySkeleton)
  {
    ReportError ("Can't find Krystal's body mesh description!");
    return;
  }

  // Create bone chain
  CS::Animation::iBodyChain* chain = bodySkeleton->CreateBodyChain
    ("body_chain", animeshFactory->GetSkeletonFactory()->FindBone ("Hips"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("Head"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("RightFoot"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("LeftFoot"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("RightHand"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory()->FindBone ("LeftHand"));
  //chain->AddAllSubChains();

  // Create ragdoll animation node factory
  csRef<CS::Animation::iSkeletonRagdollNodeFactory2> ragdollFactory =
    ragdollManager->CreateAnimNodeFactory ("krystal_ragdoll");
  ragdollFactory->SetBodySkeleton (bodySkeleton);
  ragdollFactory->AddBodyChain (chain, CS::Animation::STATE_DYNAMIC);

  // Set the ragdoll anim node as the only node of the animation tree
  animeshFactory->GetSkeletonFactory()->GetAnimationPacket()
    ->SetAnimationRoot (ragdollFactory);
}

void PhysDemo::SpawnFrankieRagdoll()
{
  // Load frankie's factory if not yet done
  csRef<iMeshFactoryWrapper> meshfact =
    engine->FindMeshFactory ("franky_frankie");
  if (!meshfact)
  {
    LoadFrankieRagdoll();
    meshfact = engine->FindMeshFactory ("franky_frankie");
  }

  if (!meshfact)
    return;

  // Create animesh
  csRef<iMeshWrapper> ragdollMesh = engine->CreateMeshWrapper (meshfact, "Frankie",
    room, csVector3 (0, -4, 0));
  csRef<CS::Mesh::iAnimatedMesh> animesh =
    scfQueryInterface<CS::Mesh::iAnimatedMesh> (ragdollMesh->GetMeshObject());

  // Close the eyes of Frankie as he is dead
  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory());

  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("eyelids_closed"), 0.7f);

  // Set the initial position of the body
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();
  ragdollMesh->QuerySceneNode()->GetMovable()->SetPosition (
    tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));

  // Start the ragdoll animation node so that the rigid bodies of the bones are created
  CS::Animation::iSkeletonAnimNode* root = animesh->GetSkeleton()->GetAnimationPacket()->
    GetAnimationRoot();
  csRef<CS::Animation::iSkeletonRagdollNode2> ragdoll =
    scfQueryInterfaceSafe<CS::Animation::iSkeletonRagdollNode2> (root);
  ragdoll->SetPhysicalSystem (physicalSystem);
  ragdoll->SetPhysicalSector (physicalSector);
  ragdoll->Play();

  // Fling the body.
  for (uint i = 0; i < ragdoll->GetBoneCount (CS::Animation::STATE_DYNAMIC); i++)
  {
    CS::Animation::BoneID boneID = ragdoll->GetBone (CS::Animation::STATE_DYNAMIC, i);
    CS::Physics::iRigidBody* rb = ragdoll->GetBoneRigidBody (boneID);
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 5, 0));
  }
}

void PhysDemo::SpawnKrystalRagdoll()
{
  // Load krystal's factory if not yet done
  
  csRef<iMeshFactoryWrapper> meshfact =
    engine->FindMeshFactory ("krystal");
  if (!meshfact)
  {
    LoadKrystalRagdoll();
    meshfact = engine->FindMeshFactory ("krystal");
  }

  if (!meshfact)
    return;

  // Create animesh
  csRef<iMeshWrapper> ragdollMesh = engine->CreateMeshWrapper (meshfact, "Krystal",
    room, csVector3 (0, -4, 0));
  csRef<CS::Mesh::iAnimatedMesh> animesh =
    scfQueryInterface<CS::Mesh::iAnimatedMesh> (ragdollMesh->GetMeshObject());

  // Set the initial position of the body
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();
  ragdollMesh->QuerySceneNode()->GetMovable()->SetPosition (
    tc.GetOrigin() + tc.GetT2O() * csVector3 (0, 0, 1));

  // Start the ragdoll animation node so that the rigid bodies of the bones are created
  CS::Animation::iSkeletonAnimNode* root = animesh->GetSkeleton()->GetAnimationPacket()->
    GetAnimationRoot();
  csRef<CS::Animation::iSkeletonRagdollNode2> ragdoll =
    scfQueryInterfaceSafe<CS::Animation::iSkeletonRagdollNode2> (root);
  ragdoll->SetPhysicalSystem (physicalSystem);
  ragdoll->SetPhysicalSector (physicalSector);
  ragdoll->Play();

  // Fling the body.
  for (uint i = 0; i < ragdoll->GetBoneCount (CS::Animation::STATE_DYNAMIC); i++)
  {
    CS::Animation::BoneID boneID = ragdoll->GetBone (CS::Animation::STATE_DYNAMIC, i);
    CS::Physics::iRigidBody* rb = ragdoll->GetBoneRigidBody (boneID);
    rb->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O() * csVector3 (5, 5, 0));
  }
}

void PhysDemo::SpawnRope()
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Spawn a box
  csRef<CS::Physics::iRigidBody> box = SpawnBox();

  // First example using ropes defined by their extremities
#if 1
  // Spawn a first rope and attach it to the box
  csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateRope
    (tc.GetOrigin() + tc.GetT2O() * csVector3 (-2, 2, 0),
    tc.GetOrigin() + tc.GetT2O() * csVector3 (-0.2f, 0, 1), 20);
  body->SetMass (2.0f);
  body->SetRigidity (0.95f);
  body->AnchorVertex (0);
  body->AnchorVertex (body->GetVertexCount() - 1, box);
  body->RebuildObject();
  physicalSector->AddCollisionObject (body);

  // Spawn a second rope and attach it to the box
  body = physicalSystem->CreateRope
    (tc.GetOrigin() + tc.GetT2O() * csVector3 (2, 2, 0),
    tc.GetOrigin() + tc.GetT2O() * csVector3 (0.2f, 0, 1), 20);
  body->SetMass (2.0f);
  body->SetRigidity (0.95f);
  body->AnchorVertex (0);
  body->AnchorVertex (body->GetVertexCount() - 1, box);
  body->RebuildObject();
  physicalSector->AddCollisionObject (body);

  // Second example using ropes defined by the position of each of their vertices
#else
  // Spawn a first rope and attach it to the box
  {
    // Define the positions of the vertices
    size_t vertexCount = 10;
    CS_ALLOC_STACK_ARRAY(csVector3, nodes, vertexCount);
    nodes[0] = tc.GetOrigin() + tc.GetT2O() * csVector3 (-2, 2, 0);
    csVector3 step = (tc.GetT2O() * csVector3 (-0.2f, 0, 1) -
      tc.GetT2O() * csVector3 (-2, 2, 0)) / (((float) vertexCount) - 1);
    for (size_t i = 1; i < vertexCount; i++)
      nodes[i] = nodes[0] + ((int) (i % 2)) * csVector3 (-0.2f, 0, 0) + ((int) i) * step;

    // Create the soft body
    CS::Physics::iSoftBody* body = physicalSystem->CreateRope
      (nodes, vertexCount);
    body->SetMass (2.0f);
    body->SetRigidity (0.95f);
    body->AnchorVertex (0);
    body->AnchorVertex (body->GetVertexCount() - 1, box);
    body->RebuildObject();
    physicalSector->AddSoftBody (body);
  }

  // Spawn a second rope and attach it to the box
  {
    // Define the positions of the vertices
    size_t vertexCount = 10;
    CS_ALLOC_STACK_ARRAY(csVector3, nodes, vertexCount);
    nodes[0] = tc.GetOrigin() + tc.GetT2O() * csVector3 (2, 2, 0);
    csVector3 step = (tc.GetT2O() * csVector3 (0.2f, 0, 1) -
      tc.GetT2O() * csVector3 (2, 2, 0)) / (((float) vertexCount) - 1);
    for (size_t i = 1; i < vertexCount; i++)
      nodes[i] = nodes[0] + ((int) (i % 2)) * csVector3 (0.2f, 0, 0) + ((int) i) * step;

    // Create the soft body
    CS::Physics::iSoftBody* body = bulletDynamicSystem->CreateRope
      (nodes, vertexCount);
    body->SetMass (2.0f);
    body->SetRigidity (0.95f);
    body->AnchorVertex (0);
    body->AnchorVertex (body->GetVertexCount() - 1, box);
    body->RebuildObject();
    physicalSector->AddSoftBody (body);
  }
#endif
}

CS::Physics::iSoftBody* PhysDemo::SpawnCloth()
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the cloth
  csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateCloth
    (tc.GetOrigin() + tc.GetT2O() * csVector3 (-2, 2, 1),
    tc.GetOrigin() + tc.GetT2O() * csVector3 (2, 2, 1),
    tc.GetOrigin() + tc.GetT2O() * csVector3 (-2, 0, 1),
    tc.GetOrigin() + tc.GetT2O() * csVector3 (2, 0, 1),
    10, 10, true);
  body->SetMass (5.0f);

  // Attach the two top corners
  body->AnchorVertex (0);
  body->AnchorVertex (9);

  // Create the cloth mesh factory
  csRef<iMeshFactoryWrapper> clothFact =
    CS::Physics::SoftBodyHelper::CreateClothGenMeshFactory
    (GetObjectRegistry(), "clothFact", body);
  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<iGeneralFactoryState>
    (clothFact->GetMeshObjectFactory());

  // Create the mesh
  gmstate->SetAnimationControlFactory (softBodyAnimationFactory);
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    clothFact, "cloth_body"));
  iMaterialWrapper* mat = CS::Material::MaterialBuilder::CreateColorMaterial
    (GetObjectRegistry(), "cloth", csColor4 (1.0f, 0.0f, 0.0f, 1.0f));
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  body->SetAttachedMovable (mesh->GetMovable());

  body->RebuildObject();
  physicalSector->AddCollisionObject (body);

  // Init the animation control for the animation of the genmesh
  // If it's a double-side soft body like cloth, you have to call SetSoftBody (body, true);
  csRef<iGeneralMeshState> meshState =
    scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject());
  csRef<CS::Physics::iSoftBodyAnimationControl> animationControl =
    scfQueryInterface<CS::Physics::iSoftBodyAnimationControl> (meshState->GetAnimationControl());
  animationControl->SetSoftBody (body, true);

  return body;
}

CS::Physics::iSoftBody* PhysDemo::SpawnSoftBody (bool setVelocity /* = true */)
{
  // Create the ball mesh factory.
  csRef<iMeshFactoryWrapper> ballFact = engine->CreateMeshFactory(
    "crystalspace.mesh.object.genmesh", "ballFact");
  if (!ballFact)
  {
    ReportError ("Error creating mesh object factory!");
    return nullptr;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (ballFact->GetMeshObjectFactory());
  const float r = 0.4f;
  csVector3 radius (r, r, r);
  csEllipsoid ellips (csVector3 (0), radius);
  gmstate->GenerateSphere (ellips, 16);

  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera()->GetTransform();

  // Create the soft body
  //csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateSoftBody(gmstate,
  //  csOrthoTransform (csMatrix3(), csVector3 (0.0f, 0.0f, 1.0f)) * tc);
  // This would have worked too
  csRef<CS::Physics::iSoftBody> body = physicalSystem->CreateSoftBody
    (gmstate->GetVertices(), gmstate->GetVertexCount(),
     gmstate->GetTriangles(), gmstate->GetTriangleCount(),
     csOrthoTransform (csMatrix3(), csVector3 (0.0f, 0.0f, 1.0f)) * tc);
  body->SetMass (5.0f);
  body->SetRigidity (0.8f);
  csRef<CS::Physics::Bullet2::iSoftBody> bulletbody = 
    scfQueryInterface<CS::Physics::Bullet2::iSoftBody> (body);
  bulletbody->SetBendingConstraint (true);
  
  if (setVelocity)
  {
    // Fling the body.
    body->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5));
  }

  // Create the mesh
  gmstate->SetAnimationControlFactory (softBodyAnimationFactory);
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    ballFact, "soft_body"));
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("spark");
  mesh->GetMeshObject()->SetMaterialWrapper (mat);

  body->SetAttachedMovable (mesh->GetMovable());

  body->RebuildObject();
  physicalSector->AddCollisionObject (body);

  // Init the animation control for the animation of the genmesh
  /*csRef<iGeneralMeshState> meshState =
    scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject());
  csRef<CS::Physics::iSoftBodyAnimationControl> animationControl =
    scfQueryInterface<CS::Physics::iSoftBodyAnimationControl> (meshState->GetAnimationControl());
  animationControl->SetSoftBody (body);*/


  // This would have worked too
  //for (size_t i = 0; i < body->GetVertexCount(); i++)
  //  body->SetLinearVelocity (tc.GetT2O() * csVector3 (0, 0, 5), i);
  return body;
}

void PhysDemo::SpawnBoxStacks(int stackNum, int stackHeight, float boxLen, float mass)
{
  // Place stacks of boxes
  // Stacks are horizontally aligned with the viewing direction

  static const float anchorDist = 2;                  // distance from pos to stack area
  static const float hSpacingFactor = 0.9f;           // how much of the box length is to be left as space between boxes horizontally
  static const float vSpacingFactor = 0.01f;          // how much of the box length is to be left as space between boxes vertically

  // position & direction
  csVector3 pos = GetCameraPosition(); GetPointOnGroundBeneathPos(pos, pos);    // move to ground
  csVector3 dir = GetCameraDirection();
  
  csVector2 pos2 = HORIZONTAL_COMPONENT(pos);
  csVector2 dir2 = HORIZONTAL_COMPONENT(dir);
  csVector2 dirOrth2 = dir2;
  dirOrth2.Rotate(HALF_PI);
  
  float hspace = hSpacingFactor * boxLen;                          // horizontal spacing between boxes
  float dist = boxLen + hspace;                                   // horizontal distance between two neighboring stacks
  csVector2 hdistDir = dist * dir2;                               // horizontal stack distance in dir
  csVector2 hdistOrth = dist * dirOrth2;                          // horizontal stack distance orthogonal to dir
  
  int numDir = int(sqrt(float(stackNum)) + 0.99999f);             // amount of stacks in dir direction
  int numOrth = int(stackNum / numDir + 1);                       // amount of stacks in orth direction

  float halfWidth = .5f * (numOrth - 1) * (boxLen + hspace);      // half the width of the OBB that covers all box centers
  
  csVector3 extents(boxLen);                                      // box size
  csVector2 anchor = pos2 + anchorDist * dir2;                    // the closest point of the stack area from pos
  csVector2 boxPos2(anchor - halfWidth * dirOrth2);               // position of the first box

  int n = 0;

  // prepare collider and material
  if (!stackBoxCollider)
  {
    stackBoxCollider = physicalSystem->CreateColliderBox (extents);
  }

  if (!stackBoxMeshFactory)
  {
    DensityTextureMapper mapper (0.3f);
    TesselatedBox tbox (-extents/2, extents/2);
    tbox.SetLevel (3);
    tbox.SetMapper (&mapper);
    if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
    {
      ReportWarning ("Could not load texture %s", CS::Quote::Single ("stone"));
    }
    stackBoxMeshFactory = GeneralMeshBuilder::CreateFactory(engine, "box factory", &tbox);
  }
  
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName ("stone");
  for (int x = 0; x < numOrth; ++x)
  {
    for (int z = 0; z < numDir && n < stackNum; ++z)
    {
      csVector3 boxPos = HV_VECTOR3(boxPos2, pos[UpAxis]);
      boxPos += (.5f * (1 + vSpacingFactor) * boxLen) * UpVector;
      for (int i = 0; i < stackHeight; ++i)
      {
        csRef<iMeshWrapper> mesh = stackBoxMeshFactory->CreateMeshWrapper();
        mesh->QueryObject()->SetName("box");
        mesh->GetMeshObject()->SetMaterialWrapper (mat);
        SpawnRigidBody(stackBoxCollider, mesh, boxPos, mass, false);
        boxPos += ((1 + vSpacingFactor) * boxLen) * UpVector;
      }
      ++n;
      boxPos2 += hdistDir;
    }
    boxPos2 += hdistOrth - numDir * hdistDir;
  }
  
}