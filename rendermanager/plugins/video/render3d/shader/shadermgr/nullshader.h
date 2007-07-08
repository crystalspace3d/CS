/*
  Copyright (C) 2005-2006 by Jorrit Tyberghein
	    (C) 2005 by Frank Richter

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


#ifndef __CS_NULLSHADER_H__
#define __CS_NULLSHADER_H__

#include "csutil/csobject.h"
#include "ivideo/shader/shader.h"
#include "iutil/selfdestruct.h"

CS_PLUGIN_NAMESPACE_BEGIN(ShaderManager)
{

class csShaderManager;

class csNullShader : public scfImplementationExt2<csNullShader, 
						  csObject,
						  iShader,
						  iSelfDestruct>
{
private:
  csShaderMetadata allShaderMeta;
  csRefArray<csShaderVariable> dummySVs;
  csShaderManager* mgr;

public:
  CS_LEAKGUARD_DECLARE (csNullShader);

  csNullShader (csShaderManager* mgr) : scfImplementationType (this),
  	mgr (mgr) { }
  virtual ~csNullShader () { }

  virtual iObject* QueryObject () 
  { return static_cast<iObject*> (this); }

  const char* GetFileName () { return 0; }
  void SetFileName (const char* /*filename*/) {  }

  virtual size_t GetTicket (const csRenderMeshModes&, 
    const iShaderVarStack*) { return 0; }
  virtual size_t GetTicket (const csRenderMeshModes& modes,
    csShaderVariable** stacks)
  {
    return 0;
  }

  virtual size_t GetNumberOfPasses (size_t) { return 0; }
  virtual bool ActivatePass (size_t, size_t) { return false; }
  virtual bool SetupPass (size_t, const csRenderMesh*,
    csRenderMeshModes&, const iShaderVarStack*)
  { return false; }
  virtual bool TeardownPass (size_t)
  { return false; }
  virtual bool DeactivatePass (size_t) { return false; }
  virtual const csShaderMetadata& GetMetadata (size_t) const
  { return allShaderMeta; }

  /**\name iShaderVariableContext implementation
   * @{ */
  void AddVariable (csShaderVariable *) { }
  csShaderVariable* GetVariable (csStringID) const { return 0; }
  const csRefArray<csShaderVariable>& GetShaderVariables () const
  { return dummySVs; }
  void PushVariables (iShaderVarStack*) const { }
  void PushVariables (csShaderVariable** stacks) const {  }

  bool IsEmpty() const { return true; }
  void ReplaceVariable (csShaderVariable*) {}
  void Clear () { }
  bool RemoveVariable (csShaderVariable*) { return false; }
  /** @} */

  /**\name iSelfDestruct implementation
   * @{ */
  virtual void SelfDestruct ();
  /** @} */
};

}
CS_PLUGIN_NAMESPACE_END(ShaderManager)

#endif // __CS_NULLSHADER_H__
