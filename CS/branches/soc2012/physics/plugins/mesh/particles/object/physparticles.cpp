/**
* Take care of particles that interact with the physical world
*/

#include "physparticles.h"

#include "iengine/sector.h"
#include "iengine/movable.h"

#include "csutil/floatrand.h"

using namespace CS::Collisions;

CS_IMPLEMENT_STATIC_VAR(GetVGen, csRandomVectorGen, ());

ParticlePhysEffectorForce::ParticlePhysEffectorForce (csRef<CS::Collisions::iCollisionSector> collisionSector)
  : scfImplementationType (this),
  collisionSector(collisionSector),
  acceleration (collisionSector->GetGravity()), force (0.0f), randomAcceleration (0.0f, 0.0f, 0.0f),
  do_randomAcceleration (false)
{
  SetRestitutionMagnitude(0.5);
}

csPtr<iParticleEffector> ParticlePhysEffectorForce::Clone () const
{
  return 0;
}

void ParticlePhysEffectorForce::EffectParticles (iParticleSystemBase* system,
  const csParticleBuffer& particleBuffer, float dt, float totalTime)
{
  for (size_t idx = 0; idx < particleBuffer.particleCount; ++idx)
  {
    csParticle& particle = particleBuffer.particleData[idx];
    csParticleAux& particleAux = particleBuffer.particleAuxData[idx];

    csVector3 a = acceleration;

    if (do_randomAcceleration)
    {
      csVector3 r = GetVGen()->Get ();
      a.x += r.x * randomAcceleration.x;
      a.y += r.y * randomAcceleration.y;
      a.z += r.z * randomAcceleration.z;
    }

    // check for collision
    csVector3 newVel = particle.linearVelocity + (a + force / particle.mass) * dt;
    csVector3 newPos = particle.position + newVel * dt;

    csRef<iMeshObject> meshObj = scfQueryInterface<iMeshObject>(system);
    iMeshWrapper* wrapper = meshObj->GetMeshWrapper();
    iMovable* movable = wrapper->GetMovable();
    csReversibleTransform trans = movable->GetFullTransform();

    csVector3 currentPos = trans.This2Other(particle.position);
    newPos = trans.This2Other(newPos);

    HitBeamResult hitResult = collisionSector->HitBeam(currentPos, newPos);
    if (hitResult.hasHit)
    {
      // resolve collision
      particle.position = trans.Other2This(hitResult.isect);
      hitResult.normal.Normalize();

      float normalSpeed = hitResult.normal * newVel;
      csVector3 normalVel = normalSpeed * hitResult.normal;
      csVector3 tangentialVel = newVel - normalVel;

      newVel = -restitution.x * normalVel + restitution.y * tangentialVel;
      //particleAux.color = csColor4(1, 0.6, 0.6);
    }
    particle.linearVelocity = newVel;
  }
}