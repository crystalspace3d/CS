/*
    Copyright (C) 2001 by Jorrit Tyberghein

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
#include "csgeom/math3d.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csutil/util.h"
#include "csutil/scanstr.h"
#include "csutil/cscolor.h"
#include "csutil/csstring.h"
#include "iutil/vfs.h"
#include "iutil/document.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "iutil/plugin.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "iengine/material.h"
#include "imesh/object.h"
#include "imesh/thing/thing.h"
#include "imesh/thing/polygon.h"
#include "imesh/thing/portal.h"
#include "imesh/thing/polytmap.h"
#include "imesh/thing/curve.h"
#include "ivideo/graph3d.h"
#include "ivideo/texture.h"
#include "ivideo/material.h"
#include "imap/services.h"
#include "imap/ldrctxt.h"
#include "imap/parser.h"
#include "ivaria/reporter.h"
#include "thingldr.h"
#include "qint.h"

CS_IMPLEMENT_PLUGIN

enum
{
  XMLTOKEN_CLONE = 1,
  XMLTOKEN_COSFACT,
  XMLTOKEN_CURVE,
  XMLTOKEN_CURVECENTER,
  XMLTOKEN_CURVECONTROL,
  XMLTOKEN_CURVESCALE,
  XMLTOKEN_FACTORY,
  XMLTOKEN_FASTMESH,
  XMLTOKEN_FOG,
  XMLTOKEN_MATSETSELECT,
  XMLTOKEN_MATERIAL,
  XMLTOKEN_MOVEABLE,
  XMLTOKEN_PART,
  XMLTOKEN_P,
  XMLTOKEN_RADIUS,
  XMLTOKEN_TEXLEN,
  XMLTOKEN_VISTREE,
  XMLTOKEN_V,

  // Below is for plane loader.
  XMLTOKEN_ORIG,
  XMLTOKEN_FIRSTLEN,
  XMLTOKEN_FIRST,
  XMLTOKEN_SECONDLEN,
  XMLTOKEN_SECOND,
  XMLTOKEN_MATRIX,
  //XMLTOKEN_V,
  XMLTOKEN_NAME,
  XMLTOKEN_UVEC,
  XMLTOKEN_VVEC,
  XMLTOKEN_SMOOTH
};

SCF_IMPLEMENT_IBASE (csThingLoader)
  SCF_IMPLEMENTS_INTERFACE (iLoaderPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csThingLoader::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csThingSaver)
  SCF_IMPLEMENTS_INTERFACE (iSaverPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csThingSaver::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csPlaneLoader)
  SCF_IMPLEMENTS_INTERFACE (iLoaderPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csPlaneLoader::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csPlaneSaver)
  SCF_IMPLEMENTS_INTERFACE (iSaverPlugin)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csPlaneSaver::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_FACTORY (csThingLoader)
SCF_IMPLEMENT_FACTORY (csThingSaver)
SCF_IMPLEMENT_FACTORY (csPlaneLoader)
SCF_IMPLEMENT_FACTORY (csPlaneSaver)

SCF_EXPORT_CLASS_TABLE (thingldr)
  SCF_EXPORT_CLASS (csThingLoader, "crystalspace.mesh.loader.factory.thing",
    "Crystal Space Thing Mesh Factory Loader")
  SCF_EXPORT_CLASS (csThingSaver, "crystalspace.mesh.saver.factory.thing",
    "Crystal Space Thing Mesh Factory Saver")
  SCF_EXPORT_CLASS (csThingLoader, "crystalspace.mesh.loader.thing",
    "Crystal Space Thing Mesh Loader")
  SCF_EXPORT_CLASS (csThingSaver, "crystalspace.mesh.saver.thing",
    "Crystal Space Thing Mesh Saver")
  SCF_EXPORT_CLASS (csPlaneLoader, "crystalspace.mesh.loader.thing.plane",
    "Crystal Space Thing Plane Loader")
  SCF_EXPORT_CLASS (csPlaneSaver, "crystalspace.mesh.saver.thing.plane",
    "Crystal Space Thing Plane Saver")
SCF_EXPORT_CLASS_TABLE_END

#define MAXLINE 200 /* max number of chars per line... */

//---------------------------------------------------------------------------

