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

#include "simpvs.h"

CS_IMPLEMENT_APPLICATION

//-----------------------------------------------------------------------------

Simple::Simple ()
{
  SetApplicationName ("CrystalSpace.SimpVS");
}

Simple::~Simple ()
{
}

bool Simple::Setup ()
{
  // The virtual clock.
  vc = CS_QUERY_REGISTRY (GetObjectRegistry (), iVirtualClock);
  if (!vc)
    return ReportError("Can't find the virtual clock!");

  // Find the pointer to engine plugin
  engine = CS_QUERY_REGISTRY (GetObjectRegistry (), iEngine);
  if (!engine)
    return ReportError("No iEngine plugin!");

  loader = CS_QUERY_REGISTRY (GetObjectRegistry (), iLoader);
  if (!loader)
    return ReportError("No iLoader plugin!");

  kbd = CS_QUERY_REGISTRY (GetObjectRegistry (), iKeyboardDriver);
  if (!kbd)
    return ReportError("No iKeyboardDriver plugin!");

  g3d = CS_QUERY_REGISTRY (GetObjectRegistry (), iGraphics3D);
  if (!g3d)
    return ReportError("No iGraphics3D plugin!");

  vfs = CS_QUERY_REGISTRY(GetObjectRegistry(), iVFS);
  if (!vfs) 
    return ReportError("Failed to locate Virtual FileSystem!");

  cegui = CS_QUERY_REGISTRY(GetObjectRegistry(), iCEGUI);
  if (!cegui) 
    return ReportError("Failed to locate CEGUI plugin");

  // First disable the lighting cache. Our app is simple enough
  // not to need this.
  engine->SetLightingCacheMode (0);

  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
    return ReportError("Error loading 'stone4' texture!");
  iMaterialWrapper* tm = engine->GetMaterialList ()->FindByName ("stone");

  room = engine->CreateSector ("room");
  csRef<iMeshWrapper> walls (engine->CreateSectorWallsMesh (room, "walls"));
  csRef<iThingFactoryState> walls_state = 
    scfQueryInterface<iThingFactoryState> (walls->GetMeshObject ()->GetFactory());
  walls_state->AddInsideBox (csVector3 (-5, 0, -5), csVector3 (5, 20, 5));
  walls_state->SetPolygonMaterial (CS_POLYRANGE_LAST, tm);
  walls_state->SetPolygonTextureMapping (CS_POLYRANGE_LAST, 3);

  csRef<iLight> light;
  iLightList* ll = room->GetLights ();

  light = engine->CreateLight (0, csVector3 (-3, 5, 0), 10,
  	csColor (1, 0, 0));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (3, 5,  0), 10,
  	csColor (0, 0, 1));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (0, 5, -3), 10,
  	csColor (0, 1, 0));
  ll->Add (light);

  engine->Prepare ();

  view.AttachNew (new csView (engine, g3d));
  view->GetCamera ()->SetSector (room);
  view->GetCamera ()->GetTransform ().SetOrigin (csVector3 (0, 5, -3));
  iGraphics2D* g2d = g3d->GetDriver2D ();
  view->SetRectangle (0, 0, g2d->GetWidth (), g2d->GetHeight ());

  CreateGui();

  return true;
}

void Simple::SetupFrame ()
{
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks ();

  // Now rotate the camera according to keyboard state
  float speed = (elapsed_time / 1000.0) * (0.03 * 20);

  iCamera* c = view->GetCamera();
  if (kbd->GetKeyState (CSKEY_RIGHT))
    c->GetTransform ().RotateThis (CS_VEC_ROT_RIGHT, speed);
  if (kbd->GetKeyState (CSKEY_LEFT))
    c->GetTransform ().RotateThis (CS_VEC_ROT_LEFT, speed);
  if (kbd->GetKeyState (CSKEY_PGUP))
    c->GetTransform ().RotateThis (CS_VEC_TILT_UP, speed);
  if (kbd->GetKeyState (CSKEY_PGDN))
    c->GetTransform ().RotateThis (CS_VEC_TILT_DOWN, speed);
  if (kbd->GetKeyState (CSKEY_UP))
    c->Move (CS_VEC_FORWARD * 4 * speed);
  if (kbd->GetKeyState (CSKEY_DOWN))
    c->Move (CS_VEC_BACKWARD * 4 * speed);

  // Tell 3D driver we're going to display 3D things.
  if (!g3d->BeginDraw (engine->GetBeginDrawFlags () | CSDRAW_3DGRAPHICS))
    return;

  // Tell the camera to render into the frame buffer.
  view->Draw ();

  if (!g3d->BeginDraw (CSDRAW_2DGRAPHICS)) 
    return;

  cegui->Render();
}

void Simple::FinishFrame ()
{
  g3d->FinishDraw ();
  g3d->Print (0);
}

