/*
    Copyright (C)2007 by Marten Svanfeldt

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

#include "cssysdef.h"

#include "csgfx/renderbuffer.h"
#include "csgfx/shadervarcontext.h"
#include "csutil/cfgacc.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/objreg.h"
#include "csutil/stringquote.h"
#include "csutil/xmltiny.h"
#include "cstool/rbuflock.h"
#include "iengine/rview.h"
#include "imap/loader.h"
#include "imap/services.h"
#include "iutil/document.h"
#include "iutil/vfs.h"
#include "ivaria/reporter.h"
#include "ivaria/view.h"
#include "ivideo/graph3d.h"
#include "ivideo/texture.h"
#include "ivideo/txtmgr.h"

#include "csplugincommon/rendermanager/rendertree.h"
#include "posteffect.h"

#include <stdarg.h>

using namespace CS::RenderManager;

CS_PLUGIN_NAMESPACE_BEGIN (PostEffect)
{

CS_LEAKGUARD_IMPLEMENT (PostEffect);

PostEffect::PostEffect (PostEffectManager* manager, const char* name)
  : scfImplementationType (this), manager (manager), name (name),
    frameNum (0), dbgIntermediateTextures (~0),
    dimCache (CS::Utility::ResourceCache::ReuseConditionFlagged (),
      CS::Utility::ResourceCache::PurgeConditionAfterTime<uint> (0)),
    currentDimData (0), currentWidth (0), currentHeight (0),
    textureFmt ("argb8"), lastLayer (0), layersDirty (true)
{
  AddLayer (0, 0, 0);
}

PostEffect::~PostEffect ()
{
}

void PostEffect::SetIntermediateTargetFormat (const char* textureFmt)
{
  this->textureFmt = textureFmt;
}

const char* PostEffect::GetIntermediateTargetFormat ()
{
  return textureFmt;
}

bool PostEffect::SetupView (uint width, uint height)
{
  if (!indices.IsValid ())
  {
    indices = csRenderBuffer::CreateIndexRenderBuffer (4,
      CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_SHORT, 0, 3);
    {
      csRenderBufferLock<unsigned short> indexLock (indices);
      for (uint i = 0; i < 4; i++)
	indexLock[(size_t)i] = i;
    }
  }
  
  bool result = false;
  if (width != currentWidth || height != currentHeight
      || layersDirty)
  {
    if (currentDimData != 0)
      dimCache.GetReuseAuxiliary (currentDimData)->reusable = true;
    
    currentWidth = width;
    currentHeight = height;

    UpdateLayers ();
    
    Dimensions key;
    key.x = currentWidth; key.y = currentHeight;
    currentDimData = dimCache.Query (key, true);
    if (currentDimData != 0) return true;
    
    DimensionData newData;
    newData.dim = key;
    currentDimData = dimCache.AddActive (newData);
    currentDimData->buckets.SetSize (buckets.GetSize ());
    if (!currentDimData->AllocatePingpongTextures (*this))
    {
      // @@@ Allocating pingpong textures failed.
      return false;
    }
    currentDimData->SetupRenderInfo (*this);
    currentDimData->UpdateSVContexts (*this);
    
    /* The textures used here can take up a lot of resources, so free up
       cache aggressively.
       Ideally, only the dimensions that are used in one frame should
       be kept.
     */
    dimCache.agedPurgeInterval = 0;

    result = true;
  } 

  return result;
}

void PostEffect::ClearIntermediates ()
{
  currentWidth = 0; currentHeight = 0;
  currentDimData = 0;
  dimCache.Clear (true);
}

iTextureHandle* PostEffect::GetScreenTarget ()
{
  if (postLayers.GetSize () > 1)
  {
    size_t bucket = GetBucketIndex (postLayers[0]->options);
    return currentDimData->buckets[bucket].textures[postLayers[0]->outTextureNum];
  }

  return target;
}

