/*
    Copyright (C) 2011 by Santiago Sanchez

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

#include "csgfx/imagememory.h"
#include "csgfx/shadervar.h"
//#include "csgfx/ssdo.h"
#include "ssdo.h"
#include "csutil/cfgacc.h"
#include "csutil/scfstr.h"
#include "csutil/stringquote.h"

#include "igraphic/imageio.h"
#include "imap/loader.h"
#include "ivideo/graph2d.h"
#include "ivideo/shader/shader.h"

#include "gbuffer.h"

SSDOManager::SSDOManager ()
  : enabled (true)
{

}

// TODO: in a plugin instead?
bool SSDOManager::Initialize (iObjectRegistry *objRegistry)
{
  graphics3D = csQueryRegistry<iGraphics3D> (objRegistry);

  showAmbientOcclusion = false;
  showSSDO = false;
  blurEnabled = true;

  isSSDOBufferAttached  = false;
  isIntermediateBufferAttached = false;
  isDepthNormalBufferAttached  = false;
      
  objectRegistry = objRegistry;

  // TODO: in a specific config file
  csConfigAccess cfg (objRegistry);
/*
  enabled = cfg->GetBool ("RenderManager.Deferred.SSDO.Enable", true);
*/

  // Find the render manager post-effect interface and create the SSDO post-effect
  csRef<iRenderManager> renderManager =
    csQueryRegistry<iRenderManager> (objectRegistry);
  if (!renderManager)
    return ReportError ("Could not find any render manager");

  // TODO: check that it is the deferred rm
  // or: query "crystalspace.rendermanager.deferred"?

  csRef<iRenderManagerPostEffects> postEffects =
    scfQueryInterface<iRenderManagerPostEffects> (renderManager);
  if (!postEffects)
    return ReportError ("The render manager does not have support for post-effects");

  postEffect = postEffects->CreatePostEffect ("ssdo");
  postEffects->AddPostEffect (postEffect);

  // Initialize buffers
  SSDOBufferFormat =
    cfg->GetStr ("RenderManager.Deferred.GlobalIllum.BufferFormat",
		 "rgba16_f");
      
  bufferResolution =
    cfg->GetStr ("RenderManager.Deferred.GlobalIllum.BufferResolution",
		 "full");
  if (bufferResolution.CompareNoCase ("full"))
    bufferDownscaleFactor = 1.0f;
  else if (bufferResolution.CompareNoCase ("half"))
    bufferDownscaleFactor = 0.5f;
  else if (bufferResolution.CompareNoCase ("quarter"))
    bufferDownscaleFactor = 0.25f;
  else
    bufferDownscaleFactor = 0.5f;      

  // TODO: rename
  blurEnabled = cfg->GetBool
    ("RenderManager.Deferred.GlobalIllum.ApplyBlur", true);
      
  depthNormalsResolution = cfg->GetStr
    ("RenderManager.Deferred.GlobalIllum.DepthAndNormalsResolution",
     "full");
  if (depthNormalsResolution.CompareNoCase ("full"))
    depthNormalsBufferScale = 1.0f;
  else if (depthNormalsResolution.CompareNoCase ("half"))
    depthNormalsBufferScale = 0.5f;
  else if (depthNormalsResolution.CompareNoCase ("quarter"))
    depthNormalsBufferScale = 0.25f;
  else
    depthNormalsBufferScale = 1.0f;

  csRef<iLoader> loader = csQueryRegistry<iLoader> (objRegistry);
  shaderManager = csQueryRegistry<iShaderManager> (objRegistry);
  svStringSet = shaderManager->GetSVNameStringset ();

  colorBufferSVName = svStringSet->Request ("tex gbuffer 3");
  depthBufferSVName = svStringSet->Request ("tex gbuffer depth");

  // TODO: put shader in /posteffects
  // TODO: rename shaders
  SSDOShader = loader->LoadShader ("/shader/deferred/globalillum.xml");
  if (!SSDOShader)
  {
    enabled = false;
    return ReportError ("Could not load deferred_globalillum shader");
  }

  horizontalBlurShader = loader->LoadShader ("/shader/deferred/horizontal_blur.xml");
  if (!horizontalBlurShader)
  {
    enabled = false;
    return ReportError ("Could not load deferred_horizontal_blur shader");
  }

  verticalBlurShader = loader->LoadShader ("/shader/deferred/vertical_blur.xml");
  if (!verticalBlurShader)
  {
    enabled = false;
    return ReportError ("Could not load deferred_vertical_blur shader");
  }

  lightCompositionShader = loader->LoadShader ("/shader/deferred/composition.xml");
  if (!lightCompositionShader)
  {
    enabled = false;
    return ReportError ("Could not load deferred_composition shader");
  }

  downsampleShader = loader->LoadShader ("/shader/deferred/downsample.xml");
  if (!downsampleShader)
  {
    enabled = false;
    return ReportError ("Could not load deferred_downsample shader");
  }

  SetupShaderVars (cfg);

  printf ("test1\n");
  if (!InitRenderTargets ())
    return false;

  LoadRandomNormalsTexture (loader, graphics3D, cfg);          

  return true;
}

