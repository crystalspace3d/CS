/*
    Copyright (C) 2008 by Joe Forte

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

#ifndef __DEFERRED_H__
#define __DEFERRED_H__

#include "cssysdef.h"

#include "csplugincommon/rendermanager/standardtreetraits.h"
#include "csplugincommon/rendermanager/dependenttarget.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/debugcommon.h"
#include "csplugincommon/rendermanager/renderlayers.h"

#include "iutil/comp.h"
#include "csutil/scf_implementation.h"
#include "iengine/rendermanager.h"
#include "itexture.h"

#include "deferredlightrender.h"

CS_PLUGIN_NAMESPACE_BEGIN(RMDeferred)
{
  typedef CS::RenderManager::RenderTree<CS::RenderManager::RenderTreeStandardTraits> 
    RenderTreeType;

  template<typename RenderTreeType, typename LayerConfigType>
  class StandardContextSetup;

  class RMDeferred : public scfImplementation3<RMDeferred, 
                                               iRenderManager, 
                                               iComponent, 
                                               scfFakeInterface<iDebugHelper> >,
                     public CS::RenderManager::RMDebugCommon<RenderTreeType>
  {
  public:

    /// Constructor.
    RMDeferred(iBase *parent);

    //---- iComponent Interface ----
    virtual bool Initialize(iObjectRegistry *registry);

    //---- iRenderManager Interface ----
    virtual bool RenderView(iView *view);
    virtual bool PrecacheView(iView *view);

    typedef StandardContextSetup<RenderTreeType, CS::RenderManager::MultipleRenderLayer> 
      ContextSetupType;

    typedef CS::RenderManager::StandardPortalSetup<RenderTreeType, ContextSetupType> 
      PortalSetupType;

  public:

    bool AttachGbuffer(iGraphics3D *graphics3D);
    bool DetachGBuffer(iGraphics3D *graphics3D);

    bool AttachAccumBuffer(iGraphics3D *graphics3D);
    bool DetachAccumBuffer(iGraphics3D *graphics3D);

    iObjectRegistry *objRegistry;

    RenderTreeType::PersistentData treePersistent;
    PortalSetupType::PersistentData portalPersistent;
    DeferredLightRenderer::PersistentData lightRenderPersistent;

    CS::RenderManager::MultipleRenderLayer renderLayer;

    csRef<iShaderManager> shaderManager;
    csRef<iStringSet> stringSet;

    csRef<iTextureHandle> accumBuffer;

    // The textures that make up the GBuffer.
    csRef<iTextureHandle> colorBuffer0;
    csRef<iTextureHandle> colorBuffer1;
    csRef<iTextureHandle> colorBuffer2;
    csRef<iTextureHandle> depthBuffer;

    int maxPortalRecurse;
  };
}
CS_PLUGIN_NAMESPACE_END(RMDeferred)

#endif // __DEFERRED_H__
