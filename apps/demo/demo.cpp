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
#include "cssys/sysfunc.h"
#include "demo.h"
#include "demoseq.h"
#include "csutil/cscolor.h"
#include "csutil/cmdhelp.h"
#include "csgeom/path.h"
#include "cstool/csfxscr.h"
#include "cstool/csview.h"
#include "cstool/initapp.h"
#include "ivideo/graph3d.h"
#include "ivideo/natwin.h"
#include "ivideo/fontserv.h"
#include "ivideo/graph2d.h"
#include "ivideo/txtmgr.h"
#include "ivaria/conout.h"
#include "iengine/engine.h"
#include "iengine/sector.h"
#include "iengine/light.h"
#include "iengine/statlght.h"
#include "iengine/camera.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/halo.h"
#include "iengine/material.h"
#include "imesh/thing/thing.h"
#include "imesh/thing/polygon.h"
#include "imesh/thing/ptextype.h"
#include "imesh/particle.h"
#include "imesh/sprite2d.h"
#include "imesh/sprite3d.h"
#include "imesh/ball.h"
#include "imesh/stars.h"
#include "imesh/object.h"
#include "imap/reader.h"
#include "imap/parser.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/event.h"
#include "iutil/eventq.h"
#include "iutil/objreg.h"
#include "iutil/virtclk.h"
#include "iutil/csinput.h"
#include "igraphic/imageio.h"
#include "ivaria/reporter.h"
#include "qsqrt.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"

CS_IMPLEMENT_APPLICATION

//-----------------------------------------------------------------------------

// The global system driver
Demo *System;

void Demo::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  iReporter* rep = CS_QUERY_REGISTRY (System->object_reg, iReporter);
  if (rep)
  {
    rep->ReportV (severity, "crystalspace.application.demo", msg, arg);
    rep->DecRef ();
  }
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

Demo::Demo ()
{
  vc = NULL;
  engine = NULL;
  seqmgr = NULL;
  loader = NULL;
  myG3D = NULL;
  myG2D = NULL;
  myVFS = NULL;
  kbd = NULL;
  view = NULL;
  message[0] = 0;
  font = NULL;
}

Demo::~Demo ()
{
  if (vc) vc->DecRef ();
  if (font) font->DecRef ();
  if (view) view->DecRef ();
  if (engine) engine->DecRef ();
  if (myG3D) myG3D->DecRef ();
  if (myG2D) myG2D->DecRef ();
  if (myVFS) myVFS->DecRef ();
  if (kbd) kbd->DecRef ();
  if (loader) loader->DecRef ();
  delete seqmgr;
}

void Cleanup ()
{
  csPrintf ("Cleaning up...\n");
  iObjectRegistry* object_reg = System->object_reg;
  delete System; System = NULL;
  csInitializer::DestroyApplication (object_reg);
}

iMeshWrapper* Demo::LoadObject (const char* objname, const char* filename,
	const char* classId, const char* loaderClassId,
	iSector* sector, const csVector3& pos)
{
  iDataBuffer* databuf = myVFS->ReadFile (filename);
  if (!databuf || !databuf->GetSize ())
  {
    if (databuf) databuf->DecRef ();
    Report (CS_REPORTER_SEVERITY_ERROR, "Could not open file '%s' on VFS!",
    	filename);
    exit (0);
  }
  iMeshWrapper* obj = engine->LoadMeshWrapper (objname,
  	loaderClassId, databuf, sector, pos);
  databuf->DecRef ();
  if (!obj)
  {
    Report (CS_REPORTER_SEVERITY_ERROR,
    	"There was an error loading object from file '%s'!",
	filename);
    exit (0);
  }
  return obj;
}

static void SetTexSpace (iPolygon3D* poly,
  int size, const csVector3& orig, const csVector3& upt, float ulen,
  const csVector3& vpt, float vlen)
{
  csVector3 texorig = orig;
  csVector3 texu = upt;
  float texulen = ulen;
  csVector3 texv = vpt;
  float texvlen = vlen;
  /// copied, now adjust
  csVector3 uvector = upt - orig;
  csVector3 vvector = vpt - orig;
  /// to have 1 pixel going over the edges.
  texorig -= uvector / float(size);
  texorig -= vvector / float(size);
  texu += uvector / float(size);
  texv += vvector / float(size);
  texulen += ulen * 2.0f / float(size);
  texvlen += vlen * 2.0f / float(size);
  poly->SetTextureSpace (texorig, texu, texulen, texv, texvlen);
}

