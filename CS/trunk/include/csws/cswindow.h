/*
    Crystal Space Windowing System: window class
    Copyright (C) 1998,1999 by Andrew Zabolotny <bit@eltech.ru>

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

#ifndef __CSWINDOW_H__
#define __CSWINDOW_H__

#include "csutil/csbase.h"
#include "cscomp.h"
#include "csbutton.h"
#include "cstheme.h"

/// Window system menu button ID
#define CSWID_BUTSYSMENU	0xC500
/// Window close button ID
#define CSWID_BUTCLOSE		0xC501
/// Window hide button ID
#define CSWID_BUTHIDE		0xC502
/// Window maximize button ID
#define CSWID_BUTMAXIMIZE	0xC503
/// Window title bar ID
#define CSWID_TITLEBAR		0xC504
/// Window menu bar ID
#define CSWID_MENUBAR		0xC505
/// Client window ID
#define CSWID_CLIENT		0xC506
/// System menu ID
#define CSWID_SYSMENU		0xC507
/// Tool bar ID
#define CSWID_TOOLBAR		0xC508

/// Window style flags: does window have a system menu?
#define CSWS_BUTSYSMENU		0x00000001
/// Does window have a close button?
#define CSWS_BUTCLOSE		0x00000002
/// Does window have a hide button?
#define CSWS_BUTHIDE		0x00000004
/// Does window have a maximize/restore button?
#define CSWS_BUTMAXIMIZE	0x00000008
/// Does window have a titlebar?
#define CSWS_TITLEBAR		0x00000010
/// Does window have a menu bar?
#define CSWS_MENUBAR		0x00000020
/// Does window draw a thin 3D frame around client window?
#define CSWS_CLIENTBORDER	0x00000040
/// Does window have a tool bar?
#define CSWS_TOOLBAR		0x00000080
/// Toolbar position mask
#define CSWS_TBPOS_MASK		0x00000300
/// Toolbar is automatically placed at top of window (below menu bar)
#define CSWS_TBPOS_TOP		0x00000000
/// Toolbar is automatically placed at bottom of window
#define CSWS_TBPOS_BOTTOM	0x00000100
/// Toolbar is automatically placed at left of window
#define CSWS_TBPOS_LEFT		0x00000200
/// Toolbar is automatically placed at right of window
#define CSWS_TBPOS_RIGHT	0x00000300
/// Default window style
#define CSWS_DEFAULTVALUE	(CSWS_BUTSYSMENU | CSWS_BUTCLOSE | \
				 CSWS_BUTHIDE | CSWS_BUTMAXIMIZE | \
				 CSWS_TITLEBAR | CSWS_MENUBAR)

/// Possible window frame styles
enum csWindowFrameStyle
{
  cswfsNone,
  cswfsThin,
  cswfs3D,
  cswfsTexture,
  cswfsTheme
};

/**
 * The following commands are generated by titlebar buttons.
 * Commands generated by buttons contains in Info field a pointer
 * to object that generated the command (usually this is a object
 * derived from csButton).<p>
 */
enum
{
  /**
   * The "show system menu" command
   */
  cscmdWindowSysMenu = 0x00000100,
  /**
   * Set given component as client window
   * <pre>
   * IN:  (csComponent *)client;
   * OUT: NULL if successful
   * </pre>
   */
  cscmdWindowSetClient
};


/**
 * A csWindow object is a rectangular area of screen with border
 * which optionally contains a titlebar, a menubar and a client
 * component. The client window is the rectangle where the actual
 * window contents (a dialog, a picture etc) are drawn.
 */
class csWindow : public csComponent
{
protected:
  /// Window style
  int WindowStyle;
  /// Window frame style
  csWindowFrameStyle FrameStyle;
  /// Window border width/height
  int BorderWidth, BorderHeight;
  /// Titlebar height (this also defines min/max buttons width and height)
  int TitlebarHeight;
  /// Menu height
  int MenuHeight;
  /// Border Light Color index
  int BorderLightColor;
  /// Border Dark Color index
  int BorderDarkColor;
  /// Background Color index
  int BackgroundColor;
  /// Background Pixmap
  csPixmap * BackgroundPixmap;
  /// Border Pixmap
  csPixmap * BorderPixmap;

