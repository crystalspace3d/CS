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

#include "cssysdef.h"
#include "video/canvas/openglx/glx2d.h"
#include "csutil/scf.h"
#include "cssys/csevent.h"
#include "cssys/csinput.h"
#include "cssys/unix/iunix.h"
#include "csutil/csrect.h"
#include "csutil/inifile.h"
#include "isystem.h"
#include "itexture.h"
#include "video/canvas/common/dyntex2d.h"

IMPLEMENT_FACTORY (csGraphics2DGLX)

EXPORT_CLASS_TABLE (glx2d)
  EXPORT_CLASS (csGraphics2DGLX, "crystalspace.graphics2d.glx",
    "GL/X OpenGL 2D graphics driver for Crystal Space")
EXPORT_CLASS_TABLE_END

#define DEF_OGLDISP "crystalspace.graphics2d.glx.disp.empty"

// csGraphics2DGLX function
csGraphics2DGLX::csGraphics2DGLX (iBase *iParent) :
  csGraphics2DGLCommon (iParent), cmap (0)
{

}

bool csGraphics2DGLX::Initialize (iSystem *pSystem)
{
  if (!csGraphics2DGLCommon::Initialize (pSystem))
    return false;

  UnixSystem = QUERY_INTERFACE (System, iUnixSystemDriver);
  if (!UnixSystem)
  {
    CsPrintf (MSG_FATAL_ERROR, "FATAL: The system driver does not support "
                               "the IUnixSystemDriver interface\n");
    return false;
  }

  iVFS* v = pSystem->GetVFS();
  csIniFile *config = new csIniFile(v,  "/config/opengl.cfg" );
  v->DecRef(); 
  v = NULL;
  
  dispdriver = NULL;
  const char *strDriver;
  if (config)
  {
    if ((strDriver = config->GetStr ("Display", "Driver", DEF_OGLDISP)))
    {
      dispdriver = LOAD_PLUGIN (pSystem, strDriver, NULL, iOpenGLDisp);
      if (!dispdriver)
        CsPrintf (MSG_FATAL_ERROR, "Could not create an instance of %s ! Using NULL instead.\n", strDriver);
      else
      {
        if (!dispdriver->open ())
        {
          printf ("open of displaydriver %s failed!\n", strDriver);
          return false;
        }
      }
    }
    delete config;
  }
  else
    CsPrintf (MSG_FATAL_ERROR, "could not opengl.cfg, will use NULL as displaydriver\n");
  
  Screen* screen_ptr;

  // Query system settings
  int sim_depth;
  bool do_shm;
  UnixSystem->GetExtSettings (sim_depth, do_shm, do_hwmouse);

  dpy = XOpenDisplay (NULL);
  if (!dpy)
  {
    CsPrintf (MSG_FATAL_ERROR, "FATAL: Cannot open X display\n");
    exit (-1);
  }

  screen_num = DefaultScreen (dpy);
  root_window = RootWindow (dpy, screen_num);
  screen_ptr = DefaultScreenOfDisplay (dpy);
  display_width = DisplayWidth (dpy, screen_num);
  display_height = DisplayHeight (dpy, screen_num);

  // First make a window which is never mapped (trick from gtk to get the main
  // window to behave under certain window managers, themes and circumstances)
  leader_window = XCreateSimpleWindow(dpy, root_window,
					  10, 10, 10, 10, 0, 0 , 0);
  XClassHint *class_hint = XAllocClassHint();
  class_hint->res_name = "glX Crystal Space";
  class_hint->res_class = "Crystal Space";
  XmbSetWMProperties (dpy, leader_window,
                      NULL, NULL, NULL, 0, 
                      NULL, NULL, class_hint);

  XFree (class_hint);


  // The texture manager only needs to know this:
  pfmt.PalEntries = 0;

  // Tell system driver to call us on every frame
  System->CallOnEvents (this, CSMASK_Nothing);

  return true;
}

csGraphics2DGLX::~csGraphics2DGLX ()
{
  // Destroy your graphic interface
  XFree ((void*)active_GLVisual);
  Close ();
  if (UnixSystem)
    UnixSystem->DecRef ();
  if (dispdriver)
    dispdriver->DecRef();
 
}