void Demo::SetupSector ()
{
  room = engine->CreateSector ("room");
  iMeshWrapper* walls = engine->CreateSectorWallsMesh (room, "walls");
  walls->GetFlags ().Set (CS_ENTITY_CAMERA);
  walls->SetRenderPriority (engine->GetRenderPriority ("starLevel1"));
  walls->SetZBufMode (CS_ZBUF_NONE);
  iThingState* walls_state = SCF_QUERY_INTERFACE (walls->GetMeshObject (),
      iThingState);
  walls_state->SetMovingOption (CS_THING_MOVE_OCCASIONAL);

  float size = 500.0; /// Size of the skybox -- around 0,0,0 for now.
  iPolygon3D* p;
  p = walls_state->CreatePolygon ("d");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_d"));
  p->CreateVertex (csVector3 (-size, -size, size));
  p->CreateVertex (csVector3 (size, -size, size));
  p->CreateVertex (csVector3 (size, -size, -size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (2), p->GetVertex (3),
  	2.0f * size, p->GetVertex (1), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  p = walls_state->CreatePolygon ("u");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_u"));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (-size, size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	2.0f * size, p->GetVertex (3), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  p = walls_state->CreatePolygon ("f");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_f"));
  p->CreateVertex (csVector3 (-size, size, size));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (size, -size, size));
  p->CreateVertex (csVector3 (-size, -size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
    2.0f * size, p->GetVertex (3), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  p = walls_state->CreatePolygon ("r");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_r"));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (size, -size, -size));
  p->CreateVertex (csVector3 (size, -size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	2.0f * size, p->GetVertex (3), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  p = walls_state->CreatePolygon ("l");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_l"));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (-size, size, size));
  p->CreateVertex (csVector3 (-size, -size, size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	2.0f * size, p->GetVertex (3), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  p = walls_state->CreatePolygon ("b");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("nebula_b"));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  p->CreateVertex (csVector3 (size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
    2.0f * size, p->GetVertex (3), 2.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);

  walls_state->DecRef ();
  walls->DecRef ();

  //====================================================================
  // Create the stars.
  //====================================================================
  walls = engine->CreateSectorWallsMesh (room, "stars");
  walls->GetFlags ().Set (CS_ENTITY_CAMERA);
  walls->SetRenderPriority (engine->GetRenderPriority ("starLevel2"));
  walls->SetZBufMode (CS_ZBUF_NONE);
  walls_state = SCF_QUERY_INTERFACE (walls->GetMeshObject (), iThingState);
  walls_state->SetMovingOption (CS_THING_MOVE_OCCASIONAL);

  size = 200.0; /// Size of the skybox -- around 0,0,0 for now.
  p = walls_state->CreatePolygon ("d");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (-size, -size, size));
  p->CreateVertex (csVector3 (size, -size, size));
  p->CreateVertex (csVector3 (size, -size, -size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (2), p->GetVertex (3),
  	1.0f * size, p->GetVertex (1), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  iPolyTexType* pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  p = walls_state->CreatePolygon ("u");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (-size, size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	1.0f * size, p->GetVertex (3), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  p = walls_state->CreatePolygon ("f");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (-size, size, size));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (size, -size, size));
  p->CreateVertex (csVector3 (-size, -size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
    1.0f * size, p->GetVertex (3), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  p = walls_state->CreatePolygon ("r");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (size, size, size));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (size, -size, -size));
  p->CreateVertex (csVector3 (size, -size, size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	1.0f * size, p->GetVertex (3), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  p = walls_state->CreatePolygon ("l");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (-size, size, size));
  p->CreateVertex (csVector3 (-size, -size, size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
  	1.0f * size, p->GetVertex (3), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  p = walls_state->CreatePolygon ("b");
  p->SetMaterial (engine->GetMaterialList ()->
  	FindByName ("stars"));
  p->CreateVertex (csVector3 (size, size, -size));
  p->CreateVertex (csVector3 (-size, size, -size));
  p->CreateVertex (csVector3 (-size, -size, -size));
  p->CreateVertex (csVector3 (size, -size, -size));
  SetTexSpace (p, 256, p->GetVertex (0), p->GetVertex (1),
    1.0f * size, p->GetVertex (3), 1.0f * size);
  p->GetFlags ().Set (CS_POLY_LIGHTING, 0);
  pt = p->GetPolyTexType ();
  pt->SetMixMode (CS_FX_ADD);

  walls_state->DecRef ();
  walls->DecRef ();
  //====================================================================

  iStatLight* light;
  light = engine->CreateLight (NULL, csVector3 (-500, 300, 900), 1000000,
  	csColor (1, 1, 1), false);
  iLight* il = SCF_QUERY_INTERFACE (light, iLight);
  iFlareHalo* flare = il->CreateFlareHalo ();
  iMaterialWrapper* ifmc = engine->GetMaterialList ()->
  	FindByName ("flare_center");
  iMaterialWrapper* ifm1 = engine->GetMaterialList ()->
  	FindByName ("flare_spark1");
  iMaterialWrapper* ifm2 = engine->GetMaterialList ()->
  	FindByName ("flare_spark2");
  iMaterialWrapper* ifm3 = engine->GetMaterialList ()->
  	FindByName ("flare_spark3");
  iMaterialWrapper* ifm4 = engine->GetMaterialList ()->
  	FindByName ("flare_spark4");
  iMaterialWrapper* ifm5 = engine->GetMaterialList ()->
  	FindByName ("flare_spark5");
  flare->AddComponent (0.0f, 1.2f, 1.2f, CS_FX_ADD, ifmc); // pos, w, h, mixmode
  flare->AddComponent (0.3f, 0.1f, 0.1f, CS_FX_ADD, ifm3);
  flare->AddComponent (0.6f, 0.4f, 0.4f, CS_FX_ADD, ifm4);
  flare->AddComponent (0.8f, 0.05f, 0.05f, CS_FX_ADD, ifm5);
  flare->AddComponent (1.0f, 0.7f, 0.7f, CS_FX_ADD, ifm1);
  flare->AddComponent (1.3f, 0.1f, 0.1f, CS_FX_ADD, ifm3);
  flare->AddComponent (1.5f, 0.3f, 0.3f, CS_FX_ADD, ifm4);
  flare->AddComponent (1.8f, 0.1f, 0.1f, CS_FX_ADD, ifm5);
  flare->AddComponent (2.0f, 0.5f, 0.5f, CS_FX_ADD, ifm2);
  flare->AddComponent (2.1f, 0.15f, 0.15f, CS_FX_ADD, ifm3);

  flare->AddComponent (2.5f, 0.2f, 0.2f, CS_FX_ADD, ifm3);
  flare->AddComponent (2.8f, 0.4f, 0.4f, CS_FX_ADD, ifm4);
  flare->AddComponent (3.0f, 3.0f, 3.0f, CS_FX_ADD, ifm1);
  flare->AddComponent (3.1f, 0.05f, 0.05f, CS_FX_ADD, ifm5);
  flare->AddComponent (3.3f, 0.15f, 0.15f, CS_FX_ADD, ifm2);

  il->DecRef ();
  room->GetLights ()->Add (light->QueryLight ());
  light->DecRef ();

  //====================================================================
  iMeshFactoryWrapper *mf = engine->CreateMeshFactory(
    "crystalspace.mesh.object.stars", "starFact");
  csVector3 starsmeshposition(0,0,0);
  iMeshWrapper *starbox = engine->CreateMeshWrapper(mf,
    "starbox", room, starsmeshposition);
  //starbox->SetRenderPriority(); for skyboxes...
  starbox->SetZBufMode(CS_ZBUF_NONE);
  starbox->SetRenderPriority (engine->GetRenderPriority ("starLevel2"));
  starbox->GetFlags().Set(CS_ENTITY_NOSHADOWS | CS_ENTITY_NOLIGHTING);
    //CS_ENTITY_CAMERA | CS_ENTITY_NOSHADOWS |
    //CS_ENTITY_NOLIGHTING);
  iStarsState *starstate = SCF_QUERY_INTERFACE( starbox->GetMeshObject(),
    iStarsState);
  starstate->SetBox(csBox3(-500, -500, -500, 500, 500, 500));
  starstate->SetColor(csColor(0.9f, 0.9f, 1.0f));
  starstate->SetDensity(0.0001f);
  starstate->SetMaxDistance(100.0f);
  starstate->SetMaxColor( csColor(0.0f, 0.0f, 0.0f) );
  starstate->DecRef();
  mf->DecRef();
  starbox->DecRef();

}

void Demo::SetupObjects ()
{
  iBallState* bs;

  // Create saturn.
  iMeshWrapper* sat = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ball_factory"), "Saturn",
  	NULL, csVector3 (0));
  sat->SetRenderPriority (engine->GetRenderPriority ("object"));
  sat->SetZBufMode (CS_ZBUF_USE);
  bs = SCF_QUERY_INTERFACE (sat->GetMeshObject (), iBallState);
  bs->SetRadius (50, 50, 50);
  bs->SetMaterialWrapper (engine->GetMaterialList ()->
  	FindByName ("saturn"));
  bs->SetCylindricalMapping (true);
  bs->SetRimVertices (16);
  sat->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  bs->DecRef ();
  sat->DecRef ();

  // Create jupiter.
  iMeshWrapper* jup = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ball_factory"), "Jupiter",
  	NULL, csVector3 (0));
  jup->SetRenderPriority (engine->GetRenderPriority ("object"));
  jup->SetZBufMode (CS_ZBUF_USE);
  bs = SCF_QUERY_INTERFACE (jup->GetMeshObject (), iBallState);
  bs->SetRadius (50, 50, 50);
  bs->SetMaterialWrapper (engine->GetMaterialList ()->
  	FindByName ("jupiter"));
  bs->SetCylindricalMapping (true);
  bs->SetRimVertices (16);
  jup->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  bs->DecRef ();
  jup->DecRef ();

  // Create the earth.
  iMeshWrapper* earth = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ball_factory"), "Earth",
  	NULL, csVector3 (0));
  earth->SetRenderPriority (engine->GetRenderPriority ("object"));
  earth->SetZBufMode (CS_ZBUF_USE);
  bs = SCF_QUERY_INTERFACE (earth->GetMeshObject (), iBallState);
  bs->SetRadius (25, 25, 25);
  bs->SetMaterialWrapper (engine->GetMaterialList ()->
  	FindByName ("earth"));
  bs->SetCylindricalMapping (true);
  bs->SetRimVertices (16);
  earth->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  bs->DecRef ();
  earth->DecRef ();

  // Create the clouds for earth.
  iMeshWrapper* clouds = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ball_factory"), "Clouds",
  	NULL, csVector3 (0));
  clouds->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  clouds->SetZBufMode (CS_ZBUF_TEST);
  bs = SCF_QUERY_INTERFACE (clouds->GetMeshObject (), iBallState);
  bs->SetRadius (27.5, 27.5, 27.5);
  bs->SetMaterialWrapper (engine->GetMaterialList ()->
  	FindByName ("earthclouds"));
  bs->SetCylindricalMapping (true);
  bs->SetRimVertices (16);
  bs->SetMixMode (CS_FX_ADD);
  clouds->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  bs->DecRef ();
  clouds->DecRef ();

  // Create fighters.
  iMeshWrapper* spr3d;
  iSprite3DState* s3d;
  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("fighter"), "Fighter1",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);
  spr3d->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetBaseColor (csColor (0.15f, 0.15f, 0.15f));
  s3d->DecRef ();

  iMeshWrapper* tail = LoadObject ("FighterTail1",
  	"/data/demo/objects/fightertail",
  	"crystalspace.mesh.object.fire",
	"crystalspace.mesh.loader.fire",
	NULL, csVector3 (0));
  tail->SetZBufMode (CS_ZBUF_TEST);
  spr3d->GetChildren ()->Add (tail);
  tail->DecRef ();
  spr3d->DecRef ();

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("fighter"), "Fighter2",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);
  spr3d->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetBaseColor (csColor (0.15f, 0.15f, 0.15f));
  s3d->DecRef ();

  tail = LoadObject ("FighterTail2",
  	"/data/demo/objects/fightertail",
  	"crystalspace.mesh.object.fire",
	"crystalspace.mesh.loader.fire",
	NULL, csVector3 (0));
  tail->SetZBufMode (CS_ZBUF_TEST);
  spr3d->GetChildren ()->Add (tail);
  tail->DecRef ();

  spr3d->DecRef ();

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("shuttle"), "Shuttle",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);
  spr3d->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetBaseColor (csColor (0.15f, 0.15f, 0.15f));
  s3d->DecRef ();

  tail = LoadObject ("ShuttleTail",
  	"/data/demo/objects/shuttletail",
  	"crystalspace.mesh.object.fire",
	"crystalspace.mesh.loader.fire",
	NULL, csVector3 (0));
  tail->SetZBufMode (CS_ZBUF_TEST);
  spr3d->GetChildren ()->Add (tail);
  tail->DecRef ();

  spr3d->DecRef ();

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("th_ship"), "Shuttle2",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);
  spr3d->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetBaseColor (csColor (0.15f, 0.15f, 0.15f));
  s3d->DecRef ();

  tail = LoadObject ("ShuttleTail2",
  	"/data/demo/objects/shuttletail2",
  	"crystalspace.mesh.object.fire",
	"crystalspace.mesh.loader.fire",
	NULL, csVector3 (0));
  tail->SetZBufMode (CS_ZBUF_TEST);
  spr3d->GetChildren ()->Add (tail);
  tail->DecRef ();

  spr3d->DecRef ();

  // Create laser.
  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("laser"), "LaserBeam1",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  spr3d->SetZBufMode (CS_ZBUF_TEST);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetMixMode (CS_FX_ADD);
  s3d->SetLighting (false);
  s3d->SetBaseColor (csColor (0.1f, 0.1f, 1.0f));
  s3d->DecRef ();
  spr3d->DecRef ();

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("laser"), "LaserBeam2",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  spr3d->SetZBufMode (CS_ZBUF_TEST);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetMixMode (CS_FX_ADD);
  s3d->SetLighting (false);
  s3d->SetBaseColor (csColor (0.1f, 0.1f, 1.0f));
  s3d->DecRef ();
  spr3d->DecRef ();

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("laser"), "LaserBeam3",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  spr3d->SetZBufMode (CS_ZBUF_TEST);
  s3d = SCF_QUERY_INTERFACE (spr3d->GetMeshObject (), iSprite3DState);
  s3d->SetMixMode (CS_FX_ADD);
  s3d->SetLighting (false);
  s3d->SetBaseColor (csColor (0.1f, 0.1f, 1.0f));
  s3d->DecRef ();
  spr3d->DecRef ();

  //=====
  // Setup the space station.
  //=====

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("station"), "Station2",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);
  spr3d->DecRef ();

  //=====
  // Setup the space station.
  //=====

  spr3d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_dummy"), "Station1",
  	NULL, csVector3 (0));
  spr3d->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d->SetZBufMode (CS_ZBUF_USE);

  iMeshWrapper* spr3d_tower = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_tower"), "SS1_Tower",
  	NULL, csVector3 (0));
  spr3d_tower->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_tower->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_tower);
  spr3d_tower->DecRef ();

  iMeshWrapper* spr3d_spoke = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_spoke"), "SS1_Spoke",
  	NULL, csVector3 (0));
  spr3d_spoke->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_spoke->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_spoke);
  spr3d_spoke->DecRef ();

  iMeshWrapper* spr3d_dome = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_dome"), "SS1_Dome",
  	NULL, csVector3 (0));
  spr3d_dome->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_dome->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_dome);
  spr3d_dome->DecRef ();

  iMeshWrapper* spr3d_tail = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_tail"), "SS1_Tail",
  	NULL, csVector3 (0));
  spr3d_tail->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_tail->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_tail);
  spr3d_tail->DecRef ();

  iMeshWrapper* spr3d_arm = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_arm1"), "SS1_Arm1",
  	NULL, csVector3 (0));
  spr3d_arm->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_arm->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_arm);
  spr3d_arm->DecRef ();

  spr3d_arm = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_arm1"), "SS1_Arm1",
  	NULL, csVector3 (0));
  spr3d_arm->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_arm->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_arm);
  spr3d_arm->GetMovable ()->GetTransform ().RotateThis (csVector3 (0, 1, 0),
  	HALF_PI);
  spr3d_arm->GetMovable ()->UpdateMove ();
  spr3d_arm->DecRef ();

  iMeshWrapper* spr3d_pod1 = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("ss1_pod1"), "SS1_Pod1",
  	NULL, csVector3 (0));
  spr3d_pod1->SetRenderPriority (engine->GetRenderPriority ("object"));
  spr3d_pod1->SetZBufMode (CS_ZBUF_USE);
  spr3d->GetChildren ()->Add (spr3d_pod1);
  spr3d_pod1->DecRef ();
  spr3d->DecRef ();

  //=====

  iMeshWrapper* spr2d;
  iSprite2DState* s2d;
  iParticle* part;
  spr2d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("photonTorpedo"), "PhotonTorpedo1",
  	NULL, csVector3 (0));
  spr2d->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  spr2d->SetZBufMode (CS_ZBUF_TEST);
  s2d = SCF_QUERY_INTERFACE (spr2d->GetMeshObject (), iSprite2DState);
  s2d->CreateRegularVertices (4, true);
  s2d->DecRef ();

  part = SCF_QUERY_INTERFACE (spr2d->GetMeshObject (), iParticle);
  part->ScaleBy (3);
  part->DecRef ();

  spr2d->DecRef ();

  spr2d = engine->CreateMeshWrapper (
  	engine->GetMeshFactories ()->FindByName ("photonTorpedo"), "PhotonTorpedo2",
  	NULL, csVector3 (0));
  spr2d->SetRenderPriority (engine->GetRenderPriority ("alpha"));
  spr2d->SetZBufMode (CS_ZBUF_TEST);
  s2d = SCF_QUERY_INTERFACE (spr2d->GetMeshObject (), iSprite2DState);
  s2d->CreateRegularVertices (4, true);
  s2d->DecRef ();

  part = SCF_QUERY_INTERFACE (spr2d->GetMeshObject (), iParticle);
  part->ScaleBy (3);
  part->DecRef ();

  spr2d->DecRef ();

}

