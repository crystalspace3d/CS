#ifndef __PHYSPARTICLES_H__
#define __PHYSPARTICLES_H__

#include "cssysdef.h"
#include "csutil/scf_implementation.h"
#include "csutil/scf.h"

#include "ivaria/physics.h"
#include "ivaria/bullet2.h"
#include "ivaria/collisions.h"

#include "imesh/emit.h"
#include "imesh/particles.h"

/**
* Behaves like the default ParticleEffectorForce but does not penetrate physical boundaries
*/ 
class ParticlePhysEffectorForce : public 
  scfImplementation2<ParticlePhysEffectorForce,
  iParticleBuiltinEffectorForce,
  scfFakeInterface<iParticleEffector> >
{
private:
  csVector3 acceleration;
  csVector3 force;
  csVector3 randomAcceleration;
  bool do_randomAcceleration;

  csRef<CS::Collisions::iCollisionSector> collisionSector;

public:
  ParticlePhysEffectorForce (csRef<CS::Collisions::iCollisionSector> collisionSector)
    : scfImplementationType (this),
    collisionSector(collisionSector),
    acceleration (0.0f), force (0.0f), randomAcceleration (0.0f, 0.0f, 0.0f),
    do_randomAcceleration (false)
  {
  }

  //-- iParticleEffector
  virtual csPtr<iParticleEffector> Clone () const;

  virtual void EffectParticles (iParticleSystemBase* system,
    const csParticleBuffer& particleBuffer, float dt, float totalTime);

  //-- iParticleBuiltinEffectorForce
  virtual void SetAcceleration (const csVector3& acceleration)
  {
    this->acceleration = acceleration;
  }

  virtual const csVector3& GetAcceleration () const
  {
    return acceleration;
  }

  virtual void SetForce (const csVector3& force)
  {
    this->force = force;
  }

  virtual const csVector3& GetForce () const
  {
    return force; 
  }

  virtual void SetRandomAcceleration (const csVector3& magnitude)
  {
    randomAcceleration = magnitude;
    if (randomAcceleration < .000001f)
      do_randomAcceleration = false;
    else
      do_randomAcceleration = true;
  }

  virtual const csVector3& GetRandomAcceleration () const
  {
    return randomAcceleration;
  }
};


#endif

