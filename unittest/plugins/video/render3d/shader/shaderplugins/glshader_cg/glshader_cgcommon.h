/*
Copyright (C) 2002 by Anders Stenberg
                      Marten Svanfeldt

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

#ifndef __GLSHADER_CGCOMMON_H__
#define __GLSHADER_CGCOMMON_H__

#include "cg_common.h"

#include "csplugincommon/opengl/shaderplugin.h"
#include "csplugincommon/shader/shaderplugin.h"
#include "csplugincommon/shader/shaderprogram.h"
#include "csgfx/shadervarcontext.h"
#include "ivideo/shader/shader.h"
#include "csutil/strhash.h"
#include "csutil/leakguard.h"

using namespace CS::PluginCommon;

CS_PLUGIN_NAMESPACE_BEGIN(GLShaderCg)
{

  class csGLShader_CG;
  struct ProfileLimits;

  struct iShaderDestinationResolverCG : public virtual iBase
  {
    SCF_INTERFACE(iShaderDestinationResolverCG, 0,0,1);

    virtual const csSet<csString>& GetUnusedParameters () = 0;
  };
  
  struct ShaderParameter
  {
    bool assumeConstant;
    CGparameter param;
    uint baseSlot;
    CGtype paramType;
    csArray<ShaderParameter*> arrayItems;
    
    ShaderParameter() : assumeConstant (false), param (0),
      baseSlot ((uint)~0),
      paramType ((CGtype)0) {}
  };
  
  class ParamValueCache;

class csShaderGLCGCommon : public scfImplementationExt1<csShaderGLCGCommon,
                                                        csShaderProgram,
                                                        iShaderDestinationResolverCG>
{
protected:
  csStringHash xmltokens;
#define CS_TOKEN_ITEM_FILE \
  "plugins/video/render3d/shader/shaderplugins/glshader_cg/glshader_cg.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE

  csRef<csGLShader_CG> shaderPlug;

  CGprogram program;
  CGprofile programProfile;
  csString cg_profile;
  csString entrypoint;
  csRefArray<iDocumentNode> cacheKeepNodes;
  csString objectCode;

  bool validProgram;

  enum ProgramType
  {
    progVP, progFP
  };
  ProgramType programType;
  ArgumentArray compilerArgs;
  csRef<iShaderDestinationResolverCG> cgResolve;
  csSet<csString> unusedParams;
  
  struct Clip
  {
    ShaderProgramPluginGL::ClipPlanes::ClipSpace space;
    
    ProgramParam plane;
    ProgramParam distance;
    int distComp;
    bool distNeg;
  };
  csArray<Clip> clips;
  // Magic SV names
  enum { svClipPackedDist0 = ~23, svClipPackedDist1 = ~42 };
  csRef<csShaderVariable> clipPackedDists[2];
  bool ParseClip (iDocumentNode* node);
  bool ParseVmap (iDocumentNode* node);

  csString debugFN;
  void EnsureDumpFile();

  void FreeShaderParam (ShaderParameter* sparam);
  void FillShaderParam (ShaderParameter* sparam, CGparameter param);
  void GetShaderParamSlot (ShaderParameter* sparam);
  /**
   * Go over variablemaps and fetch Cg parameters for them
   */
  void GetParamsFromVmap();
  //@{
  /**
   * Set properties for a mapped parameter which can only be determined
   * after compilation
   */
  void GetPostCompileParamProps ();
  bool GetPostCompileParamProps (ShaderParameter* sparam);
  //@}

  enum
  {
    loadLoadToGL = 1,
    loadIgnoreErrors = 2,
    loadApplyVmap = 4,
    loadIgnoreConfigProgramOpts = 8,
    loadFlagUnusedV2FForInit = 16
  };
  bool DefaultLoadProgram (iShaderDestinationResolverCG* cgResolve,
    const char* programStr, CGGLenum type, 
    CGprofile maxProfile,
    uint flags = loadLoadToGL | loadApplyVmap | loadFlagUnusedV2FForInit,
    const ProfileLimits* customLimits = 0);
  void DoDebugDump ();
  void WriteAdditionalDumpInfo (const char* description, const char* content);
  const char* GetProgramType()
  {
    switch (programType)
    {
      case progVP: return "vertex";
      case progFP: return "fragment";
    }
    return 0;
  }
  
  void ClipsToVmap ();
  void OutputClipPreamble (csString& str);
  void WriteClipApplications (csString& str);
  
  void CollectUnusedParameters (csSet<csString>& unusedParams);
  template<typename Setter>
  void SetParameterValue (const Setter& setter,
    ShaderParameter* sparam, csShaderVariable* var);
  void SetParameterValueCg (ShaderParameter* sparam, csShaderVariable* var);
  
  void SVtoCgMatrix3x3 (csShaderVariable* var, float* matrix);
  void SVtoCgMatrix4x4 (csShaderVariable* var, float* matrix);

  template<typename Array, typename ParamSetter>
  void ApplyVariableMapArray (const Array& array, const ParamSetter& setter,
    const csShaderVariableStack& stack);
  void ApplyVariableMapArrays (const csShaderVariableStack& stack);
  
  bool WriteToCache (iHierarchicalCache* cache, const ProfileLimits& limits);
public:
  CS_LEAKGUARD_DECLARE (csShaderGLCGCommon);

  csShaderGLCGCommon (csGLShader_CG* shaderPlug, ProgramType type);
  virtual ~csShaderGLCGCommon ();

  void SetValid(bool val) { validProgram = val; }
  virtual bool Precache (const ProfileLimits& limits,
    iHierarchicalCache* cache) = 0;
    
  CGprofile CustomProfile ()
  { return cg_profile.IsEmpty() ? CG_PROFILE_UNKNOWN : cgGetProfile (cg_profile); }

  ////////////////////////////////////////////////////////////////////
  //                      iShaderProgram
  ////////////////////////////////////////////////////////////////////

  /// Sets this program to be the one used when rendering
  virtual void Activate ();

  /// Deactivate program so that it's not used in next rendering
  virtual void Deactivate();

  /// Setup states needed for proper operation of the shader
  virtual void SetupState (const CS::Graphics::RenderMesh* mesh,
    CS::Graphics::RenderMeshModes& modes,
    const csShaderVariableStack& stack);

  /// Reset states to original
  virtual void ResetState ();

  /// Check if valid
  virtual bool IsValid () { return validProgram;} 

  /// Loads from a document-node
  virtual bool Load (iShaderDestinationResolver*, iDocumentNode* node);

  /// Loads from raw text
  virtual bool Load (iShaderDestinationResolver*, const char*, 
    csArray<csShaderVarMapping>&)
  { return false; }

  const csSet<csString>& GetUnusedParameters ()
  { return unusedParams; }
  
  virtual iShaderProgram::CacheLoadResult LoadFromCache (
    iHierarchicalCache* cache, csRef<iString>* failReason = 0);
};

}
CS_PLUGIN_NAMESPACE_END(GLShaderCg)

#endif //__GLSHADER_CGCOMMON_H__