csThingLoader::csThingLoader (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csThingLoader::~csThingLoader ()
{
}

bool csThingLoader::Initialize (iObjectRegistry* object_reg)
{
  csThingLoader::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  synldr = CS_QUERY_REGISTRY (object_reg, iSyntaxService);

  xmltokens.Register ("clone", XMLTOKEN_CLONE);
  xmltokens.Register ("cosfact", XMLTOKEN_COSFACT);
  xmltokens.Register ("curve", XMLTOKEN_CURVE);
  xmltokens.Register ("curvecenter", XMLTOKEN_CURVECENTER);
  xmltokens.Register ("curvecontrol", XMLTOKEN_CURVECONTROL);
  xmltokens.Register ("curvescale", XMLTOKEN_CURVESCALE);
  xmltokens.Register ("factory", XMLTOKEN_FACTORY);
  xmltokens.Register ("fastmesh", XMLTOKEN_FASTMESH);
  xmltokens.Register ("fog", XMLTOKEN_FOG);
  xmltokens.Register ("matsetselect", XMLTOKEN_MATSETSELECT);
  xmltokens.Register ("material", XMLTOKEN_MATERIAL);
  xmltokens.Register ("moveable", XMLTOKEN_MOVEABLE);
  xmltokens.Register ("part", XMLTOKEN_PART);
  xmltokens.Register ("p", XMLTOKEN_P);
  xmltokens.Register ("smooth", XMLTOKEN_SMOOTH);
  xmltokens.Register ("radius", XMLTOKEN_RADIUS);
  xmltokens.Register ("texlen", XMLTOKEN_TEXLEN);
  xmltokens.Register ("vistree", XMLTOKEN_VISTREE);
  xmltokens.Register ("v", XMLTOKEN_V);
  return true;
}

bool csThingLoader::ParseCurve (iCurve* curve, iLoaderContext* ldr_context,
	iDocumentNode* node)
{
  int num_v = 0;
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_MATERIAL:
	{
	  const char* matname = child->GetContentsValue ();
          iMaterialWrapper* mat = ldr_context->FindMaterial (matname);
          if (mat == NULL)
          {
	    synldr->ReportError (
	      "crystalspace.bezierloader.parse.material",
              child, "Couldn't find material named '%s'!", matname);
            return false;
          }
          curve->SetMaterial (mat);
	}
        break;
      case XMLTOKEN_V:
        {
          if (num_v >= 9)
          {
	    synldr->ReportError (
	      "crystalspace.bezierloader.parse.vertices",
              child, "Wrong number of vertices to bezier! Should be 9!");
            return false;
          }
	  curve->SetVertex (num_v, child->GetContentsValueAsInt ());
	  num_v++;
        }
        break;
      default:
	synldr->ReportBadToken (child);
	return false;
    }
  }
  
  if (num_v != 9)
  {
    synldr->ReportError (
      "crystalspace.bezierloader.parse.vertices",
      node, "Wrong number of vertices to bezier! %d should be 9!", num_v);
    return false;
  }
  return true;
}

