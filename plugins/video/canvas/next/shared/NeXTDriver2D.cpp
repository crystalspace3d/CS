//=============================================================================
//
//	Copyright (C)1999-2001 by Eric Sunshine <sunshine@sunshineco.com>
//
// The contents of this file are copyrighted by Eric Sunshine.  This work is
// distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  You may distribute this file provided that this
// copyright notice is retained.  Send comments to <sunshine@sunshineco.com>.
//
//=============================================================================
//-----------------------------------------------------------------------------
// NeXTDriver2D.cpp
//
//	NeXT-specific subclass of csGraphics2D which implements 2D graphic
//	functionality via the AppKit.  This file contains code which is shared
//	between the MacOS/X, MacOS/X Server 1.0 (Rhapsody), OpenStep, and
//	NextStep platforms.  See NeXTDelegate2D.m for the platform-specific
//	portion of the 2D driver implementation.
//
//-----------------------------------------------------------------------------
#include "cssysdef.h"
#include "NeXTDriver2D.h"
#include "NeXTFrameBuffer15.h"
#include "NeXTFrameBuffer32.h"
#include "csutil/cfgacc.h"
#include "iutil/cfgfile.h"
#include "iutil/cmdline.h"
#include "iutil/objreg.h"
#include "ivaria/reporter.h"
#include "isys/event.h"
#include "isys/system.h"
#include "csver.h"

#define NeXT_REPORTER_ID "crystalspace.canvas.next"

typedef void* NeXTDriverHandle2D;
#define N2D_PROTO(RET,FUNC) extern "C" RET NeXTDriver2D_##FUNC

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY(NeXTDriver2D)

SCF_EXPORT_CLASS_TABLE(next2d)
  SCF_EXPORT_CLASS_DEP(NeXTDriver2D, "crystalspace.graphics2d.next",
    "Crystal Space 2D driver for MacOS/X Server, OpenStep, and NextStep",
    "crystalspace.font.server.")
SCF_EXPORT_CLASS_TABLE_END

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
NeXTDriver2D::~NeXTDriver2D()
{
  if (controller != 0)
    NeXTDelegate2D_dispose(controller);
  if (frame_buffer != 0)
    delete frame_buffer;
  Memory = 0;
}


//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------
bool NeXTDriver2D::Initialize(iObjectRegistry* s)
{
  bool ok = superclass::Initialize(s);
  if (ok)
  {
    iSystem* sys = CS_GET_SYSTEM (s);	//@@@
    sys->CallOnEvents(&scfiPlugin, CSMASK_Broadcast);
    controller = NeXTDelegate2D_new(this);
    ok = init_driver(get_desired_depth());
  }
  return ok;
}


//-----------------------------------------------------------------------------
// init_driver
//-----------------------------------------------------------------------------
bool NeXTDriver2D::init_driver(int desired_depth)
{
  bool ok = true;
  switch (determine_bits_per_sample(desired_depth))
  {
    case 4: frame_buffer = new NeXTFrameBuffer15(Width, Height); break;
    case 8: frame_buffer = new NeXTFrameBuffer32(Width, Height); break;
    default: ok = false; break;
  }

  if (ok)
  {
    Depth  = frame_buffer->depth();
    Memory = frame_buffer->get_raw_buffer();
    pfmt.RedMask    = frame_buffer->red_mask();
    pfmt.GreenMask  = frame_buffer->green_mask();
    pfmt.BlueMask   = frame_buffer->blue_mask();
    pfmt.PixelBytes = frame_buffer->bytes_per_pixel();
    pfmt.PalEntries = frame_buffer->palette_entries();
    pfmt.complete();
    switch (Depth)
    {
      case 15: setup_rgb_15(); break;
      case 32: setup_rgb_32(); break;
    }
  }
  else
    csReport(object_reg, CS_REPORTER_SEVERITY_ERROR,
      NeXT_REPORTER_ID, "Bizarre internal error; support for 15- and 32-bit "
      "RGB only");
  return ok;
}


//-----------------------------------------------------------------------------
// get_desired_depth()
//	Check if default device depth setting is overriden with a command-line
//	switch or via a configuration file setting.
//
//	Only depths of 15- and 32-bit are supported.  16-bit is treated as an
//	alias for 15-bit.  See CS/docs/texinfo/internal/platform/next.txi for
//	complete details concerning this limitation.
//-----------------------------------------------------------------------------
int NeXTDriver2D::get_desired_depth() const
{
  int depth = 0;
  iCommandLineParser* cmdline = CS_QUERY_REGISTRY (object_reg,
  	iCommandLineParser);
  char const* s = cmdline->GetOption("simdepth");
  if (s != 0)
    depth = atoi(s);
  else
  {
    csConfigAccess cfg(object_reg, "/config/video.cfg");
    depth = cfg->GetInt("Video.SimulateDepth", 0);
  }
  if (depth != 0 && depth != 15 && depth != 16 && depth != 32)
  {
    csReport(object_reg,
      CS_REPORTER_SEVERITY_WARNING, NeXT_REPORTER_ID, 
      "WARNING: Ignoring request to simulate %d-bit RGB depth.\n"
      "WARNING: Can only simulate 15-, 16-, or 32-bit RGB depth.", depth);
    depth = 0;
  }
  return depth;
}


//-----------------------------------------------------------------------------
// determine_bits_per_sample
//	Only depths of 15- and 32-bit are supported.  16-bit is treated as an
//	alias for 15-bit.  See CS/docs/texinfo/internal/platform/next.txi for
//	complete details concerning this limitation.
//-----------------------------------------------------------------------------
int NeXTDriver2D::determine_bits_per_sample(int desired_depth) const
{
  int bps = 0;
  switch (desired_depth)
  {
    case  0: bps = NeXTDelegate2D_best_bits_per_sample(controller); break;
    case 15: bps = 4; break;
    case 16: bps = 4; break; // An alias for 15-bit.
    case 32: bps = 8; break;
  }
  return bps;
}


//-----------------------------------------------------------------------------
// setup_rgb_15
//-----------------------------------------------------------------------------
void NeXTDriver2D::setup_rgb_15()
{
  _DrawPixel = DrawPixel16;
  _WriteString = WriteString16;
  _GetPixelAt = GetPixelAt16;
}


//-----------------------------------------------------------------------------
// setup_rgb_32
//-----------------------------------------------------------------------------
void NeXTDriver2D::setup_rgb_32()
{
  _DrawPixel = DrawPixel32;
  _WriteString = WriteString32;
  _GetPixelAt = GetPixelAt32;
}


//-----------------------------------------------------------------------------
// system_extension
//-----------------------------------------------------------------------------
bool NeXTDriver2D::system_extension(char const* msg, va_list args) const
{
  iSystem* sys = CS_GET_SYSTEM (object_reg);	//@@@
  return sys->PerformExtensionV(msg, args);
}

N2D_PROTO(int,system_extension_v)
  (NeXTDriverHandle2D handle, char const* msg, va_list args)
  { return ((NeXTDriver2D*)handle)->system_extension(msg, args); }

N2D_PROTO(int,system_extension)
  (NeXTDriverHandle2D handle, char const* msg, ...)
{
  va_list args;
  va_start(args, msg);
  int const rc = NeXTDriver2D_system_extension_v(handle, msg, args);
  va_end(args);
  return rc;
}


//-----------------------------------------------------------------------------
// user_close
//-----------------------------------------------------------------------------
void NeXTDriver2D::user_close() const
{
  iSystem* sys = CS_GET_SYSTEM (object_reg);	//@@@
  sys->GetSystemEventOutlet()->Broadcast(
    cscmdContextClose, (iGraphics2D*)this);
  sys->PerformExtension("requestshutdown");
}

N2D_PROTO(void,user_close)(NeXTDriverHandle2D handle)
  { ((NeXTDriver2D*)handle)->user_close(); }


//-----------------------------------------------------------------------------
// Open
//-----------------------------------------------------------------------------
bool NeXTDriver2D::Open()
{
  csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY,
    NeXT_REPORTER_ID, CS_PLATFORM_NAME " 2D graphics driver for Crystal Space "
    CS_VERSION_NUMBER "\nWritten by Eric Sunshine <sunshine@sunshineco.com>");
  
  int ok = 0;
  if (superclass::Open())
    ok = NeXTDelegate2D_open_window(controller, win_title, Width, Height,
       frame_buffer->get_cooked_buffer(),
       frame_buffer->bits_per_sample());
  return (bool)ok;
}


//-----------------------------------------------------------------------------
// Close
//-----------------------------------------------------------------------------
void NeXTDriver2D::Close()
{
  NeXTDelegate2D_close_window(controller);
  superclass::Close();
}


//-----------------------------------------------------------------------------
// SetTitle
//-----------------------------------------------------------------------------
void NeXTDriver2D::SetTitle(char const* s)
{
  if (controller != 0)
    NeXTDelegate2D_set_window_title(controller, s);
  superclass::SetTitle(s);
}


//-----------------------------------------------------------------------------
// Print -- A misnomer; actually flushes frame buffer to display.
//-----------------------------------------------------------------------------
void NeXTDriver2D::Print(csRect*)
{
  frame_buffer->cook();
  NeXTDelegate2D_flush(controller);
}


//-----------------------------------------------------------------------------
// SetMouseCursor
//-----------------------------------------------------------------------------
bool NeXTDriver2D::SetMouseCursor(csMouseCursorID shape)
{
  return (bool)NeXTDelegate2D_set_mouse_cursor(controller, shape);
}


//-----------------------------------------------------------------------------
// HandleEvent
//-----------------------------------------------------------------------------
bool NeXTDriver2D::HandleEvent(iEvent& e)
{
  bool rc = false;
  if (e.Type == csevBroadcast)
  {
    switch (e.Command.Code)
    {
      case cscmdCommandLineHelp:
	usage_summary();
	rc = true;
	break;
      case cscmdFocusChanged:
	NeXTDelegate2D_focus_changed(controller, (bool)e.Command.Info);
	rc = true;
	break;
    }
  }
  return rc;
}


//-----------------------------------------------------------------------------
// usage_summary
//-----------------------------------------------------------------------------
void NeXTDriver2D::usage_summary() const
{
  if (object_reg != 0)
    printf (
      "Options for " CS_PLATFORM_NAME " 2D graphics driver:\n"
      "  -simdepth=<depth>  Simulate depth (15, 16, or 32) (default=none)\n");
}

#undef N2D_PROTO