static bool DemoEventHandler (iEvent& ev)
{
  if (ev.Type == csevBroadcast && ev.Command.Code == cscmdProcess)
  {
    System->SetupFrame ();
    return true;
  }
  else if (ev.Type == csevBroadcast && ev.Command.Code == cscmdFinalProcess)
  {
    System->FinishFrame ();
    return true;
  }
  else
  {
    return System ? System->DemoHandleEvent (ev) : false;
  }
}

bool Demo::Initialize (int argc, const char* const argv[],
  const char *iConfigName)
{
  object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg) return false;

  if (!csInitializer::SetupConfigManager (object_reg, iConfigName))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't initialize app!");
    return false;
  }

  if (!csInitializer::RequestPlugins (object_reg, CS_REQUEST_END))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't initialize app!");
    return false;
  }

  if (!csInitializer::SetupEventHandler (object_reg, DemoEventHandler))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't initialize event handler!");
    return false;
  }

  // Check for commandline help.
  if (csCommandLineHelper::CheckHelp (object_reg))
  {
    csCommandLineHelper::Help (object_reg);
    exit (0);
  }

  // The virtual clock.
  vc = CS_QUERY_REGISTRY (object_reg, iVirtualClock);

  kbd = CS_QUERY_REGISTRY (object_reg, iKeyboardDriver);
  if (!kbd)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No keyboard driver!");
    return false;
  }

  engine = CS_QUERY_REGISTRY (object_reg, iEngine);
  if (!engine)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No engine!");
    return false;
  }

  loader = CS_QUERY_REGISTRY (object_reg, iLoader);
  if (!loader)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No loader!");
    return false;
  }

  myG3D = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  if (!myG3D)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No 3D driver!");
    return false;
  }

  myG2D = CS_QUERY_REGISTRY (object_reg, iGraphics2D);
  if (!myG2D)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No 2D driver!");
    return false;
  }

  myVFS = CS_QUERY_REGISTRY (object_reg, iVFS);
  if (!myVFS)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No VFS!");
    return false;
  }

  // Open the main system. This will open all the previously loaded plug-ins.
  iNativeWindow* nw = myG2D->GetNativeWindow ();
  if (nw) nw->SetTitle ("The Crystal Space Demo.");
  if (!csInitializer::OpenApplication (object_reg))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Error opening system!");
    Cleanup ();
    exit (1);
  }

  // Setup the texture manager
  iTextureManager* txtmgr = myG3D->GetTextureManager ();
  txtmgr->SetVerbose (true);

  // Initialize the texture manager
  txtmgr->ResetPalette ();
  txtmgr->SetPalette ();

  font = myG2D->GetFontServer ()->LoadFont (CSFONT_LARGE);

  // Some commercials...
  Report (CS_REPORTER_SEVERITY_NOTIFY,
    "The Crystal Space Demo.");

  // First disable the lighting cache. Our app is simple enough
  // not to need this.
  engine->SetLightingCacheMode (0);

  // Create our world.
  Report (CS_REPORTER_SEVERITY_NOTIFY, "Creating world!...");

  engine->RegisterRenderPriority ("starLevel1", 1);
  engine->RegisterRenderPriority ("starLevel2", 2);
  engine->RegisterRenderPriority ("object", 3);
  engine->RegisterRenderPriority ("alpha", 4);

  if (!loader->LoadLibraryFile ("/data/demo/library"))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "There was an error loading library!");
    exit (0);
  }

  SetupSector ();
  seqmgr = new DemoSequenceManager (this);
  SetupObjects ();
  seqmgr->Setup ("/data/demo/sequences");

  engine->Prepare ();

  Report (CS_REPORTER_SEVERITY_NOTIFY, "--------------------------------------");

  view = new csView (engine, myG3D);
  view->GetCamera ()->SetSector (room);
  view->GetCamera ()->GetTransform ().SetOrigin (csVector3 (0.0f, 0.0f, -900.0f));
  view->GetCamera ()->GetTransform ().RotateThis (csVector3 (0.0f, 1.0f, 0.0f), 0.8f);
  view->SetRectangle (0, 0, myG2D->GetWidth (), myG2D->GetHeight ());

  txtmgr->SetPalette ();
  col_red = txtmgr->FindRGB (255, 0, 0);
  col_blue = txtmgr->FindRGB (0, 0, 255);
  col_white = txtmgr->FindRGB (255, 255, 255);
  col_gray = txtmgr->FindRGB (50, 50, 50);
  col_black = txtmgr->FindRGB (0, 0, 0);
  col_yellow = txtmgr->FindRGB (255, 255, 0);
  col_cyan = txtmgr->FindRGB (0, 255, 255);
  col_green = txtmgr->FindRGB (0, 255, 0);

  return true;
}

