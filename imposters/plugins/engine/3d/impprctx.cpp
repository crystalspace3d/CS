/*
    Copyright (C) 2002 by Keith Fulton and Jorrit Tyberghein

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
#include "csgfx/memimage.h"
#include "cstool/csview.h"
#include "cstool/proctex.h"
#include "csutil/cscolor.h"
#include "iengine/camera.h"
#include "iengine/engine.h"
#include "iengine/rview.h"
#include "iengine/sector.h"
#include "iutil/objreg.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/texture.h"
#include "ivideo/txtmgr.h"
#include "plugins/engine/3d/engine.h"
#include "plugins/engine/3d/impprctx.h"

#include "plugins/engine/3d/impmesh.h"
#include "plugins/engine/3d/meshobj.h"

//@@@ debugging
#include "cstool/debugimagewriter.h"
#include "csgfx/memimage.h"


//#include "iutil/vfs.h"

csImposterProcTex::csImposterProcTex (csEngine* engine, 
                                      csImposterMesh *parent) : 
  csProcTexture (), engine (engine)
{
  mesh = parent;

  mat_w = mat_h = 256;

  //texFlags = CS_TEXTURE_3D | CS_TEXTURE_NOMIPMAPS;

  mesh_on_texture = new csMeshOnTexture(engine->objectRegistry);

  csRef<iGraphics3D> g3d =
    csQueryRegistry<iGraphics3D>(engine->objectRegistry);
  csProcTexture::Initialize (engine->objectRegistry, engine,
                            g3d->GetTextureManager(), "test");
  SetAlwaysAnimate(true);
  if (PrepareAnim()) printf("prepare success\n");
}

csImposterProcTex::~csImposterProcTex ()
{
}

bool csImposterProcTex::PrepareAnim ()
{
  if (anim_prepared) return true;
  if (!csProcTexture::PrepareAnim ()) return false;

  // special things may be necessary here

  return true;
}

void csImposterProcTex::Animate (csTicks CurrentTime)
{
  printf("animating imposter\n");
  csRef<iTextureHandle> handle = tex->GetTextureHandle ();
  csRef<iMeshWrapper> originalmesh = mesh->GetParent ();
  int transparent = g2d->FindRGB (0,255,255,0);
  if (mesh_on_texture->Render(originalmesh,handle,0,transparent))
  {
    printf("rendered\n");
    mesh->SetImposterReady(true);
  }
}