void SSDOManager::SetEnabled (bool enabled)
{
  this->enabled = enabled;
}

bool SSDOManager::GetEnabled ()
{
  return enabled;
}

void SSDOManager::SetBlurEnabled (bool enabled)
{
  blurEnabled = enabled;
}

bool SSDOManager::GetBlurEnabled ()
{
  return blurEnabled;
}

void SSDOManager::SetBufferResolution (const char *bufferResolution)
{
  if (!enabled || !bufferResolution)
    return;

  csString bufferRes (bufferResolution);
  float resolutionFactor;

  if (bufferRes.CompareNoCase ("full"))
    resolutionFactor = 1.0f;
  else if (bufferRes.CompareNoCase ("half"))
    resolutionFactor = 0.5f;
  else if (bufferRes.CompareNoCase ("quarter"))
    resolutionFactor = 0.25f;
  else
  {
    ReportError ("Could not change SSDO buffer resolution: invalid argument %s",
		 CS::Quote::Single (bufferResolution));
    return;
  }

  if (fabs (resolutionFactor - bufferDownscaleFactor) > EPSILON)
  {
    bufferDownscaleFactor = resolutionFactor;

    DestroyIntermediateBuffers ();
    InitRenderTargets ();

    SSDOBufferSV->SetValue (SSDOBuffer);
    intermediateBufferSV->SetValue (intermediateBuffer);
  }
}

const char *SSDOManager::GetBufferResolution ()
{
  return bufferResolution;
}

void SSDOManager::SetNormalsAndDepthBufferResolution (const char *resolution)
{
  if (csString (resolution).CompareNoCase (depthNormalsResolution))
    return;

  depthNormalsResolution = csString (resolution);

  if (depthNormalsResolution.CompareNoCase ("full"))
  {
    // Use full resolution depth and normals from deferred RM's gbuffer
    DetachDepthNormalBuffer ();
    iTextureHandle* colorBuffer;
    shaderManager->GetVariable (colorBufferSVName)->GetValue (colorBuffer);
    depthNormalBufferSV->SetValue (colorBuffer);
    depthNormalsBufferScale = 1.0f;
    depthNormalBuffer = nullptr;
    return;
  }

  iGraphics2D *g2D = graphics3D->GetDriver2D ();

  depthNormalsBufferScale = 0.5f;
  if (depthNormalsResolution.CompareNoCase ("quarter"))
    depthNormalsBufferScale = 0.25f;

  bool changeRes = !depthNormalBuffer;
  if (!changeRes)
  {
    // Check if depthNormalBuffer resolution is not already the desired
    int bufwidth = 0, bufheight = 0;
    depthNormalBuffer->GetRendererDimensions (bufwidth, bufheight);

    changeRes = !(fabs (depthNormalsBufferScale * g2D->GetWidth () - (float)bufwidth) < EPSILON) ||
      !(fabs (depthNormalsBufferScale * g2D->GetHeight () - (float)bufheight) < EPSILON);
  }

  if (changeRes)
  {
    const int flags = CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_CLAMP | CS_TEXTURE_NPOTS;
    scfString errString;

    iTextureHandle* colorBuffer;
    shaderManager->GetVariable (colorBufferSVName)->GetValue (colorBuffer);

    depthNormalBuffer = graphics3D->GetTextureManager ()->CreateTexture (
      g2D->GetWidth () * depthNormalsBufferScale, g2D->GetHeight () * depthNormalsBufferScale,
      csimg2D, gbuffer->GetColorBufferFormat (), flags, &errString);

    if (!depthNormalBuffer)
    {
      // Can't use downsampled buffer -> use full resolution depth and normals from
      // deferred RM's gbuffer
      DetachDepthNormalBuffer ();
      depthNormalBufferSV->SetValue (colorBuffer);
      depthNormalsResolution = csString ("full");
      depthNormalsBufferScale = 1.0f;

      ReportError ("Could not create depth and normals buffer! %s",
		   errString.GetCsString ().GetDataSafe ());
    }
  }

  // Use downsampled depth and normals
  depthNormalBufferSV->SetValue (depthNormalBuffer);
}

