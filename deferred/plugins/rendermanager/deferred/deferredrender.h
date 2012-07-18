/*
    Copyright (C) 2010 by Joe Forte

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __DEFERREDRENDER_H__
#define __DEFERREDRENDER_H__

#include "cssysdef.h"

#include "csplugincommon/rendermanager/render.h"
#include "csplugincommon/rendermanager/rendertree.h"

#include "deferredlightrender.h"
#include "deferredoperations.h"
#include "gbuffer.h"

CS_PLUGIN_NAMESPACE_BEGIN(RMDeferred)
{
  /**
   * Deferred renderer for multiple contexts.
   *
   * Example:
   * \code
   * // ... contexts setup etc. ...
   *
   * {
   *   DeferredTreeRenderer<RenderTree> 
   *     render (graphics3D, shaderManager, stringSet, 
   *             lightRenderPersistent, deferredLayer, 
   *             zonlyLayer, drawLightVolumes);
   *
   *   ForEachContextReverse (renderTree, render);
   * }
   *
   * // ... apply post processing ...
   * \endcode
   */
  template<typename RenderTree, typename ShadowHandler>
  class DeferredTreeRenderer
  {
  public:

    typedef typename RenderTree::ContextNode ContextNodeType;
    typedef CS::RenderManager::SimpleContextRender<RenderTree> RenderType;
    typedef DeferredLightRenderer<ShadowHandler> LightRenderType;

    DeferredTreeRenderer(iGraphics3D *g3d, 
                         iShaderManager *shaderMgr,
                         iStringSet *stringSet,
                         typename LightRenderType::PersistentData &lightRenderPersistent,
			 GBuffer& gbuffer,
                         int deferredLayer,
			 int lightingLayer,
                         int zonlyLayer,
                         bool drawLightVolumes)
      : 
    meshRender(g3d, shaderMgr),
    graphics3D(g3d),
    shaderMgr(shaderMgr),
    stringSet(stringSet),
    lightRenderPersistent(lightRenderPersistent),
    gbuffer(gbuffer),
    deferredLayer(deferredLayer),
    lightingLayer(lightingLayer),
    zonlyLayer(zonlyLayer),
    drawLightVolumes(drawLightVolumes),
    useDeferredShading(lightingLayer < 0),
    context(nullptr),
    rview(nullptr),
    hasTarget(false)
    {}

    ~DeferredTreeRenderer() 
    {
      if(context) RenderContextStack ();
    }

    /**
     * Render all contexts.
     */
    void operator()(ContextNodeType* newContext)
    {
      if (IsNew (newContext))
      {
        // New context, render out the old ones
        if(context) RenderContextStack ();

	// set comparison variables accordingly
        context = newContext;
        rview = context->renderView;

	// check whether this stack will be rendered off-screen
	hasTarget = false;
	for(int i = 0; i < rtaNumAttachments; ++i)
	{
	  if(context->renderTargets[i].texHandle != (iTextureHandle*)nullptr)
	  {
	    hasTarget = true;
	    break;
	  }
	}
      }
      
      contextStack.Push (context);
    }

  protected:

    /**
     * Returns true if the given context is different from the last context.
     */
    bool IsNew(ContextNodeType* newContext)
    {
      if(!context) return true;
      else return !HasSameTargets(newContext) || !HasSameRenderView(newContext);
    }

    /**
     * Returns true if the given context has the same target buffers 
     * as the last context.
     */
    bool HasSameTargets(ContextNodeType* newContext)
    {
      for(int i = 0; i < rtaNumAttachments; ++i)
      {
	if(newContext->renderTargets[i].subtexture != context->renderTargets[i].subtexture
	|| newContext->renderTargets[i].texHandle  != context->renderTargets[i].texHandle)
	{
	  return false;
	}
      }

      return true;
    }

    /**
     * Returns true if the given context has the same render view as the 
     * last context.
     */
    bool HasSameRenderView(ContextNodeType* newContext)
    {
      return newContext->renderView == rview;
    }

    void SetupTargets()
    {
      // check whether we have any targets to attach
      if(hasTarget)
      {
	// check whether we need persistent targets
	bool persist = !(context->drawFlags & CSDRAW_CLEARSCREEN);

	// do the attachment
	for (int a = 0; a < rtaNumAttachments; a++)
	  graphics3D->SetRenderTarget (context->renderTargets[a].texHandle, persist,
	      context->renderTargets[a].subtexture, csRenderTargetAttachment (a));

	// validate the targets
	CS_ASSERT(graphics3D->ValidateRenderTargets ());
      }
    }

    void RenderContextStack()
    {
      // obtain some variables we'll need
      const size_t ctxCount = contextStack.GetSize ();
      iCamera *cam = rview->GetCamera ();

      // seriously, we need those
      CS_ASSERT(cam);

      // shared setup for all passes - projection only
      CS::Math::Matrix4 projMatrix = context->perspectiveFixup * cam->GetProjectionMatrix();

      bool doDeferred = true;
      size_t layerCount = 0;
      {
	/* Different contexts may have different numbers of layers,
	 * so determine the upper layer number */
	for (size_t i = 0; i < ctxCount; ++i)
	{
	  layerCount = csMax (layerCount,
	    contextStack[i]->svArrays.GetNumLayers());
	}
	doDeferred &= layerCount > deferredLayer;
	doDeferred &= useDeferredShading || layerCount > lightingLayer;
      }

      // not a deferred stack, just render by layer
      if(!doDeferred)
      {
	// setup projection matrix
	graphics3D->SetProjectionMatrix (projMatrix);

	// attach targets
	SetupTargets();

	// setup clipper
	graphics3D->SetClipper (rview->GetClipper(), CS_CLIPPER_TOPLEVEL);

        int drawFlags = CSDRAW_3DGRAPHICS | context->drawFlags;
	drawFlags |= CSDRAW_CLEARZBUFFER;

	// start the draw
        CS::RenderManager::BeginFinishDrawScope bd (graphics3D, drawFlags);

	// we don't have a z-buffer, yet, use pass 1 modes
	graphics3D->SetZMode (CS_ZBUF_MESH);

	// render out all layers
	for(int i = 0; i < layerCount; ++i)
	{
	  RenderLayer<false>(i, ctxCount);
	}

	// clear clipper
	graphics3D->SetClipper (nullptr, CS_CLIPPER_TOPLEVEL);

	contextStack.Empty ();

	return;
      }

      // create the light render here as we'll use it a lot
      LightRenderType lightRender(graphics3D, shaderMgr, stringSet,
				  rview, gbuffer, lightRenderPersistent);

      // shared setup for deferred passes
      graphics3D->SetProjectionMatrix (context->gbufferFixup * projMatrix);

      // set tex scale to default
      lightRenderPersistent.scale->SetValue(csVector4(0.5,0.5,0.5,0.5));

      // gbuffer fill step
      {
	// attach the gbuffer
	gbuffer.Attach ();

	// setup clipper
	graphics3D->SetClipper (rview->GetClipper(), CS_CLIPPER_TOPLEVEL);

        int drawFlags = CSDRAW_3DGRAPHICS | context->drawFlags;
        drawFlags |= CSDRAW_CLEARSCREEN | CSDRAW_CLEARZBUFFER;

	// start the draw
        CS::RenderManager::BeginFinishDrawScope bd(graphics3D, drawFlags);

	// we want to fill the depth buffer, use pass 1 modes
        graphics3D->SetZMode (CS_ZBUF_MESH);

        // z only pass - maybe we shouldn't allow disabling it.
	if(zonlyLayer >= 0)
        {
	  RenderLayer<false>(zonlyLayer, ctxCount);
        }

	// deferred pass
        // @@@TODO: we could check for CS_ENTITY_NOLIGHTING and
        //          CS_ENTITY_NOSHADOWS here and use it to fill
        //          the stencil buffer so those parts can be
        //          skipped during the lighting pass
	RenderLayer<true>(deferredLayer, ctxCount);

	// clear clipper
	graphics3D->SetClipper (nullptr, CS_CLIPPER_TOPLEVEL);
      }

      // light accumulation step
      {
	// attach accumulation buffers
	gbuffer.AttachAccumulation();

	// set clipper
	graphics3D->SetClipper(rview->GetClipper(), CS_CLIPPER_TOPLEVEL);

        int drawFlags = CSDRAW_3DGRAPHICS | context->drawFlags;
	drawFlags |= CSDRAW_CLEARSCREEN | CSDRAW_CLEARZBUFFER;

	// start the draw
	CS::RenderManager::BeginFinishDrawScope bd (graphics3D, drawFlags);

	// use pass 1 zmodes for re-populating the zbuffer
	graphics3D->SetZMode (CS_ZBUF_MESH);

	// we use the inaccurate version from our gbuffer here - it's sufficient
        // @@@NOTE: appearently using an explicit depth output prevents hardware
        //          optimizations for the zbuffer to kick in - maybe we should
	//	    just go for a fragkill solution?
	lightRender.OutputDepth();

	// we're done with all depth writes - use pass 2 modes
	graphics3D->SetZMode (CS_ZBUF_MESH2);

	// accumulate lighting data
	RenderLights(deferredLayer, ctxCount, lightRender);

	// clear clipper
	graphics3D->SetClipper (nullptr, CS_CLIPPER_TOPLEVEL);
      }

      // setup projection matrix for final pass
      graphics3D->SetProjectionMatrix (projMatrix);

      // attach output render targets if any.
      SetupTargets();
      {
	// setup clipper
	graphics3D->SetClipper (rview->GetClipper(), CS_CLIPPER_TOPLEVEL);

        int drawFlags = CSDRAW_3DGRAPHICS | context->drawFlags;
	drawFlags |= CSDRAW_CLEARZBUFFER;

	// start the draw
        CS::RenderManager::BeginFinishDrawScope bd (graphics3D, drawFlags);

	// we want to re-populate the depth buffer, use pass 1 modes.
	graphics3D->SetZMode (CS_ZBUF_MESH);

	// Visibility Culling
	{
	  // visculling only needs to be rendered once per sector.
	  csSet<iSector*> sectors;
	  for (size_t c = 0; c < contextStack.GetSize (); ++c)
	  {
	    ContextNodeType* ctx = contextStack[c];

	    if (!sectors.Contains(ctx->sector))
	    {
	      sectors.AddNoTest(ctx->sector);

              graphics3D->SetWorldToCamera (ctx->cameraTransform.GetInverse ());
	      ctx->sector->GetVisibilityCuller ()->RenderViscull (rview, ctx->shadervars);
	    }
	  }
	}

	// set tex scale for lookups.
	lightRenderPersistent.scale->SetValue(context->texScale);

	// for deferred shading the inaccurate gbuffer version is enough.
	if(useDeferredShading)
	{
	  lightRender.OutputDepth();
	}
	// early z pass - could be disabled if occluvis is used - but how would we know?
	else if(zonlyLayer >= 0)
        {
	  RenderLayer<false>(zonlyLayer, ctxCount);
        }

	// deferred shading - output step
	if(useDeferredShading)
	{
	  lightRender.OutputResults();
	}
	// deferred lighting - output step
	else
	{
	  RenderLayer<true>(lightingLayer, ctxCount);
	}

        // forward rendering
	RenderForwardMeshes(layerCount, ctxCount);

	// deferred rendering - debug step if wanted
	if(drawLightVolumes)
	{
          LightVolumeRenderer<LightRenderType> lightVolumeRender (lightRender, true, 0.2f);

	  // output light volumes
	  RenderLights(deferredLayer, ctxCount, lightVolumeRender);
	}

	// clear clipper
	graphics3D->SetClipper (nullptr, CS_CLIPPER_TOPLEVEL);
      }

      contextStack.Empty ();
    }

    template<bool deferredOnly>
    void RenderLayer(int layer, const size_t ctxCount)
    {
      // set layer
      meshRender.SetLayer(layer);

      // render meshes
      if(deferredOnly)
	RenderObjects<RenderType, ForEachDeferredMeshNode>(layer, ctxCount, meshRender);
      else
	RenderObjects<RenderType, CS::RenderManager::ForEachMeshNode>(layer, ctxCount, meshRender);
    }

    template<typename T>
    void RenderLights(int layer, const size_t ctxCount, T& render)
    {
      // render all lights
      RenderObjects<T, ForEachLight>(layer, ctxCount, render);
    }

    void RenderForwardMeshes(size_t layerCount, const size_t ctxCount)
    {
      // iterate over all layers
      for (int layer = 0; layer < layerCount; ++layer)
      {
	// set layer
	meshRender.SetLayer(layer);

	// render all forward meshes
	RenderObjects<RenderType, ForEachForwardMeshNode>(layer, ctxCount, meshRender);
      }
    }

    template<typename T, void fn(ContextNodeType&,T&)>
    inline void RenderObjects(int layer, const size_t ctxCount, T& render)
    {
      for(size_t i = 0; i < ctxCount; ++i)
      {
        ContextNodeType *ctx = contextStack[i];

        // check whether this context needs to be rendered
        size_t layerCount = ctx->svArrays.GetNumLayers();
        if(layer >= layerCount)
	  continue;

        graphics3D->SetWorldToCamera (ctx->cameraTransform.GetInverse ());

	// render all objects given a render
	fn(*ctx, render);
      }
    }

    bool IsDeferredLayer(int layer)
    {
      return layer == deferredLayer || layer == lightingLayer
	  || layer == zonlyLayer;
    }

  private:

    // renderer
    RenderType meshRender;

    // data from parent
    iGraphics3D *graphics3D;
    iShaderManager *shaderMgr;
    iStringSet *stringSet;

    // render objects from parent
    typename LightRenderType::PersistentData& lightRenderPersistent;
    GBuffer& gbuffer;

    // render layer data from parent
    int deferredLayer;
    int lightingLayer;
    int zonlyLayer;

    // render options from parent
    bool drawLightVolumes;
    bool useDeferredShading;

    // current context stack we're going to render
    csArray<ContextNodeType*> contextStack;

    // data for current context
    ContextNodeType* context;
    CS::RenderManager::RenderView* rview;
    bool hasTarget;
  };

}
CS_PLUGIN_NAMESPACE_END(RMDeferred)

#endif // __DEFERREDRENDER_H__
