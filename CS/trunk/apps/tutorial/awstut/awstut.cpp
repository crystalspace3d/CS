/*
    Copyright (C) 2002 by Jorrit Tyberghein

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
#include "cssys/sysfunc.h"
#include "csutil/cscolor.h"
#include "csutil/csevent.h"
#include "csutil/cmdhelp.h"
#include "csgeom/csrect.h"
#include "csgeom/csrectrg.h"
#include "cstool/csfxscr.h"
#include "cstool/initapp.h"
#include "qsqrt.h"

#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/fontserv.h"
#include "ivideo/txtmgr.h"
#include "ivideo/natwin.h"
#include "igraphic/imageio.h"
#include "ivaria/reporter.h"
#include "iutil/eventq.h"
#include "iutil/virtclk.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"

#include "awstut.h"


CS_IMPLEMENT_APPLICATION

// The global system driver
AwsTutorial *System;


//-----------------------------------------------------------------------------

#define QUERY_REG(myPlug, iFace, errMsg) \
  myPlug = CS_QUERY_REGISTRY (object_reg, iFace); \
  if (!myPlug) \
  { \
    Report (CS_REPORTER_SEVERITY_ERROR, errMsg); \
    return false; \
  }

AwsTutorial::AwsTutorial()
{
}

AwsTutorial::~AwsTutorial()
{
}

static bool AwsEventHandler (iEvent& ev)
{
  if (ev.Type == csevBroadcast && ev.Command.Code == cscmdProcess)
  {
    System->SetupFrame ();
    return true;
  }
  else if (ev.Type == csevBroadcast && ev.Command.Code == cscmdFinalProcess)
  {
    System->FinishFrame ();
    return true;
  }
  else
  {
    return System ? System->HandleEvent (ev) : false;
  }
}

void AwsTutorial::SetPass (void* awst, iAwsSource *)
{
  AwsTutorial* tut = (AwsTutorial*)awst;
  (void)tut; // @@@ TODO
  printf ("SetPass\n"); fflush (stdout);
}

void AwsTutorial::SetUser (void* awst, iAwsSource *)
{
  AwsTutorial* tut = (AwsTutorial*)awst;
  (void)tut; // @@@ TODO
  printf ("SetUser\n"); fflush (stdout);
}

void AwsTutorial::Login (void* awst, iAwsSource *)
{
  AwsTutorial* tut = (AwsTutorial*)awst;
  (void)tut; // @@@ TODO
  printf ("Login\n"); fflush (stdout);
}

bool AwsTutorial::Initialize (int argc, const char* const argv[])
{
  object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg) return false;

  if (!csInitializer::RequestPlugins (object_reg,
  	CS_REQUEST_VFS,
	CS_REQUEST_SOFTWARE3D,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_PLUGIN("crystalspace.window.alternatemanager", iAws),
	CS_REQUEST_END))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Could not init app!");
    return false;
  }

  if (!csInitializer::SetupEventHandler (object_reg, AwsEventHandler))
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "Could not setup event handler!");
    return false;
  }

  // Check for commandline help.
  if (csCommandLineHelper::CheckHelp (object_reg))
  {
    csCommandLineHelper::Help (object_reg);
    exit (0);
  }

  QUERY_REG (vc, iVirtualClock, "Couldn't find virtual clock!");
  QUERY_REG (myG3D, iGraphics3D, "Couldn't load iGraphics3D plugin!");
  QUERY_REG (myG2D, iGraphics2D, "Couldn't load iGraphics2D plugin!");
  QUERY_REG (aws, iAws, "Couldn't load iAws plugin!");

  // Open the main system. This will open all the previously loaded plug-ins.
  iNativeWindow* nw = myG2D->GetNativeWindow ();
  if (nw) nw->SetTitle ("AWS Tutorial");

  if (!csInitializer::OpenApplication (object_reg))
  {
    Report(CS_REPORTER_SEVERITY_ERROR, "Error opening system!");
    return false;
  }

  awsCanvas = csPtr<iAwsCanvas> (aws->CreateCustomCanvas (myG2D, myG3D));

  // In combination with the g2d->Clear() that happens in SetupFrame()
  // we also set the AWSF_AlwaysRedrawWindows flag so that resizing our
  // window doesn't leave trails.
  aws->SetFlag (AWSF_AlwaysRedrawWindows);
  aws->SetCanvas (awsCanvas);

  // Setup sink.
  iAwsSink* sink = aws->GetSinkMgr ()->CreateSink ((void*)this);
  sink->RegisterTrigger ("SetUserName", &SetUser);
  sink->RegisterTrigger ("SetPassword", &SetPass);
  sink->RegisterTrigger ("Login", &Login);
  aws->GetSinkMgr ()->RegisterSink ("testButtonSink", sink);

  // now load preferences
  if (!aws->GetPrefMgr()->Load ("/this/data/temp/windows_skin.def"))
    Report(CS_REPORTER_SEVERITY_ERROR, "couldn't load skin definition file!");
  if (!aws->GetPrefMgr()->Load ("/this/data/temp/awstut.def"))
    Report(CS_REPORTER_SEVERITY_ERROR, "couldn't load definition file!");
  aws->GetPrefMgr ()->SelectDefaultSkin ("Windows");

  iAwsWindow *test = aws->CreateWindowFrom ("LoginWindow");
  if (test) test->Show ();

  return true;
}

void AwsTutorial::SetupFrame()
{
  // Start drawing 2D graphics.
  if (!myG3D->BeginDraw (CSDRAW_2DGRAPHICS)) return;

  myG2D->Clear (0);
  aws->Redraw ();
  aws->Print (myG3D, 64);
}

void AwsTutorial::FinishFrame ()
{
  myG3D->FinishDraw ();
  myG3D->Print (0);
}

bool AwsTutorial::HandleEvent (iEvent &Event)
{
  if (Event.Type == csevKeyDown && Event.Key.Code == CSKEY_ESC)
  {
    csRef<iEventQueue> q (CS_QUERY_REGISTRY (object_reg, iEventQueue));
    if (q)
      q->GetEventOutlet()->Broadcast (cscmdQuit);
    return true;
  }

  return aws->HandleEvent (Event);
}

void AwsTutorial::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (CS_QUERY_REGISTRY (object_reg, iReporter));
  if (rep)
    rep->ReportV (severity, "crystalspace.application.awstut", msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

int main (int argc, char *argv[])
{
  // Create our main class.
  System = new AwsTutorial();

  // Initialize the main system. This will load all needed plug-ins
  // and initialize them.
  if (System->Initialize (argc, argv))
    csDefaultRunLoop (System->object_reg);// Main loop.
  else
    System->Report (CS_REPORTER_SEVERITY_ERROR, "Error initializing system!");

  delete System;
  return 0;
}