#define MAP_OFF 0
#define MAP_OVERLAY 1
#define MAP_EDIT 2
#define MAP_EDIT_FORWARD 3
static int map_enabled = MAP_OFF;
static csVector2 map_tl (-1000, 1000);
static csVector2 map_br (1000, -1000);
static int map_selpoint = 0;
static char map_selpath[255] = { 0 };

void Demo::GfxWrite (int x, int y, int fg, int bg, char *str, ...)
{
  va_list arg;
  char buf[256];

  va_start (arg, str);
  vsprintf (buf, str, arg);
  va_end (arg);

  myG2D->Write (font, x, y, fg, bg, buf);
}

void Demo::FileWrite (iFile* file, char *str, ...)
{
  va_list arg;
  char buf[256];

  va_start (arg, str);
  vsprintf (buf, str, arg);
  va_end (arg);

  file->Write (buf, strlen (buf));
}

void Demo::ShowMessage (const char* msg, ...)
{
  message_error = false;
  va_list arg;
  va_start (arg, msg);
  vsprintf (message, msg, arg);
  va_end (arg);
  message_timer = csGetTicks () + 1500;
}

void Demo::ShowError (const char* msg, ...)
{
  message_error = true;
  va_list arg;
  va_start (arg, msg);
  vsprintf (message, msg, arg);
  va_end (arg);
  message_timer = csGetTicks () + 1500;
}

