/*
    Copyright (C) 2010 by Jelle Hellemans

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

#include "csterrainedtest.h"

#include "cstool/enginetools.h"
#include "cstool/materialbuilder.h"
#include "imesh/modifiableterrain.h"
#include "ivideo/material.h"

CS_IMPLEMENT_APPLICATION

#define MODIFIER_SIZE_DELTA 5.f

//---------------------------------------------------------------------------

TerrainEd::TerrainEd ()
  : DemoApplication ("Terrain Editor Test"),
    rectSize (50.0f), rectHeight (-20.0f), lastUpdate (0), decal (nullptr)
{}

TerrainEd::~TerrainEd ()
{
  if (decal) decalManager->DeleteDecal (decal);
}

bool TerrainEd::OnInitialize (int argc, char* argv[])
{
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  // RequestPlugins () will load all plugins we specify. In addition
  // it will also check if there are plugins that need to be loaded
  // from the config system (both the application config and CS or
  // global configs). In addition it also supports specifying plugins
  // on the commandline.
  if (!csInitializer::RequestPlugins (GetObjectRegistry (),
    CS_REQUEST_PLUGIN ("crystalspace.decal.manager",
		       iDecalManager),
    CS_REQUEST_END))
    return ReportError ("Failed to initialize plugins!");

  // "Warm up" the event handler so it can interact with the world
  csBaseEventHandler::Initialize (GetObjectRegistry ());

  return true;
}

bool TerrainEd::Application ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

  // Setup the decal
  decalManager = csQueryRegistry<iDecalManager> (GetObjectRegistry ());

  if (decalManager)
  {
    // Create the decal material
    iMaterialWrapper* material = CS::Material::MaterialBuilder::CreateColorMaterial
      (GetObjectRegistry (), "decal", csColor (1.f, 0.f, 0.f));

    // Setup the decal template
    decalTemplate = decalManager->CreateDecalTemplate (material);
    decalTemplate->SetDecalOffset (0.1f);
    decalTemplate->SetMaximumVertexCount (128000);
    decalTemplate->SetMaximumTriangleCount (64000);
  }

  CreateRoom ();

  // Define the available keys
  hudManager->GetKeyDescriptions ()->Empty ();
  hudManager->GetKeyDescriptions ()->Push ("b: add cell");
  hudManager->GetKeyDescriptions ()->Push ("n: remove cell");
  hudManager->GetKeyDescriptions ()->Push ("MMU: enlarge brush");
  hudManager->GetKeyDescriptions ()->Push ("MMD: reduce brush");
  hudManager->GetKeyDescriptions ()->Push ("LMB: apply modifier");
  hudManager->GetKeyDescriptions ()->Push ("z: undo modifier");

  // Run the application
  Run ();

  return true;
}

bool TerrainEd::CreateRoom ()
{  
  // Default behavior from DemoApplication
  if (!DemoApplication::CreateRoom ())
    return false;

  // Configure the camera manager
  cameraManager->SetMotionSpeed (10.f);
  cameraManager->SetStartPosition (csVector3 (0.0f, 50.f, 0.0f));
  cameraManager->SetMouseMoveEnabled (false);
  cameraManager->ResetCamera ();

  // Now we need light to see something.
  csRef<iLight> light;
  iLightList* ll = room->GetLights ();

  light = engine->CreateLight (0, csVector3 (-3, 200, 0), 1000, csColor (1, 1, 1));
  ll->Add (light);

  // Create the textures used by the terrain
  iTextureWrapper* grass = loader->LoadTexture
    ("grass", "/this/data/terrained/cobgrass.png", 2, 0, true, false);
  if (!grass)
    ReportError ("Error loading %s texture!",
		 CS::Quote::Single ("grass"));
  
  iTextureWrapper* stone = loader->LoadTexture
    ("stone", "/this/data/terrained/cliff34ug6.jpg", 2, 0, true, false);
  if (!stone)
    ReportError ("Error loading %s texture!",
		 CS::Quote::Single ("stone"));

  // Create the shaders used by the terrain
  csRef<iShader> shader = loader->LoadShader ("/shader/terrain/terrain.xml");
  if (!stone)
    ReportError ("Error loading %s shader!",
		 CS::Quote::Single ("terrain"));

  // Create the materials used by the terrain
  csRef<iMaterialWrapper> terrainmat = engine->CreateMaterial ("terrain", 0);

  csRef<iShaderVarStringSet> stringSet = csQueryRegistryTagInterface<iShaderVarStringSet>
    (object_reg, "crystalspace.shader.variablenameset");

  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>
    (object_reg, "crystalspace.shared.stringset");
  csStringID ambient = strings->Request ("ambient");
  csStringID diffuse = strings->Request ("diffuse");

  terrainmat->GetMaterial ()->SetShader (ambient, shader);
  terrainmat->GetMaterial ()->SetShader (diffuse, shader);

  CS::ShaderVarStringID mat1 = stringSet->Request ("tex material1");
  CS::ShaderVarStringID mat1s = stringSet->Request ("material1 texscale");
  CS::ShaderVarStringID mat1a = stringSet->Request ("material1 attributes");

  CS::ShaderVarStringID mat2 = stringSet->Request ("tex material2");
  CS::ShaderVarStringID mat2s = stringSet->Request ("material2 texscale");
  CS::ShaderVarStringID mat2a = stringSet->Request ("material2 attributes");

  csRef<csShaderVariable> sv;
  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat1);
  sv->SetValue (grass);
  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat1s);
  sv->SetValue (csVector2 (15,15));
  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat1a);
  sv->SetValue (csVector4 (-1.0,17.0,-0.001,0.6));

  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat2);
  sv->SetValue (stone);
  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat2s);
  sv->SetValue (csVector2 (15,15));
  sv = terrainmat->GetMaterial ()->GetVariableAdd (mat2a);
  sv->SetValue (csVector4 (0.0,17.0,0.3,1.0));

  // Create the terrain factory
  csRef<iPluginManager> pluginManager =
    csQueryRegistry<iPluginManager> (GetObjectRegistry ());

  csRef<iMeshObjectType> meshType = csLoadPlugin<iMeshObjectType>
    (GetObjectRegistry (), "crystalspace.mesh.object.terrain2");

  csRef<iMeshObjectFactory> meshFactory = meshType->NewFactory ();
  terrainFactory = scfQueryInterface<iTerrainFactory> (meshFactory);

  csRef<iTerrainRenderer> renderer = csLoadPluginCheck<iTerrainRenderer>
    (pluginManager, "crystalspace.mesh.object.terrain2.bruteblockrenderer");
  csRef<iTerrainCollider> collider = csLoadPluginCheck<iTerrainCollider>
    (pluginManager, "crystalspace.mesh.object.terrain2.collider");
  csRef<iTerrainDataFeeder> feeder = csLoadPluginCheck<iTerrainDataFeeder>
    (pluginManager, "crystalspace.mesh.object.terrain2.modifiabledatafeeder");

  terrainFactory->SetRenderer (renderer);
  terrainFactory->SetCollider (collider);
  terrainFactory->SetFeeder (feeder);

  terrainFactory->SetAutoPreLoad (true);
  terrainFactory->SetMaxLoadedCells (20);

  // Create the terrain cell factories
  csRef<iTerrainFactoryCell> defaultCell (terrainFactory->GetDefaultCell ());
  defaultCell->SetSize (csVector3 (256.0f, 16.0f, 256.0f));
  defaultCell->SetGridWidth (257);
  defaultCell->SetGridHeight (257);
  defaultCell->SetMaterialMapWidth (256);
  defaultCell->SetMaterialMapHeight (256);
  defaultCell->SetMaterialPersistent (false);

  defaultCell->SetBaseMaterial (terrainmat);

  defaultCell->GetRenderProperties ()->SetParameter ("block resolution", "16");
  defaultCell->GetRenderProperties ()->SetParameter ("lod splitcoeff", "16");

  defaultCell->GetFeederProperties ()->SetParameter
    ("heightmap source", "/lev/terrain/heightmap.png");
  //defaultCell->GetFeederProperties ()->SetParameter
  //("materialmap source", "/lev/terrain/materialmap.png");

  {
    csRef<iTerrainFactoryCell> cell (terrainFactory->AddCell ());
    cell->SetName ("1");
    cell->SetPosition (csVector2 (-128.0f, -128.0f));
    //cell->GetFeederProperties ()->SetParameter
    //("heightmap source", "/lev/terraini/heightmap_01.png");
  }

  {
    csRef<iTerrainFactoryCell> cell (terrainFactory->AddCell ());
    cell->SetName ("2");
    cell->SetPosition (csVector2 (128.0f, -128.0f));
    //cell->GetFeederProperties ()->SetParameter
    //("heightmap source", "/lev/terraini/heightmap_11.png");
  }

  // Create the mesh and its factory
  csRef<iMeshFactoryWrapper> meshf = engine->CreateMeshFactory (meshFactory, "terrainFact");
  csRef<iMeshWrapper> mesh = engine->CreateMeshWrapper (meshf, "terrain", room);

  terrain = scfQueryInterface<iTerrainSystem> (mesh->GetMeshObject ());
  if (!terrain) return false;

  return true;
}

void TerrainEd::Frame ()
{
  // Default behavior from DemoApplication
  DemoApplication::Frame ();

  // Check if the modifier needs to be updated
  csTicks time = vc->GetCurrentTicks ();
  if (time - lastUpdate > 250)
    UpdateModifier (true);
}

bool TerrainEd::OnKeyboard (iEvent& ev)
{
  // Default behavior from DemoApplication
  DemoApplication::OnKeyboard (ev);

  // We got a keyboard event.
  csKeyEventType eventtype = csKeyEventHelper::GetEventType (&ev);
  if (eventtype == csKeyEventTypeDown)
  {
    // The user pressed a key (as opposed to releasing it).
    utf32_char code = csKeyEventHelper::GetCookedCode (&ev);
    utf32_char rawcode = csKeyEventHelper::GetRawCode (&ev);
    if (code == 'b')
    {
      csRef<iTerrainFactoryCell> cellf (terrainFactory->AddCell ());
      cellf->SetPosition (csVector2 (-128.0f + (terrainFactory->GetCellCount () - 1) * 256.0f, -128.0f));
      csRef<iTerrainCell> cell (terrain->AddCell (cellf));
    }
    else if (code == 'n')
    {
      if (terrain->GetCellCount ())
      {
	csRef<iTerrainCell> cell (terrain->GetCell (terrain->GetCellCount () - 1));
	terrain->RemoveCell (cell);
	csRef<iTerrainFactoryCell> cellf
	  (terrainFactory->GetCell (terrainFactory->GetCellCount () - 1));
	terrainFactory->RemoveCell (cellf);
      }
    }
    else if (rawcode == CSKEY_PADPLUS)
    {
      rectHeight += 20.0f;
      UpdateModifier ();
    }
    else if (rawcode == CSKEY_PADMINUS)
    {
      rectHeight -= 20.0f;
      if (rectHeight < 0.0f) rectHeight = 0.0f;
      UpdateModifier ();
    }
    else if (code == 'z')
    {
      if (undoStack.GetSize ())
      {
        csRef<iModifiableDataFeeder> feeder =
	  scfQueryInterface<iModifiableDataFeeder> (terrainFactory->GetFeeder ());
        csRef<iTerrainModifier> m = undoStack.Pop ();
        feeder->RemoveModifier (m);
      }
    }

  }

  return false;
}

bool TerrainEd::OnMouseClick (iEvent& ev)
{
  if (csMouseEventHelper::GetButton (&ev) == csmbLeft)
  {
    if (modifier) undoStack.Push (modifier);
    modifier.Invalidate ();
  }

  return false;
}

bool TerrainEd::OnMouseDown (iEvent& ev)
{
  if (csMouseEventHelper::GetButton (&ev) == csmbWheelUp)
  {
    rectSize += MODIFIER_SIZE_DELTA;
    if (rectSize > 100.f)
      rectSize = 100.f;
    UpdateModifier ();
  }
  else if (csMouseEventHelper::GetButton (&ev) == csmbWheelDown)
  {
    rectSize -= MODIFIER_SIZE_DELTA;
    if (rectSize < MODIFIER_SIZE_DELTA)
      rectSize = MODIFIER_SIZE_DELTA;
    UpdateModifier ();
  }

  return false;
}

void TerrainEd::UpdateModifier (bool checkPosition)
{
  csRef<iModifiableDataFeeder> feeder =
    scfQueryInterface<iModifiableDataFeeder> (terrainFactory->GetFeeder ());

  csTicks time = vc->GetCurrentTicks ();
  lastUpdate = time;

  // Find where the mouse is pointing at
  iCamera* camera = view->GetCamera ();
  csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
  csVector3 v3d = view->InvProject (v2d, 1000.0f);
  csVector3 startBeam = camera->GetTransform ().GetOrigin ();
  csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

  //if (modifier) feeder->RemoveModifier (modifier);

  csRef<iMeshObject> meshObject = scfQueryInterface<iMeshObject> (terrain);
  csVector3 position;
  // TODO: this test should be made without the modifier applied, otherwise
  // the result will flicker
  if (!meshObject->HitBeamObject (startBeam, endBeam, position, nullptr))
  {
    RemoveModifier ();
    return;
  }

  // Check if the last position of the modifier has really changed
  if (checkPosition && (position - lastPosition).Norm () < 0.1f)
  {
    return;
  }

  lastPosition = position;

  // Remove the previous modifier
  RemoveModifier ();

  // Create the terrain modifier
  //TODO: why this minus on Z???
  modifier = feeder->AddModifier (csVector3 (position.x, rectHeight, -position.z), rectSize, rectSize);    

  // Create a new decal
  csVector3 up (0.f, 1.f, 0.f);
  csVector3 direction (0.f, 1.f, 0.f);
  iMeshWrapper* meshWrapper = meshObject->GetMeshWrapper ();

  decal = decalManager->CreateDecal (decalTemplate, meshWrapper, position,
				     up, direction, rectSize, rectSize, decal);
}

void TerrainEd::RemoveModifier ()
{
  // Remove the previous modifier
  csRef<iModifiableDataFeeder> feeder =
    scfQueryInterface<iModifiableDataFeeder> (terrainFactory->GetFeeder ());
  if (modifier) feeder->RemoveModifier (modifier);
  modifier.Invalidate ();

  // Remove the previous decal
  if (decal) decalManager->DeleteDecal (decal);
  decal = nullptr;
}

/*-------------------------------------------------------------------------*
 * Main function
 *-------------------------------------------------------------------------*/

int main (int argc, char* argv[])
{
  return csApplicationRunner<TerrainEd>::Run (argc, argv);
}