bool csGraphics2DGLX::CreateContext (int *desired_attributes)
{
  // find a visual that supports all the features we need
  active_GLVisual = glXChooseVisual (dpy, screen_num, desired_attributes);

  // if a visual was found that we can use, make a graphics context which
  // will be bound to the application window.  If a visual was not
  // found, then try to figure out why it failed
  if (active_GLVisual)
  {
    active_GLContext = glXCreateContext(dpy,active_GLVisual,0,GL_TRUE);
    cmap = XCreateColormap (dpy, RootWindow (dpy, active_GLVisual->screen),
			    active_GLVisual->visual, AllocNone);

    CsPrintf (MSG_INITIALIZATION, "Seized Visual ID %d\n", 
	      active_GLVisual->visual->visualid);
  }
  else
  {
    CsPrintf (MSG_FATAL_ERROR, "FATAL: Could not find proper GLX visual\n");
 
    // what attribute was not supplied? we know that trying to get
    // all the attributes at once doesn't work.  provide more user info by
    // trying each of the pieces and seeing if any single piece is not provided 
    
    // try to get a visual with 12 bit color
    int color_attributes[] = 
    { 
      GLX_RGBA,
      GLX_RED_SIZE,4,
      GLX_BLUE_SIZE,4,
      GLX_GREEN_SIZE,4,
      None 
    };

    if (!glXChooseVisual(dpy, screen_num, color_attributes) )
    {
      CsPrintf(MSG_FATAL_ERROR, "Graphics display does not support at least 12 bit color\n");
    }

    // try to get visual with a depth buffer
    int depthbuffer_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE,1, None };
    if (!glXChooseVisual(dpy,screen_num,depthbuffer_attributes) )
    {
      CsPrintf(MSG_FATAL_ERROR,"Graphics display does not support a depth buffer\n");
    }
    if (is_double_buffered)
    {
      // try to get a visual with double buffering
      int doublebuffer_attributes[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None };
      if (!glXChooseVisual(dpy,screen_num,doublebuffer_attributes) )
	CsPrintf(MSG_FATAL_ERROR,"Graphics display does not provide double buffering\n");
      return false;
    }
  }

  return true;
}

const char *visual_class_name (int cls)
{
   switch (cls) {
      case StaticColor:
         return "StaticColor";
      case PseudoColor:
         return "PseudoColor";
      case StaticGray:
         return "StaticGray";
      case GrayScale:
         return "GrayScale";
      case TrueColor:
         return "TrueColor";
      case DirectColor:
         return "DirectColor";
      default:
         return "";
   }
}

