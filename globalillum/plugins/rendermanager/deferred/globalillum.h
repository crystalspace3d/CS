/*
    Copyright (C) 2011 by Santiago Sanchez

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

#ifndef __GLOBALILLUM_H__
#define __GLOBALILLUM_H__

#include "cssysdef.h"

#include "csgfx/shadervar.h"
#include "csgfx/imagememory.h"
#include "igraphic/imageio.h"
#include "imap/loader.h"
#include "csutil/cfgacc.h"
#include "csutil/scfstr.h"
#include "ivideo/shader/shader.h"
#include "deferredrender.h"

CS_PLUGIN_NAMESPACE_BEGIN(RMDeferred)
{
  /**
   * Renderer for screen-space global illumination techniques.
   */
  class csGlobalIllumRenderer
  {
  public:

    csGlobalIllumRenderer() : graphics3D (nullptr), globalIllumBuffer (nullptr),
      intermediateBuffer (nullptr), gbuffer (nullptr), enabled (true), isInitialized (false) 
    {         
    }
    
    ~csGlobalIllumRenderer() 
    {
      CS_ASSERT (!isGlobalIllumBufferAttached && !isIntermediateBufferAttached
                 && !isDepthNormalBufferAttached);
    }
    
    bool Initialize(iGraphics3D *g3D, iObjectRegistry *objRegistry, GBuffer *gbuffer, 
        bool readEnableKeyFromConfig = true)
    {
      reporterMessageID = "crystalspace.rendermanager.globalillum";

      if (isInitialized)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_NOTIFY, reporterMessageID,
          "Component already initialized!");
        return true;
      }

      graphics3D = g3D;
      isGlobalIllumBufferAttached  = false;
      isIntermediateBufferAttached = false;
      isDepthNormalBufferAttached  = false;
      
      objectRegistry = objRegistry;
      csConfigAccess cfg (objRegistry);

      if (readEnableKeyFromConfig)
        enabled = cfg->GetBool ("RenderManager.Deferred.GlobalIllum.Enable", true);

      if (!enabled)
      {        
        return false;
      }

      this->gbuffer = gbuffer;

      // Initialize buffers
      globalIllumBufferFormat = cfg->GetStr ("RenderManager.Deferred.GlobalIllum.GlobalIllumBufferFormat",
        "rgba16_f");
      
      csString bufferRes (cfg->GetStr ("RenderManager.Deferred.GlobalIllum.BufferResolution", "half"));
      if (bufferRes.CompareNoCase ("full"))
        bufferDownscaleFactor = 1.0f;
      else if (bufferRes.CompareNoCase ("half"))
        bufferDownscaleFactor = 0.5f;
      else if (bufferRes.CompareNoCase ("quarter"))
        bufferDownscaleFactor = 0.25f;
      else
        bufferDownscaleFactor = 0.5f;

      if (!InitRenderBuffers())
      {
        enabled = false;
        return false;
      }     
      
      csRef<iLoader> loader = csQueryRegistry<iLoader> (objRegistry);
      shaderManager = csQueryRegistry<iShaderManager> (objRegistry);
      svStringSet = shaderManager->GetSVNameStringset ();

      globalIllumShader = loader->LoadShader ("/shader/deferred/globalillum.xml");
      if (!globalIllumShader)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load deferred_globalillum shader");
        enabled = false;
        return false;
      }

      horizontalBlurShader = loader->LoadShader ("/shader/deferred/horizontal_blur.xml");
      if (!horizontalBlurShader)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load deferred_horizontal_blur shader");
        enabled = false;
        return false;
      }

      verticalBlurShader = loader->LoadShader ("/shader/deferred/vertical_blur.xml");
      if (!verticalBlurShader)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load deferred_vertical_blur shader");
        enabled = false;
        return false;
      }

      lightCompositionShader = loader->LoadShader ("/shader/deferred/composition.xml");
      if (!lightCompositionShader)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load deferred_composition shader");
        enabled = false;
        return false;
      }

      downsampleShader = loader->LoadShader ("/shader/deferred/downsample.xml");
      if (!downsampleShader)
      {
        csReport (objRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load deferred_downsample shader");
        enabled = false;
        return false;
      }

      LoadRandomNormalsTexture (loader, graphics3D, cfg);
      LoadIrradianceEnvironmentMap (loader, graphics3D);
      //GenerateSampleDirections (graphics3D);
      
      SetupShaderVars(cfg);

      CreateFullscreenQuad();

      isInitialized = true;
      applyBlur = true;
      return true;
    }    

    void UpdateShaderVars()
    {
      if (!enabled)
        return;

      lightCompositionShader->GetVariableAdd (
        svStringSet->Request ("debug show ambocc"))->SetValue ((int)showAmbientOcclusion);
      lightCompositionShader->GetVariableAdd (
        svStringSet->Request ("debug show globalillum"))->SetValue ((int)showGlobalIllumination);
    }

    void RenderGlobalIllum(iTextureHandle *accumBuffer)
    {
      if (!enabled || !accumBuffer)
        return;

      // Downsample buffer with normals and depth
      /*AttachDepthNormalBuffer();
      {
        graphics3D->SetZMode (CS_ZBUF_MESH);        

        DrawFullscreenQuad (downsampleShader);
      }
      DetachDepthNormalBuffer();*/

      // Render ambient occlusion + indirect light
      AttachGlobalIllumBuffer();
      {
        graphics3D->SetZMode (CS_ZBUF_MESH);        

        DrawFullscreenQuad (globalIllumShader);
      }
      DetachGlobalIllumBuffer();

      // Apply edge-aware blur
      if (applyBlur)
      {
        AttachIntermediateBuffer();
        {
          graphics3D->SetZMode (CS_ZBUF_MESH);

          DrawFullscreenQuad (horizontalBlurShader);
        }
        DetachIntermediateBuffer();
	          
        AttachGlobalIllumBuffer();
        {
          graphics3D->SetZMode (CS_ZBUF_MESH);

		      DrawFullscreenQuad (verticalBlurShader);
        }
        DetachGlobalIllumBuffer();
      }
      
      // Upsample and combine with direct light
      AttachBuffer (accumBuffer);
      {
        graphics3D->SetZMode (CS_ZBUF_MESH);
        
        DrawFullscreenQuad (lightCompositionShader);
      }
      graphics3D->UnsetRenderTargets();
    }

    void ChangeBufferResolution(const char *bufferResolution)
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
        csReport (objectRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not change global illumination buffer resolution: invalid argument");
        return;
      }

      if (! (fabs (resolutionFactor - bufferDownscaleFactor) < EPSILON))
      {
        bufferDownscaleFactor = resolutionFactor;

        DestroyIntermediateBuffers();
        InitRenderBuffers();

        globalIllumBufferSV->SetValue (globalIllumBuffer);
        intermediateBufferSV->SetValue (intermediateBuffer);
      }
    }

    bool AttachGlobalIllumBuffer(bool useGBufferDepth = false)
    {
      if (!enabled || isGlobalIllumBufferAttached)
        return false;

      if (!graphics3D->SetRenderTarget (globalIllumBuffer, false, 0, rtaColor0))
          return false;

	    if (useGBufferDepth && gbuffer->HasDepthBuffer())
      {
        if (!graphics3D->SetRenderTarget (gbuffer->GetDepthBuffer(), false, 0, rtaDepth))
          return false;
      }

      if (!graphics3D->ValidateRenderTargets())
        return false;

      isGlobalIllumBufferAttached = true;
      return true;
    }

    void DetachGlobalIllumBuffer()
    {
      if (enabled && isGlobalIllumBufferAttached)
      {
        graphics3D->UnsetRenderTargets();
        isGlobalIllumBufferAttached = false;
      }      
    }

    bool AttachIntermediateBuffer(bool useGBufferDepth = false)
    {
      if (!enabled || isIntermediateBufferAttached)
        return false;

      if (!graphics3D->SetRenderTarget (intermediateBuffer, false, 0, rtaColor0))
          return false;

      if (useGBufferDepth && gbuffer->HasDepthBuffer())
      {
        if (!graphics3D->SetRenderTarget (gbuffer->GetDepthBuffer(), false, 0, rtaDepth))
          return false;
      }

      if (!graphics3D->ValidateRenderTargets())
        return false;

      isIntermediateBufferAttached = true;
      return true;
    }

    void DetachIntermediateBuffer()
    {
      if (enabled && isIntermediateBufferAttached)
      {
        graphics3D->UnsetRenderTargets();
        isIntermediateBufferAttached = false;
      }      
    }

    bool AttachDepthNormalBuffer()
    {
      if (!enabled || isDepthNormalBufferAttached)
        return false;

      if (!graphics3D->SetRenderTarget (depthNormalBuffer, false, 0, rtaColor0))
          return false;      

      if (!graphics3D->ValidateRenderTargets())
        return false;

      isDepthNormalBufferAttached = true;
      return true;
    }

    void DetachDepthNormalBuffer()
    {
      if (enabled && isDepthNormalBufferAttached)
      {
        graphics3D->UnsetRenderTargets();
        isDepthNormalBufferAttached = false;
      }      
    }

    iTextureHandle *GetGlobalIllumBuffer()
    {
      return globalIllumBuffer;
    }

    iTextureHandle *GetIntermediateBuffer()
    {
      return intermediateBuffer;
    }

    bool IsInitialized()
    {
      return isInitialized;
    }

    bool IsEnabled() 
    { 
      return this->enabled;
    }

    void SetEnabled(bool value)
    {
      this->enabled = value;
    }    

  private:

    bool InitRenderBuffers()
    {
      const int flags = CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_CLAMP | CS_TEXTURE_NPOTS;
      scfString errString;
      iGraphics2D *g2D = graphics3D->GetDriver2D();

      if (!globalIllumBuffer)
      {
        globalIllumBuffer = graphics3D->GetTextureManager()->CreateTexture (
          g2D->GetWidth() * bufferDownscaleFactor, g2D->GetHeight() * bufferDownscaleFactor, 
          csimg2D, globalIllumBufferFormat, flags, &errString);

        if (!globalIllumBuffer)
        {
          csReport (objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
            "Could not create global illumination buffer! %s", errString.GetCsString().GetDataSafe());
          return false;
        }
      }

      if (!intermediateBuffer)
      {
        intermediateBuffer = graphics3D->GetTextureManager()->CreateTexture (
          g2D->GetWidth() * bufferDownscaleFactor, g2D->GetHeight() * bufferDownscaleFactor, 
          csimg2D, globalIllumBufferFormat, flags, &errString);

        if (!intermediateBuffer)
        {
          csReport (objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
            "Could not create intermediate buffer! %s", errString.GetCsString().GetDataSafe());        
          return false;
        }
      }      

      if (!depthNormalBuffer)
      {
        depthNormalBuffer = graphics3D->GetTextureManager()->CreateTexture (
          g2D->GetWidth() * 0.5f/*bufferDownscaleFactor*/, g2D->GetHeight() * 0.5f/*bufferDownscaleFactor*/,
          csimg2D, gbuffer->GetColorBufferFormat(), flags, &errString);

        if (!depthNormalBuffer)
        {
          csReport (objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
            "Could not create depth and normals buffer! %s", errString.GetCsString().GetDataSafe());        
          return false;
        }
      }

      // Test if the buffer formats are supported. Only when initializing for the first time.
      if (!isInitialized)
      {
        if (!AttachGlobalIllumBuffer())
        {
          csReport(objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
              "Failed to attach global illumination buffer to the device!");
          return false;
        }

        if (!graphics3D->ValidateRenderTargets())
        {
          DetachGlobalIllumBuffer();
          csReport(objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
              "Global illumination buffer format is not supported by the device!");
          return false;
        }
        DetachGlobalIllumBuffer();

        if (!AttachIntermediateBuffer())
        {
          csReport(objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
              "Failed to attach intermediate buffer to the device!");
          return false;
        }

        if (!graphics3D->ValidateRenderTargets())
        {
          DetachIntermediateBuffer();
          csReport(objectRegistry, CS_REPORTER_SEVERITY_ERROR, reporterMessageID, 
              "Intermediate buffer format is not supported by the device!");
          return false;
        }
        DetachIntermediateBuffer();
      }

      return true;
    }

    bool AttachBuffer(iTextureHandle *buffer)
    {
      if (!graphics3D->SetRenderTarget (buffer, false, 0, rtaColor0))
          return false;

      if (!graphics3D->ValidateRenderTargets())
        return false;

      return true;
    }

    void DestroyIntermediateBuffers()
    {
      graphics3D->UnsetRenderTargets();
      isGlobalIllumBufferAttached = isIntermediateBufferAttached = false;

      if (globalIllumBuffer)
      {
        //delete globalIllumBuffer;
        globalIllumBuffer.Invalidate();
      }

      if (intermediateBuffer)
      {
        //delete intermediateBuffer;
        intermediateBuffer.Invalidate();
      }
    }

    void SetupShaderVars(csConfigAccess &cfg)
    {       
      globalIllumBufferSV = shaderManager->GetVariableAdd (
          svStringSet->Request ("tex global illumination"));
      globalIllumBufferSV->SetValue (globalIllumBuffer);

      intermediateBufferSV = verticalBlurShader->GetVariableAdd (
          svStringSet->Request ("tex horizontal blur"));
      intermediateBufferSV->SetValue (intermediateBuffer);

      depthNormalBufferSV = shaderManager->GetVariableAdd (
          svStringSet->Request ("tex downsampled normal depth"));
      depthNormalBufferSV->SetValue (depthNormalBuffer);
      
      csRef<csShaderVariable> shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("tex random normals"));      
      shaderVar->SetValue (randomNormalsTexture);
      
      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("tex globalillum envmap"));
      shaderVar->SetValue (irradianceEnvironmentMap);      

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("num passes"));
      int sampleCount = cfg->GetInt ("RenderManager.Deferred.GlobalIllum.SSDO.NumPasses", 2);
      shaderVar->SetValue (sampleCount);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("sample radius"));
      float sampleRadius = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.SampleRadius", 0.1f);
      shaderVar->SetValue (sampleRadius);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("detail sample radius"));
      float sampleRadiusFar = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.SampleRadiusFar", 0.6f);
      shaderVar->SetValue (sampleRadiusFar);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("self occlusion"));
      float depthBias = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.SelfOcclusion", 0.0f);
      shaderVar->SetValue (depthBias);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("occlusion strength"));
      float occlusionStrength = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.OcclusionStrength",
        0.7f);
      shaderVar->SetValue (occlusionStrength);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("max occluder distance"));
      float maxOccluderDistance = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.MaxOccluderDistance",
        0.8f);
      shaderVar->SetValue (maxOccluderDistance);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("occluder angle bias"));
      float lightRotationAngle = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.OccluderAngleBias",
        0.0f);
      shaderVar->SetValue (lightRotationAngle);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("bounce strength"));
      float bounceStrength = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.BounceStrength", 6.0f);
      shaderVar->SetValue (bounceStrength);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("enable ambient occlusion"));
      float enableAO = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.EnableAO", 1.0f);
      shaderVar->SetValue (enableAO);

      shaderVar = globalIllumShader->GetVariableAdd (
          svStringSet->Request ("enable indirect light"));
      float enableIL = cfg->GetFloat ("RenderManager.Deferred.GlobalIllum.SSDO.EnableIndirectLight", 1.0f);
      shaderVar->SetValue (enableIL);

      shaderVar = shaderManager->GetVariableAdd (
          svStringSet->Request ("ssao blur kernelsize"));
      int blurKernelSize = cfg->GetInt ("RenderManager.Deferred.GlobalIllum.SSDO.BlurKernelSize", 3);
      shaderVar->SetValue (blurKernelSize);

      shaderVar = shaderManager->GetVariableAdd (
          svStringSet->Request ("ssao blur position threshold"));
      float blurPositionThreshold = cfg->GetFloat (
        "RenderManager.Deferred.GlobalIllum.SSDO.BlurPositionThreshold", 0.5f);
      shaderVar->SetValue (blurPositionThreshold);

      shaderVar = shaderManager->GetVariableAdd (
          svStringSet->Request ("ssao blur normal threshold"));
      float blurNormalThreshold = cfg->GetFloat (
        "RenderManager.Deferred.GlobalIllum.SSDO.BlurNormalThreshold", 0.1f);
      shaderVar->SetValue (blurNormalThreshold);

      showAmbientOcclusion = false;
      showGlobalIllumination = false;
      shaderVar = lightCompositionShader->GetVariableAdd (
          svStringSet->Request ("debug show ambocc"));
      shaderVar->SetValue ((int)showAmbientOcclusion);

      shaderVar = lightCompositionShader->GetVariableAdd (
          svStringSet->Request ("debug show globalillum"));
      shaderVar->SetValue ((int)showGlobalIllumination);
    }

    void CreateFullscreenQuad()
    {      
      float w = graphics3D->GetDriver2D ()->GetWidth ();
      float h = graphics3D->GetDriver2D ()->GetHeight ();

      quadVerts[0] = csVector3 (0.0f, 0.0f, 0.0f);
      quadVerts[1] = csVector3 (0.0f,    h, 0.0f);
      quadVerts[2] = csVector3 (   w,    h, 0.0f);
      quadVerts[3] = csVector3 (   w, 0.0f, 0.0f);

      uint mixModeNoBlending = CS_MIXMODE_BLEND(ONE, ZERO) | CS_MIXMODE_ALPHATEST_DISABLE;

      quadMesh.meshtype = CS_MESHTYPE_TRIANGLEFAN;
      quadMesh.vertices = quadVerts;
      quadMesh.vertexCount = 4;
      quadMesh.z_buf_mode = CS_ZBUF_NONE;
      quadMesh.mixmode = mixModeNoBlending;
      quadMesh.alphaType.autoAlphaMode = false;
      quadMesh.alphaType.alphaType = csAlphaMode::alphaNone;
    }

    void LoadRandomNormalsTexture(iLoader *loader, iGraphics3D *graphics3D, csConfigAccess &cfg)
    {
      csRef<iImage> image;
      int flags = CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS;

      const char *randomNormalsFilePath = cfg->GetStr (
        "RenderManager.Deferred.GlobalIllum.SSDO.RandomNormalsFilePath", "/data/random_normals64.png");
        //"/data/InterleavedSphereJittered4x4.png");

      randomNormalsTexture = loader->LoadTexture (randomNormalsFilePath, flags, 
        graphics3D->GetTextureManager(), &image);
      if (!randomNormalsTexture)
      {
        csReport (objectRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load random_normals.png texture!");
      }
    }

    void LoadIrradianceEnvironmentMap(iLoader *loader, iGraphics3D *graphics3D)
    {
      csRef<iImage> image;
      int flags = CS_TEXTURE_2D | CS_TEXTURE_CLAMP | CS_TEXTURE_NOMIPMAPS;

      irradianceEnvironmentMap = loader->LoadTexture ("/data/blurred_grace.tga", flags, 
        graphics3D->GetTextureManager(), &image);
      if (!irradianceEnvironmentMap)
      {
        csReport (objectRegistry, CS_REPORTER_SEVERITY_WARNING, reporterMessageID, 
          "Could not load blurred_grace.tga texture!");
      }
    }

    void DrawFullscreenQuad(iShader *shader)
    {
      // Switches to using orthographic projection. 
      csReversibleTransform oldView = graphics3D->GetWorldToCamera ();
      CS::Math::Matrix4 oldProj = graphics3D->GetProjectionMatrix ();

      graphics3D->SetWorldToCamera (csReversibleTransform ());
      graphics3D->SetProjectionMatrix (CreateOrthoProj (graphics3D));
      
      quadMesh.shader = shader;  
      
      graphics3D->DrawSimpleMesh (quadMesh, 0);

      // Restores old transforms.
      graphics3D->SetWorldToCamera (oldView);
      graphics3D->SetProjectionMatrix (oldProj);
    }

  public:
    bool showAmbientOcclusion;
    bool showGlobalIllumination;
    bool applyBlur;
  
    csRef<iShader> globalIllumShader;
    csRef<iShader> horizontalBlurShader;    
    csRef<iShader> verticalBlurShader;
    csRef<iShader> lightCompositionShader;
    csRef<iShader> downsampleShader;

  private:
    csSimpleRenderMesh quadMesh;
    csVector3 quadVerts[4];

    bool enabled;
    bool isInitialized;

    GBuffer *gbuffer;
    
    float bufferDownscaleFactor;

    csRef<iTextureHandle> globalIllumBuffer;
    bool isGlobalIllumBufferAttached;
    const char *globalIllumBufferFormat;
    
    csRef<iTextureHandle> intermediateBuffer;
    bool isIntermediateBufferAttached;

    csRef<iTextureHandle> depthNormalBuffer;
    bool isDepthNormalBufferAttached;

    csRef<iTextureHandle> randomNormalsTexture;
    csRef<iTextureHandle> sampleDirectionsTexture;
    csRef<iTextureHandle> irradianceEnvironmentMap;    
    
    csRef<csShaderVariable> globalIllumBufferSV;
    csRef<csShaderVariable> intermediateBufferSV;
    csRef<csShaderVariable> depthNormalBufferSV;

    iGraphics3D *graphics3D;
    csRef<iShaderManager> shaderManager;
    iObjectRegistry *objectRegistry;
    csRef<iShaderVarStringSet> svStringSet;

    const char *reporterMessageID;
  };
}
CS_PLUGIN_NAMESPACE_END(RMDeferred)

#endif //__GLOBALILLUM_H__