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

#include "csplugincommon/rendermanager/posteffects.h"

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

#include <stdarg.h>

using namespace CS::RenderManager;

namespace
{
  static const char messageID[] = "crystalspace.posteffects.parser";
        
#define CS_TOKEN_ITEM_FILE "libs/csplugincommon/rendermanager/posteffects.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
}

PostEffectLayersParser::PostEffectLayersParser (iObjectRegistry* objReg)
 : objReg (objReg)
{
  InitTokenTable (xmltokens);
  synldr = csQueryRegistry<iSyntaxService> (objReg);
}

PostEffectLayersParser::~PostEffectLayersParser()
{
}

bool PostEffectLayersParser::ParseInputs (iDocumentNode* node, 
                                          iPostEffect* effect,
		                          ParsedLayers& layers,
                                          ShadersLayers& shaders, 
		                          InputsArray& inputs) const
{
  csRef<iDocumentNodeIterator> inputsIt = node->GetNodes ("input");
  while (inputsIt->HasNext())
  {
    csRef<iDocumentNode> child = inputsIt->Next();
    if (child->GetType() != CS_NODE_ELEMENT) continue;
    
    const char* layerInpID = child->GetAttributeValue ("layer");
    if (!layerInpID || !*layerInpID)
    {
      synldr->ReportError (messageID, child, "Expected %s attribute",
			   CS::Quote::Single ("layer"));
      return false;
    }
    iPostEffectLayer* inpLayer = 0;
    if (strcmp (layerInpID, "*screen") == 0)
      inpLayer = effect->GetScreenLayer();
    else
      inpLayer = layers.Get (layerInpID, 0);
    if (inpLayer == 0)
    {
      synldr->ReportError (messageID, child, "Invalid input layer");
      return false;
    }
    
    PostEffectLayerInputMap inp;
    if (child->GetAttribute ("texname").IsValid())
      inp.svTextureName = child->GetAttributeValue ("texname");
    if (child->GetAttribute ("texcoord").IsValid())
      inp.svTexcoordName = child->GetAttributeValue ("texcoord");
    inputs.Push (inp);
  }
  return true;
}

bool PostEffectLayersParser::ParseLayer (iDocumentNode* node, 
                                         iPostEffect* effect,
		                         ParsedLayers& layers,
                                         ShadersLayers& shaders) const
{
  const char* layerID = node->GetAttributeValue ("name");
  PostEffectLayerOptions layerOpts;
  layerOpts.info.mipmap = node->GetAttributeValueAsBool ("mipmap", false);
  layerOpts.info.downsample = node->GetAttributeValueAsInt ("downsample");
  if (node->GetAttribute ("maxmipmap").IsValid())
    layerOpts.info.maxMipmap = node->GetAttributeValueAsInt ("maxmipmap");
    
  csRefArray<csShaderVariable> shaderVars;
  bool hasInputs = false;
  csDirtyAccessArray<PostEffectLayerInputMap> inputs;
  csRef<iDocumentNodeIterator> it = node->GetNodes();
  while (it->HasNext())
  {
    csRef<iDocumentNode> child = it->Next();
    if (child->GetType() != CS_NODE_ELEMENT) continue;
    
    csStringID id = xmltokens.Request (child->GetValue());
    switch (id)
    {
      case XMLTOKEN_INPUTS:
        {
          hasInputs = true;
          if (!ParseInputs (child, effect, layers, shaders, inputs))
            return false;
        }
	break;
      case XMLTOKEN_SHADERVAR:
        {
          csRef<csShaderVariable> sv;
          sv.AttachNew (new csShaderVariable);
          if (!synldr->ParseShaderVar (0, child, *sv))
            return false;
          shaderVars.Push (sv);
        }
        break;
      default:
	synldr->ReportBadToken (child);
	return false;
    }
  }
    
  const char* shader = node->GetAttributeValue ("shader");
  if (!shader || (*shader == 0))
  {
    synldr->ReportError (messageID, node, "Expected %s attribute",
			 CS::Quote::Single ("shader"));
    return false;
  }
  csRef<iShader> shaderObj = shaders.Get (shader, 0);
  if (!shaderObj.IsValid())
  {
    csRef<iLoader> loader (csQueryRegistry<iLoader> (objReg));
    shaderObj = loader->LoadShader (shader);
    if (!shaderObj.IsValid()) return false;
    shaders.Put (shader, shaderObj);
  }
  
  iPostEffectLayer* layer;
  LayerDesc desc;
  if (hasInputs)
    desc = LayerDesc(shaderObj, inputs, layerOpts);
  else
    desc = LayerDesc(shaderObj, layerOpts);

  desc.name = layerID;
  layer = effect->AddLayer (desc);

  if (layerID && *layerID)
    layers.Put (layerID, layer);
    
  for (size_t i = 0; i < shaderVars.GetSize(); i++)
    layer->GetSVContext()->AddVariable (shaderVars[i]);
    
  return true;
}

bool PostEffectLayersParser::AddLayersFromDocument (iDocumentNode* node, 
                                                    iPostEffect* effect) const
{
  ParsedLayers layers;
  ShadersLayers shaders;

  csRef<iDocumentNodeIterator> it = node->GetNodes();
  while (it->HasNext())
  {
    csRef<iDocumentNode> child = it->Next();
    if (child->GetType() != CS_NODE_ELEMENT) continue;
    
    csStringID id = xmltokens.Request (child->GetValue());
    switch (id)
    {
      case XMLTOKEN_LAYER:
        if (!ParseLayer (child, effect, layers, shaders))
          return false;
	break;
      default:
	synldr->ReportBadToken (child);
	return false;
    }
  }
  
  return true;
}

bool PostEffectLayersParser::AddLayersFromFile (const char* filename, 
						iPostEffect* effect) const
{
  csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem> (
    objReg);
  if (!docsys.IsValid())
    docsys.AttachNew (new csTinyDocumentSystem ());
  
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (objReg);
  CS_ASSERT(vfs);
  csRef<iFile> file = vfs->Open (filename, VFS_FILE_READ);
  if (!file)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Error opening %s", CS::Quote::Single (filename));
    return false;
  }
  
  csRef<iDocument> doc = docsys->CreateDocument();
  const char* error = doc->Parse (file);
  if (error != 0)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Error parsing %s: %s", CS::Quote::Single (filename), error);
    return false;
  }
  
  csRef<iDocumentNode> docRoot = doc->GetRoot();
  if (!docRoot) return false;
  csRef<iDocumentNode> postEffectNode = docRoot->GetNode ("posteffect");
  if (!postEffectNode)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "No <posteffect> in %s", CS::Quote::Single (filename));
    return false;
  }

  return AddLayersFromDocument (postEffectNode, effect);
}