bool csGraphics2DGLX::Open(const char *Title)
{
  // We now select the visual here as with a mesa bug it is not possible
  // to destroy double buffered contexts and then create a single buffered
  // one.
  if (!is_double_buffered)
  {
    int desired_attributes[] =
    {
      GLX_RGBA, 
      GLX_DEPTH_SIZE, 8, 
      GLX_RED_SIZE, 4,
      GLX_BLUE_SIZE, 4,
      GLX_GREEN_SIZE, 4,
      GLX_AUX_BUFFERS, 1,
      None
    };

    if (!CreateContext (desired_attributes))
      return false;
  }
  else
  {
    int desired_attributes[] =
    {
      GLX_RGBA, 
      GLX_DOUBLEBUFFER,
      GLX_DEPTH_SIZE, 8, 
      GLX_RED_SIZE, 4,
      GLX_BLUE_SIZE, 4,
      GLX_GREEN_SIZE, 4,
      GLX_AUX_BUFFERS, 1,
      None
    };

    if (!CreateContext (desired_attributes))
      return false;
  }

  CsPrintf (MSG_INITIALIZATION, "\nVideo driver GL/X version ");
  if (glXIsDirect (dpy, active_GLContext))
    CsPrintf (MSG_INITIALIZATION, "(direct renderer)\n");

  Depth = active_GLVisual->depth;

  if (Depth == 24 || Depth == 32)
    pfmt.PixelBytes = 4;
  else pfmt.PixelBytes = 2;

  CsPrintf (MSG_INITIALIZATION, "Visual ID: %x,  depth %dbit,  %s\n",
	    active_GLVisual->visualid, Depth, 
	    visual_class_name (active_GLVisual->c_class));

  int ctype, frame_buffer_depth, double_buffer, size_depth_buffer, level;
  glXGetConfig(dpy, active_GLVisual, GLX_RGBA, &ctype);
  glXGetConfig(dpy, active_GLVisual, GLX_DOUBLEBUFFER, &double_buffer);
  glXGetConfig(dpy, active_GLVisual, GLX_BUFFER_SIZE, &frame_buffer_depth);
  glXGetConfig(dpy, active_GLVisual, GLX_DEPTH_SIZE, &size_depth_buffer);
  glXGetConfig(dpy, active_GLVisual, GLX_LEVEL, &level);

  int alpha_bits = 0;
  char *colour_type;
  if (ctype)
  {
    pfmt.RedMask = active_GLVisual->red_mask;
    pfmt.GreenMask = active_GLVisual->green_mask;
    pfmt.BlueMask = active_GLVisual->blue_mask;
    glXGetConfig(dpy, active_GLVisual, GLX_RED_SIZE, &pfmt.RedBits);
    glXGetConfig(dpy, active_GLVisual, GLX_GREEN_SIZE, &pfmt.GreenBits);
    glXGetConfig(dpy, active_GLVisual, GLX_BLUE_SIZE, &pfmt.BlueBits);
    glXGetConfig(dpy, active_GLVisual, GLX_ALPHA_SIZE, &alpha_bits);

    if (pfmt.RedMask > pfmt.BlueMask)
    {
      if (alpha_bits)
	colour_type = "RGBA";
      else 
	colour_type = "RGB";
    }
    else
    {
      if (alpha_bits)
	colour_type = "BGRA";
      else
	colour_type = "BGR";
    }
  }

  CsPrintf (MSG_INITIALIZATION, 
	    "Frame buffer: Type %s,  depth %dbit, level %d, %s\n",
	    ctype ? colour_type : "ci", frame_buffer_depth, level,
	    double_buffer ? "double buffered" : "single buffered");
  CsPrintf (MSG_INITIALIZATION, "Depth buffer: %dbit\n", size_depth_buffer);

  if (ctype)
  {
    CsPrintf (MSG_INITIALIZATION, 
	      "Red  : bits %d, mask %d\n", pfmt.RedBits, pfmt.RedMask);
    CsPrintf (MSG_INITIALIZATION, 
	      "Green: bits %d, mask %d\n", pfmt.GreenBits, pfmt.GreenMask);
    CsPrintf (MSG_INITIALIZATION, 
	      "Blue : bits %d, mask %d\n", pfmt.BlueBits, pfmt.BlueMask);
    CsPrintf (MSG_INITIALIZATION, 
	      "Alpha: bits %d\n", alpha_bits);
  } 
 
  int aux_buffers;
  glXGetConfig(dpy, active_GLVisual, GLX_AUX_BUFFERS, &aux_buffers);

  CsPrintf (MSG_INITIALIZATION, 
	    "Auxiliary buffers %d,  depth %dbit\n\n",
	    aux_buffers, aux_buffers ? frame_buffer_depth : 0);

  // Create window
  XSetWindowAttributes winattr;
  winattr.border_pixel = 0;
  winattr.colormap = cmap;
  winattr.bit_gravity = CenterGravity;
  winattr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
    CWBitGravity | FocusChangeMask | PointerMotionMask | ButtonPressMask | 
    ButtonReleaseMask;

  window = XCreateWindow(dpy, root_window, 0, 0, Width,Height, 0 /*border*/,
    active_GLVisual->depth, InputOutput, active_GLVisual->visual,
    CWBorderPixel | CWColormap | CWEventMask | CWBitGravity, &winattr);

  XStoreName (dpy, window, Title);

  // Intern WM_DELETE_WINDOW and set window manager protocol
  // (Needed to catch user using window manager "delete window" button)
  wm_delete_window = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (dpy, window, &wm_delete_window, 1);

  // Now communicate fully to the window manager our wishes using the non-mapped
  // leader_window to form a window_group
  XSizeHints normal_hints;
  normal_hints.flags = PSize;
  normal_hints.width = Width;
  normal_hints.height = Height;
  XSetWMNormalHints (dpy, window, &normal_hints);

  XWMHints wm_hints;
  wm_hints.flags = InputHint | StateHint | WindowGroupHint;
  wm_hints.input = True;
  wm_hints.window_group = leader_window;
  wm_hints.initial_state = NormalState;
  XSetWMHints (dpy, window, &wm_hints);

  Atom wm_client_leader = XInternAtom (dpy, "WM_CLIENT_LEADER", False);
  XChangeProperty (dpy, window, wm_client_leader, XA_WINDOW, 32, 
		   PropModeReplace, (const unsigned char*)&leader_window, 1);
  XmbSetWMProperties (dpy, window, Title, Title,
                      NULL, 0, NULL, NULL, NULL);

  XRaiseWindow (dpy, window);
  XMapWindow (dpy, window);

  // Create mouse cursors
  XColor Black;
  memset (&Black, 0, sizeof (Black));
  memset (MouseCursor, 0, sizeof (Cursor) * (int(csmcWait)+1));
  EmptyPixmap = XCreatePixmap (dpy, window, 1, 1, 1);
  EmptyMouseCursor = XCreatePixmapCursor (dpy, EmptyPixmap, EmptyPixmap,
    &Black, &Black, 0, 0);
  MouseCursor [csmcArrow] = XCreateFontCursor (dpy, XC_left_ptr);
//MouseCursor [csmcLens] = XCreateFontCursor (dpy,
  MouseCursor [csmcCross] = XCreateFontCursor (dpy, 33/*XC_crosshair*/);
  MouseCursor [csmcPen] = XCreateFontCursor (dpy, XC_hand2/*XC_pencil*/);
  MouseCursor [csmcMove] = XCreateFontCursor (dpy, XC_fleur);
  /// Diagonal (\) resizing cursor
//MouseCursor [csmcSizeNWSE] = XCreateFontCursor (dpy,
  /// Diagonal (/) resizing cursor
//MouseCursor [csmcSizeNESW] = XCreateFontCursor (dpy,
  /// Vertical sizing cursor
  MouseCursor [csmcSizeNS] = XCreateFontCursor (dpy, XC_sb_v_double_arrow);
  /// Horizontal sizing cursor
  MouseCursor [csmcSizeEW] = XCreateFontCursor (dpy, XC_sb_h_double_arrow);
  /// Invalid operation cursor
//MouseCursor [csmcStop] = XCreateFontCursor (dpy, XC_pirate);
  /// Wait (longplay operation) cursor
  MouseCursor [csmcWait] = XCreateFontCursor (dpy, XC_watch);

  // Wait for expose event
  XEvent event;
  for (;;)
  {
    XNextEvent (dpy, &event);
    if (event.type == Expose)
      break;
  }

//    Pixmap pixmap = XCreatePixmap (dpy, window, 128, 128, active_GLVisual->depth);

//    GLXPixmap glx_pixmap = glXCreateGLXPixmap (dpy, active_GLVisual, pixmap);

  // this makes the context we created in Initialize() the current
  // context, so that all subsequent OpenGL calls will set state and
  // draw stuff on this context.  You could of couse make
  // some more contexts and switch around between several of them...
  // but we use only one here.
  glXMakeCurrent (dpy, window, active_GLContext);

  // Open your graphic interface
  if (!csGraphics2DGLCommon::Open (Title))
    return false;

  Clear (0);
  return true;
}