  /// Record if the theme is active for various pieces
  struct ThemeWindowActive
  {
    unsigned int FrameStyle:1;
    unsigned int BorderWidth:1;
    unsigned int BorderHeight:1;
    unsigned int BorderPixmap:1;
    unsigned int TitlebarHeight:1;
    unsigned int MenuHeight:1;
    unsigned int CloseButton:1;
    unsigned int HideButton:1;
    unsigned int MaximizeButton:1;
    unsigned int TitleBar:1;
    unsigned int BorderLightColor:1;
    unsigned int BorderDarkColor:1;
    unsigned int BackgroundColor:1;
    unsigned int BackgroundPixmap:1;
  } ThemeActive;

public:
  /// Create a window object
  csWindow (csComponent *iParent, char *iTitle, int iWindowStyle = CSWS_DEFAULTVALUE,
    csWindowFrameStyle iFrameStyle = cswfsTheme);

  /// Rescale titlebar, menu etc before passing to original SetRect
  virtual bool SetRect (int xmin, int ymin, int xmax, int ymax);

  /// Draw the window
  virtual void Draw ();

  /// Handle input events
  virtual bool HandleEvent (iEvent &Event);

  /// Don't allow too small windows
  virtual void FixSize (int &newW, int &newH);

  /// Maximize window if it is not already and if DragStyle has CS_DRAG_SIZEABLE
  virtual bool Maximize ();

  /// Restore window if it is maximized and if DragStyle has CS_DRAG_SIZEABLE
  virtual bool Restore ();

  /// Change titlebar text
  virtual void SetText (const char *iText);
  /// Query window title text
  virtual void GetText (char *oText, int iTextSize) const;
  /// Same, but returns a readonly value
  virtual const char *GetText () const;

  /// Handle a theme change event
  virtual void ThemeChanged ();
  /// Reset the window to use all theme values.
  virtual void ResetTheme();

  /// Set window border width and height
  void SetBorderSize (int w, int h);
  /// Get window border width and height
  void GetBorderSize (int &bw, int &bh)
  { bw = BorderWidth; bh = BorderHeight; }

  /// Set title bar height and redraws the window
  void SetTitleHeight (int iHeight);
  /// Get window titlebar height
  int GetTitlebarHeight ()
  { return TitlebarHeight; }

  /// Set menu bar height and redraws the window
  void SetMenuBarHeight (int iHeight);
  /// Get window menu height
  int GetMenuHeight ()
  { return MenuHeight; }

  /// Transform client window size into window size
  void ClientToWindow (int &ClientW, int &ClientH);
  /// Transform window size into client window size
  void WindowToClient (int &ClientW, int &ClientH);

  /// Set background color of the window
  void SetBackgroundColor (int Color);
  /// Set background Pixmap of the window
  void SetBackgroundPixmap (csPixmap *pixmap);
  /// Get background color of the window
  int GetBackgroundColor ();
  /// Get background Pixmap of the window
  csPixmap *GetBackgroundPixmap ();

  /// Set BorderDarkColor
  void SetBorderDarkColor (int Color);
  /// Set BorderLightColor
  void SetBorderLightColor (int Color);
  /// Set BorderPixmap
  void SetBorderPixmap (csPixmap * pixmap);
  /// Get BorderDarkColor
  int GetBorderDarkColor ();
  /// Get BorderLightColor
  int GetBorderLightColor ();
  /// Get BorderPixmap
  csPixmap *GetBorderPixmap ();

protected:
  /// Set button bitmaps to one of those read from csws.cfg
  void SetButtBitmap (csButton *button, char *id_n, char *id_p);

  /// Override SetState method to change titlebar when window focused flag changes
  virtual void SetState (int mask, bool enable);
};

#endif // __CSWINDOW_H__
