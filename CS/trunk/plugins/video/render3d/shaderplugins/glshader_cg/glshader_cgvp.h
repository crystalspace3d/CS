/*
Copyright (C) 2002 by Anders Stenberg
                      M�rten Svanfeldt

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

#ifndef __GLSHADER_CGVP_H__
#define __GLSHADER_CGVP_H__

#include "ivideo/shader/shader.h"
#include "csutil/strhash.h"

#include "Cg/cg.h"
#include "Cg/cgGL.h"

class csShaderGLCGVP : public iShaderProgram
{
private:
  enum
  {
    XMLTOKEN_CGVP = 1,
    XMLTOKEN_DECLARE,
    XMLTOKEN_VARIABLEMAP,
    XMLTOKEN_PROGRAM
  };

  struct variablemapentry
  {
    variablemapentry() { name = 0; }
    ~variablemapentry() { if(name) delete name; if(cgvarname) delete cgvarname; }
    char* name;
    char* cgvarname;
    int namehash;
    CGparameter parameter;
  };

  csArray<csSymbolTable> symtabs;
  csSymbolTable *symtab;

  struct matrixtrackerentry
  {
    CGGLenum matrix;
    CGGLenum modifier;
    CGparameter parameter;
  };

  csRef<iObjectRegistry> object_reg;
  CGcontext context;

  CGprogram program;

  csHashMap variables;
  csBasicVector matrixtrackers;
  csStringHash xmltokens;

  void BuildTokenHash();

  char* programstring;

  bool validProgram;

public:
  SCF_DECLARE_IBASE;

  csShaderGLCGVP(iObjectRegistry* objreg, CGcontext context)
  {
    validProgram = true;
    SCF_CONSTRUCT_IBASE (0);
    this->object_reg = objreg;
    this->context = context;
    programstring = 0;
    program = 0;
  }
  virtual ~csShaderGLCGVP ()
  {
    if (programstring) 
      delete programstring;
    if (program)
      cgDestroyProgram (program);
  }

  bool LoadProgramStringToGL( const char* programstring );

  void SetValid(bool val) { validProgram = val; }

  ////////////////////////////////////////////////////////////////////
  //                      iShaderProgram
  ////////////////////////////////////////////////////////////////////

  virtual csPtr<iString> GetProgramID();

  /// Sets this program to be the one used when rendering
  virtual void Activate(iShaderPass* current, csRenderMesh* mesh);

  /// Deactivate program so that it's not used in next rendering
  virtual void Deactivate(iShaderPass* current);

  virtual void SetupState (iShaderPass *current, csRenderMesh *mesh) {}

  virtual void ResetState () {}

  /* Propertybag - get property, return false if no such property found
  * Which properties there is is implementation specific
  */
  virtual bool GetProperty(const char* name, iString* string) {return false;};
  virtual bool GetProperty(const char* name, int* string) {return false;};
  virtual bool GetProperty(const char* name, csVector3* string) {return false;};
  //  virtual bool GetProperty(const char* name, csVector4* string) {};

  /* Propertybag - set property.
  * Which properties there is is implementation specific
  */
  virtual bool SetProperty(const char* name, iString* string) {return false;};
  virtual bool SetProperty(const char* name, int* string) {return false;};
  virtual bool SetProperty(const char* name, csVector3* string) {return false;};
  //  virtual bool SetProperty(const char* name, csVector4* string) {return false;};

/*  virtual void AddChild(iShaderBranch *c)
    { symtab->AddChild(c->GetSymbolTable()); }
  virtual void AddVariable(iShaderVariable* variable) 
    { return false; } // Don't allow externals to add variables
  virtual iShaderVariable* GetVariable(csStringID name)
    { return (iShaderVariable *) symtab->GetSymbol(name); }
  virtual csSymbolTable* GetSymbolTable()
    { return symtab; }
  virtual void SelectSymbolTable(int index) {
    if (symtabs.Length() < index) symtabs.SetLength(index + 1);
    symtab = & symtabs[index];
  }*/
  /// Add a variable to this context
  virtual bool AddVariable(iShaderVariable* variable) 
  { /*do not allow externals to add variables*/ return false; };
  /// Get variable
  virtual iShaderVariable* GetVariable(int namehash);
  /// Get all variable stringnames added to this context (used when creatingthem)
  virtual csBasicVector GetAllVariableNames(); 
  virtual csSymbolTable* GetSymbolTable();


  /// Check if valid
  virtual bool IsValid() { return validProgram;} 

    /// Loads shaderprogram from buffer
  virtual bool Load(iDataBuffer* program);

  /// Loads from a document-node
  virtual bool Load(iDocumentNode* node);

  /// Prepares the shaderprogram for usage. Must be called before the shader is assigned to a material
  virtual bool Prepare();
};


#endif //__GLSHADER_CGVP_H__

