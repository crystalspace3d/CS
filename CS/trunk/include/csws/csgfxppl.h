/*
    Crystal Space Windowing System: Graphics Pipeline class
    Copyright (C) 1998,1999 by Andrew Zabolotny <bit@eltech.ru>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the 9License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CSGFXPPL_H__
#define __CSGFXPPL_H__

#include "csutil/csbase.h"
#include "csutil/csrect.h"
#include "csengine/cspixmap.h"
#include "igraph2d.h"

struct iGraphics2D;
struct iSystem;

// Maximal number of primitives which can be drawn IN ONE FRAME
#define MAX_CSWS_PIPELINE_LENGTH 16384
// Maximal number of video pages to sync image
#define MAX_SYNC_PAGES 8

/**
 * Graphics System pipeline class
 *<p>
 * This class implements all actual drawing operations (which are then
 * passed to iGraphics2D/iGraphics3D objects). The rectangle which
 * encapsulates all changes made to the screen is tracked. Upon frame
 * completion, this rectangle is kept in a memory buffer and then
 * propagated to all other videopages, thus for optimal performance
 * you should switch to single-buffered mode, if possible.
 *<p>
 * At the end of each frame, application object calls FinishFrame() method.
 * It will remember the dirty rectangle for current frame and call
 * iGraphics3D::Print() method to update the screen.
 *<p>
 * Since all drawing is made in real-time, the pipeline should call
 * BeginDraw with appropiate flags when needed. The graphics pipeline
 * tracks whenever the BeginDraw() with appropiate flags has been already
 * called, and doesn't do that again if it was done already. But if you
 * are doing interleaved drawing of 2D/3D primitives BeginDraw/FinishDraw
 * will be often called, thus reducing performance. That's why you should
 * try to group all 3D primitives together (like polygons) and draw all
 * them in one shot.
 *<p>
 * All methods and variables of this class are private. Only csApp should
 * have access to its internals, all graphics pipeline management is done
 * through main application object.
 */
class csGraphicsPipeline : public csBase
{
private:
  /// Only csApp can manipulate the graphics pipeline
  friend class csApp;

  /// Used to propagate changes to all pages
  /// The contents of dirty area on each page
  csImageArea *SyncArea [MAX_SYNC_PAGES];
  /// The rectangle we should refresh for current page (incremental)
  csRect RefreshRect;
  /// Maximum video pages in system
  int MaxPage;
  /// Current video page
  int CurPage;

  /// Current draw mode (used to exclude rendundant BeginDraw's)
  int DrawMode;
  /// Current clipping rectangle
  csRect ClipRect;
  /// The original clipping rectangle (set by user manually)
  csRect OrigClip;
  /// The original font set by user (before drawing with CSWS)
  int OrigFont, OrigFontSize;

  // Frame width and height
  int FrameWidth, FrameHeight;

  // The 2D graphics driver
  iGraphics2D *G2D;
  // The 3D graphics driver
  iGraphics3D *G3D;

  // Force redrawing of whole screen instead of changed area only
  bool bFullRedraw;
  
  /// Initialize pipeline
  csGraphicsPipeline (iSystem *System);
  /// Deinitialize pipeline
  virtual ~csGraphicsPipeline ();

  /// Synchronise image on this page with previous pages
  void Sync (int CurPage, int &xmin, int &ymin, int &xmax, int &ymax);

  /// Drop all synchronization rectangles
  void Desync ();

  /// Draw a box
  void Box (int xmin, int ymin, int xmax, int ymax, int color);

  /// Draw a line
  void Line (float x1, float y1, float x2, float y2, int color);

  /// Draw a pixel
  void Pixel (int x, int y, int color);

  /// Draw a text string: if bg < 0 background is not drawn
  void Text (int x, int y, int fg, int bg, int font, int fontsize, const char *s);

  /// Draw a 2D sprite
  void Sprite2D (csPixmap *s2d, int x, int y, int w, int h);

  /// Save a part of screen
  void SaveArea (csImageArea **Area, int x, int y, int w, int h);

  /// Restore a part of screen
  void RestoreArea (csImageArea *Area, bool Free);

  /// Free buffer used to keep an area of screen
  void FreeArea (csImageArea *Area);

  /// Clear screen with specified color
  void Clear (int color);

  /// Set clipping rectangle
  void SetClipRect (int xmin, int ymin, int xmax, int ymax);

  /// Restore clipping rectangle to (0, 0, ScreenW, ScreenH);
  void RestoreClipRect();

  /// Draw a 3D polygon using DrawPolygonFX
  void Polygon3D (G3DPolygonDPFX &poly, UInt mode);

  /// Clip a line against a rectangle and return true if its clipped out
  bool ClipLine (float &x1, float &y1, float &x2, float &y2,
    int ClipX1, int ClipY1, int ClipX2, int ClipY2);

  /// Get R,G,B at given screen location
  void GetPixel (int x, int y, UByte &oR, UByte &oG, UByte &oB);

  /// Change system mouse cursor and return success status
  bool SwitchMouseCursor (csMouseCursorID Shape);

  /// Return the width of given text using selected font and size
  int TextWidth (const char *text, int Font, int FontSize);

  /// Return the height of selected font and size
  int TextHeight (int Font, int FontSize);

  /// Clear Z-buffer (takes effect before next 3D draw operation)
  void ClearZbuffer ()
  { DrawMode |= CSDRAW_CLEARZBUFFER; }

  /// Clear the Z-buffer in the given area
  void ClearZbuffer (int x1, int y1, int x2, int y2);

  /// Set the respective Z-buffer mode (one of CS_ZBUF_XXX constants)
  void SetZbufferMode (unsigned mode)
  { G3D->SetRenderState (G3DRENDERSTATE_ZBUFFERMODE, mode); }

  /// Called at the start of every frame
  void StartFrame (int iCurPage);

  /// Flush graphics pipeline
  void FinishFrame ();

  /// Begin painting: no-op if we're already in draw mode
  bool BeginDraw (int iMode)
  { return (iMode != DrawMode) ? BeginDrawImp (iMode) : true; }

  /// Finish painting
  void FinishDraw ();

  /// Do the actual work for BeginDraw ()
  bool BeginDrawImp (int iMode);

  /// Do the actual work for FinishDraw ()
  void FinishDrawImp ();

  /// Force redraw of the specified rectangle
  void Invalidate (csRect &rect);
};

#endif // __CSGFXPPL_H__