const char *SSDOManager::GetNormalsAndDepthBufferResolution ()
{
  return depthNormalsResolution;
}

csShaderVariable* SSDOManager::GetSSDOVariableAdd (const char *svName)
{
  if (!svName) return nullptr;
  return SSDOShader->GetVariableAdd (svStringSet->Request (svName));  
}

csShaderVariable* SSDOManager::GetBlurVariableAdd (const char *svName)
{
  if (!svName) return nullptr;
  return shaderManager->GetVariableAdd (svStringSet->Request (svName));
}

csShaderVariable* SSDOManager::GetCompositionVariableAdd (const char *svName)
{
  if (!svName) return nullptr;
  return lightCompositionShader->GetVariableAdd (svStringSet->Request (svName));
}

bool SSDOManager::InitRenderTargets ()
{
  if (depthNormalsBufferScale < 1.0f - EPSILON)
  {
    // TODO: convert depthNormalsBufferScale into downscale
    iTextureHandle* colorBuffer;
    csShaderVariable* variable = shaderManager->GetVariable (colorBufferSVName);
    if (!variable)
      return ReportError ("Could not find the deferred color buffer");
    variable->GetValue (colorBuffer);

    CS::StructuredTextureFormat format;
    csRef<iDataBuffer> buffer = colorBuffer->Readback (format, 0);

    // TODO: correct? to be defined per layer instead?
    postEffect->SetIntermediateTargetFormat (format.GetCanonical ());

    //iPostEffectLayer* AddLayer (iShader* shader);
    depthNormalsLayer = postEffect->AddLayer (downsampleShader);
    printf ("test\n");
  }

  // TODO: use bufferDownscaleFactor
  // TODO: use SSDO format
  ssdoLayer = postEffect->AddLayer (SSDOShader);

  if (blurEnabled)
  {
    // TODO: use SSDO format & scale
    horizontalBlurLayer = postEffect->AddLayer (horizontalBlurShader);
    verticalBlurLayer = postEffect->AddLayer (verticalBlurShader);
  }

  // TODO: use SSDO format & scale
  lightCompositionLayer = postEffect->AddLayer (lightCompositionShader);

/*
  const int flags =
    CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_CLAMP | CS_TEXTURE_NPOTS;
  scfString errString;
  iGraphics2D *g2D = graphics3D->GetDriver2D ();

  if (!SSDOBuffer)
  {
    SSDOBuffer = graphics3D->GetTextureManager ()->CreateTexture (
      g2D->GetWidth () * bufferDownscaleFactor, g2D->GetHeight () * bufferDownscaleFactor, 
      csimg2D, SSDOBufferFormat, flags, &errString);

    if (!SSDOBuffer)
      return ReportError ("Could not create SSDO buffer! %s",
			  errString.GetCsString ().GetDataSafe ());
  }

  if (!intermediateBuffer)
  {
    intermediateBuffer = graphics3D->GetTextureManager ()->CreateTexture (
      g2D->GetWidth () * bufferDownscaleFactor, g2D->GetHeight () * bufferDownscaleFactor, 
      csimg2D, SSDOBufferFormat, flags, &errString);

    if (!intermediateBuffer)
      return ReportError ("Could not create intermediate buffer! %s",
			  errString.GetCsString ().GetDataSafe ());        
  }      

  if (depthNormalsBufferScale < 1.0f && !depthNormalBuffer)
  {
    depthNormalBuffer = graphics3D->GetTextureManager ()->CreateTexture (
      g2D->GetWidth () * depthNormalsBufferScale, g2D->GetHeight () * depthNormalsBufferScale,
      csimg2D, gbuffer->GetColorBufferFormat (), flags, &errString);

    if (!depthNormalBuffer)
      return ReportError ("Could not create depth and normals buffer! %s",
			  errString.GetCsString ().GetDataSafe ());        
  }

  // Test if the buffer formats are supported
  if (!AttachSSDOBuffer ())
    return ReportError ("Failed to attach SSDO buffer to the device!");

  if (!graphics3D->ValidateRenderTargets ())
  {
    DetachSSDOBuffer ();
    return ReportError ("SSDO buffer format is not supported by the device!");
  }

  DetachSSDOBuffer ();

  if (!AttachIntermediateBuffer ())
    return ReportError ("Failed to attach intermediate buffer to the device!");

  if (!graphics3D->ValidateRenderTargets ())
  {
    DetachIntermediateBuffer ();
    return ReportError ("Intermediate buffer format is not supported by the device!");
  }

  DetachIntermediateBuffer ();
*/
  return true;
}