void PostEffect::DrawPostEffect (RenderTreeBase& renderTree)
{ 
  manager->graphics3D->FinishDraw ();
  
  if (dbgIntermediateTextures == (uint)~0)
    dbgIntermediateTextures = renderTree.RegisterDebugFlag ("textures.postprocess");

  UpdateLayers ();

  size_t lastLayerToTarget = postLayers.GetSize () - 1;
  while (postLayers[lastLayerToTarget]->options.renderOn != 0)
  {
    lastLayerToTarget--;
  }
  for (size_t layer = 1; layer < postLayers.GetSize (); ++layer)
  {
    // Draw and ping-pong   
    iTextureHandle* targetTex;
    const iPostEffectLayer* outputLayer = GetRealOutputLayer (postLayers[layer]);
    if (outputLayer->GetOptions ().manualTarget.IsValid ())
    {
      targetTex = outputLayer->GetOptions ().manualTarget;
    }
    else
    {
      if (layer < lastLayerToTarget)
      {
	      size_t bucket = GetBucketIndex (outputLayer->GetOptions ());
	      targetTex =
	        currentDimData->buckets[bucket].textures[outputLayer->GetOutTextureNum ()];
      }
      else
      {
	      targetTex = target;
      }
    }
    manager->graphics3D->SetRenderTarget (targetTex);

    int drawflags = CSDRAW_CLEARZBUFFER | CSDRAW_3DGRAPHICS;
    if (outputLayer->GetOptions ().readback
        && ((layer == postLayers.GetSize () - 1)
          || GetRealOutputLayer (postLayers[layer+1]) != outputLayer))
    {
      drawflags |= CSDRAW_READBACK;
    }

    manager->graphics3D->BeginDraw (drawflags);
    manager->graphics3D->DrawSimpleMesh (currentDimData->layerRenderInfos[layer].fullscreenQuad,
      csSimpleMeshScreenspace);
    manager->graphics3D->FinishDraw ();
  }
  
  if (renderTree.IsDebugFlagEnabled (dbgIntermediateTextures))
  {
    for (size_t layer = 0; layer < postLayers.GetSize ()-1; ++layer)
    {
      
      if (postLayers[layer]->options.renderOn != 0)
	      // If rendering onto another layer the bucket of that layer will be displayed.
	      continue;
      if (postLayers[layer]->options.manualTarget.IsValid ())
	      // If a manual target is given we don't display the texture here
	      continue;

      // Actual intermediate layers
      size_t bucket = GetBucketIndex (postLayers[layer]->options);
      renderTree.AddDebugTexture (
	      currentDimData->buckets[bucket].textures[postLayers[layer]->outTextureNum],
	      float (currentWidth)/float (currentHeight));
    }
  }
  
  dimCache.AdvanceTime (++frameNum);
  // Reset to avoid purging every frame
  dimCache.agedPurgeInterval = 60;
}
    
iPostEffectLayer* PostEffect::AddLayer (iShader* shader)
{
  PostEffectLayerOptions opt;
  return AddLayer (shader, opt);
}

iPostEffectLayer* PostEffect::AddLayer (iShader* shader, const PostEffectLayerOptions& opt)
{
  PostEffectLayerInputMap map;
  map.inputLayer = lastLayer;
  return AddLayer (shader, opt, 1, &map);
}

iPostEffectLayer* PostEffect::AddLayer (iShader* shader, size_t numMaps, 
                                        const PostEffectLayerInputMap* maps)
{
  PostEffectLayerOptions opt;
  return AddLayer (shader, opt, numMaps, maps);
}

iPostEffectLayer* PostEffect::AddLayer (iShader* shader, const PostEffectLayerOptions& opt,
                                        size_t numMaps, const PostEffectLayerInputMap* maps)
{
  Layer* newLayer = new Layer ();
  newLayer->effectShader = shader;
  for (size_t n = 0; n < numMaps; n++)
    newLayer->inputs.Push (maps[n]);

  newLayer->options = opt;
  postLayers.Push (newLayer);
  textureDistributionDirty = true;
  lastLayer = newLayer;
  layersDirty = true;
  return newLayer;
}

