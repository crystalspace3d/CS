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

#ifndef __IVARIA_VEHICLE__
#define __IVARIA_VEHICLE__

#include "csutil/csobject.h"
#include "csutil/array.h"
#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "iutil/objreg.h"
#include "iengine/engine.h"
#include "cstool/primitives.h"

#include "ivaria/collisionscommon.h"
#include "ivaria/physicscommon.h"

namespace CS 
{ 
  namespace Collisions 
  {
    struct iCollider;
    struct iCollisionCallback;
    struct iCollisionObject;
  }
}

namespace CS 
{
  namespace Physics 
  {
    struct iJoint;
    struct iObject;
    struct iRigidBody;
    struct iSoftBody;
    struct iDynamicActor;
    struct iKinematicCallback;
    struct iPhysicalSystem;
    struct iPhysicalSector;
    struct iRigidBodyFactory;

    struct iVehicle;
    struct iVehicleWheelFactory;

    struct iVehicleWheel : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleWheel, 1, 0, 0);

      /// SceneNode that represents this wheel
      virtual iSceneNode* GetSceneNode() const = 0;
      /// SceneNode that represents this wheel
      virtual void SetSceneNode(iSceneNode* node) = 0;

      /// Collider of the wheel
      virtual CS::Collisions::iCollider* GetCollider() const = 0;
      /// Collider of the wheel
      virtual void SetCollider(CS::Collisions::iCollider* s) = 0;

      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual csScalar GetSuspensionStiffness() const = 0;
      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual void SetSuspensionStiffness(csScalar s) = 0;

      /// Suspension with more damping needs more force to be compressed and to relax
      virtual csScalar GetSuspensionDamping() const = 0;
      /// Suspension with more damping needs more force to be compressed and to relax
      virtual void SetSuspensionDamping(csScalar s) = 0;

      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual csScalar GetMaxSuspensionDisplacementCM() const = 0;
      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual void SetMaxSuspensionDisplacementCM(csScalar s) = 0;

      /** 
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual csScalar GetFrictionCoefficient() const = 0;
      /** 
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual void SetFrictionCoefficient(csScalar s) = 0;

      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual csScalar GetMaxSuspensionForce() const = 0;
      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual void SetMaxSuspensionForce(csScalar s) = 0;
      
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual csScalar GetRollInfluence() const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetRollInfluence(csScalar infl) = 0;
      

      // Geometry & Other

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelDriven() const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelDriven(bool d) = 0;

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelAffectedByBrake() const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelAffectedByBrake(bool b) = 0;

      /// Length of the suspension in equilibrium
      virtual csScalar GetSuspensionLength() const = 0;
      /// Length of the suspension in equilibrium
      virtual void SetSuspensionLength(csScalar s) = 0;

      /// Radius of the wheel
      virtual csScalar GetRadius() const = 0;
      /// Radius of the wheel
      virtual void SetRadius(csScalar s) = 0;

      /// The position of the wheel relative to the chassis
      virtual csVector3 GetWheelPos() const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetWheelPos(const csVector3& p) = 0;

      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual csVector3 GetWheelOrientation() const = 0;
      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual void SetSuspensionOrientation(const csVector3& o) = 0;

      /// Unit vector that describes the axle about which the wheel rotates
      virtual csVector3 GetAxleOrientation() const = 0;
      /// Unit vector that describes the axle about which the wheel rotates
      virtual void SetAxleOrientation(const csVector3& o) = 0;
      

      // Run-time parameters

      /// Rotation in radians
      virtual const csScalar GetRotation() const = 0;
      /// Rotation in radians
      virtual void SetRotation(csScalar r) = 0;
    };

    struct iVehicleWheelFactory : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleWheelFactory, 1, 0, 0);
      ///// Creates a new wheel
      //virtual csPtr<iVehicleWheel> CreateWheel() = 0;

      /// Collider of the wheel
      virtual CS::Collisions::iCollider* GetCollider() const = 0;
      /// Collider of the wheel
      virtual void SetCollider(CS::Collisions::iCollider* s) = 0;

      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual csScalar GetSuspensionStiffness() const = 0;
      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual void SetSuspensionStiffness(csScalar s) = 0;

      /// Suspension with more damping needs more force to be compressed and to relax
      virtual csScalar GetSuspensionDamping() const = 0;
      /// Suspension with more damping needs more force to be compressed and to relax
      virtual void SetSuspensionDamping(csScalar s) = 0;

      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual csScalar GetMaxSuspensionDisplacementCM() const = 0;
      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual void SetMaxSuspensionDisplacementCM(csScalar s) = 0;

      /** 
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual csScalar GetFrictionCoefficient() const = 0;
      /** 
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual void SetFrictionCoefficient(csScalar s) = 0;

      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual csScalar GetMaxSuspensionForce() const = 0;
      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual void SetMaxSuspensionForce(csScalar s) = 0;
      
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual csScalar GetRollInfluence() const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetRollInfluence(csScalar infl) = 0;
    };

    /**
     * All info needed to produce a specific instance of a wheel, using a factory and geometric details.
     */
    struct iVehicleWheelInfo : public virtual iBase
    {
      /// The factory to produce new wheels
      virtual iVehicleWheelFactory* GetFactory() const = 0;
      /// The factory to produce new wheels
      virtual void SetFactory(iVehicleWheelFactory* f) = 0;

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelDriven() const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelDriven(bool d) = 0;