void csGraphics2DGLX::Close(void)
{
  if (EmptyMouseCursor)
  {
    XFreeCursor (dpy, EmptyMouseCursor);
    EmptyMouseCursor = 0;
    XFreePixmap (dpy, EmptyPixmap);
    EmptyPixmap = 0;
  XSync (dpy, False);
  }
  for (int i = sizeof (MouseCursor) / sizeof (Cursor) - 1; i >= 0; i--)
  {
    if (MouseCursor [i])
      XFreeCursor (dpy, MouseCursor [i]);
  XSync (dpy, False);
    MouseCursor [i] = None;
  }
  if (window)
  {
    XDestroyWindow (dpy, window);
    window = 0;
  }

  if ( dispdriver ){
      dispdriver->close();
  }
  // Close your graphic interface
  csGraphics2DGLCommon::Close ();
  if (active_GLContext != NULL)
  {
    glXDestroyContext(dpy,active_GLContext);
    active_GLContext = NULL;
  }
  if (leader_window)
  {
    XDestroyWindow (dpy, leader_window);
    leader_window = 0;
  }
}


void csGraphics2DGLX::Print (csRect * /*area*/)
{
  glXSwapBuffers (dpy,window);
  //glFlush (); // not needed?
  XSync (dpy, False);
}

bool csGraphics2DGLX::SetMousePosition (int x, int y)
{
  XWarpPointer (dpy, None, window, 0, 0, 0, 0, x, y);
  return true;
}

bool csGraphics2DGLX::SetMouseCursor (csMouseCursorID iShape)
{
  if (do_hwmouse
   && (iShape >= 0)
   && (iShape <= csmcWait)
   && (MouseCursor [iShape] != None))
  {
    XDefineCursor (dpy, window, MouseCursor [iShape]);
    return true;
  }
  else
  {
    XDefineCursor (dpy, window, EmptyMouseCursor);
    return (iShape == csmcNone);
  } /* endif */
}

static Bool CheckKeyPress (Display * /*dpy*/, XEvent *event, XPointer arg)
{
  XEvent *curevent = (XEvent *)arg;
  if ((event->type == KeyPress)
   && (event->xkey.keycode == curevent->xkey.keycode)
   && (event->xkey.state == curevent->xkey.state))
    return true;
  return false;
}

// XCheckMaskEvent() doesn't get ClientMessage Events so use XCheckIfEvent()
//    with this Predicate function as a work-around ( ClientMessage events
//    are needed in order to catch "WM_DELETE_WINDOW")
static Bool AlwaysTruePredicate (Display*, XEvent*, char*) { return True; }