void Demo::SetupFrame ()
{
  csTicks elapsed_time, current_time;
  elapsed_time = vc->GetElapsedTicks ();
  current_time = vc->GetCurrentTicks ();

  // since no time has passed, the animated screen image stays the same.
  // avoid drawing this, it will only fill up queues and cause jerky
  // movement on some hardware/drivers.
  if(elapsed_time == 0) return;

  // Now rotate the camera according to keyboard state
  csReversibleTransform& camtrans = view->GetCamera ()->GetTransform ();
  if (map_enabled < MAP_EDIT)
  {
    float speed = (elapsed_time / 1000.0f) * (0.03f * 20.0f);
    if (kbd->GetKeyState (CSKEY_RIGHT))
      camtrans.RotateThis (CS_VEC_ROT_RIGHT, speed);
    if (kbd->GetKeyState (CSKEY_LEFT))
      camtrans.RotateThis (CS_VEC_ROT_LEFT, speed);
    if (kbd->GetKeyState (CSKEY_PGUP))
      camtrans.RotateThis (CS_VEC_TILT_UP, speed);
    if (kbd->GetKeyState (CSKEY_PGDN))
      camtrans.RotateThis (CS_VEC_TILT_DOWN, speed);
    if (kbd->GetKeyState (CSKEY_UP))
      view->GetCamera ()->Move (CS_VEC_FORWARD * 400.0f * speed);
    if (kbd->GetKeyState (CSKEY_DOWN))
      view->GetCamera ()->Move (CS_VEC_BACKWARD * 400.0f * speed);
  }

  if (map_enabled < MAP_EDIT_FORWARD)
    seqmgr->ControlPaths (view->GetCamera (), elapsed_time);
  else if (map_enabled == MAP_EDIT_FORWARD)
  {
    csTicks debug_time;
    csTicks start, total;
    csNamedPath* np = seqmgr->GetSelectedPath (map_selpath, start, total);
    if (np)
    {
      float r = np->GetTimeValue (map_selpoint);
      np->Calculate (r);
      debug_time = csTicks (start + total * r);
    }
    else
      debug_time = 0;	// Not possible!
    seqmgr->DebugPositionObjects (view->GetCamera (), debug_time);
  }

  if (map_enabled == MAP_EDIT_FORWARD)
  {
    csNamedPath* np = seqmgr->GetSelectedPath (map_selpath);
    if (np)
    {
      float r = np->GetTimeValue (map_selpoint);
      np->Calculate (r);
      csVector3 pos, up, forward;
      np->GetInterpolatedPosition (pos);
      np->GetInterpolatedUp (up);
      np->GetInterpolatedForward (forward);
      view->GetCamera ()->GetTransform ().SetOrigin (pos);
      view->GetCamera ()->GetTransform ().LookAt (forward.Unit (), up.Unit ());
    }
  }

  // Tell 3D driver we're going to display 3D things.
  if (!myG3D->BeginDraw (engine->GetBeginDrawFlags () | CSDRAW_3DGRAPHICS
  	| CSDRAW_CLEARZBUFFER))
    return;

  if (map_enabled != MAP_EDIT)
  {
    view->Draw ();
    seqmgr->Draw3DEffects (myG3D);
    if (map_enabled == MAP_EDIT_FORWARD)
      csfxFadeToColor (myG3D, 0.3f, csColor (0.0f, 0.0f, 1.0f));
  }

  // Start drawing 2D graphics.
  if (!myG3D->BeginDraw (CSDRAW_2DGRAPHICS)) return;

  if (map_enabled == MAP_EDIT)
    myG2D->Clear (0);
  else if (map_enabled < MAP_EDIT)
    seqmgr->Draw2DEffects (myG2D);
  if (map_enabled >= MAP_OVERLAY)
    seqmgr->DebugDrawPaths (view->GetCamera (), map_selpath,
    	map_tl, map_br, map_selpoint);
  if (map_enabled == MAP_EDIT)
    DrawEditInfo ();

  int fw, fh;
  font->GetMaxSize (fw, fh);
  int tx = 10;
  int ty = myG2D->GetHeight ()-fh-3;
  char messageLine[100];
  messageLine[0] = 0;
  switch (map_enabled)
  {
    case MAP_OFF:
      if (seqmgr->IsSuspended ())
        GfxWrite (tx, ty, col_black, col_white, "[PAUSED]");
      break;
    case MAP_OVERLAY:
      GfxWrite (tx, ty, col_black, col_white, "%sOverlay (%s)",
        seqmgr->IsSuspended () ? "[PAUSED] " : "",
      	map_selpath);
      break;
    case MAP_EDIT:
      GfxWrite (tx, ty, col_black, col_white, "Edit (%s)",
      	map_selpath);
      break;
    case MAP_EDIT_FORWARD:
      GfxWrite (tx, ty, col_black, col_white, "Forward/Up (%s)",
      	map_selpath);
      break;
  }

  if (message[0])
  {
    GfxWrite (10, 10, col_black, message_error ? col_red : col_white, message);
    if (current_time > message_timer) message[0] = 0;
  }
}

void Demo::FinishFrame ()
{
  myG3D->FinishDraw ();
  myG3D->Print (NULL);
}

void Demo::DrawEditInfo ()
{
  int fw, fh;
  font->GetMaxSize (fw, fh);
  fh += 2;
  int dim = myG2D->GetHeight ()-10;
  myG2D->DrawBox (dim+5, 0, myG2D->GetWidth ()-dim-5,
  	myG2D->GetHeight (), col_white);
  csTicks start, total;
  csNamedPath* np = seqmgr->GetSelectedPath (map_selpath, start, total);
  if (np)
  {
    int ww = dim+10;
    int hh = 10;
    GfxWrite (ww, hh, col_black, col_white, "Point %d", map_selpoint); hh += fh;
    csVector3 v, fwd, up;
    np->GetPositionVector (map_selpoint, v);
    np->GetForwardVector (map_selpoint, fwd);
    np->GetUpVector (map_selpoint, up);
    GfxWrite (ww, hh, col_black, col_white, "P(%g,%g,%g)",
    	v.x, v.y, v.z); hh += fh;
    GfxWrite (ww, hh, col_black, col_white, "F(%.2g,%.2g,%.2g)",
    	fwd.x, fwd.y, fwd.z); hh += fh;
    GfxWrite (ww, hh, col_black, col_white, "U(%.2g,%.2g,%.2g)",
    	up.x, up.y, up.z); hh += fh;
    float t = np->GetTimeValue (map_selpoint);
    csTicks tms = int (t*total);
    GfxWrite (ww, hh, col_black, col_white, "tot time %d ms", total); hh += fh;
    GfxWrite (ww, hh, col_black, col_white, "rel time %d ms", tms); hh += fh;
    GfxWrite (ww, hh, col_black, col_white, "Left Path Info:"); hh += fh;
    if (map_selpoint > 0)
    {
      csVector3 v1;
      np->GetPositionVector (map_selpoint-1, v1);
      float d = qsqrt (csSquaredDist::PointPoint (v, v1));
      float t1 = np->GetTimeValue (map_selpoint-1);
      float dr = t-t1;
      float speed = (float) fabs (dr) / d;
      csTicks tms1 = int (t1*total);
      GfxWrite (ww+20, hh, col_black, col_white, "len %g", d); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "dr %g", dr); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "speed %g", speed); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "rel time %d ms",
      	tms-tms1); hh += fh;
    }
    GfxWrite (ww, hh, col_black, col_white, "Right Path Info:"); hh += fh;
    if (map_selpoint < np->GetPointCount ()-1)
    {
      csVector3 v1;
      np->GetPositionVector (map_selpoint+1, v1);
      float t1 = np->GetTimeValue (map_selpoint+1);
      float dr = t1-t;
      float d = qsqrt (csSquaredDist::PointPoint (v, v1));
      float speed = (float) fabs (dr) / d;
      csTicks tms1 = int (t1*total);
      GfxWrite (ww+20, hh, col_black, col_white, "len %g", d); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "dr %g", dr); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "speed %g", speed); hh += fh;
      GfxWrite (ww+20, hh, col_black, col_white, "rel time %d ms",
      	tms1-tms); hh += fh;
    }
  }
}

