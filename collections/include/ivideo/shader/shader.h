/*
    Copyright (C) 2002 by Marten Svanfeldt
                          Anders Stenberg

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


#ifndef __CS_IVIDEO_SHADER_H__
#define __CS_IVIDEO_SHADER_H__

/**\file
 * Shader-related interfaces
 */

#include "csutil/scf.h"

#include "iutil/array.h"

#include "csgfx/shadervar.h"
#include "csutil/array.h"
#include "csutil/refarr.h"
#include "csutil/set.h"
#include "csutil/strset.h"
#include "csutil/weakrefarr.h"

struct iDocumentNode;
struct iLight;
struct iObject;
struct iLoaderContext;

namespace CS
{
  namespace Graphics
  {
    struct RenderMesh;
  }
}
class csShaderVariable;

struct iShader;
struct iShaderCompiler;
struct iShaderManager;

//@{
/**
 * A "shader variable stack".
 * Stores a list of shader variables, indexed by it's name.
 */
typedef csArray<csShaderVariable*> csShaderVarStack;
struct iShaderVarStack : public iArrayChangeAll<csShaderVariable*>
{
  SCF_IARRAYCHANGEALL_INTERFACE (iShaderVarStack);
};
//@}

//@{
/**
 * Helper function to retrieve a single value from a shader variable stack.
 */
static inline csShaderVariable* csGetShaderVariableFromStack 
  (const csShaderVarStack& stack, const csStringID &name)
{
  if ((name != csInvalidStringID) &&
      (name < (csStringID)stack.GetSize ()))
  {
    return stack[name];
  }
  return 0;
}
static inline csShaderVariable* csGetShaderVariableFromStack 
  (const iShaderVarStack* stack, const csStringID &name)
{
  if ((name != csInvalidStringID) &&
      (name < (csStringID)stack->GetSize ()))
  {
    return static_cast<const iArrayReadOnly<csShaderVariable*>*> (stack)
      ->Get (name);
  }
  return 0;
}
//@}

/**
 * This is a baseclass for all interfaces which provides shadervariables
 * both dynamically and static
 */
struct iShaderVariableContext : public virtual iBase
{
  SCF_INTERFACE(iShaderVariableContext, 2, 1, 1);

  /**
   * Add a variable to this context
   * \remarks If a variable of the same name exists in the current context,
   *   its contents are replaced with those of \a variable.
   */
  virtual void AddVariable (csShaderVariable *variable) = 0;
  
  /// Get a named variable from this context
  virtual csShaderVariable* GetVariable (csStringID name) const = 0;

  /// Like GetVariable(), but it also adds it if doesn't exist already.
  csShaderVariable* GetVariableAdd (csStringID name)
  {
    csShaderVariable* sv;
    sv = GetVariable (name);
    if (sv == 0)
    {
      csRef<csShaderVariable> nsv (
	csPtr<csShaderVariable> (new csShaderVariable (name)));
      AddVariable (nsv);
      sv = nsv; // OK, sv won't be destructed, SV context takes ownership
    }
    return sv;
  }

  /// Get Array of all ShaderVariables
  virtual const csRefArray<csShaderVariable>& GetShaderVariables () const = 0;

  /**
   * Push the variables of this context onto the variable stacks
   * supplied in the "stacks" argument
   */
  virtual void PushVariables (iShaderVarStack* stacks) const = 0;

  /// Determine whether this SV context contains any variables at all.
  virtual bool IsEmpty () const = 0;

  /**
   * Replace the current variable object of the same name as \a variable with
   * the latter, add \a variable otherwise.
   * \remarks This differs from AddVariable() as this method replaces the
   *   variable *object*, not just the contents.
   */
  virtual void ReplaceVariable (csShaderVariable* variable) = 0;
  
  /// Remove all variables from this context.
  virtual void Clear() = 0;

  /// Remove the given variable from this context.
  virtual bool RemoveVariable (csShaderVariable* variable) = 0;
};

/**
 * Possible settings regarding a techique tag's presence.
 */
enum csShaderTagPresence
{
  /**
   * The tag is neither required nor forbidden. However, it's priority still
   * contributes to technique selection.
   */
  TagNeutral,
  /**
   * Techniques were this tag is present are rejected to be loaded.
   */
  TagForbidden,
  /**
   * Techniques are required to have one such tag. If at least one required 
   * tag exists and no required tag is present in a technique, it doesn't 
   * validate. 
   */
  TagRequired
};

/**
 * A manager for all shaders. Will only be one at a given time
 */
struct iShaderManager : public virtual iShaderVariableContext
{
  SCF_INTERFACE (iShaderManager, 2, 0, 0);
  /**
   * Register a shader to the shadermanager.
   * Compiler should register all shaders
   */
  virtual void RegisterShader (iShader* shader) = 0;
  /// Unregister a shader.
  virtual void UnregisterShader (iShader* shader) = 0;
  /// Remove all shaders.
  virtual void UnregisterShaders () = 0;
  /// Get a shader by name
  virtual iShader* GetShader (const char* name) = 0;
  /// Returns all shaders that have been created.
  virtual const csRefArray<iShader> &GetShaders ()  = 0;

  /// Register a compiler to the manager
  virtual void RegisterCompiler (iShaderCompiler* compiler) = 0;
  /// Get a shadercompiler by name
  virtual iShaderCompiler* GetCompiler(const char* name) = 0;

  /**
   * Register a named shader variable accessor.
   */
  virtual void RegisterShaderVariableAccessor (const char* name,
      iShaderVariableAccessor* accessor) = 0;
  /**
   * Unregister a shader variable accessor.
   */
  virtual void UnregisterShaderVariableAccessor (const char* name,
      iShaderVariableAccessor* accessor) = 0;
  /**
   * Find a shader variable accessor.
   */
  virtual iShaderVariableAccessor* GetShaderVariableAccessor (
      const char* name) = 0;