      /// Length of the suspension in equilibrium
      virtual csScalar GetSuspensionLength() const = 0;
      /// Length of the suspension in equilibrium
      virtual void SetSuspensionLength(csScalar s) = 0;

      /// Radius of the wheel
      virtual csScalar GetRadius() const = 0;
      /// Radius of the wheel
      virtual void SetRadius(csScalar r) = 0;

      /// The position of the wheel relative to the chassis
      virtual const csVector3& GetWheelPos() const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetWheelPos(const csVector3& p) = 0;

      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual csVector3 GetWheelOrientation() const = 0;
      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual void SetSuspensionOrientation(const csVector3& o) = 0;

      /// Unit vector that describes the axle about which the wheel rotates
      virtual const csVector3& GetAxleOrientation() const = 0;
      /// Unit vector that describes the axle about which the wheel rotates
      virtual void SetAxleOrientation(const csVector3& o) = 0;
    };

    /**
     * TODO: Wrap other array methods, too.
     */
    struct iVehicleFactory : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleFactory, 1, 0, 0);


      /// Creates a new vehicle
      virtual csPtr<iVehicle> CreateVehicle(CS::Physics::iPhysicalSector* sector) = 0;

      /// The info for all wheels of the vehicle to be created
      virtual const csRefArray<iVehicleWheelInfo>& GetWheelInfos() const = 0;

      /// Add a new wheel to this vehicle factory
      virtual void AddWheelInfo(iVehicleWheelInfo* info) = 0;

      /// The chassis of the vehicle
      virtual iRigidBodyFactory* GetChassisFactory() const = 0;
      /// The chassis of the vehicle
      virtual void SetChassisFactory(iRigidBodyFactory* b) = 0;
    };
    
    /** 
     * A brake acts on a given set of tires with a given amount of force.
     */
    class VehicleBrakeInfo : public scfImplementationExt0<VehicleBrakeInfo, csObject>
    {
      csScalar maxForce;
      csArray<size_t> affectedWheelIndices;

    public:
      VehicleBrakeInfo(csScalar maxForce = 1000) : scfImplementationType(this),
        maxForce(maxForce)
      {
      }

      virtual ~VehicleBrakeInfo() {}

      /// Max breaking force
      csScalar GetMaxForce() const { return maxForce; }
      /// Max breaking force
      void SetMaxForce(csScalar f) { maxForce = f; }

      /// The amount of wheels affected by this brake
      size_t GetAffectedWheelCount() const { return affectedWheelIndices.GetSize(); }
      
      /// The indices of the wheels to be affected by this brake
      csArray<size_t>& GetAffectedWheelIndices() { return affectedWheelIndices; }
      /// The indices of the wheels to be affected by this brake
      const csArray<size_t>& GetAffectedWheelIndices() const { return affectedWheelIndices; }
    };
    
    /** 
     * A steering device steers a given set of tires with a given max amount 
     */
    class VehicleSteeringDevice : public scfImplementationExt0<VehicleSteeringDevice, csObject>
    {
      csScalar maxSteering;
      csArray<size_t> affectedWheelIndices;

    public:
      VehicleSteeringDevice(csScalar maxSteering = csScalar(.4)) : scfImplementationType(this),
        maxSteering(maxSteering)
      {
      }

      virtual ~VehicleSteeringDevice() {}

      /// Max breaking force
      csScalar GetMaxSteering() const { return maxSteering; }
      /// Max breaking force
      void SetMaxSteering(csScalar s) { maxSteering = s; }

      /// The amount of wheels affected by this brake
      size_t GetAffectedWheelCount() const { return affectedWheelIndices.GetSize(); }

      /// The indices of the wheels to be affected by this brake
      csArray<size_t>& GetAffectedWheelIndices() { return affectedWheelIndices; }
      /// The indices of the wheels to be affected by this brake
      const csArray<size_t>& GetAffectedWheelIndices() const { return affectedWheelIndices; }
    };

    struct iVehicle : public virtual iUpdatable
    {
      SCF_INTERFACE (CS::Physics::iVehicle, 1, 0, 0);
      
      /// The engine force for the next time-step
      virtual csScalar GetEngineForce() const = 0;
      /// The engine force for the next time-step
      virtual void SetEngineForce(csScalar f) = 0;

      /// The amount of wheels affected by engine force
      virtual size_t GetDrivenWheelCount() const = 0;

      /// Apply the given brake for the next simulation step. Scale it's braking force with the given factor.
      virtual void ApplyBrake(VehicleBrakeInfo* brake, csScalar factor = 1) = 0;

      /**
       * Use the given steering device to increase the steering angle of the device-controlled wheels 
       * and clamp to the device's allowed max angle.
       */
      virtual void IncrementSteering(VehicleSteeringDevice* steeringWheel, csScalar increment) = 0;

      /// The chassis of the vehicle
      virtual iRigidBody* GetChassis() const = 0;
      /// The chassis of the vehicle
      virtual void SetChassis(iRigidBody* b) = 0;

      /// All wheels of this vehicle
      virtual const csRefArray<iVehicleWheel>& GetWheels() const = 0;

      /// Current speed in km/h
      virtual csScalar GetSpeedKMH() const = 0;
    };
  }
}

#endif
