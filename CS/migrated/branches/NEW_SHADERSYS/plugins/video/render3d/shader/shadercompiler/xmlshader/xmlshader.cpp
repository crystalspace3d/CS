/*
  Copyright (C) 2003 by M�rten Svanfeldt

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

#include "cssysdef.h"
#include "iutil/comp.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"
#include "imap/services.h"
#include "ivideo/rendermesh.h"
#include "csutil/util.h"
#include "csutil/scfstr.h"
#include "csutil/scfstrset.h"
#include "csutil/xmltiny.h"
#include "xmlshader.h"

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY (csXMLShaderCompiler)

SCF_IMPLEMENT_IBASE(csXMLShaderCompiler)
  SCF_IMPLEMENTS_INTERFACE(iComponent)
  SCF_IMPLEMENTS_INTERFACE(iShaderCompiler)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE(csXMLShader)
  SCF_IMPLEMENTS_INTERFACE(iShader)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE(csXMLShader::shaderPass)
  SCF_IMPLEMENTS_INTERFACE(iShaderVariableContext)
SCF_IMPLEMENT_IBASE_END

csXMLShaderCompiler::csXMLShaderCompiler(iBase* parent)
{
  SCF_CONSTRUCT_IBASE(parent);
  init_token_table (xmltokens);
}

csXMLShaderCompiler::~csXMLShaderCompiler()
{

}

bool csXMLShaderCompiler::Initialize (iObjectRegistry* object_reg)
{
  objectreg = object_reg;

  strings = CS_QUERY_REGISTRY_TAG_INTERFACE (
    object_reg, "crystalspace.renderer.stringset", iStringSet);
  if (!strings)
  {
    strings = csPtr<iStringSet> (new csScfStringSet ());
    object_reg->Register (strings, "crystalspace.renderer.stringset");
  }

  g3d = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  synldr = CS_QUERY_REGISTRY (objectreg, iSyntaxService);

  return true;
}

int csXMLShaderCompiler::CompareTechniqueKeeper(void const* item1, void const* item2)
{
  techniqueKeeper *t1, *t2;
  t1 = (techniqueKeeper*) item1;
  t2 = (techniqueKeeper*) item2;
  if (t1->priority < t2->priority)
    return 1;
  else if (t1->priority > t2->priority)
    return -1;
  return 0;
}

csPtr<iShader> csXMLShaderCompiler::CompileShader (iDocumentNode *templ)
{
  if (!ValidateTemplate (templ))
    return csPtr<iShader> (0);

  csRef<iDocumentNodeIterator> it = templ->GetNodes();

  csArray<techniqueKeeper> techniquesTmp;

  //read in the techniques
  while(it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () == CS_NODE_ELEMENT &&
      xmltokens.Request (child->GetValue ()) == XMLTOKEN_TECHNIQUE)
    {
      //save it
      unsigned int p = child->GetAttributeValueAsInt ("priority");
      techniquesTmp.Push (techniqueKeeper(child, p));
    }
  }

  techniquesTmp.Sort (&CompareTechniqueKeeper);

  //now try to load them one in a time, until we are successful
  csXMLShader* shader;
  csArray<techniqueKeeper>::Iterator techIt = techniquesTmp.GetIterator ();
  while (techIt.HasNext())
  {
    techniqueKeeper tk = techIt.Next();
    shader = CompileTechnique (tk.node, templ);
    if (shader != 0) break;
  }

  if (shader != 0)
    shader->name = csStrNew ((const char*)templ->GetAttributeValue("name"));

  return shader;
}

csXMLShader* csXMLShaderCompiler::CompileTechnique (iDocumentNode *node,
						    iDocumentNode *parentSV)
{
  //check nodetype
  if (node->GetType()!=CS_NODE_ELEMENT || 
    xmltokens.Request (node->GetValue()) != XMLTOKEN_TECHNIQUE)
    return 0;

  csXMLShader* newShader = new csXMLShader (g3d);

  //count passes
  newShader->passesCount = 0;
  csRef<iDocumentNodeIterator> it = node->GetNodes(xmltokens.Request (XMLTOKEN_PASS));
  while(it->HasNext ())
  {
    it->Next ();
    newShader->passesCount++;
  }

  //load shadervariable definitions
  if (parentSV)
  {
    csRef<iDocumentNode> varNode = parentSV->GetNode(
      xmltokens.Request (XMLTOKEN_SHADERVAR));
    if (varNode)
      LoadSVBlock (varNode, &newShader->staticVariables, &newShader->dynamicVariables);
  }

  csRef<iDocumentNode> varNode = node->GetNode(
    xmltokens.Request (XMLTOKEN_SHADERVAR));

  if (varNode)
    LoadSVBlock (varNode, &newShader->staticVariables, &newShader->dynamicVariables);

  //alloc passes
  newShader->passes = 
    new csXMLShader::shaderPass[newShader->passesCount];
  uint i;
  for (i = 0; i < newShader->passesCount; i++)
  {
    csXMLShader::shaderPass& pass = newShader->passes[i];
    pass.alphaMode.autoAlphaMode = true;
    pass.alphaMode.autoModeTexture = 
      strings->Request (CS_MATERIAL_TEXTURE_DIFFUSE);
  }


  //first thing we load is the programs for each pass.. if one of them fail,
  //fail the whole technique
  int currentPassNr = 0;
  it = node->GetNodes(xmltokens.Request (XMLTOKEN_PASS));
  while (it->HasNext ())
  {
    csRef<iDocumentNode> passNode = it->Next ();
    newShader->passes[currentPassNr].owner = newShader;
    if (!LoadPass(passNode, &newShader->passes[currentPassNr++]))
    {
      newShader->DecRef ();
      return 0;
    }
  }


  return newShader;
}

bool csXMLShaderCompiler::LoadPass (iDocumentNode *node, 
				    csXMLShader::shaderPass *pass)
{
  //Load shadervar block
  csRef<iDocumentNode> varNode = node->GetNode(
    xmltokens.Request (XMLTOKEN_SHADERVAR));
 
  if (varNode)
    LoadSVBlock (varNode, &pass->staticVariables, &pass->dynamicVariables);

  //load vp
  csRef<iDocumentNode> programNode;
  csRef<iShaderProgram> program;
  programNode = node->GetNode(xmltokens.Request (XMLTOKEN_VP));

  if (programNode)
  {
    program = LoadProgram (programNode, pass);
    if (program)
      pass->vp = program;
    else
    {
      //report error!!
      return false;
    }
  }

  programNode = node->GetNode(xmltokens.Request (XMLTOKEN_FP));

  if (programNode)
  {
    program = LoadProgram (programNode, pass);
    if (program)
      pass->fp = program;
    else
    {
      //report error!!
      return false;
    }
  }

  {
    csRef<iDocumentNode> nodeMixMode = node->GetNode ("mixmode");
    if (nodeMixMode != 0)
    {
      uint mm;
      if (synldr->ParseMixmode (nodeMixMode, mm))
	pass->mixMode = mm;
    }

    csRef<iDocumentNode> nodeAlphaMode = node->GetNode ("alphamode");
    if (nodeAlphaMode != 0)
    {
      csAlphaMode am;
      if (synldr->ParseAlphaMode (nodeAlphaMode, strings, am))
      {
	pass->alphaMode = am;
      }
    }
  }
  if (pass->alphaMode.autoAlphaMode)
  {
    if (pass->alphaMode.autoModeTexture != csInvalidStringID)
    {
      csShaderVariable *var;
      var = pass->GetVariableRecursive(pass->alphaMode.autoModeTexture);

      if(!var)
	var = pass->owner->GetVariableRecursive(
	  pass->alphaMode.autoModeTexture);
      if (var)
	pass->autoAlphaTexRef = var;
      else
      {
	pass->dynamicVariables.InsertSorted (csShaderVariableProxy (
	  pass->alphaMode.autoModeTexture, 0, &pass->autoAlphaTexRef));
      }
    }
  }

  //if we got this far, load buffermappings
  csRef<iDocumentNodeIterator> it;
  it = node->GetNodes(xmltokens.Request (XMLTOKEN_BUFFER));
  pass->bufferCount = 0;
  while(it->HasNext ())
  {
    csRef<iDocumentNode> mapping = it->Next ();
    if (mapping->GetType () != CS_NODE_ELEMENT) continue;
    
    const char* dest = mapping->GetAttributeValue ("destination");
    csVertexAttrib attrib = CS_VATTRIB_0;
    bool found = false;
    int i;
    for(i=0;i<16;i++)
    {
      char str[13];
      sprintf (str, "attribute %d", i);
      if (strcasecmp(str, dest)==0)
      {
        found = true;
        attrib = (csVertexAttrib)(CS_VATTRIB_0 + i);
        break;
      }
    }
    if (!found)
    {
      if (strcasecmp (dest, "position") == 0)
      {
        attrib = CS_VATTRIB_POSITION;
        found = true;
      }
      else if (strcasecmp (dest, "normal") == 0)
      {
        attrib = CS_VATTRIB_NORMAL;
        found = true;
      }
      else if (strcasecmp (dest, "color") == 0)
      {
        attrib = CS_VATTRIB_COLOR;
        found = true;
      }
      else if (strcasecmp (dest, "primary color") == 0)
      {
        attrib = CS_VATTRIB_PRIMARY_COLOR;
        found = true;
      }
      else if (strcasecmp (dest, "secondary color") == 0)
      {
        attrib = CS_VATTRIB_SECONDARY_COLOR;
        found = true;
      }
      else if (strcasecmp (dest, "texture coordinate") == 0)
      {
        attrib = CS_VATTRIB_TEXCOORD;
        found = true;
      }
      else
      {
        static const char mapName[] = "texture coordinate %d";
        char buf[sizeof (mapName)];

        for (int u = 0; u < 8; u++)
        {
          sprintf (buf, mapName, u);
          if (strcasecmp (dest, buf) == 0)
          {
            attrib = (csVertexAttrib)((int)CS_VATTRIB_TEXCOORD0 + u);
            found = true;
            break;
          }
        }
      }
    }
    if (found)
    {
      int a = CS_VATTRIB_IS_SPECIFIC (attrib) ? 
	attrib - CS_VATTRIB_SPECIFIC_FIRST : attrib;
      csStringID varID = strings->Request (mapping->GetAttributeValue ("name"));
      pass->bufferID[a] = varID; //MUST HAVE STRINGS
      pass->bufferGeneric[a] = CS_VATTRIB_IS_GENERIC (attrib);

      csShaderVariable *varRef=0;
      //csRef<csShaderVariable>& varRef = pass->bufferRef[a];
      varRef = pass->GetVariableRecursive(pass->bufferID[a]);

      if(!varRef)
        varRef = pass->owner->GetVariableRecursive(pass->bufferID[a]);

      //pass->bufferRef[a] = varRef; //FETCH REF IF IT EXSISTS AT THIS POINT
      if (!varRef)
      {
	pass->dynamicVariables.InsertSorted (csShaderVariableProxy (varID, 
	  0, &pass->bufferRef[pass->bufferCount]));
      }
      else
	pass->bufferRef[a] = varRef;
      pass->vertexattributes[pass->bufferCount] = attrib;
      pass->bufferCount++;
      //pass->bufferCount = MAX (pass->bufferCount, a + 1);
    }
  }

  //get texturemappings
  pass->textureCount = 0;
  it = node->GetNodes (xmltokens.Request (XMLTOKEN_TEXTURE));
  while(it->HasNext ())
  {
    csRef<iDocumentNode> mapping = it->Next ();
    if (mapping->GetType() != CS_NODE_ELEMENT) continue;
    if (mapping->GetAttribute ("name") && mapping->GetAttribute ("destination"))
    {
      const char* dest = mapping->GetAttributeValue ("destination");
      char unitName[8];
      int i;
      int texUnit = -1;
      for (i = 0; i < csXMLShader::shaderPass::TEXTUREMAX; i++)
      {
	sprintf (unitName, "unit %d", i);
	if (strcasecmp(unitName, dest) == 0)
	{
	  texUnit = i;
	  break;
	}
      }
      if (texUnit < 0) continue;
      csStringID varID = strings->Request (mapping->GetAttributeValue ("name"));
      pass->textureID[texUnit] = varID;
      //pass->textureUnits[pass->textureCount] = texUnit;

      csShaderVariable *varRef=0;
      //csRef<csShaderVariable>& varRef = pass->bufferRef[a];
      varRef = pass->GetVariableRecursive (varID);

      if(!varRef)
        varRef = pass->owner->GetVariableRecursive(varID);

      //pass->bufferRef[a] = varRef; //FETCH REF IF IT EXSISTS AT THIS POINT
      if (!varRef)
      {
	pass->dynamicVariables.InsertSorted (csShaderVariableProxy (varID, 
	  0, &pass->textureRef[texUnit]));
      }
      else
	pass->textureRef[texUnit] = varRef;

      pass->textureCount = MAX(pass->textureCount, texUnit + 1);
      //pass->textureCount++;
    }
  }

  return true;
}

bool csXMLShaderCompiler::LoadSVBlock (iDocumentNode *node,
                                       csShaderVariableContextHelper *staticVariables,
                                       csShaderVariableProxyList *dynamicVariables)
{
  (void)dynamicVariables;
  csRef<csShaderVariable> svVar;
  
  csRef<iDocumentNodeIterator> it = node->GetNodes ("define");
  while (it->HasNext ())
  {
    csRef<iDocumentNode> var = it->Next ();
    svVar.AttachNew (
      new csShaderVariable(strings->Request(var->GetAttributeValue ("name"))));

    csStringID idtype = xmltokens.Request (var->GetAttributeValue ("type"));
    switch (idtype)
    {
      case XMLTOKEN_INT:
	svVar->SetValue (var->GetAttributeValueAsInt("default"));
	break;
      case XMLTOKEN_FLOAT:
	svVar->SetValue (var->GetAttributeValueAsFloat("default"));
	break;
      case XMLTOKEN_STRING:
	svVar->SetValue (new scfString( var->GetAttributeValue("default")) );
	break;
      case XMLTOKEN_VECTOR3:
	const char* def = var->GetAttributeValue("default");
	csVector3 v;
	sscanf(def, "%f,%f,%f", &v.x, &v.y, &v.z);
	svVar->SetValue (v);
	break;
    }
    staticVariables->AddVariable(svVar);
  }

  return true;
}

csPtr<iShaderProgram> csXMLShaderCompiler::LoadProgram (
  iDocumentNode *node, csXMLShader::shaderPass *pass)
{
  if (node->GetAttributeValue("plugin") == 0)
    // @@@ Report
    return 0;

  csRef<iShaderProgram> program;

  const char *pluginprefix = "crystalspace.graphics3d.shader.";
  char *plugin = new char[strlen(pluginprefix) + 255 + 1];
  strcpy (plugin, pluginprefix);

  strncat (plugin, node->GetAttributeValue("plugin"), 255);
  
  //load the plugin
  csRef<iPluginManager> plugin_mgr = CS_QUERY_REGISTRY  (objectreg,
    iPluginManager);

  csRef<iShaderProgramPlugin> plg;
  plg = CS_QUERY_PLUGIN_CLASS(plugin_mgr, plugin, iShaderProgramPlugin);
  if(!plg)
  {
    plg = CS_LOAD_PLUGIN(plugin_mgr, plugin, iShaderProgramPlugin);
    if (!plg)
      return 0;
  }

  const char* programType = node->GetAttributeValue("type");
  if (programType == 0)
    programType = node->GetValue ();
  program = plg->CreateProgram (programType);
  if (program == 0)
    return 0;
  csRef<iDocumentNode> programNode;
  if (node->GetAttributeValue ("file") != 0)
  {
    csRef<iVFS> VFS = CS_QUERY_REGISTRY(objectreg, iVFS);
    csRef<iFile> programFile = VFS->Open (node->GetAttributeValue ("file"),
      VFS_FILE_READ);
    csRef<iDocumentSystem> docsys (
      CS_QUERY_REGISTRY(objectreg, iDocumentSystem));
    if (docsys == 0)
      docsys.AttachNew (new csTinyDocumentSystem ());

    csRef<iDocument> programDoc = docsys->CreateDocument ();
    programDoc->Parse (programFile);
    /*csRef<iDocumentNodeIterator> it = programDoc->GetRoot ()->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      programNode = child;
      break;
    }*/
    programNode = programDoc->GetRoot ();
  }
  else
    programNode = node;
  program->Load (programNode);

  csArray<iShaderVariableContext*> staticDomains;
  staticDomains.Push (pass);
  staticDomains.Push (pass->owner);
  if (!program->Compile (staticDomains))
    return 0;

  return csPtr<iShaderProgram> (program);
}

