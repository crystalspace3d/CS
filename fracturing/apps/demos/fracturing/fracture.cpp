/*
Copyright (C) 2001 by Jorrit Tyberghein

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

#include "fracture.h"

using namespace CS::Collisions;

CS_IMPLEMENT_APPLICATION

//---------------------------------------------------------------------------

Fracture::Fracture()
{
	SetApplicationName("CrystalSpace.Fracture1");
}

Fracture::~Fracture()
{
}

void Fracture::Frame()
{
	// First get elapsed time from the virtual clock.
	csTicks elapsed_time = vc->GetElapsedTicks();
	// Now rotate the camera according to keyboard state
	float speed = (elapsed_time / 1000.0) * (0.06 * 20);

	iCamera* c = view->GetCamera();

	if (kbd->GetKeyState(CSKEY_SHIFT))
	{
		// If the user is holding down shift, the arrow keys will cause
		// the camera to strafe up, down, left or right from it's
		// current position.
		if (kbd->GetKeyState(CSKEY_RIGHT))
			c->Move(CS_VEC_RIGHT * 4 * speed);
		if (kbd->GetKeyState(CSKEY_LEFT))
			c->Move(CS_VEC_LEFT * 4 * speed);
		if (kbd->GetKeyState(CSKEY_UP))
			c->Move(CS_VEC_UP * 4 * speed);
		if (kbd->GetKeyState(CSKEY_DOWN))
			c->Move(CS_VEC_DOWN * 4 * speed);
	}
	else
	{
		// left and right cause the camera to rotate on the global Y
		// axis; page up and page down cause the camera to rotate on the
		// _camera's_ X axis (more on this in a second) and up and down
		// arrows cause the camera to go forwards and backwards.
		if (kbd->GetKeyState(CSKEY_RIGHT))
			rotY += speed;
		if (kbd->GetKeyState(CSKEY_LEFT))
			rotY -= speed;
		if (kbd->GetKeyState(CSKEY_PGUP))
			rotX += speed;
		if (kbd->GetKeyState(CSKEY_PGDN))
			rotX -= speed;
		if (kbd->GetKeyState(CSKEY_UP))
			c->Move(CS_VEC_FORWARD * 4 * speed);
		if (kbd->GetKeyState(CSKEY_DOWN))
			c->Move(CS_VEC_BACKWARD * 4 * speed);
	}

	// We now assign a new rotation transformation to the camera.  You
	// can think of the rotation this way: starting from the zero
	// position, you first rotate "rotY" radians on your Y axis to get
	// the first rotation.  From there you rotate "rotX" radians on the
	// your X axis to get the final rotation.  We multiply the
	// individual rotations on each axis together to get a single
	// rotation matrix.  The rotations are applied in right to left
	// order .
	csMatrix3 rot = csXRotMatrix3(rotX) * csYRotMatrix3(rotY);
	csOrthoTransform ot(rot, c->GetTransform().GetOrigin());
	c->SetTransform(ot);

	rm->RenderView(view);

	if (handle)
	{
		g3d->BeginDraw(CSDRAW_2DGRAPHICS);
		g3d->DrawPixmap(handle, 0, 0, 64, 64, 0, 0, 64, 64);
	}
}


bool Fracture::OnKeyboard(iEvent& ev)
{
	// We got a keyboard event.
	csKeyEventType eventtype = csKeyEventHelper::GetEventType(&ev);
	if (eventtype == csKeyEventTypeDown)
	{
		// The user pressed a key (as opposed to releasing it).
		utf32_char code = csKeyEventHelper::GetCookedCode(&ev);
		if (code == CSKEY_ESC)
		{
			// The user pressed escape to exit the application.
			// The proper way to quit a Crystal Space application
			// is by broadcasting a csevQuit event. That will cause the
			// main runloop to stop. To do that we get the event queue from
			// the object registry and then post the event.
			csRef<iEventQueue> q =
				csQueryRegistry<iEventQueue>(GetObjectRegistry());
			if (q.IsValid()) q->GetEventOutlet()->Broadcast(
				csevQuit(GetObjectRegistry()));
		}
		if (code == 't')
		{
			iTextureManager* txtmgr = g3d->GetTextureManager();

			handle = txtmgr->CreateTexture(64, 64, csimg2D, "rgba8", CS_TEXTURE_3D);
			g3d->SetRenderTarget(handle);
			g3d->BeginDraw(CSDRAW_2DGRAPHICS);
			iGraphics2D* g2d = g3d->GetDriver2D();

			int color = g2d->FindRGB(255, 0, 0);
			g2d->DrawBox(0, 0, 64, 64, color);

			csRef<iFont> font = g2d->GetFontServer()->LoadFont(CSFONT_COURIER);
			int fg = g2d->FindRGB(255, 255, 255);
			g2d->Write(font, 10, 10, fg, -1, "TESTING");

			g3d->FinishDraw();
			g3d->SetRenderTarget(0);
		}
	}

	return false;
}

bool Fracture::OnInitialize(int /*argc*/, char* /*argv*/[])
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
		CS_REQUEST_PLUGIN("crystalspace.utilities.bugplug",iBugPlug),
		CS_REQUEST_PLUGIN("crystalspace.dynamics.bullet",iDynamics),
		CS_REQUEST_PLUGIN("crystalspace.mesh.convexdecompose.hacd",iConvexDecomposer),
		CS_REQUEST_LEVELLOADER,
		CS_REQUEST_REPORTER,
		CS_REQUEST_REPORTERLISTENER,
		CS_REQUEST_CONSOLEOUT,
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
		csevFrame(GetObjectRegistry()),
		csevKeyboardEvent(GetObjectRegistry()),
		csevMouseEvent(GetObjectRegistry()),
		CS_EVENTLIST_END
	};

	if (!RegisterQueue(GetObjectRegistry(), events))
		return ReportError("Failed to set up event handler!");

	// Report success
	return true;
}

