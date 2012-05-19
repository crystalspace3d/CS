#include "test.h"
#include <iostream>
CS_IMPLEMENT_APPLICATION

Simple::Simple ()
{
  SetApplicationName ("CrystalSpace.Simple1");
}

Simple::~Simple ()
{
}

void Simple::Frame ()
{  
  csTicks elapsed_time = vc->GetElapsedTicks ();
  float speed = (elapsed_time / 1000.0) * (0.06 * 20);
  iCamera* c = view->GetCamera();
  if (kbd->GetKeyState (CSKEY_SHIFT))
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      c->Move (CS_VEC_RIGHT * 4 * speed);
    if (kbd->GetKeyState (CSKEY_LEFT))
      c->Move (CS_VEC_LEFT * 4 * speed);
    if (kbd->GetKeyState (CSKEY_UP))
      c->Move (CS_VEC_UP * 4 * speed);
    if (kbd->GetKeyState (CSKEY_DOWN))
      c->Move (CS_VEC_DOWN * 4 * speed);

  }
  else
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      rotY += speed;
    if (kbd->GetKeyState (CSKEY_LEFT))
      rotY -= speed;
    if (kbd->GetKeyState (CSKEY_PGUP))
      rotX += speed;
    if (kbd->GetKeyState (CSKEY_PGDN))
      rotX -= speed;
    if (kbd->GetKeyState (CSKEY_UP))
      c->Move (CS_VEC_FORWARD * 4 * speed);
    if (kbd->GetKeyState (CSKEY_DOWN))
      c->Move (CS_VEC_BACKWARD * 4 * speed);
  }
  csMatrix3 rot = csXRotMatrix3 (rotX) * csYRotMatrix3 (rotY);
  csOrthoTransform ot (rot, c->GetTransform().GetOrigin ());
  c->SetTransform (ot);  
}

bool Simple::OnKeyboard(iEvent& ev)
{
  csKeyEventType eventtype = csKeyEventHelper::GetEventType(&ev);
  if (eventtype == csKeyEventTypeDown)
  {
    utf32_char code = csKeyEventHelper::GetCookedCode(&ev);
    if (code == CSKEY_ESC)
    {
      csRef<iEventQueue> q = 
        csQueryRegistry<iEventQueue> (GetObjectRegistry());
      if (q.IsValid()) q->GetEventOutlet()->Broadcast(
      	csevQuit(GetObjectRegistry()));
    }
  }
  return false;
}

bool Simple::OnInitialize(int /*argc*/, char* /*argv*/ [])
{
  // RequestPlugins() will load all plugins we specify. In addition
  // it will also check if there are plugins that need to be loaded
  // from the config system (both the application config and CS or
  // global configs). In addition it also supports specifying plugins
  // on the commandline.
  if (!csInitializer::RequestPlugins(GetObjectRegistry(),
    CS_REQUEST_VFS,
    CS_REQUEST_OPENGL3D,
    CS_REQUEST_ENGINE,
    CS_REQUEST_FONTSERVER,
    CS_REQUEST_IMAGELOADER,
    CS_REQUEST_LEVELLOADER,
    CS_REQUEST_REPORTER,
    CS_REQUEST_REPORTERLISTENER,
    CS_REQUEST_PLUGIN("crystalspace.clouds", iClouds),	// Our clouds
    CS_REQUEST_END))
    return ReportError("Failed to initialize plugins!");

  // "Warm up" the event handler so it can interact with the world
  csBaseEventHandler::Initialize(GetObjectRegistry());

  // Now we need to register the event handler for our application.
  // Crystal Space is fully event-driven. Everything (except for this
  // initialization) happens in an event.
  // Rather than simply handling all events, we subscribe to the
  // particular events we're interested in.
  csEventID events[] = {
    csevFrame (GetObjectRegistry()),
    csevKeyboardEvent (GetObjectRegistry()),
    CS_EVENTLIST_END
  };
  if (!RegisterQueue(GetObjectRegistry(), events))
    return ReportError("Failed to set up event handler!"); 
  // Report success
  return true;
}

void Simple::OnExit()
{
  // Shut down the event handlers we spawned earlier.
  drawer.Invalidate();
  printer.Invalidate();
}

bool Simple::Application()
{
  // Open the main system. This will open all the previously loaded plug-ins.
  // i.e. all windows will be opened.
  if (!OpenApplication(GetObjectRegistry()))
    return ReportError("Error opening system!");

  if (SetupModules()) {
    // This calls the default runloop. This will basically just keep
    // broadcasting process events to keep the game going.
    Run();
  }

  return true;
}