bool SSDOManager::AttachBuffer (iTextureHandle *buffer)
{
  if (!graphics3D->SetRenderTarget (buffer, false, 0, rtaColor0))
    return false;

  if (!graphics3D->ValidateRenderTargets ())
    return false;

  return true;
}

bool SSDOManager::AttachSSDOBuffer (bool useGBufferDepth)
{
  if (!enabled || isSSDOBufferAttached)
    return false;

  if (!graphics3D->SetRenderTarget (SSDOBuffer, false, 0, rtaColor0))
    return false;

  if (useGBufferDepth && gbuffer->HasDepthBuffer ())
  {
    if (!graphics3D->SetRenderTarget (gbuffer->GetDepthBuffer (), false, 0, rtaDepth))
      return false;
  }

  if (!graphics3D->ValidateRenderTargets ())
    return false;

  isSSDOBufferAttached = true;
  return true;
}

void SSDOManager::DetachSSDOBuffer ()
{
  if (enabled && isSSDOBufferAttached)
  {
    graphics3D->UnsetRenderTargets ();
    isSSDOBufferAttached = false;
  }      
}

bool SSDOManager::AttachIntermediateBuffer (bool useGBufferDepth)
{
  if (!enabled || isIntermediateBufferAttached)
    return false;

  if (!graphics3D->SetRenderTarget (intermediateBuffer, false, 0, rtaColor0))
    return false;

  if (useGBufferDepth && gbuffer->HasDepthBuffer ())
  {
    if (!graphics3D->SetRenderTarget (gbuffer->GetDepthBuffer (), false, 0, rtaDepth))
      return false;
  }

  if (!graphics3D->ValidateRenderTargets ())
    return false;

  isIntermediateBufferAttached = true;
  return true;
}

void SSDOManager::DetachIntermediateBuffer ()
{
  if (enabled && isIntermediateBufferAttached)
  {
    graphics3D->UnsetRenderTargets ();
    isIntermediateBufferAttached = false;
  }      
}

void SSDOManager::DestroyIntermediateBuffers ()
{
  graphics3D->UnsetRenderTargets ();
  isSSDOBufferAttached = isIntermediateBufferAttached = false;

  if (SSDOBuffer)
    SSDOBuffer.Invalidate ();

  if (intermediateBuffer)
    intermediateBuffer.Invalidate ();
}

bool SSDOManager::AttachDepthNormalBuffer ()
{
  if (!enabled || isDepthNormalBufferAttached)
    return false;

  if (!graphics3D->SetRenderTarget (depthNormalBuffer, false, 0, rtaColor0))
    return false;      

  if (!graphics3D->ValidateRenderTargets ())
    return false;

  isDepthNormalBufferAttached = true;
  return true;
}

void SSDOManager::DetachDepthNormalBuffer ()
{
  if (enabled && isDepthNormalBufferAttached)
  {
    graphics3D->UnsetRenderTargets ();
    isDepthNormalBufferAttached = false;
  }      
}

