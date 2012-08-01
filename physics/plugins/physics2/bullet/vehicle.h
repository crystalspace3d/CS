#ifndef __CS_BULLET_VEHICLE_H__
#define __CS_BULLET_VEHICLE_H__

#include "ivaria/ivehicle.h"
#include "rigidbody2.h"

#include "BulletDynamics/Vehicle/btRaycastVehicle.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"

/**
 * Vehicle TODO:
 * - Add sockets for seats to make sure that passenger transforms are relative to vehicle
 * - Add mesh factories & materials to Vehicle- and VehicleWheel- factories
 * - Add convex ray caster for wheels (currently wheels are physically assumed to be thin disks)
 * - Add proper ray casting and collision support for vehicle wheels (by adding ghost objects for wheels)
 * - Add gears and gearbox
 * - Produce a pretty sample vehicle in phystut2 that looks *real*
 * - Make sure that actor animations interact correctly with vehicles (correct entering/leaving of vehicles of different size & velocity etc)
 */

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
  class BulletVehicle;
  class BulletVehicleFactory;
  class csBulletRigidBody;
  

  class BulletVehicleWheel : public scfVirtImplementationExt1<
    BulletVehicleWheel, csObject, CS::Physics::iVehicleWheel>
  {
    friend class BulletVehicle;
    friend class BulletVehicleFactory;
    friend class BulletVehicleWheelFactory;

    csBulletSystem* sys;
    csRef<iMovable> movable;
    btWheelInfo& btWheel;
    bool isDriven, isAffectedByBrake;

  public:
    BulletVehicleWheel(csBulletSystem* sys, btWheelInfo& btWheel);
    virtual ~BulletVehicleWheel();

    /// Movables can move
    virtual iMovable* GetMovable() const { return movable; }
    /// Movables can move
    virtual void SetMovable(iMovable* m) { movable = m; }

    /// Collider of the wheel
    virtual CS::Collisions::iCollider* GetCollider() const { return nullptr; }
    /// Collider of the wheel
    virtual void SetCollider(CS::Collisions::iCollider* c) { }

    /// The spring constant. Stiffer suspension generates more force when spring is displaced.
    virtual csScalar GetSuspensionStiffness() const { return btWheel.m_suspensionStiffness; }
    /// The spring constant. Stiffer suspension generates more force when spring is displaced.
    virtual void SetSuspensionStiffness(csScalar s) { btWheel.m_suspensionStiffness = s; }

    /// Suspension with more damping needs more force to be compressed and to relax
    virtual csScalar GetSuspensionDamping() const { return btWheel.m_wheelsDampingCompression; }
    /// Suspension with more damping needs more force to be compressed and to relax
    virtual void SetSuspensionDamping(csScalar s) { btWheel.m_wheelsDampingCompression = btWheel.m_wheelsDampingRelaxation = s; }

    /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
    virtual csScalar GetMaxSuspensionDisplacementCM() const { return btWheel.m_maxSuspensionTravelCm; }
    /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
    virtual void SetMaxSuspensionDisplacementCM(csScalar s) { btWheel.m_maxSuspensionTravelCm = s; }

    /** 
    * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
    * It is very similar to muh in Coulomb's law.
    * A greater value reduces the chance of the vehicle slipping.
    * When on a wet or slippery road, the coefficient should be very small.
    */
    virtual csScalar GetFrictionCoefficient() const { return btWheel.m_frictionSlip; }
    /** 
    * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
    * It is very similar to muh in Coulomb's law.
    * A greater value reduces the chance of the vehicle slipping.
    * When on a wet or slippery road, the coefficient should be very small.
    */
    virtual void SetFrictionCoefficient(csScalar s) { btWheel.m_frictionSlip = s; }

    /// The max force to be applied from the wheel to the chassis, caused by an impact
    virtual csScalar GetMaxSuspensionForce() const { return btWheel.m_maxSuspensionForce; }
    /// The max force to be applied from the wheel to the chassis, caused by an impact
    virtual void SetMaxSuspensionForce(csScalar s) { btWheel.m_maxSuspensionForce = s; }

    /// Value between 0 and 1 that determines how easily the car can roll over its side
    virtual csScalar GetRollInfluence() const { return btWheel.m_rollInfluence; }
    /// Value between 0 and 1 that determines how easily the car can roll over its side
    virtual void SetRollInfluence(csScalar infl) { btWheel.m_rollInfluence = infl; }


    // Geometry & other

    /// Whether this wheel is driven by the engine
    virtual bool GetIsWheelDriven() const { return isDriven; }
    /// Whether this wheel is driven by the engine
    virtual void SetIsWheelDriven(bool d) { isDriven = d; }

    /// Whether this wheel is driven by the engine
    virtual bool GetIsWheelAffectedByBrake() const { return isAffectedByBrake; }
    /// Whether this wheel is driven by the engine
    virtual void SetIsWheelAffectedByBrake(bool b) { isAffectedByBrake = b; }

    /// Length of the suspension in equilibrium
    virtual csScalar GetSuspensionLength() const { return btWheel.m_suspensionRestLength1; }
    /// Length of the suspension in equilibrium
    virtual void SetSuspensionLength(csScalar s) { btWheel.m_suspensionRestLength1 = s; }

    /// Radius of the wheel
    virtual csScalar GetRadius() const { return btWheel.m_wheelsRadius; }
    /// Radius of the wheel
    virtual void SetRadius(csScalar s) { btWheel.m_wheelsRadius = s; }

    /// The position of the wheel relative to the chassis
    csVector3 GetWheelPos() const { return BulletToCS(btWheel.m_chassisConnectionPointCS, sys->getInternalScale()); }
    /// Value between 0 and 1 that determines how easily the car can roll over its side
    void SetWheelPos(const csVector3& p)  { btWheel.m_chassisConnectionPointCS = CSToBullet(p, sys->getInverseInternalScale()); }

    /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
    csVector3 GetWheelOrientation() const { return BulletToCS(btWheel.m_wheelDirectionCS, sys->getInternalScale()); }
    /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
    void SetSuspensionOrientation(const csVector3& o) { btWheel.m_wheelDirectionCS = CSToBullet(o, sys->getInverseInternalScale()); }

    /// Unit vector that describes the axle about which the wheel rotates
    csVector3 GetAxleOrientation() const { return BulletToCS(btWheel.m_wheelAxleCS, sys->getInternalScale()); }
    /// Unit vector that describes the axle about which the wheel rotates
    void SetAxleOrientation(const csVector3& o) { btWheel.m_wheelAxleCS = CSToBullet(o, sys->getInverseInternalScale()); }


    // Run-time parameters

    /// Rotation in radians
    const csScalar GetRotation() const { return btWheel.m_rotation; }
    /// Rotation in radians
    void SetRotation(csScalar r) { btWheel.m_rotation = r; }
  };

  class BulletVehicleWheelFactory : public scfVirtImplementationExt1<
    BulletVehicleWheelFactory, csObject, CS::Physics::iVehicleWheelFactory>
  {
    friend class BulletVehicleFactory;

  private:
    csBulletSystem* sys;
    btRaycastVehicle::btVehicleTuning tuning;
    csScalar rollInfluence;

  public:
    BulletVehicleWheelFactory(csBulletSystem* sys);
    virtual ~BulletVehicleWheelFactory();

    /// Creates a new wheel
    csPtr<CS::Physics::iVehicleWheel> CreateWheel(btWheelInfo& btWheel);

    /// Collider of the wheel
    virtual CS::Collisions::iCollider* GetCollider() const { return nullptr; }
    /// Collider of the wheel
    virtual void SetCollider(CS::Collisions::iCollider* c) { }

    /// The spring constant. Stiffer suspension generates more force when spring is displaced.
    virtual csScalar GetSuspensionStiffness() const { return tuning.m_suspensionStiffness; }
    /// The spring constant. Stiffer suspension generates more force when spring is displaced.
    virtual void SetSuspensionStiffness(csScalar s) { tuning.m_suspensionStiffness = s; }

    /// Suspension with more damping needs more force to be compressed and to relax
    virtual csScalar GetSuspensionDamping() const { return tuning.m_suspensionCompression; }
    /// Suspension with more damping needs more force to be compressed and to relax
    virtual void SetSuspensionDamping(csScalar s) { tuning.m_suspensionCompression = tuning.m_suspensionDamping = s; }

    /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
    virtual csScalar GetMaxSuspensionDisplacementCM() const { return tuning.m_maxSuspensionTravelCm; }
    /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
    virtual void SetMaxSuspensionDisplacementCM(csScalar s) { tuning.m_maxSuspensionTravelCm = s; }

    /** 
    * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
    * It is very similar to muh in Coulomb's law.
    * A greater value reduces the chance of the vehicle slipping.
    * When on a wet or slippery road, the coefficient should be very small.
    */
    virtual csScalar GetFrictionCoefficient() const { return tuning.m_frictionSlip; }
    /** 
    * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
    * It is very similar to muh in Coulomb's law.
    * A greater value reduces the chance of the vehicle slipping.
    * When on a wet or slippery road, the coefficient should be very small.
    */
    virtual void SetFrictionCoefficient(csScalar s) { tuning.m_frictionSlip = s; }

    /// The max force to be applied from the wheel to the chassis, caused by an impact
    virtual csScalar GetMaxSuspensionForce() const { return tuning.m_maxSuspensionForce; }
    /// The max force to be applied from the wheel to the chassis, caused by an impact
    virtual void SetMaxSuspensionForce(csScalar s) { tuning.m_maxSuspensionForce = s; }

    /// Value between 0 and 1 that determines how easily the car can roll over its side
    virtual csScalar GetRollInfluence() const { return rollInfluence; }
    /// Value between 0 and 1 that determines how easily the car can roll over its side
    virtual void SetRollInfluence(csScalar infl) { rollInfluence = infl; }
  };

    /**
     * All info needed to produce a specific instance of a wheel, using a factory and geometric details.
     */
    class BulletVehicleWheelInfo : public scfVirtImplementationExt1<
      BulletVehicleWheelInfo, csObject, CS::Physics::iVehicleWheelInfo>
    {
      csRef<CS::Physics::iVehicleWheelFactory> factory;
      csVector3 pos, wheelOrientation, axleOrientation;
      csScalar suspensionLength;
      csScalar radius;
      bool isDriven;

    public:
      BulletVehicleWheelInfo(CS::Physics::iVehicleWheelFactory* f) :
          scfImplementationType(this),
          factory(f),
          suspensionLength(1),
          radius(.5),
          isDriven(true)
      {
      }

      virtual ~BulletVehicleWheelInfo() {}

      virtual CS::Physics::iVehicleWheelFactory* GetFactory() const { return factory; }
      virtual void SetFactory(CS::Physics::iVehicleWheelFactory* f) { factory = f; }

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelDriven() const { return isDriven; }
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelDriven(bool d) { isDriven = d; }

      /// Length of the suspension in equilibrium
      virtual csScalar GetSuspensionLength() const { return suspensionLength; }
      /// Length of the suspension in equilibrium
      virtual void SetSuspensionLength(csScalar s) { suspensionLength = s; }

      /// Radius of the wheel
      virtual csScalar GetRadius() const { return radius; }
      /// Radius of the wheel
      virtual void SetRadius(csScalar r) { radius = r; }

      /// The position of the wheel relative to the chassis
      const csVector3& GetWheelPos() const { return pos; }
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      void SetWheelPos(const csVector3& p) { pos = p; }

      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      csVector3 GetWheelOrientation() const { return wheelOrientation; }
      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      void SetSuspensionOrientation(const csVector3& o) { wheelOrientation = o; }

      /// Unit vector that describes the axle about which the wheel rotates
      const csVector3& GetAxleOrientation() const { return axleOrientation; }
      /// Unit vector that describes the axle about which the wheel rotates
      void SetAxleOrientation(const csVector3& o) { axleOrientation = o; }
    };


  /**
   * Spits out vehicles based on a given set of properties
   */
  class BulletVehicleFactory : public scfVirtImplementationExt1<
    BulletVehicleFactory, csObject, CS::Physics::iVehicleFactory>
  {
  private:
    csBulletSystem* sys;
    csRefArray<CS::Physics::iVehicleWheelInfo> wheelInfos;
    csRef<CS::Physics::iRigidBodyFactory> chassisFactory;

  public:
    BulletVehicleFactory(csBulletSystem* sys);
    virtual ~BulletVehicleFactory();

    /// Creates a new vehicle
    virtual csPtr<CS::Physics::iVehicle> CreateVehicle(CS::Physics::iPhysicalSector* sector);

    /// The factories for all wheels of the vehicle to be created
    virtual const csRefArray<CS::Physics::iVehicleWheelInfo>& GetWheelInfos() const { return wheelInfos; }
    /// The factories for all wheels of the vehicle to be created
    virtual csRefArray<CS::Physics::iVehicleWheelInfo>& GetWheelInfos() { return wheelInfos; }

    /// The chassis of the vehicle
    virtual CS::Physics::iRigidBodyFactory* GetChassisFactory() const { return chassisFactory; }
    /// The chassis of the vehicle
    virtual void SetChassisFactory(CS::Physics::iRigidBodyFactory* f) { chassisFactory = f; }
  };

  class BulletVehicle : public scfVirtImplementationExt2<BulletVehicle, csObject, 
    CS::Physics::iVehicle, BulletActionWrapper>
  {
    friend class BulletVehicleFactory;

  private:
    csBulletSystem* sys;
    btRaycastVehicle* btVehicle;
    csRef<CS::Physics::iRigidBody> chassis;
    csRefArray<CS::Physics::iVehicleWheel> wheels;

  public:
    BulletVehicle(csBulletSystem* sys, csBulletRigidBody* chassis);
    virtual ~BulletVehicle();

    /// The current engine force
    virtual csScalar GetEngineForce() const;
    /// The current engine force
    virtual void SetEngineForce(csScalar f);
    
      /// Apply the given brake for the next simulation step. Scale it's braking force with the given factor.
    virtual void ApplyBrake(CS::Physics::VehicleBrakeInfo* brake, csScalar factor = 1);

    /**
    * Use the given steering device to increase the steering angle of the device-controlled wheels 
    * and clamp to the device's allowed max angle.
    */
    virtual void IncrementSteering(CS::Physics::VehicleSteeringDevice* steeringWheel, csScalar increment);

    /// The amount of wheels that are directly powered by the engine
    virtual size_t GetDrivenWheelCount() const;

    /// The chassis of the vehicle
    virtual CS::Physics::iRigidBody* GetChassis() const { return chassis; }

    /// The chassis of the vehicle (does nothing because btRaycastVehicle doesn't support it)
    virtual void SetChassis(CS::Physics::iRigidBody* b) { /*chassis = b; b->SetMayBeDeactivated(false);*/ }
    
    /// The collision object associated with this updatable (if any)
    virtual CS::Collisions::iCollisionObject* GetCollisionObject() { return chassis; }

    /// All wheels of this vehicle
    virtual const csRefArray<CS::Physics::iVehicleWheel>& GetWheels() const { return wheels; }
    
    /// Current speed in km/h
    virtual csScalar GetSpeedKMH() const { return btVehicle->getCurrentSpeedKmHour(); }

    virtual void DoStep(csScalar dt);

    /// Called when updatable is added to the given sector
    virtual void OnAdded(CS::Physics::iPhysicalSector* sector);

    /// Called when updatable is removed from the given sector
    virtual void OnRemoved(CS::Physics::iPhysicalSector* sector);

    

    virtual btActionInterface* GetBulletAction() { return btVehicle; }
  };

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif