//=============================================================================
//
//	Copyright (C)1999 by Eric Sunshine <sunshine@sunshineco.com>
//
// The contents of this file are copyrighted by Eric Sunshine.  This work is
// distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  You may distribute this file provided that this
// copyright notice is retained.  Send comments to <sunshine@sunshineco.com>.
//
//=============================================================================
//-----------------------------------------------------------------------------
// NeXTProxy2D.cpp
//
//	C++ object which interacts with Objective-C world on behalf of
//	NeXTDriver2D which can not directly interface with Objective-C on
//	account of COM-related conflicts.  See README.NeXT for details.
//
// *WARNING* Do NOT include any COM headers in this file.
//-----------------------------------------------------------------------------
#include "sysdef.h"
#include "NeXTProxy2D.h"
#include "NeXTDelegate.h"
#include "NeXTFrameBuffer15.h"
#include "NeXTFrameBuffer32.h"
#include "NeXTView.h"

extern "Objective-C" {
#import <AppKit/NSApplication.h>
#import <AppKit/NSBitmapImageRep.h>
#import <AppKit/NSCursor.h>
#import <AppKit/NSDPSContext.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSWindow.h>
}
extern "C" {
#include <assert.h>
}

//-----------------------------------------------------------------------------
// window_server_depth
//	Directly query the Window Server for its preferred depth.  Note that
//	this value may be different from the depth limit reported by the
//	NSWindow class.  See get_device_depth() for a full discussion.  The
//	Window Server is queried for its preferred depth by creating a small
//	image cache which is painted with color so as to promote it from gray
//	to color.  The Window Server is then asked to return an
//	NSBitmapImageRep holding the contents of the image cache.  The Window
//	Server always returns the NSBitmapImageRep in the format which it most
//	prefers.
//-----------------------------------------------------------------------------
static NSWindowDepth window_server_depth()
    {
    NSRect const r = {{ 0, 0 }, { 4, 4 }};
    NSImage* image = [[NSImage alloc] initWithSize:r.size];
    [image lockFocus];
    [[NSColor blueColor] set];
    NSRectFill(r);
    NSBitmapImageRep* p = [[NSBitmapImageRep alloc] initWithFocusedViewRect:r];
    NSWindowDepth depth = NSBestDepth( [p colorSpaceName], [p bitsPerSample],
	[p bitsPerPixel], [p isPlanar], 0 );
    [p release];
    [image unlockFocus];
    [image release];
    return depth;
    }


//-----------------------------------------------------------------------------
// best_bits_per_sample
//	Determine the ideal number of bits per sample for the default display
//	depth.  All display depths are supported, though optimizations only
//	work for 12-bit RGB and 24-bit RGB.  Consequently this function only
//	reports 4 or 8 bits per sample, representing 12-bit and 24-bit depths,
//	respectively.  Other depths still work, but more slowly.  See
//	README.NeXT for details.
//
//	Note that as of OpenStep 4.1, the Window Server may prefer a depth
//	greater than that of the "deepest" screen as reported by the Window
//	class.  The reason for this is that "true" RGB/5:5:5 was implemented
//	in OpenStep 4.1.  Previously this had been simulated with 4:4:4.  A
//	consequence of this change is that for 5:5:5 displays, the Window
//	Server actually prefers 24-bit rather than 12-bit RGB as was the case
//	with previous versions.  It is important to note that the NSWindow
//	class still reports a depth limit of 12-bit even though the Window
//	Server prefers 24-bit.  Consequently, window_server_depth() is used to
//	directly query the WindowServer for its preference instead.  The
//	behavior in the OpenStep Window Server impacts all applications,
//	including those compiled with earlier versions of OpenStep or
//	NextStep.
//-----------------------------------------------------------------------------
static int best_bits_per_sample()
    {
    NSWindowDepth const depth = window_server_depth();
    NSString* const s = NSColorSpaceFromDepth( depth );
    if ([s isEqualToString:NSCalibratedRGBColorSpace] ||
	[s isEqualToString:NSDeviceRGBColorSpace])
        {
	int const bps = NSBitsPerSampleFromDepth( depth );
	if (bps == 4 || bps == 8)
	    return bps;
	}
    return 4;
    }