bool PostEffect::RemoveLayer (iPostEffectLayer* layer)
{
  size_t layerIndex = 0;
  for (; layerIndex < postLayers.GetSize (); layerIndex++)
  {
    if (postLayers[layerIndex] == layer) break;
  }
  if (layerIndex >= postLayers.GetSize ()) return false;
  // If this layer was input to some other, replace these
  const PostEffectLayerInputMap& input = layer->GetInputs ()[0];
  for (size_t l = layerIndex+1; l < postLayers.GetSize (); l++)
  {
    Layer* fixupLayer = postLayers[l];
    for (size_t i = 0; i < fixupLayer->inputs.GetSize (); i++)
    {
      PostEffectLayerInputMap& inputFixup = fixupLayer->inputs[i];
      if (inputFixup.inputLayer == layer)
        inputFixup.inputLayer = input.inputLayer;
    }
  }
  
  if (layer == lastLayer)
    lastLayer = postLayers[layerIndex-1];
  
  postLayers.DeleteIndex (layerIndex);
  textureDistributionDirty = true;
  layersDirty = true;
  return true;
}
    
void PostEffect::ClearLayers ()
{
  postLayers.DeleteAll ();
  currentWidth = 0; currentHeight = 0;
  lastLayer = 0;
  
  AddLayer (0, 0, 0);
}

iTextureHandle* PostEffect::GetLayerOutput (const iPostEffectLayer* layer)
{
  size_t bucket = GetBucketIndex (layer->GetOptions ());
  return currentDimData->buckets[bucket].textures[layer->GetOutTextureNum ()];
}
    
void PostEffect::GetLayerRenderSVs (const iPostEffectLayer* layer,
					   csShaderVariableStack& svStack) const
{
  layer->GetSVContext ()->PushVariables (svStack);
  
  // Add dummy SVs for other layer stuff
  for (size_t i = 0; i < layer->GetInputs ().GetSize (); i++)
  {
    const PostEffectLayerInputMap& input = layer->GetInputs ()[i];
    
    csRef<csShaderVariable> sv;
    if (input.manualInput.IsValid ())
    {
      svStack[input.manualInput->GetName ()] = input.manualInput;
    }
    else
    {
      CS::StringIDValue svName = manager->svStrings->Request (input.textureName);
      if (svName < svStack.GetSize ())
      {
        sv.AttachNew (new csShaderVariable (svName));
        sv->SetType (csShaderVariable::TEXTURE);
        svStack[svName] = sv;
      }
    }
    
    csRenderBufferName bufferName =
      csRenderBuffer::GetBufferNameFromDescr (input.texcoordName);
    if (bufferName == CS_BUFFER_NONE)
    {
      CS::StringIDValue svName = manager->svStrings->Request (input.texcoordName);
      if (svName < svStack.GetSize ())
      {
        sv.AttachNew (new csShaderVariable (svName));
        sv->SetType (csShaderVariable::RENDERBUFFER);
        svStack[svName] = sv;
      }
    }
  }
}

