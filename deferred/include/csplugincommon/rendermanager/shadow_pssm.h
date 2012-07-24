/*
    Copyright (C) 2008 by Frank Richter

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_SHADOW_PSSM_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_SHADOW_PSSM_H__

/**\file
 * PSSM shadow handler
 */

#include "ivideo/shader/shader.h"

#include "csutil/cfgacc.h"

#include "cstool/meshfilter.h"

#include "csplugincommon/rendermanager/operations.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/shadow_common.h"
#include "csplugincommon/rendermanager/standardsorter.h"
#include "csplugincommon/rendermanager/viscull.h"

#include "csgeom/matrix4.h"
#include "csgeom/projections.h"

namespace CS
{
namespace RenderManager
{
  struct ShadowPSSMExtraMeshData
  {
  };

  csVector3 DowncastVector(const csVector4& v)
  {
    return csVector3(v.x, v.y, v.z);
  }

  csBox3 ProjectBox(const csBox3& box, const csTransform& transform, const CS::Math::Matrix4 proj)
  {
    CS::Math::Matrix4 projComp = proj * CS::Math::Matrix4(transform);
    csBox3 projBox;
    projBox.StartBoundingBox(DowncastVector(projComp * box.GetCorner(0)));

    for(int i = 1; i < 8; ++i)
    {
      projBox.AddBoundingVertexSmart(DowncastVector(projComp * box.GetCorner(i)));
    }

    return projBox;
  }

  template<typename RenderTreeType, typename LayerConfigType>
  class ShadowPSSM
  {
  public:
    // forward declarations
    struct LightData;
    struct PersistentData;

    // structure used by light setup - we only need to let it know the
    // number of sub lights we need - rest is done outside of light setup
    struct CachedLightData : public CS::Memory::CustomAllocated
    {
      // Number of virtual lights needed for this light
      uint GetSublightNum() const { return subLightNum; }

      /// Once per frame setup for light
      void SetupFrame(RenderTreeType&, ShadowPSSM&, iLight* l)
      {
	// set number of sub-lights
	subLightNum = l->GetType() == CS_LIGHT_POINTLIGHT ? 6 : 1;
      }

      /// Clear data needed for one frame after rendering
      void ClearFrameData() {}

    protected:
      uint subLightNum;
    };

    // context setup type for shadow map creation
    class ShadowContextSetup
    {
    public:
      // typedefs for convenience
      typedef ShadowContextSetup ThisType;
      typedef StandardPortalSetup<RenderTreeType, ThisType> PortalSetupType;

      ShadowContextSetup(const SingleRenderLayer& layerConfig, iShaderManager* shaderManager,
	typename PortalSetupType::PersistentData& portalPersist, int maxPortalRecurse, bool debugSplits)
	: layerConfig(layerConfig), shaderManager(shaderManager), portalPersist(portalPersist),
	  recurseCount(0), maxPortalRecurse(maxPortalRecurse), debugSplits(debugSplits)
      {
      }
    
      void operator()(typename RenderTreeType::ContextNode& context,
		      typename PortalSetupType::ContextSetupData& portalSetupData,
		      bool recursePortals = true)
      {
	if(recurseCount > maxPortalRecurse) return;

	CS::RenderManager::RenderView* rview = context.renderView;
	iSector* sector = rview->GetThisSector();
    
	// @@@ This is somewhat "boilerplate" sector/rview setup.
	sector->PrepareDraw(rview);

	// Make sure the clip-planes are ok
	CS::RenderViewClipper::SetupClipPlanes(rview->GetRenderContext());

	if(debugSplits)
	  context.owner.AddDebugClipPlanes(rview);
	
	// Do the culling
	iVisibilityCuller* culler = sector->GetVisibilityCuller();
	Viscull<RenderTreeType>(context, rview, culler);
	
	// Set up all portals
	if(recursePortals)
	{
	  recurseCount++;
	  PortalSetupType portalSetup(portalPersist, *this);
	  portalSetup(context, portalSetupData);
	  recurseCount--;
	}

	// Sort the mesh lists  
	{
	  StandardMeshSorter<RenderTreeType> mySorter(rview->GetEngine());
	  mySorter.SetupCameraLocation(rview->GetCamera()->GetTransform().GetOrigin());
	  ForEachMeshNode(context, mySorter);
	}
    
	// After sorting, assign in-context per-mesh indices
	{
	  SingleMeshContextNumbering<RenderTreeType> numbering;
	  ForEachMeshNode(context, numbering);
	}

	// Setup the SV arrays
	// Push the default stuff
	SetupStandardSVs(context, layerConfig, shaderManager, sector);
    
	// Setup the material&mesh SVs
	{
	  StandardSVSetup<RenderTreeType, SingleRenderLayer> svSetup(
	    context.svArrays, layerConfig);
    
	  ForEachMeshNode(context, svSetup);
	}
    
	SetupStandardShader(context, shaderManager, layerConfig);
    
	// Setup shaders and tickets
	SetupStandardTicket(context, shaderManager, layerConfig);
      }
    
    
    private:
      const SingleRenderLayer& layerConfig;
      iShaderManager* shaderManager;
      typename PortalSetupType::PersistentData& portalPersist;

      int recurseCount;
      int maxPortalRecurse;
      bool debugSplits;
    };