bool csThingLoader::LoadThingPart (iThingEnvironment* te, iDocumentNode* node,
	iLoaderContext* ldr_context,
	iObjectRegistry* object_reg, iReporter* reporter,
	iSyntaxService *synldr, ThingLoadInfo& info,
	iEngine* engine, iThingState* thing_state,
	int vt_offset, bool isParent)
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_VISTREE:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.vistree",
	    child, "'vistree' flag only for top-level thing!");
	  return false;
	}
        else
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.vistree",
	    child, "'vistree' no longer supported! Convert your level to Dynavis using 'levtool'!\n");
	  printf ("'vistree' no longer supported! Convert your level to Dynavis using 'levtool'!\n");
	  return false;
	}
        break;
      case XMLTOKEN_COSFACT:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.fastmesh",
	    child, "'cosfact' flag only for top-level thing!");
	  return false;
	}
        else thing_state->SetCosinusFactor (child->GetContentsValueAsFloat ());
        break;
      case XMLTOKEN_FASTMESH:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.fastmesh",
	    child, "'fastmesh' flag only for top-level thing!");
	  return false;
	}
        else thing_state->GetFlags ().Set (CS_THING_FASTMESH);
        break;
      case XMLTOKEN_MOVEABLE:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.moveable",
	    child, "'moveable' flag only for top-level thing!");
	  return false;
	}
        else thing_state->SetMovingOption (CS_THING_MOVE_OCCASIONAL);
        break;
      case XMLTOKEN_FACTORY:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.factory",
	    child, "'factory' statement only for top-level thing!");
	  return false;
	}
	else
        {
	  const char* factname = child->GetContentsValue ();
	  iMeshFactoryWrapper* fact = ldr_context->FindMeshFactory (factname);
          if (!fact)
          {
	    synldr->ReportError (
	      "crystalspace.thingloader.parse.factory",
              child, "Couldn't find thing factory '%s'!", factname);
            return false;
          }
	  csRef<iThingState> tmpl_thing_state (SCF_QUERY_INTERFACE (
	  	fact->GetMeshObjectFactory (), iThingState));
	  if (!tmpl_thing_state)
	  {
	    synldr->ReportError (
	      "crystalspace.thingloader.parse.factory",
              child, "Object '%s' is not a thing!", factname);
            return false;
	  }
	  thing_state->MergeTemplate (tmpl_thing_state, info.default_material);
	  if (info.use_mat_set)
          {
	    thing_state->ReplaceMaterials (engine->GetMaterialList (),
	      info.mat_set_name);
	    info.use_mat_set = false;
	  }
        }
        break;
      case XMLTOKEN_CLONE:
        if (!isParent)
	{
	  synldr->ReportError (
	    "crystalspace.thingloader.parse.clone",
	    child, "CLONE statement only for top-level thing!");
	  return false;
	}
	else
        {
	  const char* meshname = child->GetContentsValue ();
	  iMeshWrapper* wrap = ldr_context->FindMeshObject (meshname);
          if (!wrap)
          {
	    synldr->ReportError (
	      "crystalspace.thingloader.parse.clone",
              child, "Couldn't find thing '%s'!", meshname);
            return false;
          }

	  csRef<iThingState> tmpl_thing_state (SCF_QUERY_INTERFACE (
	  	wrap->GetMeshObject (), iThingState));
	  if (!tmpl_thing_state)
	  {
	    synldr->ReportError (
	      "crystalspace.thingloader.parse.clone",
              child, "Object '%s' is not a thing!", meshname);
            return false;
	  }
	  thing_state->MergeTemplate (tmpl_thing_state, info.default_material);
	  if (info.use_mat_set)
          {
	    thing_state->ReplaceMaterials (engine->GetMaterialList (),
	      info.mat_set_name);
	    info.use_mat_set = false;
	  }
        }
        break;
      case XMLTOKEN_PART:
	if (!LoadThingPart (te, child, ldr_context, object_reg, reporter,
		synldr, info, engine, thing_state,
		thing_state->GetVertexCount (), false))
	  return false;
        break;
      case XMLTOKEN_V:
        {
	  csVector3 v;
	  if (!synldr->ParseVector (child, v))
	    return false;
          thing_state->CreateVertex (v);
        }
        break;
      case XMLTOKEN_FOG:
	synldr->ReportError (
	      "crystalspace.thingloader.parse.fog",
      	      child, "FOG for things is currently not supported!\n\
Nag to Jorrit about this feature if you want it.");
	return false;

      case XMLTOKEN_P:
        {
	  iPolygon3D* poly3d = thing_state->CreatePolygon (
			  child->GetAttributeValue ("name"));
	  if (info.default_material)
	    poly3d->SetMaterial (info.default_material);
	  if (!synldr->ParsePoly3d (child, ldr_context,
	  			    engine, poly3d,
				    info.default_texlen, thing_state,
				    vt_offset))
	  {
	    poly3d->DecRef ();
	    return false;
	  }
        }
        break;

      case XMLTOKEN_CURVE:
        {
	  iCurve* p = thing_state->CreateCurve ();
	  p->QueryObject()->SetName (child->GetAttributeValue ("name"));
	  if (!ParseCurve (p, ldr_context, child))
	    return false;
        }
        break;

      case XMLTOKEN_CURVECENTER:
        {
          csVector3 c;
	  if (!synldr->ParseVector (child, c))
	    return false;
          thing_state->SetCurvesCenter (c);
        }
        break;
      case XMLTOKEN_CURVESCALE:
        {
	  float f = child->GetContentsValueAsFloat ();
	  thing_state->SetCurvesScale (f);
          break;
        }
      case XMLTOKEN_CURVECONTROL:
        {
          csVector3 v;
          csVector2 t;
	  v.x = child->GetAttributeValueAsFloat ("x");
	  v.y = child->GetAttributeValueAsFloat ("y");
	  v.z = child->GetAttributeValueAsFloat ("z");
	  t.x = child->GetAttributeValueAsFloat ("u");
	  t.y = child->GetAttributeValueAsFloat ("v");
          thing_state->AddCurveVertex (v, t);
        }
        break;

      case XMLTOKEN_MATERIAL:
	{
	  const char* matname = child->GetContentsValue ();
          info.default_material = ldr_context->FindMaterial (matname);
          if (info.default_material == NULL)
          {
	    synldr->ReportError (
	        "crystalspace.thingloader.parse.material",
                child, "Couldn't find material named '%s'!", matname);
            return false;
          }
	}
        break;
      case XMLTOKEN_TEXLEN:
	info.default_texlen = child->GetContentsValueAsFloat ();
        break;
      case XMLTOKEN_MATSETSELECT:
        info.SetTextureSet (child->GetContentsValue ());
        info.use_mat_set = true;
        break;
      case XMLTOKEN_SMOOTH:
	thing_state->SetSmoothingFlag (true);
	break;
      default:
        synldr->ReportBadToken (child);
	return false;
    }
  }
  return true;
}