bool csXMLShaderCompiler::ValidateTemplate(iDocumentNode *templ)
{
  if (!IsTemplateToCompiler(templ))
    return false;

  /*@@TODO: Validation without accual compile. should check correct xml
  syntax, and that we have at least one techqniue which can load. Also check
  that we have valid texturemapping and buffermapping*/

  return true;
}

bool csXMLShaderCompiler::IsTemplateToCompiler(iDocumentNode *templ)
{
  //Check that we got an element
  if (templ->GetType() != CS_NODE_ELEMENT) return false;

  //With name shader  (<shader>....</shader>
  if (xmltokens.Request (templ->GetValue())!=XMLTOKEN_SHADER) return false;

  //Check the type-string in <shader>
  if ((templ->GetAttributeValue ("type") == 0) || (xmltokens.Request (
    templ->GetAttributeValue ("type")) != XMLTOKEN_XMLSHADER))
    return false;

  //Check that we have children, no children == not a template to this one at least
  if (!templ->GetNodes()->HasNext()) return false;

  //Ok, passed check. We will try to validate it
  return true;
}

iRenderBuffer* csXMLShader::last_buffers[shaderPass::STREAMMAX*2];
iRenderBuffer* csXMLShader::clear_buffers[shaderPass::STREAMMAX*2];
//csVertexAttrib csXMLShader::vertexattributes[shaderPass::STREAMMAX*2];
int csXMLShader::lastBufferCount;