void Fracture::OnExit()
{
	// Shut down the event handlers we spawned earlier.
	printer.Invalidate();
}

bool Fracture::Application()
{
	// Open the main system. This will open all the previously loaded plug-ins.
	// i.e. all windows will be opened.
	if (!OpenApplication(GetObjectRegistry()))
		return ReportError("Error opening system!");

	if (SetupModules())
	{
		// This calls the default runloop. This will basically just keep
		// broadcasting process events to keep the game going.
		Run();
	}

	return true;
}

bool Fracture::SetupModules()
{
	// Now get the pointer to various modules we need. We fetch them
	// from the object registry. The RequestPlugins() call we did earlier
	// registered all loaded plugins with the object registry.
	g3d = csQueryRegistry<iGraphics3D>(GetObjectRegistry());
	if (!g3d) return ReportError("Failed to locate 3D renderer!");

	engine = csQueryRegistry<iEngine>(GetObjectRegistry());
	if (!engine) return ReportError("Failed to locate 3D engine!");

	vc = csQueryRegistry<iVirtualClock>(GetObjectRegistry());
	if (!vc) return ReportError("Failed to locate Virtual Clock!");

	kbd = csQueryRegistry<iKeyboardDriver>(GetObjectRegistry());
	if (!kbd) return ReportError("Failed to locate Keyboard Driver!");

	loader = csQueryRegistry<iLoader>(GetObjectRegistry());
	if (!loader) return ReportError("Failed to locate Loader!");

	// We need a View to the virtual world.
	view.AttachNew(new csView(engine, g3d));
	iGraphics2D* g2d = g3d->GetDriver2D();
	// We use the full window to draw the world.
	view->SetRectangle(0, 0, g2d->GetWidth(), g2d->GetHeight());

	// Here we create our world.
	CreateRoom();

	CreateTeapot();

	//PutRandomPoints();

	// Let the engine prepare the meshes and textures.
	engine->Prepare();

	// Now calculate static lighting for our geometry.
	using namespace CS::Lighting;
	SimpleStaticLighter::ShineLights(room, engine, 6);

	rm = engine->GetRenderManager();

	// These are used store the current orientation of the camera
	rotY = rotX = 0;

	// Now we need to position the camera in our world.
	view->GetCamera()->SetSector(room);
	view->GetCamera()->GetTransform().SetOrigin(csVector3(0,0,-6));
	view->GetCamera()->GetTransform().LookAt(csVector3(0,0,0),csVector3(0,1,0));

	// We use some other "helper" event handlers to handle 
	// pushing our work into the 3D engine and rendering it
	// to the screen.
	printer.AttachNew(new FramePrinter(GetObjectRegistry()));

	return true;
}