csPtr<iBase> csThingLoader::Parse (iDocumentNode* node,
			     iLoaderContext* ldr_context, iBase*)
{
  csRef<iPluginManager> plugin_mgr (CS_QUERY_REGISTRY (object_reg,
  	iPluginManager));
  csRef<iMeshObjectType> type (CS_QUERY_PLUGIN_CLASS (plugin_mgr,
  	"crystalspace.mesh.object.thing", iMeshObjectType));
  if (!type)
  {
    type = CS_LOAD_PLUGIN (plugin_mgr, "crystalspace.mesh.object.thing",
    	iMeshObjectType);
  }
  if (!type)
  {
    synldr->ReportError (
		"crystalspace.thingloader.setup.objecttype",
		node, "Could not load the thing mesh object plugin!");
    return NULL;
  }
  csRef<iThingEnvironment> te = SCF_QUERY_INTERFACE (type,
  	iThingEnvironment);
  csRef<iEngine> engine = CS_QUERY_REGISTRY (object_reg, iEngine);

  csRef<iMeshObjectFactory> fact;
  csRef<iThingState> thing_state;

  // We always do NewFactory() even for mesh objects.
  // That's because csThing implements both so a factory is a mesh object.
  fact = type->NewFactory ();
  thing_state = SCF_QUERY_INTERFACE (fact, iThingState);

  ThingLoadInfo info;
  if (!LoadThingPart (te, node, ldr_context, object_reg, reporter, synldr, info,
  	engine, thing_state, 0, true))
  {
    fact = NULL;
  }
  return csPtr<iBase> (fact);
}

//---------------------------------------------------------------------------

