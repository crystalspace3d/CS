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

/**\file
 * Tool class for SSDO (Screen Space Directional Occlusion) post effects.
 */

// TODO: move that file eg in include/csplugincommon/rendermanager/
 
/**\addtogroup gfx
 * @{ 
 */

#ifndef __CS_GFX_SSDO_H__
#define __CS_GFX_SSDO_H__

#include "iengine/rendermanager.h"
#include "ivaria/reporter.h"

class GBuffer;
class csConfigAccess;
class csShaderVariable;

struct iLoader;
//struct iPostEffect;
struct iShaderManager;

/**
 * Interface for a render manager to provide access to the parameters of 
 * global illumination techniques.
 */
// TODO: iSSDOPostEffect, iSSDOPostEffectManager Factory?
class CS_CRYSTALSPACE_EXPORT SSDOManager //: public virtual iBase
{
public:
  SSDOManager ();

  bool Initialize (iObjectRegistry *objRegistry);

  void SetEnabled (bool enabled);
  bool GetEnabled ();

  void SetBlurEnabled (bool enabled);
  bool GetBlurEnabled ();

  void SetBufferResolution (const char *resolution);
  const char *GetBufferResolution ();

  void SetNormalsAndDepthBufferResolution (const char *resolution);
  const char *GetNormalsAndDepthBufferResolution ();

  csShaderVariable* GetSSDOVariableAdd (const char *svName);

  csShaderVariable* GetBlurVariableAdd (const char *svName);

  csShaderVariable* GetCompositionVariableAdd (const char *svName);

private:
  static bool ReportError (const char* msg, ...)
  {
    va_list arg;
    va_start (arg, msg);
    csReportV (iSCF::SCF->object_reg, CS_REPORTER_SEVERITY_ERROR,
	       "crystalspace.gfx.ssdo", msg, arg);
    va_end (arg);
    return false;
  }

  static bool ReportWarning (const char* msg, ...)
  {
    va_list arg;
    va_start (arg, msg);
    csReportV (iSCF::SCF->object_reg, CS_REPORTER_SEVERITY_WARNING,
	       "crystalspace.gfx.ssdo", msg, arg);
    va_end (arg);
    return false;
  }

  bool InitRenderTargets ();
  bool AttachBuffer (iTextureHandle *buffer);
  bool AttachSSDOBuffer (bool useGBufferDepth = false);
  void DetachSSDOBuffer ();
  bool AttachIntermediateBuffer (bool useGBufferDepth = false);
  void DetachIntermediateBuffer ();
  void DestroyIntermediateBuffers ();
  bool AttachDepthNormalBuffer ();
  void DetachDepthNormalBuffer ();
  void SetupShaderVars (csConfigAccess &cfg);
  void LoadRandomNormalsTexture
    (iLoader *loader, iGraphics3D *graphics3D, csConfigAccess &cfg);

private:
  iObjectRegistry *objectRegistry;
  csRef<iGraphics3D> graphics3D;
  csRef<iShaderManager> shaderManager;
  csRef<iShaderVarStringSet> svStringSet;
  csRef<iPostEffect> postEffect;

  const char *reporterMessageID;

  bool enabled;
  bool showAmbientOcclusion;
  bool showSSDO;
  bool blurEnabled;

  GBuffer *gbuffer;

  csRef<iShader> SSDOShader;
  csRef<iShader> horizontalBlurShader;  
  csRef<iShader> verticalBlurShader;
  csRef<iShader> lightCompositionShader;
  csRef<iShader> downsampleShader;

  csString bufferResolution;
  csString depthNormalsResolution;
  float depthNormalsBufferScale;
    
  float bufferDownscaleFactor;

  csRef<iTextureHandle> SSDOBuffer;
  bool isSSDOBufferAttached;
  const char *SSDOBufferFormat;
    
  csRef<iTextureHandle> intermediateBuffer;
  bool isIntermediateBufferAttached;

  csRef<iTextureHandle> depthNormalBuffer;
  bool isDepthNormalBufferAttached;

  csRef<iTextureHandle> randomNormalsTexture;
    
  csRef<csShaderVariable> SSDOBufferSV;
  csRef<csShaderVariable> intermediateBufferSV;
  csRef<csShaderVariable> depthNormalBufferSV;

  CS::ShaderVarStringID colorBufferSVName;
  CS::ShaderVarStringID depthBufferSVName;

  iPostEffectLayer* depthNormalsLayer;
  iPostEffectLayer* ssdoLayer;
  iPostEffectLayer* horizontalBlurLayer;
  iPostEffectLayer* verticalBlurLayer;
  iPostEffectLayer* lightCompositionLayer;
};

/** @} */

#endif // __CS_GFX_SSDO_H__