    // Data used by the shadow handler that needs to persist over multiple frames.
    struct PersistentData
    {
      // Called every frame/RenderView() execution, use for housekeeping.
      void UpdateNewFrame()
      {
	// iterate over all the lights
	typename LightHash::GlobalIterator lightIt = lightHash.GetIterator();
	while(lightIt.HasNext())
	{
	  csWeakRef<iLight> light;
	  LightData& lightData = lightIt.NextNoAdvance(light);

	  // clear data if light is gone or out of date
	  bool needsDelete = !light.IsValid();
	  if(!needsDelete)
	  {
	    if(lightData.updateNumber != light->GetMovable()->GetUpdateNumber())
	    {
	      needsDelete = true;
	    }
	  }

	  if(needsDelete)
	  {
	    lightHash.DeleteElement(lightIt);
	    continue;
	  }
	  lightIt.Advance();

	  // go over the frustums for this light and clear the temp data
	  for(size_t f = 0; f < lightData.frustums.GetSize(); ++f)
	  {
	    typename LightData::Frustum& frustum = lightData.frustums[f];

	    // @@@TODO: add slice caching
	    frustum.slicesHash.DeleteAll();
	  }
	}

        csTicks time = csGetTicks();
        settings.AdvanceFrame(time);
        lightVarsPersist.UpdateNewFrame();
	portalPersist.UpdateNewFrame();
      }

      // Set the prefix for configuration settings
      void SetConfigPrefix(const char* prefix)
      {
        configPrefix = prefix;
      }

      // Called upon plugin initialization
      void Initialize(iObjectRegistry* objectReg, RenderTreeBase::DebugPersistent& dbgPersist)
      {
	// get shader manager and graphics3d handles
	graphics3D = csQueryRegistry<iGraphics3D>(objectReg);
	shaderManager = csQueryRegistry<iShaderManager>(objectReg);
	{
	  // initialize SV IDs
	  iShaderVarStringSet* strings = shaderManager->GetSVNameStringset();
	  svNames.SetStrings(strings);

	  unscaleID = strings->Request("light shadow map unscale");
	  clipID = strings->Request("light shadow map clip");
	}

	// we need those infos after reading config, but only to calculate the splits
	float fixedCloseShadow;
	float farZ;

	// start with reading config settings
	{
	  csConfigAccess cfg(objectReg);

	  csString prefix(configPrefix);
	  prefix += '.';

	  // read shadow type settings
	  settings.ReadSettings(objectReg, cfg->GetStr(prefix + csString("ShadowsType"), "Depth"));

	  // read shadow settings
	  limitedShadow = cfg->GetBool(prefix + csString("LimitedShadow"), false);
	  // @@@QUESTION: old PSSM just increased numSplits by 1 unconditionally - why?
	  numSplits = cfg->GetInt(prefix + csString("NumSplits"), 2);
	  farZ = cfg->GetFloat(prefix + csString("FarZ"), 100);
	  maxPortalRecurse = cfg->GetInt(prefix + csString("MaxShadowPortalRecurse", 3));
	  int shadowMapRes = cfg->GetInt(prefix + csString("ShadowMapResolution"), 512);
	  shadowMapResD = cfg->GetInt(prefix + csString("ShadowMapResolution.Directional"), shadowMapRes);
	  shadowMapResP = cfg->GetInt(prefix + csString("ShadowMapResolution.Point"), shadowMapRes >> 1);
	  shadowMapResS = cfg->GetInt(prefix + csString("ShadowMapResolution.Spot"), shadowMapRes >> 1);
	  shadowMapResC = cfg->GetInt(prefix + csString("CloseShadowMapResolution"), shadowMapRes);
	  fixedCloseShadow = cfg->GetFloat(prefix + csString("FixedCloseShadow"), 0);
	  doFixedCloseShadow = fixedCloseShadow > 0 && fixedCloseShadow < farZ;

	  // read debug settings
	  dbgSplit = dbgPersist.RegisterDebugFlag("draw.pssm.split.frustum");
	  dbgShadowTex = dbgPersist.RegisterDebugFlag("textures.shadow");
	}

	// initialize empty SMs
	csRef<iTextureManager> tm = graphics3D->GetTextureManager();
	for(size_t i = 0; i < settings.targets.GetSize(); ++i)
	{
	  const ShadowSettings::Target* target = settings.targets[i];
	  const TextureCache& cache = target->texCache;
	  emptySM[target->attachment] = tm->CreateTexture(1, 1, csimg2D, cache.GetFormat(),
							  cache.GetFlags() | CS_TEXTURE_CREATE_CLEAR);
	}

	// initialize persistent context setup data
	portalPersist.Initialize(shaderManager, graphics3D, dbgPersist);
	layerConfig = SingleRenderLayer(settings.shadowShaderType, settings.shadowDefaultShader);

	// now set up our splitting scheme
	{
	  float n = SMALL_Z; // near plane
	  float f = farZ; // far plane

	  // fixed closed shadowing creates an extra slice
	  if(doFixedCloseShadow)
	  {
	    splitDists.Push(n);
	    n = fixedCloseShadow;
	  }

	  // practical split scheme uses a blend of logarithmic and uniform split schemes
	  const float logStep = pow(f/n, 1.0f/(float)numSplits); // logarithmic step
	  const float uniStep = (f/n-1.0f)/(float)numSplits; // uniform step
	  const float blend = 0.5f; // blend factor for mix(uni, log, blend)

	  // for N slices we need N+1 distances
	  for(int i = 0; i <= numSplits; ++i)
	  {
	    // calculate logarithmic split distance: (far/near)^(i/N)
	    const float logSplit = pow(logStep, i);

	    // calcualte uniform split distance: 1 + (far/near-1)*(i/N);
	    const float uniSplit = 1.0f+i*uniStep;

	    // blend both with (1-blend)*uni + blend*log
	    const float split = (1.0f - blend)*uniSplit + blend*logSplit;

	    // we factored out the near plane during the calculation, so multiply by it now
	    splitDists.Push(n*split);
	  }

	  if(doFixedCloseShadow)
	  {
	    // we have an extra slice for fixed close-up shadows
	    ++numSplits;
	  }
	}
      }

