/*
    Copyright (C) 2003 by Keith Fulton

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
#include "cssys/sysfunc.h"
#include "csgeom/math3d.h"
#include "csgeom/matrix3.h"
#include "csgeom/quaterni.h"
#include "csgeom/transfrm.h"
#include "csutil/scanstr.h"
#include "csutil/util.h"
#include "imesh/object.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "iutil/plugin.h"
#include "imesh/spritecal3d.h"
#include "ivideo/graph3d.h"
#include "qint.h"
#include "iutil/vfs.h"
#include "csutil/csstring.h"
#include "iutil/object.h"
#include "iengine/material.h"
#include "iengine/motion.h"
#include "ivaria/reporter.h"
#include "iutil/objreg.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "iutil/document.h"
#include "imap/ldrctxt.h"
#include "sprcal3dldr.h"

CS_IMPLEMENT_PLUGIN

enum
{
  XMLTOKEN_PATH,
  XMLTOKEN_SCALE,
  XMLTOKEN_SKELETON,
  XMLTOKEN_ANIMATION,
  XMLTOKEN_MESH,
  XMLTOKEN_MATERIAL
};

SCF_IMPLEMENT_IBASE (csSpriteCal3DFactoryLoader)
  SCF_IMPLEMENTS_INTERFACE (iLoaderPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSpriteCal3DFactoryLoader::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csSpriteCal3DFactorySaver)
  SCF_IMPLEMENTS_INTERFACE (iSaverPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSpriteCal3DFactorySaver::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csSpriteCal3DLoader)
  SCF_IMPLEMENTS_INTERFACE (iLoaderPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSpriteCal3DLoader::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csSpriteCal3DSaver)
  SCF_IMPLEMENTS_INTERFACE (iSaverPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSpriteCal3DSaver::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_FACTORY (csSpriteCal3DFactoryLoader)
SCF_IMPLEMENT_FACTORY (csSpriteCal3DFactorySaver)
SCF_IMPLEMENT_FACTORY (csSpriteCal3DLoader)
SCF_IMPLEMENT_FACTORY (csSpriteCal3DSaver)


csSpriteCal3DFactoryLoader::csSpriteCal3DFactoryLoader (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csSpriteCal3DFactoryLoader::~csSpriteCal3DFactoryLoader ()
{
}

bool csSpriteCal3DFactoryLoader::Initialize (iObjectRegistry* object_reg)
{
  csSpriteCal3DFactoryLoader::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  synldr = CS_QUERY_REGISTRY (object_reg, iSyntaxService);

  xmltokens.Register ("path",      XMLTOKEN_PATH);
  xmltokens.Register ("scale",     XMLTOKEN_SCALE);
  xmltokens.Register ("skeleton",  XMLTOKEN_SKELETON);
  xmltokens.Register ("animation", XMLTOKEN_ANIMATION);
  xmltokens.Register ("mesh",      XMLTOKEN_MESH);
  xmltokens.Register ("material",  XMLTOKEN_MATERIAL);
  return true;
}


csPtr<iBase> csSpriteCal3DFactoryLoader::Parse (iDocumentNode* node,
				          iLoaderContext* ldr_context, 
				          iBase* context)
{
  csRef<iPluginManager> plugin_mgr (CS_QUERY_REGISTRY (object_reg,
  	iPluginManager));
  csRef<iMeshObjectType> type (CS_QUERY_PLUGIN_CLASS (plugin_mgr,
  	"crystalspace.mesh.object.sprite.cal3d", iMeshObjectType));
  if (!type)
  {
    type = CS_LOAD_PLUGIN (plugin_mgr, "crystalspace.mesh.object.sprite.cal3d",
    	iMeshObjectType);
  }
  if (!type)
  {
    synldr->ReportError (
		"crystalspace.spritecal3dfactoryloader.setup.objecttype",
		node, "Could not load the sprite.cal3d mesh object plugin!");
    return 0;
  }

  // @@@ Temporary fix to allow to set actions for objects loaded
  // with impexp. Once those loaders move to another plugin this code
  // below should be removed.
  csRef<iMeshObjectFactory> fact;
  if (context)
    fact = SCF_QUERY_INTERFACE (context, iMeshObjectFactory);
  // DecRef of fact will be handled later.
  // If there was no factory we create a new one.
  if (!fact)
    fact = type->NewFactory ();

  csRef<iSpriteCal3DFactoryState> newspr (
  	SCF_QUERY_INTERFACE (fact, iSpriteCal3DFactoryState));

  if (!newspr->Create("dummy"))
  {
      newspr->ReportLastError();
      return 0; // failed
  }

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_PATH:
      {
	const char *path = child->GetAttributeValue("dir");
	if (path)
	  newspr->SetBasePath(path);
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badpath",
		  child,"dir is a required attribute of <path> token in cal3d files.");
	  return 0;
	}
	break;
      }
      case XMLTOKEN_SCALE:
      {
	float scale = child->GetAttributeValueAsFloat("value");
	if (scale)
	  newspr->SetRenderScale(scale);
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badvalue",
		  child,"value is a required attribute of <scale> token in cal3d files.");
	  return 0;
	}
	break;
      }
      case XMLTOKEN_SKELETON:
      {
	const char *file = child->GetAttributeValue("file");
	if (file)
	{
	  if (!newspr->LoadCoreSkeleton(file))
	  {
	    newspr->ReportLastError();
	    return 0;
	  }
	}
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badfile",
		  child,"file is a required attribute of <skeleton> token in cal3d files.");
	  return 0;
	}
	break;
      }
      case XMLTOKEN_ANIMATION:
      {
	const char *file = child->GetAttributeValue("file");
	if (file)
	{
	  int animID = newspr->LoadCoreAnimation(file);
	  if (animID == -1)
	  {
	    newspr->ReportLastError();
	    return 0;
	  }
	}
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badfile",
		  child,"file is a required attribute of <animation> token in cal3d files.");
	  return 0;
	}
	break;
      }
      case XMLTOKEN_MESH:
      {
	const char *file = child->GetAttributeValue("file");
	if (file)
	{
	  if (!newspr->LoadCoreMesh(file))
	  {
	    newspr->ReportLastError();
	    return 0;
	  }
	}
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badfile",
		  child,"file is a required attribute of <mesh> token in cal3d files.");
	  return 0;
	}
	break;
      }
      case XMLTOKEN_MATERIAL:
      {
	const char *file = child->GetAttributeValue("file");
	if (file)
	{
          iMaterialWrapper* mat = ldr_context->FindMaterial (file);
	  if (!mat)
	  {
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.unknownmaterial",
		  child, "Couldn't find material named '%s'", file);
            return 0;
	  }
	  newspr->AddCoreMaterial(mat);
	}
	else
	{
	    synldr->ReportError (
		  "crystalspace.spritecal3dfactoryloader.parse.badfile",
		  child,"file is a required attribute of <material> token in cal3d files.");
	  return 0;
	}
	break;
      }

      default:
	synldr->ReportBadToken (child);
        return 0;
    }
  }
  // Wrapup cal3d initialization
  newspr->BindMaterials();

  return csPtr<iBase> (fact);
}




//---------------------------------------------------------------------------

csSpriteCal3DFactorySaver::csSpriteCal3DFactorySaver (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csSpriteCal3DFactorySaver::~csSpriteCal3DFactorySaver ()
{
}

bool csSpriteCal3DFactorySaver::Initialize (iObjectRegistry* object_reg)
{
  csSpriteCal3DFactorySaver::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  return true;
}

#define MAXLINE 100 /* max number of chars per line... */