bool Demo::DemoHandleEvent (iEvent &Event)
{
  if (Event.Type == csevKeyDown)
  {
    csTicks elapsed_time, current_time;
    elapsed_time = vc->GetElapsedTicks ();
    current_time = vc->GetCurrentTicks ();
    bool shift = (Event.Key.Modifiers & CSMASK_SHIFT) != 0;
    bool alt = (Event.Key.Modifiers & CSMASK_ALT) != 0;
    bool ctrl = (Event.Key.Modifiers & CSMASK_CTRL) != 0;

    if (map_enabled == MAP_EDIT_FORWARD)
    {
      //==============================
      // Handle keys in path_edit_forward mode.
      //==============================
      csNamedPath* np = seqmgr->GetSelectedPath (map_selpath);
      if (np)
      {
        float dx = map_br.x - map_tl.x;
        float speed;
        if (shift) speed = dx / 20.0f;
        else if (ctrl) speed = dx / 600.0f;
        else speed = dx / 100.0f;
        if (Event.Key.Code == CSKEY_UP)
        {
          csVector3 v;
	  np->GetPositionVector (map_selpoint, v);
          v.y += speed;
	  np->SetPositionVector (map_selpoint, v);
	  ShowMessage ("Y location set at '%g'", v.y);
          return true;
        }
        if (Event.Key.Code == CSKEY_DOWN)
        {
          csVector3 v;
	  np->GetPositionVector (map_selpoint, v);
          v.y -= speed;
	  np->SetPositionVector (map_selpoint, v);
	  ShowMessage ("Y location set at '%g'", v.y);
          return true;
        }
        if (Event.Key.Code == CSKEY_LEFT)
        {
          csVector3 up, forward;
	  np->GetUpVector (map_selpoint, up);
	  np->GetForwardVector (map_selpoint, forward);
	  csReversibleTransform trans = view->GetCamera ()->GetTransform ();
          trans.LookAt (forward.Unit (), up.Unit ());
	  trans.RotateThis (csVector3 (0.0f, 0.0f, 1.0f), -0.1f);
	  up = trans.This2Other (csVector3 (0.0f, 1.0f, 0.0f)) - trans.GetOrigin ();
	  np->SetUpVector (map_selpoint, up);
	  ShowMessage ("Up vector set at '%.3g,%.3g,%.3g'", up.x, up.y, up.z);
          return true;
        }
        if (Event.Key.Code == CSKEY_RIGHT)
        {
          csVector3 up, forward;
	  np->GetUpVector (map_selpoint, up);
	  np->GetForwardVector (map_selpoint, forward);
	  csReversibleTransform trans = view->GetCamera ()->GetTransform ();
          trans.LookAt (forward.Unit (), up.Unit ());
	  trans.RotateThis (csVector3 (0.0f, 0.0f, 1.0f), 0.1f);
	  up = trans.This2Other (csVector3 (0.0f, 1.0f, 0.0f)) - trans.GetOrigin ();
	  np->SetUpVector (map_selpoint, up);
	  ShowMessage ("Up vector set at '%.3g,%.3g,%.3g'", up.x, up.y, up.z);
          return true;
        }
      }
      switch (Event.Key.Char)
      {
        case 'c':
          map_enabled = MAP_EDIT;
	  return true;
	case 'y':
	  // Average the 'y' of this point so that it is on a line
	  // with the previous and next point.
	  // Make the forward vector look along the path. i.e. let it look
	  // to an average direction as specified by next and previous point.
	  if (map_selpoint <= 0 || map_selpoint >= np->GetPointCount ()-1)
	  {
	    ShowMessage ("The 'y' operation can't work on this point!\n");
	  }
	  else
	  {
            csVector3 v1, v2, v3;
	    np->GetPositionVector (map_selpoint-1, v1);
	    np->GetPositionVector (map_selpoint, v2);
	    np->GetPositionVector (map_selpoint+1, v3);
	    if (ABS (v1.x-v3.x) > ABS (v1.z-v3.z))
	      v2.y = v1.y + (v3.y-v1.y) * (v1.x-v2.x) / (v1.x-v3.x);
	    else
	      v2.y = v1.y + (v3.y-v1.y) * (v1.z-v2.z) / (v1.z-v3.z);
	    ShowMessage ("Y location set at '%g'", v2.y);
	    np->SetPositionVector (map_selpoint, v2);
	  }
	  break;
	case '0':
	  // Let the up vector point really upwards.
	  {
	    csVector3 forward;
	    np->GetForwardVector (map_selpoint, forward);
            csVector3 up;
	    up = csVector3 (0, 1, 0) % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  break;
	case CSKEY_BACKSPACE:
	  // Change direction of the forward vector.
	  {
	    csVector3 forward;
	    np->GetForwardVector (map_selpoint, forward);
	    np->SetForwardVector (map_selpoint, -forward);
	  }
	  break;
        case '=':
	  // Make the forward vector look along the path. i.e. let it look
	  // to an average direction as specified by next and previous point.
	  if (map_selpoint <= 0 || map_selpoint >= np->GetPointCount ()-1)
	  {
	    ShowMessage ("The '=' operation can't work on this point!\n");
	  }
	  else
	  {
            csVector3 v1, v2;
	    np->GetPositionVector (map_selpoint-1, v1);
	    np->GetPositionVector (map_selpoint+1, v2);
	    csVector3 forward = (v2-v1).Unit ();
	    np->SetForwardVector (map_selpoint, forward);
            csVector3 up;
	    np->GetUpVector (map_selpoint, up);
	    up = up % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  break;
        case '-':
	  // Make the forward vector look along the path. i.e. let it look
	  // backward to the previous point in the path if there is one.
	  if (map_selpoint <= 0)
	  {
            csVector3 v1, v2;
	    np->GetPositionVector (map_selpoint+1, v1);
	    np->GetPositionVector (map_selpoint, v2);
	    csVector3 forward = (v2-v1).Unit ();
	    np->SetForwardVector (map_selpoint, forward);
            csVector3 up;
	    np->GetUpVector (map_selpoint, up);
	    up = up % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  else
	  {
            csVector3 v1, v2;
	    np->GetPositionVector (map_selpoint, v1);
	    np->GetPositionVector (map_selpoint-1, v2);
	    csVector3 forward = (v2-v1).Unit ();
	    np->SetForwardVector (map_selpoint, forward);
            csVector3 up;
	    np->GetUpVector (map_selpoint, up);
	    up = up % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  break;
        case '+':
	  // Make the forward vector look along the path. i.e. let it look
	  // to the next point in the path if there is one.
	  if (map_selpoint >= np->GetPointCount ()-1)
	  {
            csVector3 v1, v2;
	    np->GetPositionVector (map_selpoint-1, v1);
	    np->GetPositionVector (map_selpoint, v2);
	    csVector3 forward = (v2-v1).Unit ();
	    np->SetForwardVector (map_selpoint, forward);
            csVector3 up;
	    np->GetUpVector (map_selpoint, up);
	    up = up % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  else
	  {
            csVector3 v1, v2;
	    np->GetPositionVector (map_selpoint, v1);
	    np->GetPositionVector (map_selpoint+1, v2);
	    csVector3 forward = (v2-v1).Unit ();
	    np->SetForwardVector (map_selpoint, forward);
            csVector3 up;
	    np->GetUpVector (map_selpoint, up);
	    up = up % forward;
	    up = - (up % forward);
	    np->SetUpVector (map_selpoint, up);
	  }
	  break;
      }
    }
    else if (map_enabled == MAP_EDIT)
    {
      //==============================
      // Handle keys in path editing mode.
      //==============================
      csNamedPath* np = seqmgr->GetSelectedPath (map_selpath);
      float dx = map_br.x - map_tl.x;
      float dy = map_br.y - map_tl.y;
      float speed;
      if (shift) speed = dx / 20.0f;
      else if (ctrl) speed = dx / 600.0f;
      else speed = dx / 100.0f;
      if (np)
      {
        if (Event.Key.Code == CSKEY_UP)
        {
	  if (alt)
	  {
	    map_tl.y -= dy / 10.0f;
	    map_br.y -= dy / 10.0f;
	  }
	  else
	  {
            csVector3 v;
	    np->GetPositionVector (map_selpoint, v);
            v.z += speed;
	    np->SetPositionVector (map_selpoint, v);
	  }
          return true;
        }
        if (Event.Key.Code == CSKEY_DOWN)
        {
	  if (alt)
	  {
	    map_tl.y += dy / 10.0f;
	    map_br.y += dy / 10.0f;
	  }
	  else
	  {
            csVector3 v;
	    np->GetPositionVector (map_selpoint, v);
            v.z -= speed;
	    np->SetPositionVector (map_selpoint, v);
	  }
          return true;
        }
        if (Event.Key.Code == CSKEY_LEFT)
        {
	  if (alt)
	  {
	    map_tl.x -= dx / 10.0f;
	    map_br.x -= dx / 10.0f;
	  }
	  else
	  {
            csVector3 v;
	    np->GetPositionVector (map_selpoint, v);
            v.x -= speed;
	    np->SetPositionVector (map_selpoint, v);
	  }
          return true;
        }
        if (Event.Key.Code == CSKEY_RIGHT)
        {
	  if (alt)
	  {
	    map_tl.x += dx / 10.0f;
	    map_br.x += dx / 10.0f;
	  }
	  else
	  {
            csVector3 v;
	    np->GetPositionVector (map_selpoint, v);
            v.x += speed;
	    np->SetPositionVector (map_selpoint, v);
	  }
          return true;
        }
      }
      switch (Event.Key.Char)
      {
	case 'm':
          map_enabled = MAP_OFF;
          return true;
	case 's':
	  if (np)
	  {
	    char buf[200], backup[200];
	    strcpy (buf, "/data/demo/paths/");
	    strcat (buf, np->GetName ());
	    // Make a backup of the original file.
	    strcpy (backup, buf);
	    strcat (backup, ".bak");
	    myVFS->DeleteFile (backup);
	    iDataBuffer* dbuf = myVFS->ReadFile (buf);
	    if (dbuf)
	    {
	      if (dbuf->GetSize ())
	        myVFS->WriteFile (backup, **dbuf, dbuf->GetSize ());
	      dbuf->DecRef ();
	    }

	    iFile* fp = myVFS->Open (buf, VFS_FILE_WRITE);
	    if (fp)
	    {
	      int i, num = np->GetPointCount ();
	      FileWrite (fp, "    NUM (%d)\n", num);
	      float* t = np->GetTimeValues ();
	      FileWrite (fp, "    TIMES (%g", t[0]);
	      for (i = 1 ; i < num ; i++)
	        FileWrite (fp, ",%g", t[i]);
	      FileWrite (fp, ")\n");
	      FileWrite (fp, "    POS (\n");
	      for (i = 0 ; i < num ; i++)
	      {
	        csVector3 v;
		np->GetPositionVector (i, v);
	        FileWrite (fp, "      V (%g,%g,%g)\n", v.x, v.y, v.z);
	      }
	      FileWrite (fp, "    )\n");
	      FileWrite (fp, "    FORWARD (\n");
	      for (i = 0 ; i < num ; i++)
	      {
	        csVector3 v;
		np->GetForwardVector (i, v);
	        FileWrite (fp, "      V (%g,%g,%g)\n", v.x, v.y, v.z);
	      }
	      FileWrite (fp, "    )\n");
	      FileWrite (fp, "    UP (\n");
	      for (i = 0 ; i < num ; i++)
	      {
	        csVector3 v;
		np->GetUpVector (i, v);
	        FileWrite (fp, "      V (%g,%g,%g)\n", v.x, v.y, v.z);
	      }
	      FileWrite (fp, "    )\n");
	      fp->DecRef ();
	      ShowMessage ("Wrote path to file '%s'", buf);
	    }
	    else
	      ShowError ("Error writing to file '%s'!", buf);
	  }
	  break;
        case 'i':
	  if (np)
	  {
	    np->InsertPoint (map_selpoint);
	    map_selpoint++;
	    if (map_selpoint == np->GetPointCount ()-1)
	    {
	      csVector3 v;
	      np->GetPositionVector (map_selpoint-1, v);
	      np->SetPositionVector (map_selpoint, v);
	      np->GetUpVector (map_selpoint-1, v);
	      np->SetUpVector (map_selpoint, v);
	      np->GetForwardVector (map_selpoint-1, v);
	      np->SetForwardVector (map_selpoint, v);
	      np->SetTimeValue (map_selpoint,
	    	  np->GetTimeValue (map_selpoint-1));
	    }
	    else
	    {
	      csVector3 v1, v2;
	      np->GetPositionVector (map_selpoint-1, v1);
	      np->GetPositionVector (map_selpoint+1, v2);
	      np->SetPositionVector (map_selpoint, (v1+v2)/2.);
	      np->GetUpVector (map_selpoint-1, v1);
	      np->GetUpVector (map_selpoint+1, v2);
	      np->SetUpVector (map_selpoint, (v1+v2)/2.);
	      np->GetForwardVector (map_selpoint-1, v1);
	      np->GetForwardVector (map_selpoint+1, v2);
	      np->SetForwardVector (map_selpoint, (v1+v2)/2.);
	      np->SetTimeValue (map_selpoint,
	    	  (np->GetTimeValue (map_selpoint-1)+
		   np->GetTimeValue (map_selpoint+1)) / 2.0f);
	    }
	  }
          break;
        case 'd':
	  if (np)
	  {
	    np->RemovePoint (map_selpoint);
	    if (map_selpoint >= np->GetPointCount ())
	      map_selpoint--;
	  }
	  break;
	case ',':
	  if (np)
	  {
	    if (map_selpoint > 0 && map_selpoint < np->GetPointCount ()-1)
	    {
	      float t = np->GetTimeValue (map_selpoint);
	      float t1 = np->GetTimeValue (map_selpoint-1);
	      float t2 = np->GetTimeValue (map_selpoint+1);
	      float dt = (t2-t1);
	      if (shift) dt /= 5.;
	      else if (ctrl) dt /= 500.;
	      else dt /= 50.;
	      t -= dt;
	      if (t < t1) t = t1;
	      np->SetTimeValue (map_selpoint, t);
	    }
	  }
	  break;
	case '.':
	  if (np)
	  {
	    if (map_selpoint > 0 && map_selpoint < np->GetPointCount ()-1)
	    {
	      float t = np->GetTimeValue (map_selpoint);
	      float t1 = np->GetTimeValue (map_selpoint-1);
	      float t2 = np->GetTimeValue (map_selpoint+1);
	      float dt = (t2-t1);
	      if (shift) dt /= 5.;
	      else if (ctrl) dt /= 500.;
	      else dt /= 50.;
	      t += dt;
	      if (t > t2) t = t2;
	      np->SetTimeValue (map_selpoint, t);
	    }
	  }
	  break;
	case '/':
	  if (np && map_selpoint > 0 && map_selpoint < np->GetPointCount ()-1)
	  {
	    float t1 = np->GetTimeValue (map_selpoint - 1);
	    float t2 = np->GetTimeValue (map_selpoint + 1);
	    np->SetTimeValue (map_selpoint, (t1+t2) / 2.0f);
	  }
	  break;
	case '?':
	  if (np)
	  {
	    int num = np->GetPointCount ();
	    float* xv, * yv, * zv;
	    xv = np->GetDimensionValues (0);
	    yv = np->GetDimensionValues (1);
	    zv = np->GetDimensionValues (2);
	    csVector3 v0, v1;
	    // Calculate the total length of the path.
	    float totlen = 0;
	    int i;
	    v0.Set (xv[0], yv[0], zv[0]);
	    for (i = 1 ; i < num ; i++)
	    {
	      v1.Set (xv[i], yv[i], zv[i]);
	      float d = qsqrt (csSquaredDist::PointPoint (v0, v1));
	      totlen += d;
	      v0 = v1;
	    }
	    float list[10000];
	    // Calculate the time value for every path segment,
	    // given the total length of the path.
	    v0.Set (xv[0], yv[0], zv[0]);
	    list[0] = 0;
	    float tot = 0;
	    for (i = 1 ; i < num ; i++)
	    {
	      v1.Set (xv[i], yv[i], zv[i]);
	      float d = qsqrt (csSquaredDist::PointPoint (v0, v1));
	      tot += d;
	      list[i] = tot / totlen;
	      v0 = v1;
	    }
	    np->SetTimeValues (list);
	  }
	  break;
        case '>':
	  if (np)
	  {
            map_selpoint++;
	    if (map_selpoint >= np->GetPointCount ())
	      map_selpoint = 0;
	  }
	  break;
        case '<':
	  if (np)
	  {
            map_selpoint--;
	    if (map_selpoint < 0)
	      map_selpoint = np->GetPointCount ()-1;
	  }
	  break;
        case 'c':
	  ShowMessage ("Edit forward and up vectors, press 'c' to exit");
	  map_enabled = MAP_EDIT_FORWARD;
	  break;
        case '+':
	  {
	    float dx = (map_br.x-map_tl.x) / 2.0f;
	    float dy = (map_br.y-map_tl.y) / 2.0f;
	    float cx = (map_br.x+map_tl.x) / 2.0f;
	    float cy = (map_br.y+map_tl.y) / 2.0f;
	    map_tl.x = cx-dx * 0.9f;
	    map_tl.y = cy-dy * 0.9f;
	    map_br.x = cx+dx * 0.9f;
	    map_br.y = cy+dy * 0.9f;
	  }
	  break;
        case '-':
	  {
	    float dx = (map_br.x-map_tl.x) / 2.0f;
	    float dy = (map_br.y-map_tl.y) / 2.0f;
	    float cx = (map_br.x+map_tl.x) / 2.0f;
	    float cy = (map_br.y+map_tl.y) / 2.0f;
	    map_tl.x = cx-dx * 1.1f;
	    map_tl.y = cy-dy * 1.1f;
	    map_br.x = cx+dx * 1.1f;
	    map_br.y = cy+dy * 1.1f;
	  }
	  break;
        case '=':
	  map_tl.Set (-1000.0f, 1000.0f);
	  map_br.Set (1000.0f, -1000.0f);
	  break;
        case '[':
	  seqmgr->SelectPreviousPath (map_selpath);
	  np = seqmgr->GetSelectedPath (map_selpath);
	  if (np)
	  {
	    if (map_selpoint >= np->GetPointCount ())
	      map_selpoint = np->GetPointCount ()-1;
	  }
	  break;
        case ']':
	  seqmgr->SelectNextPath (map_selpath);
	  np = seqmgr->GetSelectedPath (map_selpath);
	  if (np)
	  {
	    if (map_selpoint >= np->GetPointCount ())
	      map_selpoint = np->GetPointCount ()-1;
	  }
	  break;
      }
    }
    else
    {
      //==============================
      // Handle keys in demo or overlay mode.
      //==============================
      if (Event.Key.Code == CSKEY_ESC)
      {
	iEventQueue* q = CS_QUERY_REGISTRY (object_reg, iEventQueue);
	if (q)
	{
	  q->GetEventOutlet()->Broadcast (cscmdQuit);
	  q->DecRef ();
	}
        return true;
      }
      switch (Event.Key.Char)
      {
      	case 'R':
	  ShowMessage ("Restarted sequence manager");
  	  seqmgr->Restart ("/data/demo/sequences");
	  break;
        case 'p':
          if (seqmgr->IsSuspended ()) seqmgr->Resume ();
          else seqmgr->Suspend ();
	  break;
        case '.':
          seqmgr->TimeWarp (20);
	  break;
        case ',':
          seqmgr->TimeWarp ((csTicks)-20);
	  break;
        case '>':
          seqmgr->TimeWarp (2500);
	  break;
        case '<':
          seqmgr->TimeWarp ((csTicks)-2500, true);
	  break;
        case '/':
          seqmgr->TimeWarp (0, true);
	  break;
        case 'm':
	  map_enabled++;
	  if (map_enabled == MAP_EDIT)
	  {
	    ShowMessage ("Map editing mode, press 'm' to exit");
            seqmgr->Suspend ();
	  }
	  break;
      }
    }
  }
  else if (Event.Type == csevMouseDown)
  {
    if (Event.Mouse.Button == 1)
    {
      csVector2 p (Event.Mouse.x, myG2D->GetHeight ()-Event.Mouse.y);
      csVector3 v;
      view->GetCamera ()->InvPerspective (p, 1, v);
      csVector3 vw = view->GetCamera ()->GetTransform ().This2Other (v);
      if (map_enabled == MAP_EDIT_FORWARD)
      {
        csNamedPath* np = seqmgr->GetSelectedPath (map_selpath);
	if (np)
	{
          vw -= view->GetCamera ()->GetTransform ().GetOrigin ();
          np->SetForwardVector (map_selpoint, vw);
	  csVector3 up;
	  np->GetUpVector (map_selpoint, up);
	  up = up % vw;
	  up = - (up % vw);
	  np->SetUpVector (map_selpoint, up);
        }
      }
      else if (map_enabled == MAP_EDIT)
      {
        p.y = Event.Mouse.y;
	int dim = myG2D->GetHeight ()-10;
	float dx = (map_br.x-map_tl.x) / 2.0f;
	float dy = (map_br.y-map_tl.y) / 2.0f;
	float cx = map_tl.x + (map_br.x-map_tl.x)*(1-(dim-p.x)/dim);
	float cy = map_tl.y + (map_br.y-map_tl.y)*(1-(dim-p.y)/dim);
	map_tl.x = cx-dx*.9;
	map_tl.y = cy-dy*.9;
	map_br.x = cx+dx*.9;
	map_br.y = cy+dy*.9;
      }
    }
  }

  return false;
}

/*---------------------------------------------------------------------*
 * Main function
 *---------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  srand (time (NULL));

  // Create our main class.
  System = new Demo ();

  // Initialize the main system. This will load all needed plug-ins
  // and initialize them.
  if (!System->Initialize (argc, argv, "/config/csdemo.cfg"))
  {
    System->Report (CS_REPORTER_SEVERITY_ERROR, "Error initializing system!");
    Cleanup ();
    exit (1);
  }

  // Main loop.
  csDefaultRunLoop(System->object_reg);

  // Cleanup.
  Cleanup ();

  return 0;
}