      // typedefs
      typedef csHash<LightData, csWeakRef<iLight>, CS::Memory::AllocatorMalloc,
	csArraySafeCopyElementHandler<CS::Container::HashElement<LightData, csWeakRef<iLight> > > > LightHash;

      // config settings
      csString configPrefix;

      // shadow settings
      ShadowSettings settings; // shadow type specific settings
      int numSplits; // number of slices per frustum
      bool doFixedCloseShadow; // special close up shadowing
      bool limitedShadow; // limited shadowing (inclusive instead of exclusive)
      int maxPortalRecurse; // portal traversal recursion limit

      // shadow map settings
      int shadowMapResD; // directional light
      int shadowMapResP; // point light
      int shadowMapResS; // spot light
      int shadowMapResC; // close up

      // debug settings
      uint dbgSplit;
      uint dbgShadowTex;

      // runtime data

      // object refs
      csRef<iShaderManager> shaderManager;
      csRef<iGraphics3D> graphics3D;

      // SV IDs
      csLightShaderVarCache svNames;
      LightingVariablesHelper::PersistentData lightVarsPersist;
      CS::ShaderVarStringID unscaleID;
      CS::ShaderVarStringID clipID;

      // empty shadow map set
      csRef<iTextureHandle> emptySM[rtaNumAttachments];

      // hash holding the data for all known lights
      LightHash lightHash;

      // array holding our splitting distances - those are constant for PSSM
      csArray<float> splitDists;

      // persistent data for context setup
      typename ShadowContextSetup::PortalSetupType::PersistentData portalPersist;
      SingleRenderLayer layerConfig;
    };

    // structure we're using to store light data
    struct LightData
    {
      // constructor - sets the projection and creates frustums
      LightData(iLight* l, PersistentData& persist)
      {
	// set our transform
	light2world = l->GetMovable()->GetFullTransform();

	// set our update number
	updateNumber = l->GetMovable()->GetUpdateNumber();

	// set projection transform
	{
	  // get cutoff distance
	  float cutoff = l->GetCutoffDistance();
	  if(l->GetType() == CS_LIGHT_DIRECTIONAL)
	  {
	    project = CS::Math::Projections::Ortho(
	      cutoff, -cutoff,
	      cutoff, -cutoff,
	     -cutoff, -SMALL_Z);
	  }
	  else
	  {
	    // @@@QUESTION: why is this needed?
	    CS::Math::Matrix4 flipZW(
	      1, 0, 0, 0,
	      0, 1, 0, 0,
	      0, 0, -1, 0,
	      0, 0, 0, -1);
	    project = flipZW * CS::Math::Projections::Frustum(
	      cutoff, -cutoff,
	      cutoff, -cutoff, // @@@ TODO: use spot angle for spot lights
	     -cutoff, -SMALL_Z);
	  }
	}

	// array of rotation matrices
	static const csMatrix3 rotations[] =
	{
	  csMatrix3(), // identity
          csMatrix3(1,  0, 0, // -90° about x
		    0,  0, 1,
		    0, -1, 0),
          csMatrix3(1,  0, 0, // +90° about x
		    0,  0,-1,
		    0,  1, 0),
          csMatrix3(0,  0,-1, // -90° about y
		    0,  1, 0,
		    1,  0, 0),
          csMatrix3(0,  0, 1, // +90° about y
		    0,  1, 0,
		   -1,  0, 0),
          csMatrix3(1,  0, 0, // 180° about x
		    0, -1, 0,
		    0,  0, -1)
	};

	// get light bounding box
        const csBox3& lightBox = l->GetLocalBBox();

	// check how many frustums we'll need and allocate them
	int numFrustums = l->GetType() == CS_LIGHT_POINTLIGHT ? 6 : 1;
	frustums.SetSize(numFrustums);

	// create frustums
	for(int i = 0; i < numFrustums; ++i)
	{
	  Frustum& frustum = frustums[i];

	  // init setup frame to minimum
	  frustum.setupFrame = 0;

	  // set mesh filter mode
	  if(persist.limitedShadow)
	    frustum.meshFilter.SetFilterMode(CS::Utility::MESH_FILTER_INCLUDE);

	  // set transform
	  csReversibleTransform frust2light(rotations[i], csVector3(0));
	  frustum.frust2light = frust2light;

	  // create unbounded frustum bounding box in frustum space
	  csBox3 boxFS = csBox3(csVector3(-FLT_MAX, -FLT_MAX, 0),
			        csVector3(FLT_MAX, FLT_MAX, FLT_MAX));;

	  // transform box to light space
	  frustum.boxLS = boxFS / frustum.frust2light;

	  // get intersection of frustum and light box
	  frustum.boxLS *= lightBox;
	}
      }

      // structures representing this light

      // structure that represents a single frustum slice
      // each frustum has multiple of those for each view
      struct Slice
      {
	// indicator whether this slice needs to be drawn,
	// i.e. whether it's bounding box is not empty in light space
	bool draw;

	// bounding box of this slice in projection space
	csBox3 boxPS;

	// bounding box of receivers in this slice in projection space
	csBox3 receiversPS;

