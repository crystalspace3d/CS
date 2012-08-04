/**
 * Create and handle vehicles
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
#include "ivaria/ivehicle.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

static const csScalar DefaultFriction(.5);
static const csScalar DefaultElasticity(0.1f);


/// The available brakes
static VehicleBrakeInfo FrontBrake, RearBrake, HandBrake;

/// The steering wheel
static VehicleSteeringDevice SteeringWheel;


// ##############################################################################################################################
// Default vehicle parameters:

static const csScalar SteeringIncrement(csScalar(.04));
static const csScalar SteeringMax(csScalar(.3));

static const csScalar EngineForce(10000);

static const csScalar FrontBrakeForce(100);
static const csScalar RearBrakeForce(100);
static const csScalar HandBrakeForce(1000);

static const csScalar ChassisMass(800);


// The Wheels:

/// Amount of axles
const static int AxleCount(2);

/// Amount of wheels per axle
const static int WheelsPerAxle(2);

/// Length of suspension as factor of the chassis height
const static csScalar SuspensionLengthFactor(csScalar(1.2));

/// Wheel radius
const static csScalar WheelRadius(csScalar(.5));

/// Wheel width
const static csScalar WheelWidth(csScalar(.6));


// The Chassis:

/// Chassis has two parts: One top box
const static csVector3 ChassisSizeTop(3, 2, 3);

/// and one bottom box
const static csVector3 ChassisSizeBottom(csScalar(3), csScalar(.9), csScalar(5));


// The grid of tires:

/// Relative position to chassis of the first wheel on the left side
const static csVector3 WheelTopLeft(
  -csScalar(.6), 
  -csScalar(.7), 
  csScalar(.45));

/// Relative position to chassis of the last wheel on the left side
const static csVector3 WheelBottomLeft(
  -csScalar(.6), 
  -csScalar(.7), 
  -csScalar(.45));

/// Relative position to chassis of the first wheel on the right side
const static csVector3 WheelTopRight(
  csScalar(.6), 
  -csScalar(.7), 
  csScalar(.45));


// Actor <-> Vehicle interaction

/// The actor seat pos relative to a vehicle
const static csVector3 VehicleActorPos(
  0
);

/// The speed with which the actor moves sideward when jumping out of a vehicle
const static csScalar ActorBailSideSpeed(3);



void AddWheel(iVehicleFactory* vehicleFact, int axleN, int inAxleN);



// ##############################################################################################################################
// Create & setup vehicles

csPtr<iVehicle> PhysDemo::CreateVehicle()
{
  // TODO: Set mesh factory & material for wheels and chassis

  // more vehicle parameters:

  
  // compute dependent parameters
  csVector3 topPos(0, csScalar(.5) * ChassisSizeTop.y, - csScalar(.5) * (ChassisSizeBottom.z - ChassisSizeTop.z));
  csVector3 botPos(0, -csScalar(.5) * ChassisSizeBottom.y, 0);

  // setup chassis

  csRef<iColliderCompound> chassisCollider = physicalSystem->CreateColliderCompound();
  csRef<iColliderBox> chassisTopCollider = physicalSystem->CreateColliderBox(ChassisSizeTop);
  csRef<iColliderBox> chassisBottomCollider = physicalSystem->CreateColliderBox(ChassisSizeBottom);
  // TODO: Figure out meshes of arbitrary compound colliders
  //chassisCollider->AddCollider(chassisTopCollider, csOrthoTransform(csMatrix3(), topPos));
  botPos.y = 0;
  chassisCollider->AddCollider(chassisBottomCollider, csOrthoTransform(csMatrix3(), botPos));
  
  //csOrthoTransform centerOfMassTransform(csMatrix3(), csVector3(0, -csScalar(.4) * ChassisSizeBottom.y, ChassisSizeBottom.z));
  //chassisCollider->SetPrincipalAxisTransform(centerOfMassTransform);
  csRef<iRigidBodyFactory> chassisFact = physicalSystem->CreateRigidBodyFactory(chassisCollider, "chassis");
  chassisFact->SetElasticity(DefaultElasticity);
  chassisFact->SetFriction(DefaultFriction);
  chassisFact->SetMass(ChassisMass);
  
  // create vehicle factory
  csRef<iVehicleFactory> fact = physicalSystem->CreateVehicleFactory();
  fact->SetChassisFactory(chassisFact);

  // setup wheels
  csRef<iVehicleWheelFactory> wheelFact = physicalSystem->CreateVehicleWheelFactory();
  wheelFact->SetRollInfluence(csScalar(0));
  // wheelFact->SetMeshFactory(wheelMesh->GetMovable());
  //wheelFact->SetFrictionCoefficient(11);    // Use default settings
  //wheelFact->SetFrictionCoefficient(.5);
  
  for (int axle = 0; axle < AxleCount; ++axle)
  {
    csScalar axleFactor = axle / csScalar(AxleCount - 1);
    for (int inAxleN = 0; inAxleN < WheelsPerAxle; ++inAxleN)
    {
      csScalar inAxleFactor = inAxleN / csScalar(WheelsPerAxle - 1);

      // place wheels uniformly along the grid of axles
      csVector3 pos(WheelTopLeft);
      pos += inAxleFactor * (WheelTopRight - WheelTopLeft);
      pos += axleFactor * (WheelBottomLeft - WheelTopLeft);
      pos = ScaleVector3(ChassisSizeBottom, pos);

      csRef<iVehicleWheelInfo> wheel = physicalSystem->CreateVehicleWheelInfo(wheelFact);
      wheel->SetAxleOrientation(csVector3(-1, 0, 0));
      wheel->SetIsWheelDriven(axle == 0);
      wheel->SetRadius(WheelRadius);
      wheel->SetSuspensionLength(SuspensionLengthFactor * ChassisSizeBottom.y);
      wheel->SetSuspensionOrientation(csVector3(0, -1, 0));

      wheel->SetWheelPos(pos);

      //size_t index = axle * WheelsPerAxle + inAxleN;
      size_t index = fact->GetWheelInfos().GetSize();

      if (axle == 0)
      {
        // front wheel
        SteeringWheel.GetAffectedWheelIndices().Push(index);
        FrontBrake.GetAffectedWheelIndices().Push(index);
      }
      else if (axle > 0)
      {
        // rear wheel
        RearBrake.GetAffectedWheelIndices().Push(index);
        HandBrake.GetAffectedWheelIndices().Push(index);
      }

      fact->AddWheelInfo(wheel);
    }
  }

  // Setup Steering Parameters
  SteeringWheel.SetMaxSteering(SteeringMax);

  // Setup Brake Parameters
  FrontBrake.SetMaxForce(FrontBrakeForce);
  RearBrake.SetMaxForce(RearBrakeForce);
  HandBrake.SetMaxForce(HandBrakeForce);

  // Create vehicle
  csRef<iVehicle> vehicle = fact->CreateVehicle(GetCurrentSector());

  // Set meshes (FIXME)
  csRef<iMeshWrapper> chassisMesh = CreateBoxMesh(ChassisSizeBottom, "misty", "chassis");
  CS_ASSERT(chassisMesh);
  vehicle->GetChassis()->SetAttachedMovable(chassisMesh->GetMovable());
  for (size_t i = 0; i < vehicle->GetWheels().GetSize(); ++i)
  {
    csRef<iMeshWrapper> wheelMesh = CreateCylinderYMesh(WheelWidth, WheelRadius);
    CS_ASSERT(wheelMesh);
    iVehicleWheel* wheel = vehicle->GetWheels()[i];
    wheel->SetMovable(wheelMesh->GetMovable());
  }   

  // Must add to world, because else meshes will be deleted upon return
  GetCurrentSector()->AddUpdatable(vehicle);

  return csPtr<iVehicle>(vehicle);
}



// ##############################################################################################################################
// Steering, acceleration and braking

void PhysDemo::MoveActorVehicle()
{
  // actorVehicle != nullptr

  // Steering
  csScalar steering = SteeringIncrement * GetLeftRight();
  actorVehicle->IncrementSteering(&SteeringWheel, steering);

  // Acceleration
  if (GetForward())
  {
    actorVehicle->SetEngineForce(EngineForce);
  }
  else 
  {
    // do nothing because engine force is resetted automatically
    //actorVehicle->SetEngineForce(actorVehicle->GetEngineForce() * csScalar(.99));
  }

  if (GetBackward())
  {
    // Backward
    actorVehicle->SetEngineForce(-EngineForce / 10);
  }
  if (kbd->GetKeyState (KeyHandbrake))
  {
    // Apply handbrake
    actorVehicle->ApplyBrake(&HandBrake);
  }
}

void PhysDemo::UpdateVehiclePassengers()
{
  if (actorVehicle)
  {
    csOrthoTransform trans(player.GetObject()->GetTransform());
    trans.SetOrigin(actorVehicle->GetChassis()->GetTransform().GetOrigin() + VehicleActorPos);
    player.GetObject()->SetTransform(trans);
  }
}


// ##############################################################################################################################
// Enter & Exit vehicle

void PhysDemo::EnterTargetVehicle()
{
  iVehicle* vehicle = GetTargetVehicle();
  if (!vehicle) return;

  // actor should not physically interact anymore, since in this simple scene, it penetrates the chassis
  if (player.GetObject()->QueryPhysicalBody() && player.GetObject()->QueryPhysicalBody()->QueryRigidBody())
  {
    player.GetObject()->QueryPhysicalBody()->QueryRigidBody()->SetState(STATE_KINEMATIC);
  }
  player.GetObject()->SetCollisionGroup("None");

  // switch to 3rd person mode
  cameraMode = CameraMode3rdPersonFar;

  // set vehicle
  actorVehicle = vehicle; 
}

void PhysDemo::LeaveCurrentVehicle()
{
  iVehicle* vehicle = actorVehicle;
  if (!vehicle) return;

  // reset actor
  if (player.GetObject()->QueryPhysicalBody() && player.GetObject()->QueryPhysicalBody()->QueryRigidBody())
  {
    player.GetObject()->QueryPhysicalBody()->QueryRigidBody()->SetState(STATE_DYNAMIC);
  }
  player.GetObject()->SetCollisionGroup("Default");
  
  // unset vehicle
  actorVehicle = nullptr;
  
  // Move and accelerate actor:
  iCollisionObject* actorObj = player.GetObject();

  const csOrthoTransform& vehicleTrans = vehicle->GetChassis()->GetTransform();
  csVector3 sideward = vehicleTrans.GetT2O().Col1();

  // Place actor beside vehicle to avoid collision
  csOrthoTransform actorTrans = actorObj->GetTransform();
  csScalar sideDist = ChassisSizeBottom.x / csScalar(1.8) + 2 * ActorDimensions.x;
  actorTrans.Translate(sideDist * sideward);
  csVector3 pos(actorTrans.GetOrigin());
  if (!GetPointOnGroundBeneathPos(pos, pos))
  {
    if (!GetPointOnGroundBeneathPos(pos, pos)) pos = actorTrans.GetOrigin();
  }
  actorTrans.SetOrigin(pos + HV_VECTOR3(csVector2(0), ActorDimensions[UpAxis]));   // place above terrain
  actorTrans.SetT2O(csMatrix3());                                                  // no rotation
  actorObj->SetTransform(actorTrans);

  if (actorObj->QueryPhysicalBody())
  {
    // Actor bails out with a small sideward velocity + velocity of the vehicle
    iPhysicalBody* actorBody = actorObj->QueryPhysicalBody();

    csVector3 forward = vehicle->GetChassis()->GetLinearVelocity();

    actorBody->SetLinearVelocity(ActorBailSideSpeed * sideward + forward);
  }
  else
  {
    // nothing happens
  }

}

// ##############################################################################################################################
// Spawn & Delete Vehicle

void PhysDemo::SpawnVehicle()
{
  // Create a new vehicle
  csRef<iVehicle> vehicle = CreateVehicle();
  csOrthoTransform trans(player.GetObject()->GetTransform());
  csVector3 forward(trans.GetT2O().Col3());

  // Place in front of player
  csVector3 pos(trans.GetOrigin());
  pos += 2 * ChassisSizeBottom.z * forward;
  pos[UpAxis] += 3 * ChassisSizeBottom.y;

  //csMatrix3 rotation(trans.GetT2O());
  csMatrix3 rotation;
  vehicle->GetChassis()->SetTransform(csOrthoTransform(rotation, pos));
}

void PhysDemo::DeleteTargetVehicle()
{
  iVehicle* vehicle = GetTargetVehicle();
  if (!vehicle) return;

  // Remove from world
  if (vehicle == actorVehicle)
  {
    LeaveCurrentVehicle();
  }
  GetCurrentSector()->RemoveUpdatable(vehicle);
}

// ##############################################################################################################################
// Do stuff to vehicle

void PhysDemo::AccelerateTargetVehicle()
{
  iVehicle* vehicle = GetTargetVehicle();
  if (!vehicle) return;

  vehicle->SetEngineForce(EngineForce);
}


// ##############################################################################################################################
// Vehicle Utilities


iVehicle* PhysDemo::GetTargetVehicle()
{
  HitBeamResult hitResult;
  if (PickCursorObject(hitResult))
  {
    return physicalSystem->GetVehicle(hitResult.object);
  }
  return nullptr;
}