bool PostEffect::DimensionData::AllocatePingpongTextures (PostEffect& pfx)
{
  size_t layer0bucket = pfx.GetBucketIndex (pfx.postLayers[0]->options);
  for (size_t b = 0; b < buckets.GetSize (); b++)
  {
    uint texFlags =
      CS_TEXTURE_3D | CS_TEXTURE_NPOTS | CS_TEXTURE_CLAMP | CS_TEXTURE_SCALE_UP;
    if (!pfx.buckets[b].options.mipmap)
      texFlags |= CS_TEXTURE_NOMIPMAPS;
    // Always clear the texture that is used as the screen target
    if (layer0bucket == b)
      texFlags |= CS_TEXTURE_CREATE_CLEAR;
    csRef<iTextureHandle> t;
    int texW = dim.x >> pfx.buckets[b].options.downsample;
    int texH = dim.y >> pfx.buckets[b].options.downsample;
    t = pfx.manager->graphics3D->GetTextureManager ()->CreateTexture (texW, texH, 
      csimg2D, pfx.textureFmt, texFlags);
    if (!t)
    {
      printf ("Error creating texture for post effects! The application will soon crash!"
	      "\nTODO: Proper reporting...\n");
      fflush (stdout);
      return false;
    }
    if (pfx.buckets[b].options.maxMipmap >= 0)
      t->SetMipmapLimits (pfx.buckets[b].options.maxMipmap);
    buckets[b].textures.SetSize (pfx.buckets[b].textureNum);
    buckets[b].textures.Put (0, t);
  
    // Check if we actually got NPOTs textures
    int resultFlags = buckets[b].textures[0]->GetFlags ();
    if (!(resultFlags & CS_TEXTURE_NPOTS))
    {
      // Handle that we didn't get a power of 2 texture
      // Means the texture is bigger and we need to use <1 for texture coordinate
      int tw, th, td;
      buckets[b].textures[0]->GetRendererDimensions (tw, th, td);
  
      buckets[b].texMaxX = texW / (float)tw;
      buckets[b].texMaxY = texH / (float)th;
    }
    else if (buckets[b].textures[0]->GetTextureType () == iTextureHandle::texTypeRect)
    {
      // Handle a rect texture
      // Rect texture use non-normalized texture coordinates
      buckets[b].texMaxX = texW;
      buckets[b].texMaxY = texH;
    }
    else
    {
      // NPOT texture, coordinates are 0-1,0-1
      buckets[b].texMaxX = 1;
      buckets[b].texMaxY = 1;
    }  
  
    for (size_t i = 1; i < buckets[b].textures.GetSize (); i++)
    {
      t = pfx.manager->graphics3D->GetTextureManager ()->CreateTexture (texW, texH, 
	      csimg2D, pfx.textureFmt, resultFlags);
      buckets[b].textures.Put (i, t);
    }
  }

  return true;
}

void PostEffect::DimensionData::SetupRenderInfo (PostEffect& pfx)
{
  layerRenderInfos.DeleteAll ();
  layerRenderInfos.SetCapacity (pfx.postLayers.GetSize ());

  for (size_t l = 0; l < pfx.postLayers.GetSize (); l++)
  {
    const Layer& layer = *(pfx.postLayers[l]);

    LayerRenderInfo& renderInfo = layerRenderInfos.GetExtend (l);
    renderInfo.buffers.AttachNew (new csRenderBufferHolder);
  
    renderInfo.fullscreenQuad.meshtype = CS_MESHTYPE_QUADS;
    renderInfo.fullscreenQuad.vertexCount = 4;
    renderInfo.fullscreenQuad.mixmode = CS_MIXMODE_BLEND (ONE, ZERO);
    renderInfo.fullscreenQuad.shader = pfx.postLayers[l]->effectShader;
    renderInfo.fullscreenQuad.renderBuffers = renderInfo.buffers;
    renderInfo.buffers->SetRenderBuffer (CS_BUFFER_INDEX, pfx.indices);
    
    csRef<iRenderBuffer> vertBuf =
      csRenderBuffer::CreateRenderBuffer (4, CS_BUF_STATIC, 
        CS_BUFCOMP_FLOAT, 3);
    renderInfo.buffers->SetRenderBuffer (CS_BUFFER_POSITION, vertBuf);
        
    csRenderBufferLock<csVector3> screenQuadVerts (vertBuf);
  
    csRect targetRect (layer.options.targetRect);
    if (targetRect.IsEmpty ())
    {
      const iPostEffectLayer* outputLayer = pfx.GetRealOutputLayer (&layer);
      if (outputLayer->GetOptions ().manualTarget.IsValid ())
      {
        int targetW, targetH;
        outputLayer->GetOptions ().manualTarget->GetRendererDimensions (targetW, targetH);
	      targetRect.Set (0, 0, targetW, targetH);
      }
      else
      {
	      targetRect.Set (0, 0,
	        dim.x >> outputLayer->GetOptions ().downsample,
	        dim.y >> outputLayer->GetOptions ().downsample);
      }
    }
    
    // Setup the vertices & texcoords
    screenQuadVerts[(size_t)0].Set (targetRect.xmin, targetRect.ymin, 0);
    screenQuadVerts[(size_t)1].Set (targetRect.xmax, targetRect.ymin, 0);
    screenQuadVerts[(size_t)2].Set (targetRect.xmax, targetRect.ymax, 0);
    screenQuadVerts[(size_t)3].Set (targetRect.xmin, targetRect.ymax, 0);
  }
}