csThingSaver::csThingSaver (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csThingSaver::~csThingSaver ()
{
}

bool csThingSaver::Initialize (iObjectRegistry* object_reg)
{
  csThingSaver::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  return true;
}

void csThingSaver::WriteDown (iBase* /*obj*/, iFile *file)
{
  csString str;
  csRef<iFactory> fact (SCF_QUERY_INTERFACE (this, iFactory));
  char buf[MAXLINE];
  char name[MAXLINE];
  csFindReplace (name, fact->QueryDescription (), "Saver", "Loader", MAXLINE);
  sprintf (buf, "FACTORY ('%s')\n", name);
  str.Append (buf);
  file->Write ((const char*)str, str.Length ());
}

//---------------------------------------------------------------------------

csPlaneLoader::csPlaneLoader (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csPlaneLoader::~csPlaneLoader ()
{
}

bool csPlaneLoader::Initialize (iObjectRegistry* object_reg)
{
  csPlaneLoader::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  synldr = CS_QUERY_REGISTRY (object_reg, iSyntaxService);

  xmltokens.Register ("clone", XMLTOKEN_CLONE);
  xmltokens.Register ("orig", XMLTOKEN_ORIG);
  xmltokens.Register ("firstlen", XMLTOKEN_FIRSTLEN);
  xmltokens.Register ("first", XMLTOKEN_FIRST);
  xmltokens.Register ("secondlen", XMLTOKEN_SECONDLEN);
  xmltokens.Register ("second", XMLTOKEN_SECOND);
  xmltokens.Register ("matrix", XMLTOKEN_MATRIX);
  xmltokens.Register ("v", XMLTOKEN_V);
  xmltokens.Register ("name", XMLTOKEN_NAME);
  xmltokens.Register ("uvec", XMLTOKEN_UVEC);
  xmltokens.Register ("vvec", XMLTOKEN_VVEC);
  return true;
}

csPtr<iBase> csPlaneLoader::Parse (iDocumentNode* node,
			     iLoaderContext* /*ldr_context*/,
			     iBase* /*context*/)
{
  csRef<iPluginManager> plugin_mgr (CS_QUERY_REGISTRY (object_reg,
  	iPluginManager));
  csRef<iMeshObjectType> type (CS_QUERY_PLUGIN_CLASS (plugin_mgr,
  	"crystalspace.mesh.object.thing", iMeshObjectType));
  if (!type)
  {
    type = CS_LOAD_PLUGIN (plugin_mgr, "crystalspace.mesh.object.thing",
    	iMeshObjectType);
  }
  if (!type)
  {
    synldr->ReportError (
		"crystalspace.thingloader.setup.objecttype",
		node, "Could not load the thing mesh object plugin!");
    return NULL;
  }
  csRef<iThingEnvironment> te = SCF_QUERY_INTERFACE (type,
  	iThingEnvironment);
  csRef<iEngine> engine (CS_QUERY_REGISTRY (object_reg, iEngine));
  csRef<iPolyTxtPlane> ppl (te->CreatePolyTxtPlane ());

  bool tx1_given = false, tx2_given = false;
  csVector3 tx_orig (0, 0, 0), tx1 (0, 0, 0), tx2 (0, 0, 0);
  float tx1_len = 0, tx2_len = 0;
  csMatrix3 tx_matrix;
  csVector3 tx_vector (0, 0, 0);
  char name[255]; name[0] = 0;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_NAME:
	strcpy (name, child->GetContentsValue () ?
			child->GetContentsValue () : "");
	ppl->QueryObject()->SetName (name);
        break;
      case XMLTOKEN_ORIG:
        tx1_given = true;
        synldr->ParseVector (child, tx_orig);
        break;
      case XMLTOKEN_FIRST:
        tx1_given = true;
        synldr->ParseVector (child, tx1);
        break;
      case XMLTOKEN_FIRSTLEN:
	tx1_len = child->GetContentsValueAsFloat ();
        tx1_given = true;
        break;
      case XMLTOKEN_SECOND:
        tx2_given = true;
        synldr->ParseVector (child, tx2);
        break;
      case XMLTOKEN_SECONDLEN:
	tx2_len = child->GetContentsValueAsFloat ();
        tx2_given = true;
        break;
      case XMLTOKEN_MATRIX:
        synldr->ParseMatrix (child, tx_matrix);
        break;
      case XMLTOKEN_V:
        synldr->ParseVector (child, tx_vector);
        break;
      case XMLTOKEN_UVEC:
        tx1_given = true;
        synldr->ParseVector (child, tx1);
        tx1_len = tx1.Norm ();
        tx1 += tx_orig;
        break;
      case XMLTOKEN_VVEC:
        tx2_given = true;
        synldr->ParseVector (child, tx2);
        tx2_len = tx2.Norm ();
        tx2 += tx_orig;
        break;
      default:
        synldr->ReportBadToken (child);
	return NULL;
    }
  }

  if (tx1_given)
    if (tx2_given)
    {
      if (!tx1_len)
      {
	synldr->ReportError (
	  "crystalspace.planeloader.parse.badplane",
          node, "Bad texture specification for PLANE '%s'", name);
	tx1_len = 1;
      }
      if (!tx2_len)
      {
	synldr->ReportError (
	  "crystalspace.planeloader.parse.badplane",
          node, "Bad texture specification for PLANE '%s'", name);
	tx2_len = 1;
      }
      if ((tx1-tx_orig) < SMALL_EPSILON)
        printf ("Bad texture specification for PLANE '%s'\n", name);
      else if ((tx2-tx_orig) < SMALL_EPSILON)
        printf ("Bad texture specification for PLANE '%s'\n", name);
      else ppl->SetTextureSpace (tx_orig, tx1, tx1_len, tx2, tx2_len);
    }
    else
    {
      synldr->ReportError (
	  "crystalspace.planeloader.parse.badplane",
          node, "Not supported!");
      return NULL;
    }
  else
    ppl->SetTextureSpace (tx_matrix, tx_vector);

  return csPtr<iBase> (ppl);
}

//---------------------------------------------------------------------------

csPlaneSaver::csPlaneSaver (iBase* pParent)
{
  SCF_CONSTRUCT_IBASE (pParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE(scfiComponent);
}

csPlaneSaver::~csPlaneSaver ()
{
}

bool csPlaneSaver::Initialize (iObjectRegistry* object_reg)
{
  csPlaneSaver::object_reg = object_reg;
  reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  return true;
}

void csPlaneSaver::WriteDown (iBase* /*obj*/, iFile* /*file*/)
{
}