bool Simple::HandleEvent (iEvent& ev)
{
  bool res = false;
  if (ev.Name == csevProcess(object_reg))
  {
    SetupFrame ();
    res = true;
  }
  else if (ev.Name == csevFinalProcess(object_reg))
  {
    FinishFrame ();
    res = true;
  }
  else if ((ev.Name == csevKeyboardDown(object_reg)) &&
    (csKeyEventHelper::GetCookedCode (&ev) == CSKEY_ESC))
  {
    csRef<iEventQueue> q (CS_QUERY_REGISTRY (GetObjectRegistry (), iEventQueue));
    if (q)
      q->GetEventOutlet()->Broadcast (csevQuit(object_reg));
    res = true;
  }

  return res;
}

void Simple::SaveVideoPreference()
{
  csRef<iConfigFile> userConfig (csGetPlatformConfig (GetApplicationName()));
  csConfigAccess config (GetObjectRegistry (), userConfig, 
    iConfigManager::ConfigPriorityUserApp);
  config->SetStr ("System.Plugins.iGraphics3D", mode);
}

bool Simple::OnInitialize(int /*argc*/, char* /*argv*/ [])
{
  if (!csInitializer::SetupConfigManager (GetObjectRegistry (), "/config/simpvs.cfg",
    GetApplicationName()))
    return ReportError("Failed to initialize config!");

  Process = csevProcess (GetObjectRegistry ());
  FinalProcess = csevFinalProcess (GetObjectRegistry ());
  KeyboardDown = csevKeyboardDown (GetObjectRegistry ());
  Quit = csevQuit (GetObjectRegistry ());

  if (!csInitializer::RequestPlugins (GetObjectRegistry (),
	CS_REQUEST_VFS,
	CS_REQUEST_ENGINE,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_LEVELLOADER,
	CS_REQUEST_PLUGIN ("crystalspace.cegui.wrapper", iCEGUI),
	CS_REQUEST_REPORTER,
	CS_REQUEST_REPORTERLISTENER,
	CS_REQUEST_END))
    return ReportError("Can't initialize plugins!");

  csBaseEventHandler::Initialize(GetObjectRegistry());

  if (!RegisterQueue(GetObjectRegistry(), csevAllEvents(GetObjectRegistry())))
    return ReportError("Failed to set up event handler!");

  return true;
}

void Simple::CreateGui()
{
  // Initialize CEGUI wrapper
  cegui->Initialize ();

  // Set the logging level
  cegui->GetLoggerPtr ()->setLoggingLevel(CEGUI::Informative);

#if (CEGUI_VERSION_MAJOR == 0) && (CEGUI_VERSION_MINOR >= 5)
  // Use the 0.5 version of the skin
  vfs->ChDir ("/ceguitest/0.5/");
#else
  // Use the old version of the skin
  vfs->ChDir ("/ceguitest/");
#endif

  // Load the ice skin (which uses Falagard skinning system)
  cegui->GetSchemeManagerPtr ()->loadScheme("ice.scheme");

  cegui->GetSystemPtr ()->setDefaultMouseCursor("ice", "MouseArrow");

#if (CEGUI_VERSION_MAJOR == 0) && (CEGUI_VERSION_MINOR >= 5)
  CEGUI::Font* font = cegui->GetFontManagerPtr ()->createFont("FreeType",
    "Vera", "/fonts/ttf/Vera.ttf");
  font->setProperty("PointSize", "10");
  font->load();
#else
  cegui->GetFontManagerPtr ()->createFont("Vera", "/fonts/ttf/Vera.ttf", 10,
    CEGUI::Default);
#endif

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  // Load layout and set as root
  cegui->GetSystemPtr ()->setGUISheet(winMgr->loadWindowLayout("simpvs.layout"));

  CEGUI::Window* btn = 0;
  // ----[ GENERAL ]---------------------------------------------------------

  btn = winMgr->getWindow("Root/ButtonPane/SoftwareButton");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&Simple::SetSoftware, this));

  btn = winMgr->getWindow("Root/ButtonPane/OpenGLButton");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&Simple::SetOpenGL, this));
}

bool Simple::Application()
{
  // Open the main system. This will open all the previously loaded plug-ins.
  if (!Open())
    return ReportError("Error opening system!");

  if (!Setup ())
    return false;

  Run();

  return true;
}

bool Simple::SetSoftware (const CEGUI::EventArgs& e)
{
  csPrintf ("Software mode!\n"); fflush (stdout);

  mode = "crystalspace.graphics3d.software";

  SaveVideoPreference();
  Restart();

  return true;
}

bool Simple::SetOpenGL (const CEGUI::EventArgs& e)
{
  csPrintf ("OpenGL mode!\n"); fflush (stdout);

  mode = "crystalspace.graphics3d.opengl";

  SaveVideoPreference();
  Restart();

  return true;
}

/*-------------------------------------------------------------------------*
 * Main function
 *-------------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  return csApplicationRunner<Simple>::Run (argc, argv);
}
