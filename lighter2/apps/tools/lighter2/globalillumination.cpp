
#include "common.h"
#include "Photonmap.h"
#include "lighter.h"
#include "globalillumination.h"
#include "primitive.h"
#include "lighter.h"
#include "lightmap.h"
#include "lightmapuv.h"
#include "lightmapuv_simple.h"
#include "primitive.h"
#include "raygenerator.h"
#include "raytracer.h"
#include "scene.h"

namespace lighter
{
  GIRunnable::GIRunnable(Sector *sect)
  {
    sector = sect;
  }

  void GIRunnable::Run()
  {
    //GlobalIllumination gi;
    //gi.ShadeIndirectLighting(sector);
  }

  GlobalIllumination::GlobalIllumination()
  {
    finalGather = false;
  }

  GlobalIllumination::GlobalIllumination(Configuration::INDIProperties config)
  {
    finalGather = config.finalGather;
    numFinalGatherRays = config.numFinalGatherRays;
  }
	
  void GlobalIllumination::ShadeIndirectLighting(Sector *sect)
  {
    ObjectHash::GlobalIterator gitr = sect->allObjects.GetIterator();

    while (gitr.HasNext())
    {
      csRef<Object> obj = gitr.Next();
      csArray<PrimitiveArray>& submeshArray = obj->GetPrimitives ();

      for (size_t submesh = 0; submesh < submeshArray.GetSize (); ++submesh)
      {
        PrimitiveArray& primArray = submeshArray[submesh];

        float area2pixel = 1.0f;

        for (size_t pidx = 0; pidx < primArray.GetSize (); ++pidx)
        {
          Primitive& prim = primArray[pidx];

          area2pixel = 
            1.0f / (prim.GetuFormVector () % prim.GetvFormVector ()).Norm();

          //const ElementAreas& areas = prim.GetElementAreas ();
          size_t numElements = prim.GetElementCount ();        
          Lightmap* normalLM = sect->scene->GetLightmap (
            prim.GetGlobalLightmapID (), 0, (Light*)0);

          csVector2 minUV = prim.GetMinUV ();

          // Iterate all elements
          for (size_t eidx = 0; eidx < numElements; ++eidx)
          {
            //const float elArea = areas.GetElementArea (eidx);
            Primitive::ElementType elemType = prim.GetElementType (eidx);

            ElementProxy ep = prim.GetElement (eidx);
            size_t u, v;
            prim.GetElementUV (eidx, u, v);
            u += size_t (floorf (minUV.x));
            v += size_t (floorf (minUV.y));

            //float pixelArea = (elArea*area2pixel);
            const float pixelAreaPart = 
              elemType == Primitive::ELEMENT_BORDER ? prim.ComputeElementFraction (eidx) : 
                                                      1.0f;

            // Get the position, radius,and normal to pull from
            csVector3 pos = ep.primitive.ComputeElementCenter(ep.element);
            // TODO: Pull this attribute from an outside source
            float radius = 1.0f;
            csVector3 normal = ep.primitive.ComputeNormal(pos);

            // Pull the color from the photon map
            csColor c;
            // check to make sure the photon map exists
            if (sect->photonMap)
            {
              // check to see if we are supposed to do a final gather
              if (finalGather)
              {
                // average over the number of FG rays
                csColor final;
                for (size_t num = 0; num < numFinalGatherRays; ++num)
                {
                  // Todo:: Need to change this so we don't sample vectors
                  // that will point to the current surface we are on
                  lighter::HitPoint hit;
                  hit.distance = FLT_MAX*0.9f;
                  lighter::Ray ray;
                  ray.direction = randVect.Get();
                  ray.origin = pos;
                  ray.minLength = 0.01f;

                  // raytrace another ray
                  if (lighter::Raytracer::TraceClosestHit(sect->kdTree, ray, hit))
                  {
                    if (hit.primitive)
                    {
                      csVector3 hNorm = 
                        hit.primitive->ComputeNormal(hit.hitPoint);
                      final += 
                        sect->photonMap->SampleColor(hit.hitPoint, radius, hNorm);
                    }
                  }
                }
                // average the color
                c = final * (1.0 / numFinalGatherRays);
              }
              else
              {
                c = sect->photonMap->SampleColor(pos, radius, normal);
              }
            }
            
            normalLM->SetAddPixel (u, v, c * pixelAreaPart);
          }
        }
      }
    }

    
  }

  void GlobalIllumination::EmitPhotons(Sector *sect)
  {
    // TODO: Pull from some other area
    int photonsToEmitPerLight = 1000;

    // emit from the lights
    const LightRefArray& allNonPDLights = sect->allNonPDLights;
    for (size_t pdli = 0; pdli < allNonPDLights.GetSize(); ++pdli)
    {
      Light* pdl = allNonPDLights[pdli];
      const csVector3& pos = pdl->GetPosition();
      const csColor& color = pdl->GetColor();
      const csColor& power = pdl->GetPower();
      csVector3 dir;

      // send out photons in random directions since we only have point
      // lights at this point
      for (size_t num = 0; num < photonsToEmitPerLight; ++num)
      {
        // generate new random direction vector
        dir = randVect.Get();
        sect->EmitPhoton(pos, dir, color, power);
      }
    }
  }
}