	// @@@TODO: add slice caching
	//	    - csRefs for the SVs
	//	    - cached cam for the context (culling)
	// tmp SVs - uninit if Slice won't be drawn
	csShaderVariable* projectSV;
	csShaderVariable* unscaleSV;
	csShaderVariable* dimSV;
	csShaderVariable* clipSV;
	csShaderVariable* textureSV[rtaNumAttachments];
      };

      // structure holding all slices for a view together
      // with some meta-data
      struct Slices
      {
	uint setupFrame;
	csArray<Slice> slices;
      };

      // structure that represents a view frustum for our light
      // for point lights we have 6 of those, else just 1
      struct Frustum
      {
	// transform This == frustum Other == light
	csReversibleTransform frust2light;

	// bounding box of this frustum in light space
	csBox3 boxLS;

	// bounding box of casters in this frustum in projection space
	csBox3 castersPS;

	// frame at which casters were last updated
	uint setupFrame;

	// filter with all the meshes that should/shouldn't be drawn
	// (inclusive for limited shadow casting, exclusive otherwise)
	CS::Utility::MeshFilter meshFilter;

	// hash with the slice arrays for various views
	csHash<Slices, csWeakRef<CS::RenderManager::RenderView> > slicesHash;
      };

      // actual light data

      // transform This == light Other == world
      csReversibleTransform light2world;

      // movable update number
      long updateNumber;

      // projection matrix
      CS::Math::Matrix4 project;

      // the frustums that'll represent this light
      csArray<Frustum> frustums;
    };

    // Shadow method specific parameters
    // we do all the shadow setup here as we don't want to rely on light setup
    struct ShadowParameters
    {
    public:
      ShadowParameters(PersistentData& persist, RenderTreeType& renderTree, CS::RenderManager::RenderView* rview) :
	persist(persist), renderTree(renderTree), rview(rview), cam(rview->GetCamera()), sector(rview->GetThisSector()),
	svHelper(persist.lightVarsPersist), svNames(persist.svNames)
      {
      }

      // handle setup for a light - should be done before meshes are handled
      // creates frustum slices for the light in our view
      void operator()(iLight* light)
      {
	// check whether the light casts shadows
	if(light->GetFlags().Check(CS_LIGHT_NOSHADOWS))
	  return;

	typename PersistentData::LightHash& lightHash = persist.lightHash;

	// check whether this light is known already
	if(!lightHash.Contains(light))
	{
	  // new light, add it to cache
	  lightHash.Put(light, LightData(light, persist));
	}

	// get light data
	LightData& lightData = *lightHash[light];

	// get current frame
	uint currentFrame = rview->GetCurrentFrameNumber();

	// go over all frustums
	for(size_t f = 0; f < lightData.frustums.GetSize(); ++f)
	{
	  typename LightData::Frustum& frustum = lightData.frustums[f];

	  // check whether we already have slices set up
	  if(frustum.slicesHash.Contains(rview))
	  {
	    // we have one, check whether we set slices up this frame
	    if(frustum.slicesHash[rview]->setupFrame == currentFrame)
	    {
	      // nothing to be done
	      continue;
	    }
	  }
	  else
	  {
	    frustum.slicesHash.Put(rview, typename LightData::Slices());
	  }

	  // update frame setup
	  frustum.slicesHash[rview]->setupFrame = currentFrame;

	  // get slice array
	  csArray<typename LightData::Slice>& slices = frustum.slicesHash[rview]->slices;

	  // @@@TODO: add slice caching

	  // set size to num of slices we want
	  slices.SetSize(persist.numSplits);

	  // get corners of our view
	  int viewWidth = persist.graphics3D->GetWidth();
	  int viewHeight = persist.graphics3D->GetHeight();
	  csVector2 view[4] =
	  {
	    csVector2(0, 0),
	    csVector2(viewWidth, 0),
	    csVector2(viewWidth, viewHeight),
	    csVector2(0, viewHeight)
	  };

	  // get transform with This == Frustum and Other == View
	  csTransform frust2view = frustum.frust2light * lightData.light2world / cam->GetTransform();

	  // set the slice data
	  for(size_t s = 0; s < slices.GetSize(); ++s)
	  {
	    typename LightData::Slice& slice = slices[s];

	    // clear receiver box
	    slice.receiversPS.StartBoundingBox();

	    // create slice box in frustum space
	    csBox3 boxFS;
	    for(int c = 0; c < 4; ++c)
	    {
	      const csVector2& corner = view[c];
	      for(int side = 0; side < 2; ++side)
	      {
		// get corner in view space
		csVector3 vVS = cam->InvPerspective(corner, persist.splitDists[s+side]);

		// transform to frustum space
		csVector3 vFS = frust2view * vVS;

		// add corner to frustum space box
		boxFS.AddBoundingVertex(vFS);
	      }
	    }

	    if(renderTree.IsDebugFlagEnabled(persist.dbgSplit))
	    {
	      renderTree.AddDebugBBox(frustum.boxLS,
		lightData.light2world.GetInverse(),
		csColor(0, 1, 0));
	    }

	    // check if this is the fixed close up shadow slice if there is any
	    if(!(persist.doFixedCloseShadow && s == 0))
	    {
	      // intersect with frustum box for non-fixed slices
	      boxFS = boxFS * (frustum.frust2light * frustum.boxLS);
	    }

	    if(renderTree.IsDebugFlagEnabled(persist.dbgSplit))
	    {
	      renderTree.AddDebugBBox(boxFS,
		(frustum.frust2light * lightData.light2world).GetInverse(),
		csColor(1, 0, 0));
	    }

	    // transform box to projection space
	    slice.boxPS = ProjectBox(boxFS, csTransform(), lightData.project);

	    // check whether this slice will be drawn
	    slice.draw = !boxFS.Empty();
	  }
	}
      }

