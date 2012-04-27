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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTS_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTS_H__

/**\file
 * Post processing effects manager
 */

#include "csgfx/shadervarcontext.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/array.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/genericresourcecache.h"
#include "csutil/parray.h"
#include "csutil/ref.h"
#include "imap/services.h"
#include "ivideo/shader/shader.h"

struct iGraphics3D;
struct iObjectRegistry;
struct iRenderView;
struct iShader;
struct iSyntaxService;
struct iTextureHandle;
struct iView;
struct iRenderManagerPostEffects;

namespace CS
{
namespace RenderManager
{
  struct iPostEffectLayer;

  /**
    * Options for a postprocessing layer
    */
  struct PostEffectLayerOptions
  {
    /// Generate mipmaps for this layer
    bool mipmap;
    /// Highest mipmap level to generate
    int maxMipmap;
    /**
      * Reduce output size. Each downsample step reduces the output by half 
      * in each dimensions.
      */
    int downsample;
    /// Prevent texture reuse. Useful for readback or feedback effects.
    bool noTextureReuse;
    /**
      * Manually provide a texture to render on.
      * Means mipmap, maxMipmap, downsample and noTextureReuse are ignored.
      */
    csRef<iTextureHandle> manualTarget;
    /// If not empty render to this rectangle of the target texture
    csRect targetRect;
    /**
      * If given renders onto the specified layer as well.
      * This means all other options except targetRect are ignored.
      */
    iPostEffectLayer* renderOn;
    /**
      * This layer will later be read back. Sets the #CSDRAW_READBACK draw
      * flag.
      */
    bool readback;
      
    PostEffectLayerOptions () : mipmap (false), maxMipmap (-1), downsample (0),
      noTextureReuse (false), renderOn (0), readback (false) {}
      
    bool operator==(const PostEffectLayerOptions& other) const
    { 
      return (mipmap == other.mipmap)
        && (maxMipmap == other.maxMipmap)
        && (downsample == other.downsample)
        && (noTextureReuse == other.noTextureReuse)
        && (manualTarget == other.manualTarget)
        && (targetRect == other.targetRect)
        && (renderOn == other.renderOn)
        && (readback == other.readback);
    }
  };

  /// Custom input mapping for a layer
  struct PostEffectLayerInputMap
  {
    /**
      * Shader variable for manually specifying an inout texture.
      * Takes precedence over inputLayer and textureName if given.
      */
    csRef<csShaderVariable> manualInput;
    /// Input layer
    iPostEffectLayer* inputLayer;
    /// Name of the shader variable to provide the input layer texture in
    csString textureName;
    /**
      * Name of the shader variable to provide the texture coordinates for the
      * input layer texture in
      */
    csString texcoordName;
    /**
      * If not empty the SV with that name receives the 'pixel size'
      * (values to add to X/Y to get next input pixel) for this input.
      */
    csString inputPixelSizeName;
    /**
      * If not empty specifies the rectangle of the input texture, in pixels,
      * the be used as input for the layer.
      */
    csRect sourceRect;
      
    PostEffectLayerInputMap () : inputLayer (0), textureName ("tex diffuse"),
      texcoordName ("texture coordinate 0") {}
  };

  /*
   *  A post effect layer.
   */
  struct iPostEffectLayer : public virtual iBase
  {
    SCF_INTERFACE (iPostEffectLayer, 1, 0, 0);

    /// Get the shader variables for this layer.
    virtual iShaderVariableContext* GetSVContext () const = 0;
    /// Get inputs to this layer
    virtual const csArray<PostEffectLayerInputMap>& GetInputs () const = 0;
      
    /// Get layer options
    virtual const PostEffectLayerOptions& GetOptions () const = 0;
    /// Set layer options
    virtual void SetOptions (const PostEffectLayerOptions& opt) = 0;

    /// Get layer shader
    virtual void SetShader (iShader* shader) = 0;
    /// Set layer shader
    virtual iShader* GetShader () const = 0;
    /// @@@ Document me?
    virtual int GetOutTextureNum () const = 0;
  };

  /**
   * Interface for post processing effects.
   */
  struct iPostEffect : public virtual iBase
  {
    SCF_INTERFACE (iPostEffect, 1, 0, 0);

    /// Get the name of this post pocessing effect
    virtual const char* GetName () = 0;

    /// Set the texture format for the intermediate textures used.
    virtual void SetIntermediateTargetFormat (const char* textureFmt) = 0;

    /// Get the texture format for the intermediate textures used.
    virtual const char* GetIntermediateTargetFormat () = 0;
        
    /**
      * Set up post processing manager for a view.
      * \returns Whether the manager has changed. If \c true some values,
      *   such as the screen texture, must be reobtained from the manager.
      */
    virtual bool SetupView (uint width, uint height) = 0;

    /**
      * Discard (and thus cause recreation of) all intermediate textures.
      */
    virtual void ClearIntermediates () = 0;

    /// Get the texture to render a scene to for post processing.
    virtual iTextureHandle* GetScreenTarget () = 0;

    /**
      * Draw post processing effects after the scene was rendered to
      * the handle returned by GetScreenTarget ().
      */
    virtual void DrawPostEffect (RenderTreeBase& renderTree) = 0;
    
    //@{
    /// Add an effect pass. Uses last added layer as the input
    virtual iPostEffectLayer* AddLayer (iShader* shader) = 0;
    virtual iPostEffectLayer* AddLayer (iShader* shader, const PostEffectLayerOptions& opt) = 0;
    //@}
    //@{
    /// Add an effect pass with custom input mappings.
    virtual iPostEffectLayer* AddLayer (iShader* shader, size_t numMaps, 
      const PostEffectLayerInputMap* maps) = 0;
    virtual iPostEffectLayer* AddLayer (iShader* shader, const PostEffectLayerOptions& opt,
      size_t numMaps, const PostEffectLayerInputMap* maps) = 0;
    //@}
    /// Remove a layer
    virtual bool RemoveLayer (iPostEffectLayer* layer) = 0;
    /// Remove all layers
    virtual void ClearLayers () = 0;
    
    /// Get the layer representing the "screen" a scene is rendered to.
    virtual iPostEffectLayer* GetScreenLayer () = 0;
    
    /// Get the layer that was added last
    virtual iPostEffectLayer* GetLastLayer () = 0;
    
    /// Get the output texture of a layer.
    virtual iTextureHandle* GetLayerOutput (const iPostEffectLayer* layer) = 0;
    
    /**
      * Get SV context used for rendering.
      */
    virtual void GetLayerRenderSVs (const iPostEffectLayer* layer, 
      csShaderVariableStack& svStack) const = 0;
    
    /// Set the effect's output render target.
    virtual void SetOutputTarget (iTextureHandle* tex) = 0;
  
    /// Get the effect's output render target.
    virtual iTextureHandle* GetOutputTarget () const = 0;  
    
    /**
      * Returns whether the screen space is flipped in Y direction. This usually
      * happens when rendering to a texture due post effects.
      */
    virtual bool ScreenSpaceYFlipped () = 0;

    // TODO: add layers from file
  };

  /**
   * Factory for post-effects.
   */
  struct iPostEffectManager : public virtual iBase
  {
    SCF_INTERFACE (iPostEffectManager, 1, 0, 0);

    virtual csPtr<iPostEffect> CreatePostEffect (const char* name) = 0;
  };
}
}

#endif
