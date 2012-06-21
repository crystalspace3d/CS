/*
    Copyright (C) 2007-2008 by Marten Svanfeldt

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

#ifndef __POSTEFFECT_H__
#define __POSTEFFECT_H__

/**\file
 * Post processing effects
 */

#include "csgfx/shadervarcontext.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/posteffects.h"
#include "csutil/array.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/genericresourcecache.h"
#include "csutil/parray.h"
#include "csutil/ref.h"
#include "imap/services.h"
#include "ivideo/shader/shader.h"
#include "iutil/comp.h"
#include "csutil/scf_implementation.h"
#include "iengine/rendermanager.h"

struct iGraphics3D;
struct iObjectRegistry;
struct iRenderView;
struct iShader;
struct iSyntaxService;
struct iTextureHandle;
struct iView;

using namespace CS::RenderManager;

CS_PLUGIN_NAMESPACE_BEGIN (PostEffect)
{
  /// An effect layer.
  class Layer : 
    public scfImplementation1<Layer, iPostEffectLayer>
  {
  private:
    friend class PostEffect;
    friend struct DimensionData;
      
    csRef<iShader> effectShader;
    int outTextureNum;
    csArray<PostEffectLayerInputMap> inputs;
    csRef<iShaderVariableContext> svContext;
    PostEffectLayerOptions options;
    
    bool IsInput (const Layer* layer) const;

  public:
    CS_LEAKGUARD_DECLARE (Layer);

    Layer ();
    virtual ~Layer () {}

    //-- iPostEffectLayer
    /// Get the shader variables for this layer.
    iShaderVariableContext* GetSVContext () const { return svContext; }
    /// Get inputs to this layer
    const csArray<PostEffectLayerInputMap>& GetInputs () const { return inputs; }
      
    /// Get layer options
    const PostEffectLayerOptions& GetOptions () const { return options; }
    /// Set layer options
    void SetOptions (const PostEffectLayerOptions& opt) { options = opt; }

    /// Get layer shader
    void SetShader (iShader* shader) { effectShader = shader; }
    /// Set layer shader
    iShader* GetShader () const { return effectShader; }
    /// @@@ Document me?
    int GetOutTextureNum () const { return outTextureNum; }
  };

  class PostEffectManager;

  class PostEffect :
    public scfImplementation1<PostEffect, iPostEffect>
  {
    struct DimensionData;

  public:  
    CS_LEAKGUARD_DECLARE (PostEffect);

    PostEffect (PostEffectManager* manager, const char* name);
    virtual ~PostEffect ();

    //-- iPostEffect

    const char* GetName () { return name; }

    /// Set the texture format for the intermediate textures used.
    void SetIntermediateTargetFormat (const char* textureFmt);
    /// Get the texture format for the intermediate textures used.
    const char* GetIntermediateTargetFormat ();
        
    /**
     * Set up post processing manager for a view.
     * \returns Whether the manager has changed. If \c true some values,
     *   such as the screen texture, must be reobtained from the manager.   
     */    
    bool SetupView (uint width, uint height);

    /**
     * Discard (and thus cause recreation of) all intermediate textures.
     */
    void ClearIntermediates ();

    /// Get the texture to render a scene to for post processing.
    iTextureHandle* GetScreenTarget ();

    /**
     * Draw post processing effects after the scene was rendered to
     * the handle returned by GetScreenTarget ().
     */
    void DrawPostEffect (RenderTreeBase& renderTree);
    
    //@{
    /// Add an effect pass. Uses last added layer as the input
    iPostEffectLayer* AddLayer (iShader* shader);
    iPostEffectLayer* AddLayer (iShader* shader, const PostEffectLayerOptions& opt);
    //@}
    //@{
    /// Add an effect pass with custom input mappings.
    iPostEffectLayer* AddLayer (iShader* shader, size_t numMaps, const PostEffectLayerInputMap* maps);
    iPostEffectLayer* AddLayer (iShader* shader, const PostEffectLayerOptions& opt, size_t numMaps,
      const PostEffectLayerInputMap* maps);
    //@}
    /// Remove a layer
    bool RemoveLayer (iPostEffectLayer* layer);
    /// Remove all layers
    void ClearLayers ();
    
    /// Get the layer representing the "screen" a scene is rendered to.
    iPostEffectLayer* GetScreenLayer () { return postLayers[0]; }
    
    /// Get the layer that was added last
    iPostEffectLayer* GetLastLayer () { return lastLayer; }
    
    /// Get the output texture of a layer.
    iTextureHandle* GetLayerOutput (const iPostEffectLayer* layer);
    
    /**
     * Get SV context used for rendering.
     */
    void GetLayerRenderSVs (const iPostEffectLayer* layer, csShaderVariableStack& svStack) const;
    
    /// Set the effect's output render target.
    void SetOutputTarget (iTextureHandle* tex) { target = tex; }
  
    /// Get the effect's output render target.
    iTextureHandle* GetOutputTarget () const { return target; }

    /**
     * Returns whether the screen space is flipped in Y direction. This usually
     * happens when rendering to a texture due post effects.
     */
    bool ScreenSpaceYFlipped ();    

  private:
    PostEffectManager* manager;
    csString name;
    uint frameNum;
    csRef<iRenderBuffer> indices;
    csRef<iTextureHandle> target;
    uint dbgIntermediateTextures;    

    void SetupScreenQuad ();

    const iPostEffectLayer* GetRealOutputLayer (const iPostEffectLayer* layer) const
    { 
      return layer->GetOptions ().renderOn 
        ? GetRealOutputLayer (layer->GetOptions ().renderOn) : layer;
    }

    struct Dimensions
    {
      uint x, y;
    };

    /// All the data needed for one target dimension
    struct DimensionData
    {
      Dimensions dim;
      /**
       * Textures which have the same properties are managed
       * in one "bucket"
       */
      struct TexturesBucket
      {
        /// Textures in this bucket
	      csRefArray<iTextureHandle> textures;
	      /**
	       * Maximum X/Y coords (normalized for 2D textures, unnormalized for
	       * RECT textures)
	       */
	      float texMaxX, texMaxY;
	
	      TexturesBucket () : texMaxX (1), texMaxY (1) { }
      };
      csArray<TexturesBucket> buckets;
      
      struct LayerRenderInfo
      {
        /// 'Pixel size' (values to add to X/Y to get next input pixel)
	      csRef<csShaderVariable> svPixelSize;
	      /// Input vertices for layer
	      csRef<iRenderBuffer> vertBuf;
	      /// Shader vars
	      csRef<iShaderVariableContext> layerSVs;
	      /// Render buffers
	      csRef<csRenderBufferHolder> buffers;
	      /// Render mesh for layer
        csSimpleRenderMesh fullscreenQuad;
      };
      /// Render information for all layers
      csArray<LayerRenderInfo> layerRenderInfos;

      bool AllocatePingpongTextures (PostEffect& pfx);
      void UpdateSVContexts (PostEffect& pfx);
    
      void SetupRenderInfo (PostEffect& pfx);

    protected:
      csPtr<iRenderBuffer> ComputeTexCoords (iTextureHandle* tex,
        const csRect& rect, const csRect& targetRect,
        float& pixSizeX, float& pixSizeY);
    };
    
    struct DimensionCacheSorting
    {
      typedef Dimensions KeyType;

      static bool IsLargerEqual (const DimensionData& b1, 
                                 const DimensionData& b2)
      {
	return (b1.dim.x >= b2.dim.x) && (b1.dim.y >= b2.dim.y);
      }
    
      static bool IsEqual (const DimensionData& b1, 
                           const DimensionData& b2)
      {
	return (b1.dim.x == b2.dim.x) && (b1.dim.y == b2.dim.y);
      }
    
      static bool IsLargerEqual (const DimensionData& b1, 
                                 const Dimensions& b2)
      {
	return (b1.dim.x >= b2.x) && (b1.dim.y >= b2.y);
      }
    
      static bool IsEqual (const DimensionData& b1, 
                           const Dimensions& b2)
      {
	return (b1.dim.x == b2.x) && (b1.dim.y == b2.y);
      }
    
      static bool IsLargerEqual (const Dimensions& b1, 
                                 const DimensionData& b2)
      {
	return (b1.x >= b2.dim.x) && (b1.y >= b2.dim.y);
      }
    };

    CS::Utility::GenericResourceCache<DimensionData,
      uint, DimensionCacheSorting, 
      CS::Utility::ResourceCache::ReuseConditionFlagged> dimCache;
    DimensionData* currentDimData;
      
    uint currentWidth, currentHeight;

    bool textureDistributionDirty;
    void UpdateTextureDistribution ();
      
    csString textureFmt;
    Layer* lastLayer;
    csPDelArray<Layer> postLayers;
    bool layersDirty;
    void UpdateLayers ();
    
    struct BucketsCommon
    {
      PostEffectLayerOptions options;
      size_t textureNum;
    };
    csArray<BucketsCommon> buckets;
    size_t GetBucketIndex (const PostEffectLayerOptions& options);
    BucketsCommon& GetBucket (const PostEffectLayerOptions& options)
    { return buckets[GetBucketIndex (options)]; }
  };

  class PostEffectManager : 
    public scfImplementation2<PostEffectManager, iPostEffectManager, iComponent>
  {
    friend class PostEffect;

  protected:
    csHash<csRef<iPostEffect>, const char*> postEffectsHash;
    csRef<iGraphics3D> graphics3D;
    csRef<iShaderVarStringSet> svStrings;
    bool keepAllIntermediates;

  public:
    CS_LEAKGUARD_DECLARE (PostEffectManager);

    PostEffectManager (iBase* parent);
    virtual ~PostEffectManager () {}

    //-- iComponent
    virtual bool Initialize (iObjectRegistry* objectReg);

    //-- iPostEffectManager
    csPtr<iPostEffect> CreatePostEffect (const char* name);
  };

}
CS_PLUGIN_NAMESPACE_END (PostEffect)

#endif
