/*
    Crystal Space Canvas and Bitmap Interface for PicoGUI
    (C) 2003 Mat Sutcliffe <oktal@gmx.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
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

#ifndef __CS_PICOGUI_SERVER_HBITMAP_H__
#define __CS_PICOGUI_SERVER_HBITMAP_H__

#include "csgfx/memimage.h"
#include "cstool/cspixmap.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/txtmgr.h"
#include "igraphic/image.h"

extern "C"
{
  #include <picogui/types.h>
  #include <pgserver/types.h>
  #include <pgserver/render.h>
}

/**
 * This class describes a rendering canvas or bitmap.
 * PicoGUI will work with pointers to instances of this class.
 * It will pass those pointers to the static methods in csPGVideoDriver
 * and csPGFontEngine when it wants to perform operations on the
 * canvas or bitmap.
 */
class csHwrBitmap
{
private:
  void* CSID;
  bool dirty, currentlytiled;
  hwrbitmap bitmap;
  csSimplePixmap* pixmap;
  int lastx, lasty, lastw, lasth;

  struct groprender grop;
  int shmid;

 public:
  /// Construct a bitmap.
  inline csHwrBitmap (hwrbitmap bitmap0, iGraphics3D* g3d, int shmid0 = 0)
    : bitmap (bitmap0), shmid (shmid0), pixmap (0), CSID (0)
  { 
    int w = 1, h = 1;
    while (w<bitmap->w) w <<=1;
    while (h<bitmap->h) h <<=1;
    csImageMemory* image = new csImageMemory (w, h, 
      CS_IMGFMT_TRUECOLOR | CS_IMGFMT_ALPHA);
    csRef<iTextureHandle> tex;
    if (g3d && g3d->GetTextureManager ())
    {
      tex = g3d->GetTextureManager ()->RegisterTexture (
        image, CS_TEXTURE_2D |
               CS_TEXTURE_NOMIPMAPS);
      if (tex)
      {
        tex->Prepare ();
        pixmap = new csSimplePixmap (tex);
      }
    }
    delete image;
    dirty = true;
    currentlytiled = false;
    lastx = lasty = lastw = lasth = -1;
  }

  inline ~csHwrBitmap ()
  {
    if (bitmap)
      def_bitmap_free (bitmap);
    delete pixmap;
  }

  /// Get the pico bitmap
  // This is a bit ugly, but Pico forces us to do it.
  // If GetPicoBitmap is called on an erroneously casted stdbitmap*
  // it will just return "this".
  inline hwrbitmap GetPicoBitmap () { return CSID?(hwrbitmap)this:bitmap; }

  /// Get the CS bitmap (pixmap)
  inline csSimplePixmap* GetCSBitmap (
    bool tiled = false, int x = 0, int y = 0, int w = 0, int h = 0)
  { 
    if (currentlytiled == tiled && !dirty
        && ((x == lastx && y == lasty && w == lastw && h == lasth) 
             || !tiled))
      return pixmap;
    if (!tiled || 
       (bitmap->w == pixmap->Width () && bitmap->w == pixmap->Height ()
       && x == 0 && y == 0 && w == bitmap->w && h == bitmap->h))
    {
      csRef<iGraphics2D> g2d = pixmap->GetTextureHandle ()->GetCanvas ();
      g2d->BeginDraw ();
      g2d->Blit (0, 0, bitmap->w, bitmap->h, bitmap->bits);
      g2d->FinishDraw ();
    } else {
      csImageMemory* image = new csImageMemory (bitmap->w, bitmap->h, 
        CS_IMGFMT_TRUECOLOR | CS_IMGFMT_ALPHA);
      memcpy (image->GetImageData (), bitmap->bits,
        bitmap->w*bitmap->h*4);
      csRef<iImage> cropped = image->Crop (x, y, w, h);
      cropped->Rescale (pixmap->Width (), pixmap->Height ());
      csRef<iGraphics2D> g2d = pixmap->GetTextureHandle ()->GetCanvas ();
      g2d->BeginDraw ();
      g2d->Blit (0, 0, pixmap->Width (), pixmap->Height (), 
        (unsigned char*)cropped->GetImageData ());
      g2d->FinishDraw ();
      lastx = x; lasty = y; lastw = w; lasth = h;
      delete image;
    }
    dirty = false;
    currentlytiled = tiled;
    return pixmap; 
  }

  /// Tells the bitmap that the pixmap should be updated next time it's used
  inline void MarkAsDirty () { dirty = true; }

  /// Get the "grop render" structure associated with this bitmap.
  inline groprender* Grop () { return & grop; }

  /// Get the ID for a bitmap in shared memory.
  inline int ShmID () { return shmid; }
};

#endif