//-----------------------------------------------------------------------------
// determine_bits_per_sample
//-----------------------------------------------------------------------------
static int determine_bits_per_sample( int simulate_depth )
    {
    int bps;
    switch (simulate_depth)
	{
	case 15: bps = 4; break;
	case 32: bps = 8; break;
	default: bps = best_bits_per_sample(); break;
	}
    return bps;
    }


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
NeXTProxy2D::NeXTProxy2D( unsigned int w, unsigned int h, int simulate_depth ):
    window(0), view(0), width(w), height(h), frame_buffer(0)
    {
    switch (determine_bits_per_sample( simulate_depth ))
	{
	case 4: frame_buffer = new NeXTFrameBuffer15( width, height ); break;
	case 8: frame_buffer = new NeXTFrameBuffer32( width, height ); break;
	}
    }


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
NeXTProxy2D::~NeXTProxy2D()
    {
    [[NSApp delegate] showMouse];
    [[NSApp delegate] registerAnimationWindow:0];
    [window setDelegate:0];
    [window close];
    [window release];	// Window releases NeXTView.
    delete frame_buffer;
    }


//-----------------------------------------------------------------------------
// open
//	Opens a titled Window and installs a NeXTView as its contentView.
//	Registers the window with the application's delegate as its "animation
//	window".  Passes the raw frame-buffer along to the NeXTView which blits
//	it directly to the WindowServer via NXBitmapImageRep.
//
// *NOTE*
//	Window must have valid PostScript windowNum before registering with
//	application's delegate since a tracking rectangle is set up.
//	Therefore window must be on-screen before registering the window.  The
//	alternative of using a non-deferred window does not seem to be an
//	option since, for some inexplicable reason, the contents of a Retained
//	non-deferred window are never drawn.
//-----------------------------------------------------------------------------
bool NeXTProxy2D::open( char const* title )
    {
    NSRect const r = {{ 0, 0 }, { width, height }};
    window = [[NSWindow alloc]
	initWithContentRect:r
	styleMask:(NSTitledWindowMask | NSClosableWindowMask)
	backing:NSBackingStoreRetained
	defer:YES];
    [window setTitle:[NSString stringWithCString:title]];
    [window setReleasedWhenClosed:NO];

    view = [[NeXTView alloc] initWithFrame:r];
    [view setFrameBuffer:frame_buffer->get_cooked_buffer()
	bitsPerSample:frame_buffer->bits_per_sample()];
    [window setContentView:view];

    [window center];
    [window makeFirstResponder:view];
    [window makeKeyAndOrderFront:0];
    [[NSApp delegate] registerAnimationWindow:window];	// *NOTE*
    return true;
    }


//-----------------------------------------------------------------------------
// close
//-----------------------------------------------------------------------------
void NeXTProxy2D::close()
    {
    [window close];
    }


//-----------------------------------------------------------------------------
// flush
//	Convert the CrystalSpace frame-buffer into NeXT format and blit the
//	image to the display.
//-----------------------------------------------------------------------------
void NeXTProxy2D::flush()
    {
    assert( frame_buffer != 0 );
    frame_buffer->cook();
    [view flush];
    [[NSDPSContext currentContext] flush];
    }


//-----------------------------------------------------------------------------
// set_mouse_cursor
//-----------------------------------------------------------------------------
bool NeXTProxy2D::set_mouse_cursor( csMouseCursorID shape, iTextureHandle* )
    {
    bool handled = false;
    if (shape == csmcArrow)
	{
	[[NSCursor arrowCursor] set];
	handled = true;
	}

    if (handled)
	[[NSApp delegate] showMouse];
    else
	[[NSApp delegate] hideMouse];
    return handled;
    }