iTextureHandle* csXMLShader::last_textures[shaderPass::TEXTUREMAX];
iTextureHandle* csXMLShader::clear_textures[shaderPass::TEXTUREMAX];
int csXMLShader::textureUnits[shaderPass::TEXTUREMAX];
int csXMLShader::lastTexturesCount;

csXMLShader::csXMLShader (iGraphics3D* g3d) : passes(NULL), passesCount(0), 
  currentPass(~0)
{
  SCF_CONSTRUCT_IBASE(0);

  csXMLShader::g3d = g3d;
  int i;
  for (i = 0; i < shaderPass::TEXTUREMAX; i++)
    textureUnits[i] = i;
}

csXMLShader::~csXMLShader ()
{
  delete [] passes;
  SCF_DESTRUCT_IBASE();
}

bool csXMLShader::ActivatePass (unsigned int number)
{
  if(number>=passesCount)
    return false;

  currentPass = number;

  if(passes[currentPass].vp) passes[currentPass].vp->Activate ();
  if(passes[currentPass].fp) passes[currentPass].fp->Activate ();
                           
  g3d->GetWriteMask (orig_wmRed, orig_wmGreen, orig_wmBlue, orig_wmAlpha);

  return true;
}

bool csXMLShader::DeactivatePass ()
{
  if(currentPass>=passesCount)
    return false;
  shaderPass *thispass = &passes[currentPass];
  currentPass = ~0;

  if(thispass->vp) thispass->vp->Deactivate ();
  if(thispass->fp) thispass->fp->Deactivate ();

  g3d->SetBufferState(thispass->vertexattributes, clear_buffers, 
    lastBufferCount);
  lastBufferCount=0;

  g3d->SetTextureState(textureUnits, clear_textures, 
    lastTexturesCount);
  lastTexturesCount=0;
  
  g3d->SetWriteMask (orig_wmRed, orig_wmGreen, orig_wmBlue, orig_wmAlpha);

  return true;
}

