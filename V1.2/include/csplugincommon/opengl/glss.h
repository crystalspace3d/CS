/*
    Copyright (C) 2003 by Jorrit Tyberghein
              (C) 2003 by Frank Richter

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

#ifndef __CS_CANVAS_OPENGLCOMMON_GLSS_H__
#define __CS_CANVAS_OPENGLCOMMON_GLSS_H__

/**\file
 * OpenGL screen shot.
 */

#include "csextern_gl.h"
#include "csplugincommon/canvas/scrshot.h"
#include "csutil/scf_implementation.h"

class csGraphics2DGLCommon;

/**\addtogroup plugincommon
 * @{ */

/**
 * OpenGL screen shot.
 */
class CS_CSPLUGINCOMMON_GL_EXPORT csGLScreenShot :
  public scfImplementationExt0<csGLScreenShot, csImageBase>
{
  csGraphics2DGLCommon* G2D;
  int Format;
  csRGBpixel* Data;
  size_t dataSize;
  int Width, Height;

public:
  csGLScreenShot* poolNext;

  csGLScreenShot (csGraphics2DGLCommon*);
  virtual ~csGLScreenShot ();

  virtual const void *GetImageData ()
  { return Data; }
  virtual int GetWidth () const
  { return Width; }
  virtual int GetHeight () const
  { return Height; }
  virtual int GetFormat () const
  { return Format; }
  void SetData (void*);

  void IncRef ();
  void DecRef ();
};

/** @} */

#endif // __CS_CANVAS_OPENGLCOMMON_GLSS_H__