void csSpriteCal3DFactorySaver::WriteDown (iBase*, iFile*)
{
}






//---------------------------------------------------------------------------

csSpriteCal3DLoader::csSpriteCal3DLoader (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csSpriteCal3DLoader::~csSpriteCal3DLoader ()
{
}

bool csSpriteCal3DLoader::Initialize (iObjectRegistry* object_reg)
{
#if 0
  csSprite3DLoader::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  synldr = CS_QUERY_REGISTRY (object_reg, iSyntaxService);

  xmltokens.Register ("action", XMLTOKEN_ACTION);
  xmltokens.Register ("applymotion", XMLTOKEN_APPLYMOTION);
  xmltokens.Register ("basecolor", XMLTOKEN_BASECOLOR);
  xmltokens.Register ("factory", XMLTOKEN_FACTORY);
  xmltokens.Register ("lighting", XMLTOKEN_LIGHTING);
  xmltokens.Register ("material", XMLTOKEN_MATERIAL);
  xmltokens.Register ("mixmode", XMLTOKEN_MIXMODE);
  xmltokens.Register ("tween", XMLTOKEN_TWEEN);
#endif

  return true;
}

csPtr<iBase> csSpriteCal3DLoader::Parse (iDocumentNode* node,
	iLoaderContext* ldr_context, iBase*)
{
#if 0
  csRef<iMeshObject> mesh;
  csRef<iSprite3DState> spr3dLook;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_FACTORY:
	{
	  const char* factname = child->GetContentsValue ();
	  iMeshFactoryWrapper* fact = ldr_context->FindMeshFactory (factname);
	  if (!fact)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.unknownfactory",
		child, "Couldn't find factory '%s'!", factname);
	    return 0;
	  }
	  mesh = fact->GetMeshObjectFactory ()->NewInstance ();
          spr3dLook = SCF_QUERY_INTERFACE (mesh, iSprite3DState);
	}
	break;
      case XMLTOKEN_ACTION:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	spr3dLook->SetAction (child->GetContentsValue ());
        break;
      case XMLTOKEN_BASECOLOR:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	else
	{
	  csColor col;
	  if (!synldr->ParseColor (child, col))
	    return 0;
	  spr3dLook->SetBaseColor (col);
	}
        break;
      case XMLTOKEN_LIGHTING:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	else
	{
	  bool do_lighting;
	  if (!synldr->ParseBool (child, do_lighting, true))
	    return 0;
	  spr3dLook->SetLighting (do_lighting);
	}
        break;
      case XMLTOKEN_MATERIAL:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	else
	{
	  const char* matname = child->GetContentsValue ();
          iMaterialWrapper* mat = ldr_context->FindMaterial (matname);
	  if (!mat)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.unknownmaterial",
		child, "Couldn't find material '%s'!", matname);
            return 0;
	  }
	  spr3dLook->SetMaterialWrapper (mat);
	}
	break;
      case XMLTOKEN_MIXMODE:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	else
        {
	  uint mm;
	  if (!synldr->ParseMixmode (child, mm))
	    return 0;
          spr3dLook->SetMixMode (mm);
	}
	break;
      case XMLTOKEN_APPLYMOTION:
	{
	  const char* motname = child->GetContentsValue ();
	  csRef<iPluginManager> plugin_mgr (CS_QUERY_REGISTRY (object_reg,
	  	iPluginManager));
	  csRef<iMotionManager> motman (CS_QUERY_PLUGIN_CLASS (plugin_mgr,
		"crystalspace.motion.manager.default",
		iMotionManager));
	  if (!motman)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.setup.motion.motionmanager",
		child, "Could not find motion manager!");
	    return 0;
	  }
	  if (!spr3dLook)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	    return 0;
	  }
	  iSkeletonState *skel_state = spr3dLook->GetSkeletonState();
	  csRef<iSkeletonLimbState> limb (SCF_QUERY_INTERFACE (skel_state,
	  	iSkeletonLimbState));
	  limb = limb->GetChildren ();
	  if (!limb)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.nochildren",
		child, "Skeleton has no libs. Cannot apply motion!");
	    return 0;
	  }
	  csRef<iSkeletonConnectionState> con (SCF_QUERY_INTERFACE (limb,
	  	iSkeletonConnectionState));
	  csRef<iSkeletonBone> bone (SCF_QUERY_INTERFACE (con, iSkeletonBone));
	  if (!bone)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.nobones",
		child, "The skeleton has no bones!");
	    return 0;
	  }
          iMotionTemplate* motion = motman->FindMotionByName (motname);
	  if (!motion)
	  {
      	    synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.nomotion",
		child, "The motion '%s' does not exist!", motname);
	    return 0;
	  }
          iMotionController* mc = motman->AddController (bone);
          mc->SetMotion (motion);
	}
	break;
      case XMLTOKEN_TWEEN:
	if (!spr3dLook)
	{
      	  synldr->ReportError (
		"crystalspace.sprite3dloader.parse.motion.missingfactory",
		child,
		"No Factory! Please define 'factory' before 'applymotion'!");
	  return 0;
	}
	else
	{
	  bool do_tween;
	  if (!synldr->ParseBool (child, do_tween, true))
	    return 0;
          spr3dLook->EnableTweening (do_tween);
	}
	break;
      default:
	synldr->ReportBadToken (child);
	return 0;
    }
  }

  return csPtr<iBase> (mesh);
#endif
  return NULL;
}

//---------------------------------------------------------------------------

csSpriteCal3DSaver::csSpriteCal3DSaver (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csSpriteCal3DSaver::~csSpriteCal3DSaver ()
{
}

bool csSpriteCal3DSaver::Initialize (iObjectRegistry* object_reg)
{
  csSpriteCal3DSaver::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  return true;
}

void csSpriteCal3DSaver::WriteDown (iBase*, iFile*)
{
}