bool csXMLShader::SetupPass (csRenderMesh *mesh,
                             csArray<iShaderVariableContext*> &dynamicDomains)
{
  if(currentPass>=passesCount)
    return false;

  if (mesh->dynDomain) dynamicDomains.Insert(0, mesh->dynDomain);

  shaderPass *thispass = &passes[currentPass];

  //Fill all dynamic vars
  thispass->dynamicVariables.PrepareFill ();
  int varsFilled=0;
  int i;
  for(i=0;i<dynamicDomains.Length();i++)
  {
    varsFilled += dynamicDomains[i]->FillVariableList(&thispass->dynamicVariables);
    if (varsFilled>=thispass->dynamicVariables.Length ())
      break;
  }
  
  /*for(i = 0; i < thispass->dynamicVariables.Length (); i++)
  {
    *((csRef<csShaderVariable>*)thispass->dynamicVariables.Get(i).userData) = 
      thispass->dynamicVariables.Get(i).shaderVariable;
    thispass->dynamicVariables.Get(i).shaderVariable = 0;
  }*/

  //now map our buffers. all refs should be set
  for (i = 0; i < thispass->bufferCount; i++)
  {
    if (thispass->bufferRef[i])
      thispass->bufferRef[i]->GetValue(last_buffers[i]);
    else
      last_buffers[i] = 0;
  }
  g3d->SetBufferState (thispass->vertexattributes, last_buffers, 
    thispass->bufferCount);
  lastBufferCount = thispass->bufferCount;

  //and the textures
  for(i = 0; i < thispass->textureCount; i++)
  {
    if (thispass->textureRef[i])
      thispass->textureRef[i]->GetValue(last_textures[i]);
    else
      last_textures[i] = 0;
  }
  g3d->SetTextureState (textureUnits, last_textures, 
    thispass->textureCount);
  lastTexturesCount = thispass->textureCount;

  g3d->SetWriteMask (thispass->wmRed, thispass->wmGreen, thispass->wmBlue,
    thispass->wmAlpha);

  // @@@ Is it okay to modify the render mesh here?
  mesh->mixmode = thispass->mixMode;
  if (thispass->alphaMode.autoAlphaMode)
  {
    iTextureHandle* tex = 0;
    if (thispass->autoAlphaTexRef != 0)
      thispass->autoAlphaTexRef->GetValue (tex);
    if (tex != 0)
      mesh->alphaType = tex->GetAlphaType ();
    else
      mesh->alphaType = csAlphaMode::alphaNone;
  }
  else
    mesh->alphaType = thispass->alphaMode.alphaType;


  if(thispass->vp) thispass->vp->SetupState (mesh, dynamicDomains);
  if(thispass->fp) thispass->fp->SetupState (mesh, dynamicDomains);

  return true;
}

bool csXMLShader::TeardownPass ()
{
  if(passes[currentPass].vp) passes[currentPass].vp->ResetState ();
  if(passes[currentPass].fp) passes[currentPass].fp->ResetState ();
  return true;
}