void SSDOManager::SetupShaderVars (csConfigAccess &cfg)
{
  printf ("SSDOManager::SetupShaderVars\n");

  SSDOBufferSV = shaderManager->GetVariableAdd (
    svStringSet->Request ("tex global illumination"));
  SSDOBufferSV->SetValue (postEffect->GetLayerOutput (ssdoLayer));

  intermediateBufferSV = verticalBlurShader->GetVariableAdd (
    svStringSet->Request ("tex horizontal blur"));
  intermediateBufferSV->SetValue (intermediateBuffer);

  depthNormalBufferSV = shaderManager->GetVariableAdd (
    svStringSet->Request ("tex normals depth"));
  if (depthNormalsBufferScale < 1.0f)
    depthNormalBufferSV->SetValue (postEffect->GetLayerOutput (depthNormalsLayer));
  else
  {
    iTextureHandle* colorBuffer;
    // TODO
    //shaderManager->GetVariable (colorBufferSVName)->GetValue (colorBuffer);
    shaderManager->GetVariableAdd (colorBufferSVName)->GetValue (colorBuffer);
    depthNormalBufferSV->SetValue (colorBuffer);
  }
      
  csRef<csShaderVariable> shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("tex random normals"));      
  shaderVar->SetValue (randomNormalsTexture);      

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("num passes"));
  int sampleCount = cfg->GetInt
    ("RenderManager.Deferred.GlobalIllum.NumPasses", 2);
  shaderVar->SetValue (sampleCount);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("sample radius"));
  float sampleRadius = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.SampleRadius", 0.1f);
  shaderVar->SetValue (sampleRadius);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("detail sample radius"));
  float sampleRadiusFar = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.DetailSampleRadius", 0.6f);
  shaderVar->SetValue (sampleRadiusFar);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("self occlusion"));
  float depthBias = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.SelfOcclusion", 0.0f);
  shaderVar->SetValue (depthBias);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("occlusion strength"));
  float occlusionStrength = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.OcclusionStrength", 0.7f);
  shaderVar->SetValue (occlusionStrength);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("max occluder distance"));
  float maxOccluderDistance = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.MaxOccluderDistance", 0.8f);
  shaderVar->SetValue (maxOccluderDistance);      

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("bounce strength"));
  float bounceStrength = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.LightBounceStrength", 6.0f);
  shaderVar->SetValue (bounceStrength);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("enable ambient occlusion"));
  bool enableAO = cfg->GetBool
    ("RenderManager.Deferred.GlobalIllum.AmbientOcclusion.Enable", true);
  shaderVar->SetValue ((float)enableAO);

  shaderVar = SSDOShader->GetVariableAdd (
    svStringSet->Request ("enable indirect light"));
  bool enableIL = cfg->GetFloat
    ("RenderManager.Deferred.GlobalIllum.IndirectLight.Enable", true);
  shaderVar->SetValue ((float)enableIL);

  shaderVar = shaderManager->GetVariableAdd (
    svStringSet->Request ("ssao blur kernelsize"));
  int blurKernelSize = cfg->GetInt
    ("RenderManager.Deferred.GlobalIllum.Blur.KernelSize", 3);
  shaderVar->SetValue (blurKernelSize);

  shaderVar = shaderManager->GetVariableAdd (
    svStringSet->Request ("ssao blur position threshold"));
  float blurPositionThreshold = cfg->GetFloat (
    "RenderManager.Deferred.GlobalIllum.Blur.PositionThreshold", 0.5f);
  shaderVar->SetValue (blurPositionThreshold);

  shaderVar = shaderManager->GetVariableAdd (
    svStringSet->Request ("ssao blur normal threshold"));
  float blurNormalThreshold = cfg->GetFloat (
    "RenderManager.Deferred.GlobalIllum.Blur.NormalThreshold", 0.1f);
  shaderVar->SetValue (blurNormalThreshold);

  shaderVar = shaderManager->GetVariableAdd (
    svStringSet->Request ("far clip distance"));
  float farClipDistance = cfg->GetFloat (
    "RenderManager.Deferred.GlobalIllum.FarClipDistance", 100.0f);
  shaderVar->SetValue (farClipDistance);

  showAmbientOcclusion = false;
  showSSDO = false;
  shaderVar = lightCompositionShader->GetVariableAdd (
    svStringSet->Request ("debug show ambocc"));
  shaderVar->SetValue ((int) showAmbientOcclusion);

  shaderVar = lightCompositionShader->GetVariableAdd (
    svStringSet->Request ("debug show globalillum"));
  shaderVar->SetValue ((int) showSSDO);
}    

void SSDOManager::LoadRandomNormalsTexture
(iLoader *loader, iGraphics3D *graphics3D, csConfigAccess &cfg)
{
  csRef<iImage> image;
  int flags = CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS;

  const char *randomNormalsFilePath = cfg->GetStr (
    "RenderManager.Deferred.GlobalIllum.RandomNormalsFilePath",
    "/data/random_normals64.png");
    //"/data/InterleavedSphereJittered4x4.png");

  randomNormalsTexture = loader->LoadTexture
    (randomNormalsFilePath, flags, graphics3D->GetTextureManager (), &image);
  if (!randomNormalsTexture)
    ReportError ("Could not load random normals texture!");
}    