void Fracture::CreateRoom()
{
	// Load the texture from the standard library.  This is located in
	// CS/data/standard.zip and mounted as /lib/std using the Virtual
	// File System (VFS) plugin.
	if (!loader->LoadTexture("mystone2", "/lib/std/mystone2.gif"))
		ReportError("Error loading %s texture!",
		CS::Quote::Single("mystone2"));
	iMaterialWrapper* tm = engine->GetMaterialList()->FindByName("mystone2");


	// We create a new sector called "room".
	room = engine->CreateSector("room");

	// Creating the walls for our room.

	// First we make a primitive for our geometry.
	using namespace CS::Geometry;
	DensityTextureMapper mapper(0.3f);
	TesselatedBox box(csVector3(-10,-10,-10), csVector3(10,10,10));
	box.SetLevel(1);
	box.SetMapper(&mapper);
	box.SetFlags(Primitives::CS_PRIMBOX_INSIDE);

	// Now we make a factory and a mesh at once.
	csRef<iMeshWrapper> walls = GeneralMeshBuilder::CreateFactoryAndMesh(
		engine, room, "walls", "walls_factory", &box);
	walls->GetMeshObject()->SetMaterialWrapper(tm);

	//Now lets make a collider for our room mesh
	

	// Now we need light to see something.
	csRef<iLight> light;
	iLightList* ll = room->GetLights();

	light = engine->CreateLight(0, csVector3(-7,0,-7), 20, csColor(1, 1, 1));
	ll->Add(light); // Point A in notebook

	light = engine->CreateLight(0, csVector3(0,0,-7), 20, csColor(1, 1, 1));
	ll->Add(light); //Point B

	light = engine->CreateLight(0, csVector3(0,0,7), 20, csColor(1, 1, 1));
	ll->Add(light); //Point C

	light = engine->CreateLight(0, csVector3(0,7,0), 20, csColor(1, 1, 1));
	ll->Add(light); //Point D

	light = engine->CreateLight(0, csVector3(7,0,0), 20, csColor(1, 1, 1));
	ll->Add(light); //Point E

	light = engine->CreateLight(0, csVector3(0,-7,0), 20, csColor(1, 1, 1));
	ll->Add(light); //Point F
}

/*-------------------------------------------------------------------------*
* Main function
*-------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
	/* Runs the application.
	*
	* csApplicationRunner<> is a small wrapper to support "restartable"
	* applications (ie where CS needs to be completely shut down and loaded
	* again). fracturing1 does not use that functionality itself, however, it
	* allows you to later use "fracture.Restart();" and it'll just work.
	*/
	return csApplicationRunner<Fracture>::Run(argc, argv);
}

