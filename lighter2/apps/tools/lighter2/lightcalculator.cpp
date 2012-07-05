/*
  Copyright (C) 2006 by Marten Svanfeldt

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

#include "common.h"

#include "lightcalculator.h"
#include "lightcomponent.h"

#include "scene.h"
#include "lightmap.h"


#include "photonmapperlighting.h"
#include "raytracerlighting.h"

namespace lighter
{
  //--------------------------------------------------------------------------
  LightCalculator::LightCalculator (const csVector3& tangentSpaceNorm, 
    size_t subLightmapNum) : tangentSpaceNorm (tangentSpaceNorm),
    fancyTangentSpaceNorm (!(tangentSpaceNorm - csVector3 (0, 0, 1)).IsZero ()),
    subLightmapNum (subLightmapNum),objReg(globalLighter->objectRegistry),scfImplementationType(this)
  {}

  LightCalculator::~LightCalculator() {}

  void LightCalculator::addComponent(LightComponent* newComponent, float coefficient, float offset)
  {
    component.push_back(newComponent);
    componentCoefficient.push_back(coefficient);
    componentOffset.push_back(offset);
  }

  THREADED_CALLABLE_IMPL3(LightCalculator,ComputeObjectGroupLighting,
      Sector* sector, csArray<csRef<lighter::Object> >* objGroup, Statistics::Progress* progress)
  {
    Statistics::ProgressState progState(*progress,globalStats.scene.numObjects,0.85f);

    // Initialize the sampler
    SamplerSequence<2> masterSampler;

    // Reset the object looper and loop though all object again
    csArray<csRef<lighter::Object> >::Iterator objIterator = objGroup->GetIterator();
    while (objIterator.HasNext ())
    {
      // Get reference to next object
      csRef<Object> obj = objIterator.Next ();

      // Skip unlight objects
      if (!obj->GetFlags ().Check (OBJECT_FLAG_NOLIGHT))
      {
        // Create the list of affecting lights in each component
        ComputeAffectingLights (obj);

        // Either light per vertex or add light to the lightmap
        if (obj->lightPerVertex)
        {
          ComputeObjectStaticLightingForVertex (
            sector, obj, masterSampler);
        }
        else
        {
          ComputeObjectStaticLightingForLightmap (
            sector, obj, masterSampler);
        }
      }
      progState.Advance();
    }
    progState.Finish();

    delete objGroup;

    return true;
  }

  csRefArray<iThreadReturn> LightCalculator::ComputeSectorStaticLighting (
    Sector* sector,bool enableRaytracer, bool enablePhotonMapper,
    Statistics::Progress& progress)
  {
    const csVector3 bases[4] =
    {
      csVector3 (0, 0, 1),
      csVector3 (/* -1/sqrt(6) */ -0.408248f, /* 1/sqrt(2) */ 0.707107f, /* 1/sqrt(3) */ 0.577350f),
      csVector3 (/* sqrt(2/3) */ 0.816497f, 0, /* 1/sqrt(3) */ 0.577350f),
      csVector3 (/* -1/sqrt(6) */ -0.408248f, /* -1/sqrt(2) */ -0.707107f, /* 1/sqrt(3) */ 0.577350f)
    };

    int numPasses = 
      globalConfig.GetLighterProperties().directionalLMs ? 4 : 1;

    // We multiply the number of thread by 2.0 because sometimes all the jobs
    // don't request the same amount of time
    const int numJobs = ((float)globalConfig.GetLighterProperties().numThreads)*2.0f;

    csRefArray<iThreadReturn> returns;

    // Loop through lighting calculation for directional dependencies
    for (int p = 0; p < numPasses; p++)
    {
      size_t totalElements = 0;
      csArray<csArray<csRef<lighter::Object> >*> objectsThreadTab;
      csArray<LightCalculator*> lightCalculators;

      for (int i =0; i < numJobs; i++)
      {
        objectsThreadTab.Push(new csArray<csRef<lighter::Object> >());
        LightCalculator* lighting =  new LightCalculator(bases[p],p);

        // Add components to the light calculator
        RaytracerLighting *raytracerComponent = NULL;
        PhotonmapperLighting *photonmapperComponent = NULL;

        if(enableRaytracer)
        {
          raytracerComponent = new RaytracerLighting (bases[p],p);
          lighting->addComponent(raytracerComponent, 1.0f, 0.0f);
        }

        if(enablePhotonMapper)
        {
          photonmapperComponent = new PhotonmapperLighting();
          lighting->addComponent(photonmapperComponent, 1.0f, 0.0f);
        }

        // Resize the effecting light list in each component
        for(size_t i=0; i<lighting->component.size(); i++)
          lighting->component[i]->resizeAffectingLights( sector->allNonPDLights.GetSize ());

        lightCalculators.Push(lighting);
      }

      int assignedThread=0;

      // Sum up total amount of elements for progress display purposes
      ObjectHash::GlobalIterator giter = sector->allObjects.GetIterator ();
      while (giter.HasNext ())
      {
        // Get the next object and skip unlight objects
        csRef<Object> obj = giter.Next ();
        if (!obj->GetFlags ().Check (OBJECT_FLAG_NOLIGHT))
        {

          // Count elements (vertices or primitives depending on global settings)
          if (obj->lightPerVertex)
            totalElements += obj->GetVertexData().positions.GetSize();

          else
          {
            // Loop through submesses to get a count all primitives
            csArray<PrimitiveArray>& submeshArray = obj->GetPrimitives ();
            for (size_t submesh = 0; submesh < submeshArray.GetSize (); ++submesh)
            {
              PrimitiveArray& primArray = submeshArray[submesh];

              for (size_t pidx = 0; pidx < primArray.GetSize (); ++pidx)
              {
                Primitive& prim = primArray[pidx];
                totalElements += prim.GetElementCount();
              }
            }
          }

          objectsThreadTab[assignedThread]->Push(obj);
          assignedThread = (assignedThread+1)%numJobs;
        }

      }

      for (int i =0; i < numJobs; i++)
      {
        returns.Push(lightCalculators[i]->ComputeObjectGroupLighting(sector,objectsThreadTab[i],&progress));
      }
    }

    return returns;
  }

  void LightCalculator::ComputeObjectStaticLightingForLightmap (
    Sector* sector, Object* obj, 
    SamplerSequence<2>& masterSampler)
  {
    // Get submesh list for looping through elements
    csArray<PrimitiveArray>& submeshArray = obj->GetPrimitives ();

    // Get list of pseudo-dynamic lights in this sector
    const LightRefArray& allPDLights = sector->allPDLights;

    // Local lists of pseudo-dynamic lights affecting this object
    // and the lightmaps associated with them
    LightRefArray PDLights;

    // Create list of pseudo-dynamic lights that will affect object
    for (size_t pdli = 0; pdli < allPDLights.GetSize (); ++pdli)
    {
      Light* pdl = allPDLights[pdli];
      if (pdl->GetBoundingSphere().TestIntersect (obj->GetBoundingSphere()))
      {
        PDLights.Push (pdl);
      }
    }

    // Loop through all submesh elements
    for (size_t submesh = 0; submesh < submeshArray.GetSize (); ++submesh)
    {
      // Loop through all element primitives    
      PrimitiveArray& primArray = submeshArray[submesh];

    #pragma omp parallel for
      for (size_t pidx = 0; pidx < primArray.GetSize (); ++pidx)
      {
        // Get next primitive
        Primitive& prim = primArray[pidx];

        // Get reference to this primitive's lightmap (non pseudo-dynamic)
        size_t numElements = prim.GetElementCount ();        
        Lightmap* normalLM = sector->scene->GetLightmap (
          prim.GetGlobalLightmapID (), subLightmapNum, (Light*)0);

        // Lock access to this lightmap
        ScopedSwapLock<Lightmap> lightLock (*normalLM);

        // This seems to have something to do with specular maps but I'm unsure ??
        bool recordInfluence =
          globalConfig.GetLighterProperties().specularDirectionMaps
          && (subLightmapNum == 0);

        // Rebuild the list of pseudo-dynamic lightmaps
        csArray<Lightmap*> pdLightLMs;
        for (size_t pdli = 0; pdli < PDLights.GetSize (); ++pdli)
        {
          // Get reference to this light's lightmap
          Lightmap* lm = sector->scene->GetLightmap (prim.GetGlobalLightmapID (),
            subLightmapNum, PDLights[pdli]);

          // Lock the lightmap for local use only and add it to the list
          lm->Lock ();
          pdLightLMs.Push (lm);
        }

        // Compute some object space metrics (minimums and offsets)
        csVector2 minUV = prim.GetMinUV ();
        const size_t uOffs = size_t (floorf (minUV.x));
        const size_t vOffs = size_t (floorf (minUV.y));

        // Iterate all primitive elements
      #pragma omp parallel for
        for (size_t eidx = 0; eidx < numElements; ++eidx)
        {
          // Skip empty elements
          Primitive::ElementType elemType = prim.GetElementType (eidx);
          if (elemType == Primitive::ELEMENT_EMPTY)
          {
            continue;
          }

          // Some stuff (Not sure of the purpose)
          ElementProxy ep = prim.GetElement (eidx);
          size_t u, v;
          prim.GetElementUV (eidx, u, v);
          u += uOffs;
          v += vOffs;

          // Ditto
          const float pixelAreaPart = 
            elemType == Primitive::ELEMENT_BORDER ? prim.ComputeElementFraction (eidx) : 
                                                    1.0f;

          // Compute lighting for non pseudo-dynamic lights
          csColor c(0, 0, 0);
          for(size_t i=0; i<component.size(); i++)
          {
            csColor value = 
                  component[i]->ComputeElementLightingComponent(sector,
                                    ep, masterSampler, recordInfluence);
            
            if(!value.IsBlack())
            {
              c += componentCoefficient[i] * value + componentOffset[i];
            }
          }

          // Update the normal lightmap
          normalLM->SetAddPixel (u, v, c * pixelAreaPart);

          // Loop through pseudo-dynamic lights
          for (size_t pdli = 0; pdli < PDLights.GetSize (); ++pdli)
          {    
            Lightmap* lm = pdLightLMs[pdli];
            Light* pdl = PDLights[pdli];

            // Compute lighting for one pseudo-dynamic light
            csColor c(0, 0, 0);
            for(size_t i=0; i<component.size(); i++)
            {
              if(component[i]->SupportsPDLights())
              {
                csColor value =
                  component[i]->ComputeElementLightingComponent(sector, ep,
                                        masterSampler, recordInfluence, pdl);

                if(!value.IsBlack())
                {
                  c += componentCoefficient[i] * value + componentOffset[i];
                }
              }
            }

            // Update this light's light map
	  #pragma omp critical
            lm->SetAddPixel (u, v, c * pixelAreaPart);
          }
        }

        // Release the locks on the pseudo-dynamic light maps
        // for this object
        for (size_t pdli = 0; pdli < PDLights.GetSize (); ++pdli)
        {
          pdLightLMs[pdli]->Unlock();
        }

      }   // End primitive loop
    }     // End submesh element loop
  }       // End function

  void LightCalculator::ComputeObjectStaticLightingForVertex (
    Sector* sector, Object* obj, 
    SamplerSequence<2>& masterSampler)
  {
    const LightRefArray& allPDLights = sector->allPDLights;
    LightRefArray PDLights;

    Object::LitColorArray* litColors = obj->GetLitColors (subLightmapNum);
    const ObjectVertexData& vdata = obj->GetVertexData ();

    for (size_t pdli = 0; pdli < allPDLights.GetSize (); ++pdli)
    {
      Light* pdl = allPDLights[pdli];
      if (pdl->GetBoundingSphere().TestIntersect (obj->GetBoundingSphere()))
      {
        PDLights.Push (pdl);
      }
    }

    for (size_t i = 0; i < vdata.positions.GetSize (); ++i)
    {
      csColor& c = litColors->Get (i);
      const csVector3& normal = ComputeVertexNormal (obj, i);
#ifdef DUMP_NORMALS
      const csVector3 normalBiased = normal*0.5f + csVector3 (0.5f);
      c = csColor (normalBiased.x, normalBiased.y, normalBiased.z);
#else
      const csVector3& pos = vdata.positions[i];
      c.Set(0, 0, 0);
      for(size_t j=0; j<component.size(); j++)
      {
        csColor value =
          component[j]->ComputePointLightingComponent(sector, obj, pos,
                              normal, masterSampler);

        if(!value.IsBlack())
        {
          c += componentCoefficient[j] * value + componentOffset[j];
        }
      }

      // Shade PD lights
      for (size_t pdli = 0; pdli < PDLights.GetSize (); ++pdli)
      {
        Light* pdl = PDLights[pdli];
        Object::LitColorArray* pdlColors = obj->GetLitColorsPD (pdl, subLightmapNum);
        csColor& c = pdlColors->Get (i);
        for(size_t j=0; j<component.size(); j++)
        {
          csColor value =
            component[j]->ComputePointLightingComponent(sector, obj, pos,
                                normal, masterSampler, pdl);

          if(!value.IsBlack())
          {
            c += componentCoefficient[j] * value + componentOffset[j];
          }
        }
      }
#endif
    }
  }

  void LightCalculator::ComputeAffectingLights (Object* obj)
  {
    Sector* sector = obj->GetSector();
    
    for (size_t i = 0; i < sector->allNonPDLights.GetSize(); i++)
    {
      Light* light = sector->allNonPDLights[i];
      bool effect = light->GetBoundingSphere().TestIntersect (obj->GetBoundingSphere());
      for(size_t j=0; j<component.size(); j++)
        component[j]->setAffectingLight( i, effect );
    }
  }

  csVector3 LightCalculator::ComputeVertexNormal (Object* obj, 
                                                 size_t index) const
  {
    if (fancyTangentSpaceNorm)
    {
      csMatrix3 ts = obj->GetTangentSpace (index);
      csVector3 v = ts * tangentSpaceNorm;
      v.Normalize();
      return v;
    }
    else
      return obj->GetVertexData().normals[index];
  }
};