      // handles setup for a mesh node (simply forwards processing to HandleMesh for all contained meshes)
      void operator()(typename RenderTreeType::MeshNode* node)
      {
	for(size_t i = 0; i < node->meshes.GetSize(); ++i)
	{
	  HandleMesh(node->meshes[i]);
	}
      }

      // final handling during a context processing - done after all meshes are processed
      // creates SVs required for light setup for all non-empty slices
      void operator()()
      {
	// get current frame
	uint currentFrame = rview->GetCurrentFrameNumber();

	// go over all lights and get the frustums
	typename PersistentData::LightHash::GlobalIterator it = persist.lightHash.GetIterator();
	while(it.HasNext())
	{
	  // get light data
	  csWeakRef<iLight> light;
	  LightData& lightData = it.Next(light);

	  for(size_t f = 0; f < lightData.frustums.GetSize(); ++f)
	  {
	    // get our frustum
	    typename LightData::Frustum& frustum = lightData.frustums[f];

	    // skip frustum if it doesn't belong to our rview
	    if(!frustum.slicesHash.Contains(rview))
	      continue;

	    // keep track whether any slice will be drawn at all
	    bool draw = false;

	    // get slice array
	    csArray<typename LightData::Slice>& slices = frustum.slicesHash[rview]->slices;

	    // check which slices will be drawn
	    CS_ALLOC_STACK_ARRAY(bool, sliceDraw, slices.GetSize());
	    for(size_t s = 0; s < slices.GetSize(); ++s)
	    {
	      typename LightData::Slice& slice = slices[s];

	      // check whether this slice will be used
	      sliceDraw[s] = slice.draw & !slice.receiversPS.Empty();
	      draw |= sliceDraw[s];
	    }

	    // skip setup if this frustum won't be used
	    if(!draw)
	      continue;

	    // ensure projection space casters box and mesh filter are up to date
	    if(frustum.setupFrame != currentFrame)
	    {
	      // update setup-frame
	      frustum.setupFrame = currentFrame;

	      // update casters bounding box and filter
	      SetupFrustum(lightData, frustum);
	    }

	    // @@@TODO: could we use cubemaps for point lights?
	    // @@@TODO: could we draw all slices at once using instancing?

	    // setup the slices that'll be used
	    for(size_t s = 0; s < slices.GetSize(); ++s)
	    {
	      typename LightData::Slice& slice = slices[s];

	      // not used, skip
	      if(!sliceDraw[s])
		continue;

	      // set up SVs and create the target
	      SetupTarget(light, lightData, frustum, slice, persist.doFixedCloseShadow && s == 0);

	      // set clipping range
	      slice.clipSV->SetValue(csVector2(persist.splitDists[s], persist.splitDists[s+1]));
	    }
	  }
	}
      }

    protected:
      void SetupFrustum(LightData& lightData, typename LightData::Frustum& frustum)
      {
	// clear casters box
	frustum.castersPS.StartBoundingBox();

	// clear filter
	frustum.meshFilter.Clear();

	// get all meshes in the frustum box
	iVisibilityCuller* culler = sector->GetVisibilityCuller();
	csRef<iVisibilityObjectIterator> objects = culler->VisTest(frustum.boxLS / lightData.light2world);

	// calculate world -> light -> frustum transform
	csTransform frust2world = frustum.frust2light * lightData.light2world;

	// iterate over all meshes
	while(objects->HasNext())
	{
	  // get object
	  iVisibilityObject* object = objects->Next();

	  // get mesh wrapper
	  iMeshWrapper* meshWrapper = object->GetMeshWrapper();

	  // get mesh flags
	  csFlags meshFlags = meshWrapper->GetFlags();

	  // check whether this object is a caster in our mode
	  bool casting = (!persist.limitedShadow && !meshFlags.Check(CS_ENTITY_NOSHADOWCAST))
		      || ( persist.limitedShadow &&  meshFlags.Check(CS_ENTITY_LIMITEDSHADOWCAST));

	  // check whether we want this mesh filtered:
	  //   for limited casting casters are included
	  //   for normal casting non-casters are excluded
	  if(casting ^ !persist.limitedShadow)
	  {
	    frustum.meshFilter.AddFilterMesh(meshWrapper);
	  }

	  // if this mesh is a caster add it's bounding box to the caster box
	  if(casting)
	  {
	    // get world space bounding box
	    csBox3 meshBox = object->GetBBox();

	    // add projected bounding box to casters box
	    frustum.castersPS += ProjectBox(meshBox, frust2world, lightData.project);
	  }
	}
      }

