/*
    Copyright (C) 1998 by Jorrit Tyberghein

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

#include <stdarg.h>
#include <math.h>

#include "cssysdef.h"
#include "cssys/sysdriv.h"
#include "ogl_g3dcom.h"
#include "ogl_txtmgr.h"
#include "ogl_txtcache.h"
#include "ogl_proctexback.h"
#include "ogl_proctexsoft.h"
#include "csutil/scanstr.h"
#include "csutil/inifile.h"
#include "isystem.h"
#include "iimage.h"

#define SysPrintf System->Printf

//---------------------------------------------------------------------------

csTextureOpenGL::csTextureOpenGL (csTextureMM *Parent, iImage *Image)
  : csTexture (Parent)
{
  image = Image;
  w = Image->GetWidth ();
  h = Image->GetHeight ();
  compute_masks ();
}

csTextureOpenGL::~csTextureOpenGL ()
{
  if (image) image->DecRef ();
}

csTextureProcOpenGL::~csTextureProcOpenGL ()
{
  if (texG3D) texG3D->DecRef (); 
}

//---------------------------------------------------------------------------

csTextureMMOpenGL::csTextureMMOpenGL (iImage *image, int flags,
  csGraphics3DOGLCommon *iG3D) : csTextureMM (image, flags)
{
  G3D = iG3D;
}

csTextureMMOpenGL::~csTextureMMOpenGL ()
{
  if (G3D && G3D->texture_cache)
    G3D->texture_cache->Uncache (this);
}

csTexture *csTextureMMOpenGL::NewTexture (iImage *Image)
{
  if ((flags & CS_TEXTURE_PROC) == CS_TEXTURE_PROC)
    return new csTextureProcOpenGL (this, Image);
  else
    return new csTextureOpenGL (this, Image);
}

void csTextureMMOpenGL::InitTexture (csTextureManagerOpenGL *texman, 
				     csPixelFormat *pfmt)
{
  // Preserve original width/height so that in DrawPixmap subregions of
  // textures are calculated correctly. In other words, the app writer need
  // not know about opengl texture size adjustments. smgh
  if (!image) return;
  if (((flags & CS_TEXTURE_PROC) == CS_TEXTURE_PROC) && tex[0])
    return;
  orig_width = image->GetWidth ();
  orig_height = image->GetHeight ();

  // If necessary rescale if bigger than maximum texture size
  if ((orig_width > texman->max_tex_size) ||
      (orig_height > texman->max_tex_size))
  {
    int nwidth = orig_width;
    int nheight = orig_height; 
    if (orig_width > texman->max_tex_size) nwidth = texman->max_tex_size;
    if (orig_height > texman->max_tex_size) nheight = texman->max_tex_size;
    image->Rescale (nwidth, nheight);
  }

  // In opengl all textures, even non-mipmapped textures are required 
  // to be powers of 2.
  AdjustSizePo2 ();

  CreateMipmaps ();

  if ((flags & CS_TEXTURE_PROC) == CS_TEXTURE_PROC)
  {
    bool alone_hint = (flags & CS_TEXTURE_PROC_ALONE_HINT) == 
                       CS_TEXTURE_PROC_ALONE_HINT;
    switch (texman->proc_tex_type)
    {
      case BACK_BUFFER_TEXTURE:
      {
	csOpenGLProcBackBuffer *bbtexG3D = new csOpenGLProcBackBuffer(NULL);
	bool persistent = (flags & CS_TEXTURE_PROC_PERSISTENT) == 
	                   CS_TEXTURE_PROC_PERSISTENT;
	// already shares the texture cache/manager
	bbtexG3D->Prepare (texman->G3D, this, pfmt, persistent);
	((csTextureProcOpenGL*)tex[0])->texG3D = (iGraphics3D*) bbtexG3D;
	break;
      }
      case SOFTWARE_TEXTURE:
      {
	// This is always in 32bit no matter what the pfmt.
	csOpenGLProcSoftware *stexG3D = new csOpenGLProcSoftware (NULL);
	// We generate a 32 bit pfmt taking into account endianness and whether
	// frame buffer is RGB or BGR...there must be a better way...
	csPixelFormat pfmt2;
#if defined (CS_BIG_ENDIAN)
	if (pfmt->RedMask > pfmt->BlueMask)
	{
	  pfmt2.RedMask   = 0xff000000;
	  pfmt2.GreenMask = 0x00ff0000;
	  pfmt2.BlueMask  = 0x0000ff00;
	}
	else
	{
	  pfmt2.RedMask   = 0x0000ff00;
	  pfmt2.GreenMask = 0x00ff0000;
	  pfmt2.BlueMask  = 0xff000000;
	}
#else
	if (pfmt->RedMask > pfmt->BlueMask)
	{
	  pfmt2.RedMask   = 0x00ff0000;
	  pfmt2.GreenMask = 0x0000ff00;
	  pfmt2.BlueMask  = 0x000000ff;
	}
	else
	{
	  pfmt2.RedMask   = 0x000000ff;
	  pfmt2.GreenMask = 0x0000ff00;
	  pfmt2.BlueMask  = 0x00ff0000;
	}
#endif
	pfmt2.PixelBytes = 4;
	pfmt2.PalEntries = 0;
	pfmt2.complete ();

	if (stexG3D->Prepare (texman->G3D, texman->head_soft_proc_tex, 
			      this, &pfmt2, image->GetImageData (), alone_hint))
	{
	  ((csTextureProcOpenGL*)tex[0])->texG3D = (iGraphics3D*) stexG3D;
	  if (!texman->head_soft_proc_tex)
	    texman->head_soft_proc_tex = stexG3D;
	}
	break;
      }
      case AUXILIARY_BUFFER_TEXTURE:
      default:
	break;
    }
  }
}


void csTextureMMOpenGL::ComputeMeanColor ()
{
  // Compute the mean color from the smallest mipmap available
  csTextureOpenGL *tex = NULL;
  for (int i = 3; i >= 0; i--)
  {
    tex = (csTextureOpenGL *)get_texture (i);
    if (tex) break;
  }
  if (!tex) return;

  int pixels = tex->get_width () * tex->get_height ();
  RGBPixel *src = tex->get_image_data ();
  unsigned r = 0, g = 0, b = 0;
  int count = pixels;
  while (count--)
  {
    RGBPixel pix = *src++;
    r += pix.red;
    g += pix.green;
    b += pix.blue;
  }
  mean_color.red   = r / pixels;
  mean_color.green = g / pixels;
  mean_color.blue  = b / pixels;
}

iGraphics3D *csTextureMMOpenGL::GetProcTextureInterface ()
{
  if ((flags & CS_TEXTURE_PROC) != CS_TEXTURE_PROC)
    return NULL;

  return ((csTextureProcOpenGL*)tex[0])->texG3D;
}

//---------------------------------------------------------------------------

csTextureManagerOpenGL::csTextureManagerOpenGL (iSystem* iSys,
  iGraphics2D* iG2D, csIniFile *config, csGraphics3DOGLCommon *iG3D)
  : csTextureManager (iSys, iG2D)
{
  G3D = iG3D;
  head_soft_proc_tex = NULL;
  read_config (config);
  Clear ();
}

csTextureManagerOpenGL::~csTextureManagerOpenGL ()
{
  Clear ();
}

void csTextureManagerOpenGL::read_config (csIniFile *config)
{
  const char *proc_texture_type = 
    config->GetStr ("OpenGL", "PROCEDURAL_TEXTURE");

  if (!strcmp (proc_texture_type, "software"))
    proc_tex_type = SOFTWARE_TEXTURE;
  else if (!strcmp (proc_texture_type, "back_buffer"))
    proc_tex_type = BACK_BUFFER_TEXTURE;
  else if (!strcmp (proc_texture_type, "auxiliary_buffer"))
    proc_tex_type = AUXILIARY_BUFFER_TEXTURE;
  else // default
    proc_tex_type = BACK_BUFFER_TEXTURE;
}

void csTextureManagerOpenGL::PrepareTextures ()
{
  // hack, pixel format is now not yet determined until G2D is opened
  pfmt = *G3D->GetDriver2D()->GetPixelFormat ();
  max_tex_size = G3D->max_texture_size;

  if (verbose) SysPrintf(MSG_INITIALIZATION, "Preparing textures...\n");
  if (verbose) SysPrintf(MSG_INITIALIZATION, "  Creating texture mipmaps...\n");

  // Create mipmaps for all textures
  for (int i = 0; i < textures.Length (); i++)
  {
    csTextureMMOpenGL* txt =(csTextureMMOpenGL *)textures.Get (i);
    txt->InitTexture (this, &pfmt);
  }
}

iTextureHandle *csTextureManagerOpenGL::RegisterTexture (iImage* image,
  int flags)
{
  if (!image) return NULL;

  csTextureMMOpenGL* txt = new csTextureMMOpenGL (image, flags, G3D);
  textures.Push (txt);
  return txt;
}

void csTextureManagerOpenGL::PrepareTexture (iTextureHandle *handle)
{
  if (!handle) return;
  // hack, pixel format is now not yet determined until G2D is opened
  pfmt = *G3D->GetDriver2D()->GetPixelFormat ();
  max_tex_size = G3D->max_texture_size;

  ((csTextureMMOpenGL*)handle->GetPrivateObject ())->InitTexture (this, &pfmt);
}

void csTextureManagerOpenGL::UnregisterTexture (iTextureHandle* handle)
{
  csTextureMMOpenGL *tex_mm = (csTextureMMOpenGL *)handle->GetPrivateObject ();
  G3D->UncacheTexture (handle);
  int idx = textures.Find (tex_mm);
  if (idx >= 0) textures.Delete (idx);
}