csPtr<iRenderBuffer> PostEffect::DimensionData::ComputeTexCoords (
  iTextureHandle* tex, const csRect& rect, const csRect& targetRect,
  float& pixSizeX, float& pixSizeY)
{
  csRect tcRect (rect);
  if (tcRect.IsEmpty ()) tcRect = targetRect;

  int tw, th, td;
  tex->GetRendererDimensions (tw, th, td);
  
  float tcMulX, tcMulY, tcOffsY;
  // Check if we actually got NPOTs textures
  //int resultFlags = tex->GetFlags ();
  if (tex->GetTextureType () == iTextureHandle::texTypeRect)
  {
    // Handle a rect texture
    // Rect texture use non-normalized texture coordinates
    tcMulX = 1;
    tcMulY = 1;
  }
  else
  {
    // 2D texture, coordinates are 0-1,0-1
    tcMulX = 1.0f/tw;
    tcMulY = 1.0f/th;
  }
  tcOffsY = (th-targetRect.ymax+0.5f)*tcMulY;
  
  csRef<iRenderBuffer> texcoordBuf =
    csRenderBuffer::CreateRenderBuffer (4, CS_BUF_STATIC,
      CS_BUFCOMP_FLOAT, 2);
      
  csRenderBufferLock<csVector2> screenQuadTex (texcoordBuf);

  // Setup the texcoords
  screenQuadTex[(size_t)0].Set ((tcRect.xmin+0.5f)*tcMulX, tcOffsY+tcRect.ymin*tcMulY);
  screenQuadTex[(size_t)1].Set ((tcRect.xmax+0.5f)*tcMulX, tcOffsY+tcRect.ymin*tcMulY);
  screenQuadTex[(size_t)2].Set ((tcRect.xmax+0.5f)*tcMulX, tcOffsY+tcRect.ymax*tcMulY);
  screenQuadTex[(size_t)3].Set ((tcRect.xmin+0.5f)*tcMulX, tcOffsY+tcRect.ymax*tcMulY);
  
  pixSizeX = tcMulX;
  pixSizeY = tcMulY;
  
  return csPtr<iRenderBuffer> (texcoordBuf);
}

class OverlaySVC :
  public scfImplementation1<OverlaySVC,
                            scfFakeInterface<iShaderVariableContext> >,
  public CS::Graphics::OverlayShaderVariableContextImpl
{
public:
  OverlaySVC (iShaderVariableContext* parent) : scfImplementationType (this)
  {
    SetParentContext (parent);
  }
};