      void SetupTarget(iLight* light, LightData& lightData, typename LightData::Frustum& frustum, typename LightData::Slice& slice, bool fixed)
      {
	// @@@TODO: add slice caching (persistent SVs, ...)

	// create SVs
	CS::ShaderVarStringID svID;

	// shadow map dimension
	svID = svNames.GetLightSVId(csLightShaderVarCache::lightShadowMapPixelSize);
	slice.dimSV = svHelper.CreateTempSV(svID);

	// projection
	svID = svNames.GetLightSVId(csLightShaderVarCache::lightShadowMapProjection);
	slice.projectSV = svHelper.CreateTempSV(svID);
    
	// unscale transform
	slice.unscaleSV = svHelper.CreateTempSV(persist.unscaleID);

	// clipping
	slice.clipSV = svHelper.CreateTempSV(persist.clipID);

	const ShadowSettings::TargetArray& targets = persist.settings.targets;
	for(size_t t = 0; t < targets.GetSize(); ++t)
	{
	  const ShadowSettings::Target* target = targets[t];
	  slice.textureSV[target->attachment] = svHelper.CreateTempSV(target->svName);
	}

	// setup everything

	// box we'll use while calculating the crop matrix
	csBox3 objectBox;

	// for fixed close up shadows we just use the frustum box
	if(fixed)
	{
	  objectBox = slice.boxPS;
	}
	// else we use the intersection of casters and receivers boxes
	else
	{
	  objectBox = frustum.castersPS * slice.receiversPS;
	}

	// calculate crop matrix and unscale transform
	CS::Math::Matrix4 crop;
	csVector4 unscale;

	bool empty = objectBox.Empty();

	if(empty)
	{
	  // nothing to be done for crop - it's already identity
	  crop = CS::Math::Projections::Ortho(-1,1,1,-1,1.0f - EPSILON, 1.0f);

	  unscale = csVector4(1,1,0,0);
	}
	else
	{
	  // set our z-range properly
	  float nearZ = csMin(objectBox.MinZ(), slice.boxPS.MinZ());
	  // + EPSILON in case nearZ == farZ
	  float farZ = csMin(objectBox.MaxZ(), slice.boxPS.MaxZ()) + EPSILON;

	  // convenience copies of some variables
	  const float& minX = objectBox.MinX();
	  const float& minY = objectBox.MinY();
	  const float& maxX = objectBox.MaxX();
	  const float& maxY = objectBox.MaxY();

	  // calculate crop scale
	  float cropScaleX = 2.0f/(maxX - minX);
	  float cropScaleY = 2.0f/(maxY - minY);
	  float cropScaleZ = 1.0f/(farZ - nearZ);

	  // calculate crop shift
	  float cropShiftX = (minX + maxX)/(minX - maxX);
	  float cropShiftY = (minY + maxY)/(minY - maxY);
	  float cropShiftZ = -nearZ * cropScaleZ;

	  // assemble matrix
	  crop = CS::Math::Matrix4(
	    cropScaleX,	0,	    0,		cropShiftX,
	    0,		cropScaleY, 0,		cropShiftY,
	    0,		0,	    cropScaleZ,	cropShiftZ,
	    0,		0,	    0,		1
	  );

	  // calculate uncrop scale
	  unscale.x = 0.5f * (maxX - minX);
	  unscale.y = 0.5f * (maxY - minY);

	  // calculate uncrop shift
	  unscale.z = 0.5f * (minX + maxX);
	  unscale.w = 0.5f * (minY + maxY);

	  // draw cropped box if debugging splits
	  if(renderTree.IsDebugFlagEnabled(persist.dbgSplit))
	  {
	    csBox3 debugBox = objectBox * slice.boxPS;
	    debugBox.SetMin(2, nearZ);
	    debugBox.SetMax(2, farZ);

	    renderTree.AddDebugBBox(ProjectBox(debugBox, csTransform(), lightData.project.GetInverse()),
	      (frustum.frust2light * lightData.light2world).GetInverse(),
	      csColor(0, 0, 1));
	  }
	}

	// set unscale
	slice.unscaleSV->SetValue(unscale);

	// set final projection
	CS::Math::Matrix4 project = crop * lightData.project * CS::Math::Matrix4(frustum.frust2light);
	slice.projectSV->SetValue(project);

	// get our shadow map size
	int mapSize;
	if(fixed)
	{
	  mapSize = persist.shadowMapResC;
	}
	else
	{
	  // for non-fixed slices res may depend on the light-type
	  switch(light->GetType())
	  {
	    case CS_LIGHT_DIRECTIONAL:
	      mapSize = persist.shadowMapResD;
	      break;

	    case CS_LIGHT_POINTLIGHT:
	      mapSize = persist.shadowMapResP;
	      break;

	    case CS_LIGHT_SPOTLIGHT:
	      mapSize = persist.shadowMapResS;
	      break;

	    default:
	      // unknown light-type, bail out - this MUSTN'T happen
	      CS_ASSERT(false);
	      break;
	  }
	}

	// set dimSV accordingly
	slice.dimSV->SetValue(csVector4(1.0f/mapSize,1.0f/mapSize,mapSize,mapSize));

	// for an empty target we don't have to draw anything
	if(empty)
	{
	  // set textures to empty ones
	  for(size_t t = 0; t < targets.GetSize(); ++t)
	  {
	    // setup target
	    ShadowSettings::Target* target = targets[t];
	    iTextureHandle* tex = persist.emptySM[target->attachment];
	    slice.textureSV[target->attachment]->SetValue(tex);

	    // add debug draw if wanted
	    if(renderTree.IsDebugFlagEnabled(persist.dbgShadowTex))
	      renderTree.AddDebugTexture(tex);
	  }
	  return;
	}

	// create camera
	csRef<iCustomMatrixCamera> shadowCam = rview->GetEngine()->CreateCustomMatrixCamera();
	shadowCam->SetProjectionMatrix(project);
	shadowCam->GetCamera()->SetTransform(lightData.light2world);

	// create render view
	csRef<CS::RenderManager::RenderView> shadowView;
	shadowView = renderTree.GetPersistentData().renderViews.CreateRenderView(rview);
	shadowView->SetEngine(rview->GetEngine());
	shadowView->SetThisSector(rview->GetThisSector());
	shadowView->SetMeshFilter(frustum.meshFilter);
	shadowView->SetViewDimensions(mapSize, mapSize);

	// set cam on our new view
	shadowView->SetCamera(shadowCam->GetCamera());
	shadowView->SetOriginalCamera(rview->GetOriginalCamera());

	// set clipper
	{
	  csBox2 clipBox(0, 0, mapSize, mapSize);
	  csRef<iClipper2D> clipper;
	  clipper.AttachNew(new csBoxClipper(clipBox));
	  shadowView->SetClipper(clipper);
	}

	// create context
	typename RenderTreeType::ContextNode* context = renderTree.CreateContext(shadowView);
	context->drawFlags = CSDRAW_CLEARSCREEN | CSDRAW_CLEARZBUFFER;
	context->postEffects = persist.settings.postEffects;

	// setup post-effects if required
	if(context->postEffects.IsValid())
	{
  	  CS::Math::Matrix4 perspectiveFixup;
	  context->postEffects->SetupView(mapSize, mapSize, perspectiveFixup);
	  context->perspectiveFixup = perspectiveFixup;
	}

	// setup rendertargets
	for(size_t t = 0; t < targets.GetSize(); ++t)
	{
	  // setup target
	  ShadowSettings::Target* target = targets[t];
	  iTextureHandle* tex = target->texCache.QueryUnusedTexture(mapSize, mapSize);
	  slice.textureSV[target->attachment]->SetValue(tex);
	  context->renderTargets[target->attachment].texHandle = tex;

	  // add debug draw if wanted
	  if(renderTree.IsDebugFlagEnabled(persist.dbgShadowTex))
	    renderTree.AddDebugTexture(tex);
	}

	// create portal data
	typename ShadowContextSetup::PortalSetupType::ContextSetupData portalData(context);
    
	// setup the new context
	ShadowContextSetup contextSetup(persist.layerConfig, persist.shaderManager,
					persist.portalPersist, persist.maxPortalRecurse,
					renderTree.IsDebugFlagEnabled(persist.dbgSplit));
	contextSetup(*context, portalData);
      }