void Fracture::CreateTeapot()
{
	loader->LoadTexture("mystone2", "/lib/std/mystone2.gif");

	if (!loader->LoadLibraryFile("/lib/teapot/teapot.lib"))// 
	{
		ReportError("Error loading teapot library!");
		return;
	}

	
	csRef<iMeshFactoryWrapper> imeshfact (engine->FindMeshFactory("Teapot"));//Teapot

	if (imeshfact == 0)
	{
		ReportError("Error loading object from memory");
	}

	csBox3 bx(3.0f);
	CS::Geometry::Box rndrBx(bx);
	
	//csRef<iMeshWrapper> teapot = CS::Geometry::GeneralMeshBuilder::CreateFactoryAndMesh(engine, room, "dabba", "dabbaFactory", &rndrBx);
	csRef<iMeshWrapper> teapot (engine->CreateMeshWrapper(imeshfact, "my_teapot", room, csVector3(0, 3, 0)));
	
	//iMaterialWrapper* stone = engine->GetMaterialList()->FindByName("stone4");
	//teapot->GetMeshObject()->SetMaterialWrapper(stone);

	teapot->SetRenderPriority(4);
	using namespace CS::Geometry;

	//Now create a bounding box around the teapot
	//csBox3 boundingBox(teapot->GetWorldBoundingBox());

	/*
	csMatrix3 trans;
	trans.Identity();
	trans.m22 = 2;
	teapot->GetMovable()->SetTransform(trans);
	*/
	teapot->GetMovable()->UpdateMove();
	csBox3 boundingBox = teapot->GetWorldBoundingBox();
	
	PutRandomPoints(boundingBox, 5,teapot);

	
}

void Fracture::HitBeamFunction(csRef<iMeshWrapper> mesh,csVector3 v,csBox3 &box)
{
	/*
	'mesh' is the input mesh
	'box' is its bounding box.
	'v' is the point in 3d space(which we need to check whether it is inside mesh)
	v2 is the point on the back of the bounding box with same x,y as 'v' .
	*/


	




	
}



void Fracture::PutRandomPoints(csBox3 box, int numOfPoints, csRef<iMeshWrapper> whateverMesh)//CS::Geometry::Box box
{

	

	using namespace CS::Geometry;
	loader->LoadTexture("mystone2", "/lib/std/mystone2.gif");
	loader->LoadTexture("stone4", "/lib/std/stone4.gif");
	loader->LoadTexture("green", "/lib/std/green.gif");

	Box renderBox(box);

	csRef<iMeshWrapper> mesh = GeneralMeshBuilder::CreateFactoryAndMesh(engine, room, "cube", "cubeFact", &renderBox);
	

	mesh->SetRenderPriority(4);

	iMaterialWrapper* stone = engine->GetMaterialList()->FindByName("stone4");

	iMaterialWrapper* material = CS::Material::MaterialBuilder::CreateColorMaterial(object_reg, "decal", csColor(0.30f, 0.30f, 0.30f));
	
	
	iMaterialWrapper* tm = engine->GetMaterialList()->FindByName("green");

	mesh->GetMeshObject()->SetMaterialWrapper(material);//stone
	
	//mesh->SetZBufMode(CS_ZBUF_TEST);

	mesh->GetMovable()->MovePosition(csVector3(0, 0, 0));//-2,4,0

	mesh->GetMovable()->UpdateMove();

	csVector3 points[10]; // Create 5 points

	csVector3 min = box.Min();  //min of a box -2,-2,-2
	csVector3 max = box.Max(); //max of a box 2,2,2
	

	csRandomFloatGen rng;    //randomly generated float

	for (int i = 0; i < 10; i++)
	{
		points[i].x = (rng.Get(min[0], max[0]));
		points[i].y = (rng.Get(min[1], max[1]));
		points[i].z = (rng.Get(min[2], max[2]));

	}

	
	/*
	Notice that now we will make 5 meshes(spheres) with 'shpereFact' as factory and move each of these
	"points" to the their respective positions pointed by points[i] ;
	*/

	csEllipsoid ellipse(csVector3(0, 0, 0), csVector3(0.05f, 0.05f, 0.05f));
	Sphere sphere(ellipse, 10);
	csRef<iMeshFactoryWrapper> sphereFact = GeneralMeshBuilder::CreateFactory(engine, "sphereFact", &sphere);
		csRef<iMeshWrapper> pointSphere[10];
	
	for (int i = 0; i < 10; i++)
	{
		pointSphere[i] = GeneralMeshBuilder::CreateMesh(engine, room, "", sphereFact);
		pointSphere[i]->GetMeshObject()->SetMaterialWrapper(tm);
		pointSphere[i]->GetMovable()->MovePosition(points[i]);

	}
	
}