void PostEffect::DimensionData::UpdateSVContexts (PostEffect& pfx)
{
  for (size_t l = 0; l < pfx.postLayers.GetSize (); l++)
  {
    const Layer& layer = *(pfx.postLayers[l]);
    LayerRenderInfo& renderInfo = layerRenderInfos[l];
    
    csRef<iShaderVariableContext> newSVs;
    newSVs.AttachNew (new OverlaySVC (layer.GetSVContext ()));
    renderInfo.layerSVs = newSVs;
    for (size_t i = 0; i < layer.GetInputs ().GetSize (); i++)
    {
      const PostEffectLayerInputMap& input = layer.GetInputs ()[i];
      
      csRef<csShaderVariable> sv;
      iTextureHandle* inputTex;
      int texW, texH;
      if (input.manualInput.IsValid ())
      {
        // User specified input texture
        newSVs->AddVariable (input.manualInput);
        input.manualInput->GetValue (inputTex);
        
        inputTex->GetRendererDimensions (texW, texH);
      }
      else
      {
        // PFX manager manages texture
        size_t inBucket = pfx.GetBucketIndex (input.inputLayer->GetOptions ());
	      sv.AttachNew (new csShaderVariable (pfx.manager->svStrings->Request (
	        input.textureName)));
	      inputTex =
	        buckets[inBucket].textures[input.inputLayer->GetOutTextureNum ()];
	      sv->SetValue (inputTex);
	      newSVs->AddVariable (sv);
	
        texW = dim.x >> input.inputLayer->GetOptions ().downsample;
        texH = dim.y >> input.inputLayer->GetOptions ().downsample;
      }
    
      csRect fullRect (0, 0, texW, texH);
    
      float pixSizeX, pixSizeY;
      csRenderBufferName bufferName =
        csRenderBuffer::GetBufferNameFromDescr (input.texcoordName);
      csRef<iRenderBuffer> texcoordBuf = ComputeTexCoords (inputTex,
        input.sourceRect, fullRect, pixSizeX, pixSizeY);
      if (bufferName != CS_BUFFER_NONE)
      {
        renderInfo.buffers->SetRenderBuffer (bufferName, texcoordBuf);
      }
      else
      {
        sv.AttachNew (new csShaderVariable (pfx.manager->svStrings->Request (
          input.texcoordName)));
        newSVs->AddVariable (sv);
      }
      
      if (!input.inputPixelSizeName.IsEmpty ())
      {
        csRef<csShaderVariable> svInPixSize;
	      svInPixSize.AttachNew (new csShaderVariable (
	        pfx.manager->svStrings->Request (input.inputPixelSizeName)));
	      svInPixSize->SetValue (csVector2 (pixSizeX, pixSizeY));
	      newSVs->AddVariable (svInPixSize);
      }
    }
    
    renderInfo.svPixelSize.AttachNew (new csShaderVariable (
      pfx.manager->svStrings->Request ("pixel size")));

    const iPostEffectLayer* outputLayer = pfx.GetRealOutputLayer (&layer);
    if (!outputLayer->GetOptions ().manualTarget.IsValid ())
    {
      size_t b = pfx.GetBucketIndex (outputLayer->GetOptions ());
      
      int texW = dim.x >> layer.options.downsample;
      int texH = dim.y >> layer.options.downsample;
      renderInfo.svPixelSize->SetValue (
	      csVector2 (buckets[b].texMaxX/float (texW), 
	      buckets[b].texMaxY/float (texH)));
    }
    else
    {
      // Don't really know pixel size...
      renderInfo.svPixelSize->SetValue (csVector2 (0, 0));
    }   
    renderInfo.fullscreenQuad.dynDomain = renderInfo.layerSVs;
  }
}

