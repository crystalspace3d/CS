/*
    DOS Allegro support for Crystal Space 3D library
    Copyright (C) 1999 by Dan Bogdanov <dan@pshg.edu.ee>
    Modified for full Allegro by Burton Radons <loth@pacificcoast.net>

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

#ifndef __ALLEGRO_H__
#define __ALLEGRO_H__

#include "csutil/scf.h"
#include "video/canvas/common/graph2d.h"
//#include "cssys/unix/iunix.h"
#include <allegro.h>

/// DOS Allegro 2D graphics driver
class csGraphics2DAlleg : public csGraphics2D
{
  /// Palette has been changed?
  bool PaletteChanged;
  /// Pointer to Unix-specific interface
  //iUnixSystemDriver* UnixSystem;
  int keydown [128];
  int x_mouse;
  int y_mouse; /* Previously recorded mouse position */
  int button;
  int opened;
  BITMAP *bitmap;
  bool scale; /* Bitmap is not screen size, scale it */

public:
  DECLARE_IBASE;

  csGraphics2DAlleg (iBase *iParent);
  virtual ~csGraphics2DAlleg ();

  virtual bool Initialize (iSystem *pSystem);
  virtual bool Open(const char* Title);
  virtual void Close(void);

  virtual void Print (csRect *area = NULL);

  virtual void SetRGB(int i, int r, int g, int b);
  virtual bool HandleEvent (csEvent &/*Event*/);

  virtual bool BeginDraw () { return (Memory != NULL); }
  virtual bool DoubleBuffer (bool /*Enable*/) { return true; }
  virtual bool GetDoubleBufferState () { return true; }
  
  virtual bool PerformExtension (const char *args);
  /**
   * Blit between a bitmap and one twice it's size.
   * sw,sh are in source dimensions.
   */
  virtual void DoubleBlit (BITMAP *src, BITMAP *dst, int sw, int sh);

private:
  void FillEvents ();
  void printf_Enable (bool Enable);
};

#endif // __ALLEGRO_H__