bool Simple::SetupModules ()
{

  g2d = csQueryRegistry<iGraphics2D> (GetObjectRegistry());
  if (!g2d) return ReportError("Failed to locate 2D renderer!");
  g2d->DoubleBuffer(false);

  g3d = csQueryRegistry<iGraphics3D> (GetObjectRegistry());
  if (!g3d) return ReportError("Failed to locate 3D renderer!");
  engine = csQueryRegistry<iEngine> (GetObjectRegistry());
  if (!engine) return ReportError("Failed to locate 3D engine!");
  vc = csQueryRegistry<iVirtualClock> (GetObjectRegistry());
  if (!vc) return ReportError("Failed to locate Virtual Clock!");
  kbd = csQueryRegistry<iKeyboardDriver> (GetObjectRegistry());
  if (!kbd) return ReportError("Failed to locate Keyboard Driver!");
  loader = csQueryRegistry<iLoader> (GetObjectRegistry());
  if (!loader) return ReportError("Failed to locate Loader!");

  // We need a View to the virtual world.
  view.AttachNew(new csView (engine, g3d));
  iGraphics2D* g2d = g3d->GetDriver2D ();
  // We use the full window to draw the world.
  view->SetRectangle (0, 0, g2d->GetWidth (), g2d->GetHeight ());
  
  engine->SetLightingCacheMode (0);
  CreateRoom();
  engine->Prepare ();
  rotY = rotX = 0; // Cam
  // Now we need to position the camera in our world.
  view->GetCamera ()->SetSector (room);
  view->GetCamera ()->GetTransform ().SetOrigin (csVector3 (0, 5, -3));
  // We use some other "helper" event handlers to handle pushing our work into the 3D engine and rendering it to the screen.
  drawer.AttachNew(new FrameBegin3DDraw (GetObjectRegistry (), view));
  printer.AttachNew(new FramePrinter (GetObjectRegistry ()));
  return true;
}

void Simple::CloudTest (iSector* sector, int nbr, csVector3 pos, csVector3 box)
{
  csRef<iClouds> clouds = csQueryRegistry<iClouds> (GetObjectRegistry ());

  // Let s create a global linear color effector
  csRef<iParticleBuiltinEffectorFactory> eff_factory = 
        csLoadPluginCheck<iParticleBuiltinEffectorFactory> (
        object_reg, "crystalspace.mesh.object.particles.effector", false);
  csRef<iParticleBuiltinEffectorLinColor> lincol = eff_factory->CreateLinColor (); 
  lincol->AddColor (csColor4 (.05,.05,.05, 1), 10.0/* ttl */);

  //----------------------------------
  // Generate some puffs

#define PUFF 		3			// 3 differents puffs types
#define PUFFPATH	"/textures/clouds/puff"

  CloudPuff* myPuff[PUFF]; 			// Just keep a local track of our puffs
  for (int i = 0; i < PUFF; i++)
  {
    // Load materials/tex for now
    csString name ("puff"), path (PUFFPATH);
    name += i;
    path += i;
    path += ".png";
    iTextureWrapper* tex = loader->LoadTexture (name, path);
    if (!tex)
       ReportError ("Clouds::GenerateClouds:Cannot set material/load texture");
    // Per Puff parameters could be randomized in some ranges
    myPuff[i] = clouds->PushPuff (name,
                           5,			// num
                           5.2,			// speed
                           10.0,		// ttl 
                           csVector3(0.2,0,0), 	// velocity
                           csVector2(0.4, 0.4),	// particle size
                           &lincol,		// color eff
                           0			// force eff
                          );  
  }

  //----------------------------------
  // Generate a bunch of clouds in our area
  Cloud* myCloud;
  csVector3 Area(30.0, 0.0, 30.0);
  for (int i = 0; i < nbr; i++)
  {
    csString name ("cloud");
    name += i;
    // Per Cloud parameters could be randomized in some ranges
    myCloud = clouds->PushCloud (name,
                         csVector3(5,2,5),	// shape
                         csVector3(.5,.5,.5),	// min puff size
                         csVector3(1.8,1.8,1.8),// max puff size
                         csVector2(6, 12)	// min/max puff nbr
                        );

    // Add all differents kind of puff to our cloud
    for (int j = 0; j < PUFF; j++)
      clouds->CloudAddPuff (myCloud, myPuff[j]);
    srand (i);
    csVector3 myPos =  csVector3 (-Area.x / 2.0 + (float) (rand() / (double)RAND_MAX) * Area.x,
                                  (float) (rand() / (double)RAND_MAX) * Area.y,
                                  -Area.z / 2.0 + (float) (rand() / (double)RAND_MAX) * Area.z
                                 );
    clouds->CloudGen (sector, myCloud, myPos);
  }
}

void Simple::CreateRoom ()
{
  // Load the texture from the standard library.  This is located in
  // CS/data/standard.zip and mounted as /lib/std using the Virtual
  // File System (VFS) plugin.
  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
    ReportError("Error loading 'stone4' texture!");
  iMaterialWrapper* tm = engine->GetMaterialList ()->FindByName ("stone");
  // We create a new sector called "room".
  room = engine->CreateSector ("room");
  // Creating the walls for our room.
  csRef<iMeshWrapper> walls (engine->CreateSectorWallsMesh (room, "walls"));
  iMeshObject* walls_object = walls->GetMeshObject ();
  iMeshObjectFactory* walls_factory = walls_object->GetFactory();
  csRef<iThingFactoryState> walls_state = 
    scfQueryInterface<iThingFactoryState> (walls_factory);
  walls_state->AddInsideBox (csVector3 (-20, 0, -20), csVector3 (20, 20, 20));
  walls_state->SetPolygonMaterial (CS_POLYRANGE_LAST, tm);
  walls_state->SetPolygonTextureMapping (CS_POLYRANGE_LAST, 3);
  // Now we need light to see something.
  iLightList* ll = room->GetLights ();
  sun = engine->CreateLight (0, csVector3(0, 5, 0), 10, csColor(1, 1, 1));
  ll->Add (sun);
  // mound the puffs
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) ReportError ("Failed to locate VFS!");
  vfs->Mount("/textures/clouds", "data/cloud-puffs.zip");

  // Our clouds
  CloudTest(room, 10, csVector3(0,0,0), csVector3(10,10,10));
}

/*-------------------------------------------------------------------------*
 * Main function
 *-------------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  return csApplicationRunner<Simple>::Run (argc, argv);
}