bool csGraphics2DGLX::HandleEvent (csEvent &/*Event*/)
{
  static int button_mapping[6] = {0, 1, 3, 2, 4, 5};
  XEvent event;
  int state, key;
  bool down;

  while (XCheckIfEvent (dpy, &event, AlwaysTruePredicate, 0))
    switch (event.type)
    {
      case ClientMessage:
	if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_window)
	{
	  System->QueueContextCloseEvent ((void*)this);
	  System->StartShutdown();
	}
	break;
      case ButtonPress:
        state = ((XButtonEvent*)&event)->state;
        System->QueueMouseEvent (button_mapping [event.xbutton.button],
          true, event.xbutton.x, event.xbutton.y);
          break;
      case ButtonRelease:
        System->QueueMouseEvent (button_mapping [event.xbutton.button],
          false, event.xbutton.x, event.xbutton.y);
        break;
      case MotionNotify:
        System->QueueMouseEvent (0, false,
	  event.xbutton.x, event.xbutton.y);
        break;
      case KeyPress:
      case KeyRelease:
        // Neat trick: look in event queue if we have KeyPress events ahead
	// with same keycode. If this is the case, discard the KeyUp event
	// in favour of KeyDown since this is most (sure?) an autorepeat
        XCheckIfEvent (event.xkey.display, &event, CheckKeyPress, (XPointer)&event);
        down = (event.type == KeyPress);
        key = XLookupKeysym (&event.xkey, 0);
        state = event.xkey.state;
        switch (key)
        {
          case XK_Meta_L:
	  case XK_Meta_R:
	  case XK_Alt_L:
          case XK_Alt_R:      key = CSKEY_ALT; break;
          case XK_Control_L:
          case XK_Control_R:  key = CSKEY_CTRL; break;
          case XK_Shift_L:
          case XK_Shift_R:    key = CSKEY_SHIFT; break;
	  case XK_KP_Up:
	  case XK_KP_8:
          case XK_Up:         key = CSKEY_UP; break;
	  case XK_KP_Down:
	  case XK_KP_2:
          case XK_Down:       key = CSKEY_DOWN; break;
	  case XK_KP_Left:
	  case XK_KP_4:
          case XK_Left:       key = CSKEY_LEFT; break;
	  case XK_KP_Right:
	  case XK_KP_6:
          case XK_Right:      key = CSKEY_RIGHT; break;
          case XK_BackSpace:  key = CSKEY_BACKSPACE; break;
	  case XK_KP_Insert:
	  case XK_KP_0:
          case XK_Insert:     key = CSKEY_INS; break;
	  case XK_KP_Delete:
	  case XK_KP_Decimal:
          case XK_Delete:     key = CSKEY_DEL; break;
	  case XK_KP_Page_Up:
	  case XK_KP_9:
          case XK_Page_Up:    key = CSKEY_PGUP; break;
	  case XK_KP_Page_Down:
	  case XK_KP_3:
          case XK_Page_Down:  key = CSKEY_PGDN; break;
	  case XK_KP_Home:
	  case XK_KP_7:
          case XK_Home:       key = CSKEY_HOME; break;
	  case XK_KP_End:
	  case XK_KP_1:
          case XK_End:        key = CSKEY_END; break;
          case XK_Escape:     key = CSKEY_ESC; break;
          case XK_Tab:        key = CSKEY_TAB; break;
	  case XK_KP_Enter:
          case XK_Return:     key = CSKEY_ENTER; break;
          case XK_F1:         key = CSKEY_F1; break;
          case XK_F2:         key = CSKEY_F2; break;
          case XK_F3:         key = CSKEY_F3; break;
          case XK_F4:         key = CSKEY_F4; break;
          case XK_F5:         key = CSKEY_F5; break;
          case XK_F6:         key = CSKEY_F6; break;
          case XK_F7:         key = CSKEY_F7; break;
          case XK_F8:         key = CSKEY_F8; break;
          case XK_F9:         key = CSKEY_F9; break;
          case XK_F10:        key = CSKEY_F10; break;
          case XK_F11:        key = CSKEY_F11; break;
          case XK_F12:        key = CSKEY_F12; break;
          default:            break;
        }
	System->QueueKeyEvent (key, down);
        break;
      case FocusIn:
      case FocusOut:
        System->QueueFocusEvent (event.type == FocusIn);
        break;
      case Expose:
      {
        csRect rect (event.xexpose.x, event.xexpose.y,
	  event.xexpose.x + event.xexpose.width, event.xexpose.y + event.xexpose.height);
	Print (&rect);
        break;
      }
    }
  return false;
}

