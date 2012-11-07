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

/**
 * Show some particles
 */

#include "physdemo.h"
#include "imesh/particles.h"

#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"

#include "imesh/emit.h"
#include "imesh/particles.h"

void PhysDemo::AddParticles(const csVector3& origin, float yFactor, int num)
{
  iSector* sector = view->GetCamera()->GetSector();

  // load texture
  const char* materialName = "raindrop";
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName (materialName);
  if (!mat)
  {
    ReportError("Can't find material %s in memory!", CS::Quote::Single (materialName));
    return;
  }


  // create particle mesh
  csRef<iMeshFactoryWrapper> mfw = engine->CreateMeshFactory ("crystalspace.mesh.object.particles", "fire");
  if (!mfw) return;


  csRef<iMeshWrapper> exp = engine->CreateMeshWrapper (mfw, "custom fire", sector, origin);

  exp->SetZBufMode(CS_ZBUF_TEST);
  exp->GetMeshObject()->SetMixMode (CS_FX_ADD);
  exp->GetMeshObject()->SetMaterialWrapper (mat);


  // create emittors and effectors
  csRef<iParticleBuiltinEmitterFactory> emit_factory = 
      csLoadPluginCheck<iParticleBuiltinEmitterFactory> (
        GetObjectRegistry(), "crystalspace.mesh.object.particles.emitter", false);
  csRef<iParticleBuiltinEffectorFactory> eff_factory = 
      csLoadPluginCheck<iParticleBuiltinEffectorFactory> (
        GetObjectRegistry(), "crystalspace.mesh.object.particles.effector", false);

  csRef<iParticleBuiltinEmitterSphere> sphemit = emit_factory->CreateSphere();
  float velocity = 0.4f;
  float seconds_to_live = 8.0f;
  sphemit->SetRadius (.2f);
  sphemit->SetParticlePlacement (CS_PARTICLE_BUILTIN_VOLUME);
  sphemit->SetEmissionRate (float (num) / seconds_to_live);
  sphemit->SetInitialMass (5.0f, 7.5f);
  sphemit->SetUniformVelocity (true);
  sphemit->SetInitialTTL (seconds_to_live, seconds_to_live);
  sphemit->SetInitialVelocity (csVector3 (0, yFactor * velocity, 0), csVector3 (0));

  //csRef<iParticleBuiltinEffectorLinColor> lincol = eff_factory->CreateLinColor();
  //lincol->AddColor (csColor4 (0.00f, 0.00f, 0.00f, 1.00f), 2.0000f);
  //lincol->AddColor (csColor4 (1.00f, 0.35f, 0.00f, 0.00f), 1.5000f);
  //lincol->AddColor (csColor4 (1.00f, 0.22f, 0.00f, 0.10f), 1.3125f);
  //lincol->AddColor (csColor4 (1.00f, 0.12f, 0.00f, 0.30f), 1.1250f);
  //lincol->AddColor (csColor4 (0.80f, 0.02f, 0.00f, 0.80f), 0.9375f);
  //lincol->AddColor (csColor4 (0.60f, 0.00f, 0.00f, 0.90f), 0.7500f);
  //lincol->AddColor (csColor4 (0.40f, 0.00f, 0.00f, 0.97f), 0.5625f);
  //lincol->AddColor (csColor4 (0.20f, 0.00f, 0.00f, 1.00f), 0.3750f);
  //lincol->AddColor (csColor4 (0.00f, 0.00f, 0.00f, 1.00f), 0.1875f);
  //lincol->AddColor (csColor4 (0.00f, 0.00f, 0.00f, 1.00f), 0.0000f);

  csRef<iParticleBuiltinEffectorForceWithCollisions> force = eff_factory->CreateForceWithCollisions(GetCurrentSector());
  //csRef<iParticleBuiltinEffectorForce> force = csPtr<iParticleBuiltinEffectorForce>(new ParticlePhysEffectorForce(GetCurrentSector()));
  force->SetAcceleration (csVector3(0.0f, -9.81f, 0.0f));
  force->SetRandomAcceleration (csVector3(1.5f, yFactor * 1.5f, 1.5f));

  csRef<iParticleSystem> partstate = scfQueryInterface<iParticleSystem> (exp->GetMeshObject());
  //partstate->SetMinBoundingBox (bbox);
  partstate->SetParticleSize (csVector2 (0.04f, 0.08f));
  partstate->AddEmitter (sphemit);
  //partstate->AddEffector (lincol);
  partstate->AddEffector (force);
}