      // handle setup for a mesh - done after all lights are known
      // this adds the mesh box in projection space the the frusta/slices
      // depending on it's settings and our shadowing mode to do scene-dependent
      // cropping for improved shadow map usage
      void HandleMesh(typename RenderTreeType::MeshNode::SingleMesh& mesh)
      {
	// get mesh bbox
	csBox3 meshBox(mesh.renderMesh->bbox);

	// get world2object transform so we don't compute it multiple times later
	csTransform world2object = mesh.renderMesh->object2world.GetInverse();

	// for all lights
	typename PersistentData::LightHash::GlobalIterator it = persist.lightHash.GetIterator();
	while(it.HasNext())
	{
	  // get light data
	  LightData& lightData = it.Next(/*light*/);

	  // transform mesh bounding box to light space
	  csTransform light2object = lightData.light2world * world2object;

	  // for all frustums
	  for(size_t f = 0; f < lightData.frustums.GetSize(); ++f)
	  {
	    // get the frustum to process
	    typename LightData::Frustum& frustum = lightData.frustums[f];

	    // check whether this frustum is setup for our rview
	    if(!frustum.slicesHash.Contains(rview))
	      continue;

	    // check whether the mesh receives shadows
	    if(!mesh.meshFlags.Check(CS_ENTITY_NOSHADOWRECEIVE))
	    {
	      // calculate object -> world -> light -> frustum transform
	      csTransform frust2object = frustum.frust2light * light2object;

	      // transform mesh bounding box to frustum space
	      // @@@TODO: we can save this projection if we first check whether we are in any slice
	      csBox3 meshBoxPS = ProjectBox(meshBox, frust2object, lightData.project);

	      // transform mesh bounding box to view space
	      csTransform view2mesh = cam->GetTransform() * world2object;
	      csBox3 meshBoxView = view2mesh * meshBox;

	      // get view-dependent furstum slices
	      csArray<typename LightData::Slice>& slices = frustum.slicesHash[rview]->slices;

	      // for all slices
	      for(size_t s = 0; s < slices.GetSize(); ++s)
	      {
		// get frustum slice to process
		typename LightData::Slice& slice = slices[s];

		// skip this slice if it won't be drawn
		if(!slice.draw)
		  continue;

		// skip this slice if the mesh isn't part of it
		if(meshBoxView.MaxZ() < persist.splitDists[s]
		  || meshBoxView.MinZ() > persist.splitDists[s+1])
		  continue;

		// skip this slice if the mesh doesn't intersect with it
		if(!slice.boxPS.Overlap(meshBoxPS))
		  continue;

		// add frustum space mesh box to receivers
		slice.receiversPS += meshBoxPS;
	      }
	    }
	  }
	}
      }


      PersistentData& persist;
      RenderTreeType& renderTree;
      CS::RenderManager::RenderView* rview;
      csWeakRef<iCamera> cam;
      csWeakRef<iSector> sector;

      LightingVariablesHelper svHelper;
      csLightShaderVarCache& svNames;
    };

    ShadowPSSM(PersistentData& persist,
      const LayerConfigType& layerConfig,
      typename RenderTreeType::MeshNode* node, 
      ShadowParameters& param) : persist(persist), rview(node->GetOwner().renderView)
    {
    }

    ShadowPSSM(PersistentData& persist, CS::RenderManager::RenderView* rview)
      : persist(persist), rview(rview)
    {
    }

    // forward rendering handlers