  /**
   * Remove all shader variable accessors.
   */
  virtual void UnregisterShaderVariableAcessors () = 0;

  /// Get the shadervariablestack used to handle shadervariables on rendering
  virtual iShaderVarStack* GetShaderVariableStack () = 0;

  /**
   * Set a technique tag's options.
   * \param tag The ID of the tag.
   * \param presence Whether the presence of a tag is required, forbidden or
   *  neither of both.
   * \param priority The tag's priority. The sum of all tag priorities is 
   *  decisive when two shader techniques have the same technique priority.
   */
  virtual void SetTagOptions (csStringID tag, csShaderTagPresence presence, 
    int priority = 0) = 0;
  /**
   * Get a technique tag's options.
   * \copydoc SetTagOptions(csStringID,csShaderTagPresence,int)
   */
  virtual void GetTagOptions (csStringID tag, csShaderTagPresence& presence, 
    int& priority) = 0;

  /**
   * Get the list of all tags with a specific \a presence setting.
   */
  virtual const csSet<csStringID>& GetTags (csShaderTagPresence presence,
    int& count) = 0;

  /**
   * Set the list of active lights.
   * Active lights is lights that the shader should use.
   */
  virtual void SetActiveLights (const csArray<iLight*>& lights) = 0;

  /**
   * Get the list of active lights. 
   */
  virtual const csArray<iLight*>& GetActiveLights () const = 0;
};

/**
 * Shader metadata.
 * This struct holds shader metadata such as how many lights per pass
 * the shader supports, description etc that is not directly describing
 * exactly what the shader does, but tells the rest of the engine how it
 * should be used.
 */
struct csShaderMetadata
{
  /// Descriptive string
  char *description;

  /**
   * Number of lights this shader can process in a pass. 
   * 0 means either that the shader does not do any lighting,
   * or that it can provide any number of lights.
   */
  uint numberOfLights;

  /// Constructor to null out parameters
  csShaderMetadata ()
    : description (0), numberOfLights (0)
  {}
};

/**
 * Specific shader. Can/will be either render-specific or general
 * The shader in this form is "compiled" and cannot be modified.
 */
struct iShader : public virtual iShaderVariableContext
{
  SCF_INTERFACE(iShader, 3, 1, 0);

  /// Query the object.
  virtual iObject* QueryObject () = 0;

  /// Get name of the File where it was loaded from.
  virtual const char* GetFileName () = 0;

  /// Set name of the File where it was loaded from.
  virtual void SetFileName (const char* filename) = 0;

  /**
   * Query a "shader ticket".
   * Internally, a shader may choose one of several actual techniques
   * or variants at runtime. However, the variant has to be known in
   * order to determine the number of passes or to do pass preparation.
   * As the decision what variant is to be used is made based on the
   * mesh modes and the shader vars used for rendering, those have
   * to be provided to get the actual variant, which is then identified
   * by the "ticket".
   */
  virtual size_t GetTicket (const CS::Graphics::RenderMeshModes& modes,
    const iShaderVarStack* stacks) = 0;

  /// Get number of passes this shader have
  virtual size_t GetNumberOfPasses (size_t ticket) = 0;

  /// Activate a pass for rendering
  virtual bool ActivatePass (size_t ticket, size_t number) = 0;

  /// Setup a pass
  virtual bool SetupPass (size_t ticket, const CS::Graphics::RenderMesh *mesh,
    CS::Graphics::RenderMeshModes& modes,
    const iShaderVarStack* stacks) = 0;

  /**
   * Tear down current state, and prepare for a new mesh 
   * (for which SetupPass is called)
   */
  virtual bool TeardownPass (size_t ticket) = 0;

  /// Completly deactivate a pass
  virtual bool DeactivatePass (size_t ticket) = 0;

  /// Get shader metadata
  virtual const csShaderMetadata& GetMetadata (size_t ticket) const = 0;
};


/**
 * A list of priorities as returned by iShaderCompiler->GetPriorities()
 */
struct iShaderPriorityList : public virtual iBase
{
  SCF_INTERFACE (iShaderPriorityList, 1,0,0);
  /// Get number of priorities.
  virtual size_t GetCount () const = 0;
  /// Get priority.
  virtual int GetPriority (size_t idx) const = 0;
};

/**
 * Compiler of shaders. Compile from a description of the shader to a 
 * compiled shader. The exact schema for input is specific to each shader-
 * compiler.
 */
struct iShaderCompiler : public virtual iBase
{
  SCF_INTERFACE (iShaderCompiler, 0,0,1);
  /// Get a name identifying this compiler
  virtual const char* GetName() = 0;

  /**
   * Compile a template into a shader. Will return 0 if it fails.
   * If the optional 'forcepriority' parameter is given then only
   * the technique with the given priority will be considered. If
   * this technique fails then the shader cannot be compiled.
   * If no priority is forced then the highest priority technique
   * that works will be selected.
   */
  virtual csPtr<iShader> CompileShader (
	iLoaderContext* ldr_context, iDocumentNode *templ,
	int forcepriority = -1) = 0;

  /// Validate if a template is a valid shader to this compiler
  virtual bool ValidateTemplate (iDocumentNode *templ) = 0;

  /// Check if template is parsable by this compiler
  virtual bool IsTemplateToCompiler (iDocumentNode *templ) = 0;

  /**
   * Return a list of all possible priorities (for techniques)
   * for this shader. This is a full list. It might also contain
   * priorities for techniques that don't work on this hardware.
   */
  virtual csPtr<iShaderPriorityList> GetPriorities (
		  iDocumentNode* templ) = 0;
};

#endif // __CS_IVIDEO_SHADER_H__