void PostEffect::UpdateTextureDistribution ()
{
  for (size_t l = 0; l < postLayers.GetSize (); l++)
  {
    if (postLayers[l]->options.renderOn != 0)
      // If rendering onto another layer the bucket of that layer will be used.
      continue;
    if (postLayers[l]->options.manualTarget.IsValid ())
      // If a manual target is given we don't allocate a texture here
      continue;
    GetBucket (postLayers[l]->options);
  }

  csArray<csArray<csBitArray> > allUsedTextureBits;
  allUsedTextureBits.SetSize (buckets.GetSize ());
  for (size_t b = 0; b < buckets.GetSize (); b++)
  {
    allUsedTextureBits[b].SetSize (postLayers.GetSize ());
  }
  
  for (size_t l = 0; l < postLayers.GetSize () - 1; l++)
  {
    if (postLayers[l]->options.renderOn != 0)
      // If rendering onto another layer the bucket of that layer will be used.
      continue;
    if (postLayers[l]->options.manualTarget.IsValid ())
      // If a manual target is given we don't allocate a texture here
      continue;
    
    size_t bucket = GetBucketIndex (postLayers[l]->options);
    csArray<csBitArray>& usedTextureBits = allUsedTextureBits[bucket];
    
    // Look for an unused texture
    size_t freeTexture;
    if (manager->keepAllIntermediates || buckets[bucket].options.noTextureReuse || (l == 0))
      freeTexture = csArrayItemNotFound;
    else
      freeTexture = usedTextureBits[l].GetFirstBitUnset ();

    if (freeTexture == csArrayItemNotFound)
    {
      // Add a new texture
      freeTexture = usedTextureBits[l].GetSize ();
      for (size_t b = l; b < postLayers.GetSize (); b++)
        usedTextureBits[b].SetSize (freeTexture+1);
    }
    
    postLayers[l]->outTextureNum = (int) freeTexture;
    
    size_t lastLayer = l;
    // Look for last layer which has current layer as an input
    for (size_t l2 = postLayers.GetSize (); l2-- > l; )
    {
      if (postLayers[l2]->IsInput (postLayers[l]))
      {
        lastLayer = l2;
        break;
      }
    }
    // Mark texture as used
    for (size_t l2 = l; l2 <= lastLayer; l2++)
    {
      usedTextureBits[l2].SetBit (freeTexture);
    }
  }
  
  for (size_t b = 0; b < buckets.GetSize (); b++)
  {
    csArray<csBitArray>& usedTextureBits = allUsedTextureBits[b];
    //buckets[b].textures.SetSize (usedTextureBits[postLayers.GetSize ()-1].GetSize ());
    buckets[b].textureNum = usedTextureBits[postLayers.GetSize ()-1].GetSize ();
  }
}

void PostEffect::UpdateLayers ()
{
  if (!layersDirty) return;
  
  dimCache.Clear (true);
  UpdateTextureDistribution ();
  
  layersDirty = false;
}

size_t PostEffect::GetBucketIndex (const PostEffectLayerOptions& options)
{
  for (size_t i = 0; i < buckets.GetSize (); i++)
  {
    if (buckets[i].options == options) return i;
  }
  size_t index = buckets.GetSize ();
  buckets.SetSize (index+1);
  buckets[index].options = options;
  return index;
}
    
bool PostEffect::ScreenSpaceYFlipped ()
{
  return (postLayers.GetSize () > 1);
}

//--------Layer--------------------------------------------------------------

CS_LEAKGUARD_IMPLEMENT (Layer);

Layer::Layer ()
  : scfImplementationType (this)
{
  svContext.AttachNew (new csShaderVariableContext);
}

bool Layer::IsInput (const Layer* layer) const
{
  for (size_t i = 0; i < inputs.GetSize (); i++)
  {
    if (inputs[i].inputLayer == layer) return true;
  }
  return false;
}

//-------PostEffectManager---------------------------------------------------

SCF_IMPLEMENT_FACTORY (PostEffectManager);

CS_LEAKGUARD_IMPLEMENT (PostEffectManager);

PostEffectManager::PostEffectManager (iBase* parent)
  : scfImplementationType (this, parent)
{
}    

bool PostEffectManager::Initialize (iObjectRegistry* objectReg)
{
  graphics3D = csQueryRegistry<iGraphics3D> (objectReg);
  svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (objectReg,
    "crystalspace.shader.variablenameset");
    
  csConfigAccess cfg (objectReg);
  keepAllIntermediates =
    cfg->GetBool ("PostEffectManager.KeepAllIntermediates", false);

  return true;
}

csPtr<iPostEffect> PostEffectManager::CreatePostEffect (const char* name)
{
  printf ("PostEffectManager::CreatePostEffect %s\n", name);
  CS_ASSERT (!postEffectsHash.Contains (name));    

  csRef<iPostEffect> effect;
  effect.AttachNew (new PostEffect (this, name));

  return csPtr<iPostEffect> (effect);
}

}
CS_PLUGIN_NAMESPACE_END (PostEffect)