    // set up shadowing for a mesh-light combination - used by light setup
    uint HandleOneLight(typename RenderTreeType::MeshNode::SingleMesh& mesh,
                        iLight* light, CachedLightData&,
                        csShaderVariableStack* lightStacks,
                        uint l, uint f)
    {
      CS_ASSERT(rview);

      // @@@FIXME: light setup is broken and cannot handle different layer spreads for different lights
      return 1;

      // check whether the light creates shadows (if not there's nothing to be done)
      if(light->GetFlags().Check(CS_LIGHT_NOSHADOWS))
	return 1;

      // check whether the mesh receives shadows (if not there's nothing to be done)
      if(mesh.meshFlags.Check(CS_ENTITY_NOSHADOWRECEIVE))
	return 1;

      // check whether this light is known
      if(!persist.lightHash.Contains(light))
	return 1;

      // get mesh box in view space
      csTransform view2object = rview->GetCamera()->GetTransform() / mesh.renderMesh->object2world;
      csBox3 boxView = view2object * mesh.renderMesh->bbox;

      // get sv helper
      LightingVariablesHelper svHelper(persist.lightVarsPersist);

      // get our light data for that light
      LightData& lightData = *persist.lightHash.GetElementPointer(light);

      // get our frustum
      CS_ASSERT(f < lightData.frustums.GetSize());
      typename LightData::Frustum& frustum = lightData.frustums[f];

      // get the slices for our view
      CS_ASSERT(frustum.slicesHash.Contains(rview));
      csArray<typename LightData::Slice>& slices = frustum.slicesHash[rview]->slices;

      // go over the slices and check which ones will be used for this mesh
      uint spread = 0;
      uint index = 0;
      for(size_t s = 0; s < slices.GetSize(); ++s)
      {
	typename LightData::Slice& slice = slices[s];

	// check whether this slice is active
	if(!slice.draw)
	  continue;

	// check whether the mesh goes into that slice
	if(boxView.MaxZ() < persist.splitDists[s] || boxView.MinZ() > persist.splitDists[s+1])
	  continue;

	// we'll use this one, setup SVs
	svHelper.MergeAsArrayItem(lightStacks[index], slice.projectSV, l);
	svHelper.MergeAsArrayItem(lightStacks[index], slice.unscaleSV, l);
	svHelper.MergeAsArrayItem(lightStacks[index], slice.dimSV, l);
	// @@@TODO: why this if?
	if(lightStacks[index].GetSize() > slice.clipSV->GetName())
	  lightStacks[index][slice.clipSV->GetName()] = slice.clipSV;

	const ShadowSettings::TargetArray& targets = persist.settings.targets;
	for(size_t t = 0; t < targets.GetSize(); ++t)
	{
	  const ShadowSettings::Target* target = targets[t];
	  svHelper.MergeAsArrayItem(lightStacks[index], slice.textureSV[target->attachment], l);
	}

	// now mask the spread and increment the index
	spread |= 1 << index;
	++index;
      }

      return spread;
    }

    // return true when we need a final pass - used by light setup
    static bool NeedFinalHandleLight() { return false; }

    // final pass called by light setup for each light
    // if we returned true for NeedFinalHandleLight
    void FinalHandleLight(iLight*, CachedLightData&) { }

    // return which flags should be masked out for light comparison
    // by light setup
    csFlags GetLightFlagsMask() const { return csFlags(0); }
    
    // return up to how many layers shadows for a light may need in light setup
    size_t GetLightLayerSpread() const { return persist.numSplits; }

    // deferred handlers

    // set up shadowing for one sub-light for deferred rendering
    int PushVariables(iLight* light, uint f, csShaderVariableStack& svStack)
    {
      // deferred light renderer should only use us for shadow-casting lights
      CS_ASSERT(!light->GetFlags().Check(CS_LIGHT_NOSHADOWS));

      // get sv helper
      LightingVariablesHelper svHelper(persist.lightVarsPersist);

      // get our light data for that light
      typename PersistentData::LightHash& lightHash = persist.lightHash;
      CS_ASSERT(lightHash.Contains(light));
      LightData& lightData = *lightHash.GetElementPointer(light);

      // keep track into which index we'll push this map
      int index = 0;

      // get our frustum
      CS_ASSERT(f < lightData.frustums.GetSize());
      typename LightData::Frustum& frustum = lightData.frustums[f];

      // get the slices for our view
      CS_ASSERT(frustum.slicesHash.Contains(rview));
      csArray<typename LightData::Slice>& slices = frustum.slicesHash[rview]->slices;

      // go over the slices and check which ones will be used for this mesh
      for(size_t s = 0; s < slices.GetSize(); ++s)
      {
	typename LightData::Slice& slice = slices[s];

	// check whether this slice is active
	if(!slice.draw || slice.receiversPS.Empty())
	  continue;

	// we'll use this one, setup SVs
	bool success = true;
	success &= svHelper.MergeAsArrayItem(svStack, slice.projectSV, index);
	success &= svHelper.MergeAsArrayItem(svStack, slice.unscaleSV, index);
	success &= svHelper.MergeAsArrayItem(svStack, slice.dimSV, index);
	success &= svHelper.MergeAsArrayItem(svStack, slice.clipSV, index);

	const ShadowSettings::TargetArray& targets = persist.settings.targets;
	for(size_t t = 0; t < targets.GetSize(); ++t)
	{
	  const ShadowSettings::Target* target = targets[t];
	  success &= svHelper.MergeAsArrayItem(svStack, slice.textureSV[target->attachment], index);
	}
	CS_ASSERT(success);

	// increment index
	++index;
      }

      return index;
    }

    uint GetSublightNum(const iLight* light) const
    {
      return light->GetType() == CS_LIGHT_POINTLIGHT ? 6 : 1;
    }

  protected:
    PersistentData& persist;
    CS::RenderManager::RenderView* rview;
  };
} // RenderManager
} // CS

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_SHADOW_PSSM_H__
