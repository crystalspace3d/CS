
#include "cssysdef.h"
#include "vehicle.h"

#include "csutil/hash.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  // ####################################################################################################################################
  // Wheel 
  BulletVehicleWheel::BulletVehicleWheel(csBulletSystem* sys, btWheelInfo& btWheel) : scfImplementationType(this),
    sys(sys),
    btWheel(btWheel)
  {
  }

  BulletVehicleWheel::~BulletVehicleWheel()
  {
  }

  // ####################################################################################################################################
  // Wheel Factory

  BulletVehicleWheelFactory::BulletVehicleWheelFactory(csBulletSystem* sys) : scfImplementationType (this),
    sys(sys),
    rollInfluence(csScalar(.1))
  {
  }

  BulletVehicleWheelFactory::~BulletVehicleWheelFactory()
  {
  }

  csPtr<iVehicleWheel> BulletVehicleWheelFactory::CreateWheel(btWheelInfo& btWheel)
  {
    BulletVehicleWheel* wheel = new BulletVehicleWheel(sys, btWheel);
    wheel->SetCollider(GetCollider());
    wheel->SetRollInfluence(GetRollInfluence());

    return csPtr<iVehicleWheel>(wheel);
  }

  // ####################################################################################################################################
  // Vehicle Factory

  csPtr<iVehicle> BulletVehicleFactory::CreateVehicle(CS::Physics::iPhysicalSector* isector)
  {
    csRef<iRigidBody> ichassis = chassisFactory->CreateRigidBody();
    csBulletRigidBody* chassis = dynamic_cast<csBulletRigidBody*>(&*ichassis);
    chassis->SetMayBeDeactivated(false);

    csRef<BulletVehicle> vehicle = csPtr<BulletVehicle>(new BulletVehicle(sys, chassis));
    
    csBulletSector* sector = dynamic_cast<csBulletSector*>(isector);
    btVehicleRaycaster* rayCaster = new btDefaultVehicleRaycaster(sector->GetBulletWorld());

    // create vehicle
    static const btRaycastVehicle::btVehicleTuning tuning;    // this is unused anyway
    vehicle->btVehicle = new btRaycastVehicle(tuning, chassis->GetBulletRigidPointer(), rayCaster);
    vehicle->btVehicle->setCoordinateSystem(0, 1, 2);

    // create & add bullet wheel objects
    for (size_t i = 0; i < wheelInfos.GetSize(); ++i)
    {
      iVehicleWheelInfo* info = wheelInfos[i];

      iVehicleWheelFactory* iwheelFact = info->GetFactory();
      BulletVehicleWheelFactory* wheelFact = dynamic_cast<BulletVehicleWheelFactory*>(iwheelFact);

      vehicle->btVehicle->addWheel(
        CSToBullet(info->GetWheelPos(), sys->getInternalScale()),
        CSToBullet(info->GetWheelOrientation(), sys->getInternalScale()),
        CSToBullet(info->GetAxleOrientation(), sys->getInternalScale()), 
        info->GetSuspensionLength(),
        info->GetRadius(),
        wheelFact->tuning,
        true                // isFrontWheel is unnecessary and unused information
        );
    }
    
    // create & add CS wheel objects
    // must separate from bullet allocation because the wheel objects will be moved during creation time
    for (size_t i = 0; i < wheelInfos.GetSize(); ++i)
    {
      iVehicleWheelInfo* info = wheelInfos[i];

      iVehicleWheelFactory* iwheelFact = info->GetFactory();
      BulletVehicleWheelFactory* wheelFact = dynamic_cast<BulletVehicleWheelFactory*>(iwheelFact);

      btWheelInfo& btWheel = vehicle->btVehicle->getWheelInfo(i);

      csRef<iVehicleWheel> wheel = wheelFact->CreateWheel(btWheel);
      wheel->SetAxleOrientation(info->GetAxleOrientation());
      wheel->SetIsWheelDriven(info->GetIsWheelDriven());
      wheel->SetSuspensionLength(info->GetSuspensionLength());
      wheel->SetSuspensionOrientation(info->GetWheelOrientation());
      wheel->SetWheelPos(info->GetWheelPos());
      
      vehicle->wheels.Push(wheel);
    }
    
    return csPtr<iVehicle>(vehicle);
  }

  BulletVehicleFactory::BulletVehicleFactory(csBulletSystem* sys) :
  scfImplementationType (this),
    sys(sys)
  {
  }

  BulletVehicleFactory::~BulletVehicleFactory()
  {
  }



  // ####################################################################################################################################
  // Vehicle

  BulletVehicle::BulletVehicle(csBulletSystem* sys, csBulletRigidBody* chassis) : scfImplementationType (this),
    sys(sys), chassis(chassis)
  {
  }

  BulletVehicle::~BulletVehicle()
  {
    delete btVehicle;
  }

  csScalar BulletVehicle::GetEngineForce() const
  {
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven())
      {
        BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);
        return GetDrivenWheelCount() * wheel->btWheel.m_engineForce;
      }
    }
    return 0;
  }

  void BulletVehicle::SetEngineForce(csScalar f)
  {
    f /= GetDrivenWheelCount();   // divide equally among the driving wheels
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven())
      {
        BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);
        wheel->btWheel.m_engineForce = f;
      }
    }
  }

  size_t BulletVehicle::GetDrivenWheelCount() const
  {
    size_t count = 0;
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven())
      {
        ++count;
      }
    }
    return count;
  }

  void BulletVehicle::ApplyBrake(VehicleBrakeInfo* brake, csScalar factor)
  {
    // apply brake
    csScalar force = factor * brake->GetMaxForce()  / brake->GetAffectedWheelCount();
    for (size_t i = 0; i < brake->GetAffectedWheelIndices().GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[brake->GetAffectedWheelIndices()[i]];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);

      // add brake force
      wheel->btWheel.m_brake += force;
    }
  }
  
  void BulletVehicle::IncrementSteering(VehicleSteeringDevice* steeringWheel, csScalar increment)
  {
    csScalar cap = steeringWheel->GetMaxSteering();
    iVehicleWheel* iwheel = wheels[steeringWheel->GetAffectedWheelIndices()[0]];
    BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);

    // compute new steering value for the first wheel
    csScalar& steering = wheel->btWheel.m_steering;
    steering += increment;
    if (steering > cap)
      steering = cap;
    else if (steering < -cap)
      steering = -cap;

    // apply same steering to the other affected wheels
    for (size_t i = 1; i < steeringWheel->GetAffectedWheelIndices().GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[steeringWheel->GetAffectedWheelIndices()[i]];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);
      wheel->btWheel.m_steering = steering;
    }
  }

  void BulletVehicle::DoStep(csScalar dt)
  {
    // update chassis movable to prevent jittering
    chassis->GetAttachedMovable()->SetFullTransform(chassis->GetTransform());
    chassis->GetAttachedMovable()->UpdateMove();

    // post-process wheels:
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*>(iwheel);

      // reset engine force
      wheel->btWheel.m_engineForce = 0;

      // reset brake force
      wheel->btWheel.m_brake = 0;
      
      // update transformation
      wheels[i]->GetMovable()->SetFullTransform(BulletToCS(wheel->btWheel.m_worldTransform, sys->getInverseInternalScale()));
      wheels[i]->GetMovable()->UpdateMove ();
    }
  }
  
  void BulletVehicle::OnAdded(iPhysicalSector* isector)
  {
    // add to system
    csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*>& vehicleMap = sys->GetVehicleMap();
    vehicleMap.Put(chassis, this);

    // add wheels to sector
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      csBulletSector* sector = dynamic_cast<csBulletSector*>(isector);
      sector->AddMovableToSector(iwheel->GetMovable());
    }
  }
  
  void BulletVehicle::OnRemoved(iPhysicalSector* isector)
  {
    // remove from system
    csHash<CS::Physics::iVehicle*, CS::Collisions::iCollisionObject*>& vehicleMap = sys->GetVehicleMap();
    vehicleMap.DeleteAll(chassis);
    
    // remove wheels from sector
    for (size_t i = 0; i < wheels.GetSize(); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      csBulletSector* sector = dynamic_cast<csBulletSector*>(isector);
      sector->RemoveMovableFromSector(iwheel->GetMovable());
    }
  }
}
CS_PLUGIN_NAMESPACE_END (Bullet2)