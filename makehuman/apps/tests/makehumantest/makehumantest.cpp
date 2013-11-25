/*
    Copyright (C) 2012-2013 by Anthony Legrand
    Copyright (C) 2013 by Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

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

#include "cstool/animeshtools.h"
#include "cstool/rbuflock.h"
#include "csutil/scfstringarray.h"
#include "csutil/xmltiny.h"
#include "imap/writer.h"
#include "imesh/objmodel.h"
#include "iutil/document.h"
#include "iutil/object.h"
#include "iutil/stringarray.h"

// TODO: remove
#include <csutil/floatrand.h>

#include "makehumantest.h"

#define MODEL_NAME "test"

MakeHumanTest::MakeHumanTest ()
  : DemoApplication ("CrystalSpace.MakeHumanTest"), model (MODEL_NEUTRAL)
{
}

MakeHumanTest::~MakeHumanTest ()
{
  // Remove the meshes from the scene
  // Remove clothes meshes
  for (size_t index = 0; index < clothes.GetSize (); index++)
  {
    if (clothes[index] == NULL)
      continue;

    csRef<iMeshObject> clothingObject = scfQueryInterface<iMeshObject> (clothes[index]);
    engine->RemoveObject (clothingObject->GetMeshWrapper ());
  }

  // Free arrays
  clothesFactories.DeleteAll ();
  clothes.DeleteAll ();

  // Remove model mesh
  if (animesh)
  {
    csRef<iMeshObject> modelObject = scfQueryInterface<iMeshObject> (animesh);
    engine->RemoveObject (modelObject->GetMeshWrapper ());
  }
}

bool MakeHumanTest::OnInitialize (int argc, char* argv [])
{
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  if (!csInitializer::RequestPlugins (GetObjectRegistry (),
    CS_REQUEST_PLUGIN ("crystalspace.mesh.animesh.animnode.debug",
		       CS::Animation::iSkeletonDebugNodeManager),
    CS_REQUEST_PLUGIN ("crystalspace.mesh.animesh.makehuman",
		       CS::Mesh::iMakeHumanManager),
    CS_REQUEST_END))
    return ReportError ("Failed to initialize plugins!");

  return true;
}

bool MakeHumanTest::Application ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

   // Create the scene
  if (!CreateRoom ())
     return false;

  // Parse a MakeHuman model and convert it into CS
  makehumanManager =
    csQueryRegistry<CS::Mesh::iMakeHumanManager> (GetObjectRegistry ());
  if (!makehumanManager)
    return ReportError ("Failed to initialize the MakeHuman plugin!"
			" Have you installed the data files such as explained at data/makehuman/README?");

  // Print out the hierarchy of MakeHuman parameters
  csPrintf ("=========================\n");
  csPrintf ("== MakeHuman parameters:\n");
  csPrintf ("== ---------------------\n");

  csRef<iStringArray> categories = makehumanManager->GetCategories ();
  for (size_t i = 0; i < categories->GetSize (); i++)
  {
    const char* category = categories->Get (i);
    csPrintf ("== - Category: %s\n", category);

    csRef<iStringArray> subCategories = makehumanManager->GetSubCategories (category);
    for (size_t j = 0; j < subCategories->GetSize (); j++)
    {
      const char* subCategory = subCategories->Get (j);
      csPrintf ("==   - Sub-category: %s\n", subCategory);

      csRef<iStringArray> parameters = makehumanManager->GetParameters (category, subCategory);
      for (size_t k = 0; k < parameters->GetSize (); k++)
	csPrintf ("==     - Parameter: %s\n", parameters->Get (k));
    }
  }

  csPrintf ("=========================\n\n");

  // Create a human model
  //if (!CreateModel ("test", "/lib/makehuman/test.mhm"))
  if (!CreateCustomModel ())
    return ReportError ("Problem creating avatar\n");

  //*********** test clothes ***********************

  // Put clothes listed in MakeHuman properties on the model
  if (!CreateClothes ()) ReportWarning ("Problem while putting clothes on the model");

  // Put a single clothing item on the model
  //if (!CreateClothingItem ("shirt_medium"))
  //ReportWarning ("Problem while putting the clothing item on the model");

   //********** test facial expressions ***************

  if (character->GetExpressionGeneration ())
  {
    //TestMacroExpression ();
    TestMicroExpression1 ();   // smile
    //TestMicroExpression2 ();   // anger
    //TestMicroExpression3 ();   // cry
  }

  // Update the HUD
  hudManager->GetKeyDescriptions ()->Empty ();
  hudManager->GetKeyDescriptions ()->Push ("s: switch model");

  // Run the application
  Run ();

  return true;
}

void MakeHumanTest::Frame ()
{
  // Default behavior from DemoApplication
  DemoApplication::Frame ();

  if (debugNode)
    debugNode->Draw (view->GetCamera ());
}

bool MakeHumanTest::OnKeyboard (iEvent &event)
{
  // Default behavior from DemoApplication
  DemoApplication::OnKeyboard (event);

  csKeyEventType eventtype = csKeyEventHelper::GetEventType (&event);
  if (eventtype == csKeyEventTypeDown)
  {
    // Switching of model
    if (csKeyEventHelper::GetCookedCode (&event) == 's')
    {
      switch (model)
      {
      case MODEL_NEUTRAL:
	// Reset the model to the neutral state
	csPrintf ("Resetting the model to neutral\n");

	character->SetUpdateMode (CS::Mesh::MH_UPDATE_FULL);
	character->SetNeutral ();

	if (!character->UpdateMeshFactory ())
	{
	  ReportError ("Error re-generating the MakeHuman model");
	  return true;
	}

	model = MODEL_VARIETED;
	break;

      case MODEL_VARIETED:
	// Change a few properties using the 'fast' update mode
	csPrintf ("Testing the %s update mode\n", CS::Quote::Single ("fast"));

	character->SetUpdateMode (CS::Mesh::MH_UPDATE_FAST);

	character->SetParameter ("macro", "gender", 1.f);
	character->SetParameter ("macro", "age", 1.f);
	character->SetParameter ("macro", "weight", 1.f);
	character->SetParameter ("macro", "african", 1.f);
	character->SetParameter ("macro", "asian", 1.f);
	character->SetParameter ("face", "cheek1", 1.f);
	character->SetParameter ("face", "neck1", 1.f);

	// Invalidate the animated mesh factory after all those changes
        character->GetMeshFactory ()->Invalidate ();

	model = MODEL_NEUTRAL;
	break;
      }

      return true;
    }
  }

  return false;
}

bool MakeHumanTest::CreateModel (const char* factoryName, const char* filename, const char* proxy, const char* rig)
{
  character = makehumanManager->CreateCharacter ();
  character->SetExpressionGeneration (false);

  if (!character->Load (filename))
    return ReportError ("Error parsing the MakeHuman model file '%s'", filename);

  character->SetProxy (proxy);
  character->SetRig (rig);

  const char* _filename = "/this/test.mhm";
  bool res = character->Save (_filename);
  ReportInfo ("Saving file %s: %s", CS::Quote::Single (_filename), res ? "success" : "failure");

  if (!character->UpdateMeshFactory ())
    return ReportError ("Error generating the MakeHuman model '%s'", factoryName);

  return SetupAnimatedMesh ();
}

bool MakeHumanTest::CreateCustomModel ()
{
  character = makehumanManager->CreateCharacter ();

  // Disable the facial expression generation for faster updates
  character->SetExpressionGeneration (false); 
  //character->SetSkeletonGeneration (false); 

  character->SetParameter ("macro", "age", 0.2f);
  character->SetParameter ("macro", "height", 0.55f);
  character->SetParameter ("macro", "gender", 0.3f);
  character->SetParameter ("macro", "weight", 0.4f);
  character->SetParameter ("macro", "tone", 0.8f);

  const char* filename = "/this/test.mhm";
  bool res = character->Save (filename);
  ReportInfo ("Saving file %s: %s", CS::Quote::Single (filename), res ? "success" : "failure");

  if (!character->UpdateMeshFactory ())
    return ReportError ("Error generating the MakeHuman model");

  //TestTargetAccess ("face", "neck1", 1.f);

  return SetupAnimatedMesh ();
}

void MakeHumanTest::TestTargetAccess (const char* category, const char* parameter, float testValue)
{
  // Query the list of morph targets that will get activated if the given
  // property is modified
  csRefArray<CS::Mesh::iMakeHumanMorphTarget> targets;
  bool boundary = character->GetParameterTargets (category, parameter, targets);

  // Print the list of targets
  csPrintf ("\nList of targets for the parameter %s: %i - currently at boundary: %d\n",
	    CS::Quote::Single (parameter), (int) targets.GetSize (), boundary);
  for (size_t i = 0; i < targets.GetSize (); i++)
    csPrintf ("target %s scale: %f direction: %i\n", targets[i]->GetName (),
	    targets[i]->GetScale (), (int) targets[i]->GetDirection ());
  csPrintf ("\n");

  if (fabs (testValue) < SMALL_EPSILON) return;

  // Apply manually the targets on the mesh
  CS::Mesh::MakeHumanMorphTargetDirection direction =
    testValue > 0.0f ? CS::Mesh::MH_DIRECTION_UP : CS::Mesh::MH_DIRECTION_DOWN;

  // Iterate on all morph targets
  csRenderBufferLock<csVector3> vertices (character->GetMeshFactory ()->GetVertices ());
  for (size_t i = 0; i < targets.GetSize (); i++)
  {
    CS::Mesh::iMakeHumanMorphTarget* target = targets[i];

    // Check if we are at a target boundary and if the direction is OK
    if (boundary
	&& target->GetDirection () != CS::Mesh::MH_DIRECTION_BOTH
	&& target->GetDirection () != direction)
      continue;

    csPrintf ("target activated %s scale: %f vertices: %i\n", targets[i]->GetName (),
	      targets[i]->GetScale (), (int) target->GetIndices ().GetSize ());

    // Iterate on all vertices activated by the morph target
    const csArray<csVector3>& offsets = target->GetOffsets ();
    const csArray<size_t>& indices = target->GetIndices ();
    for (size_t j = 0; j < indices.GetSize (); j++)
    {
      // Compute the new position of the vertex after the application of this morph target 
      csVector3 newPosition = vertices[indices[j]] + testValue * target->GetScale () * offsets[j];

      // Test the computation of the new position by updating the original vertex
      vertices[indices[j]] = newPosition;
    }
  }
}

bool MakeHumanTest::CreateClothes ()
{
  // Place an instance of each clothing factory in the scene
  for (size_t index = 0; index < character->GetClothCount (); index++)
  {
    csRef<iMeshObjectFactory> meshFactory =
      scfQueryInterface<iMeshObjectFactory> (character->GetClothMesh (index));
    csRef<iMeshFactoryWrapper> meshWrap = 
      engine->CreateMeshFactory (meshFactory, "", true);
    csRef<iMeshWrapper> clothingMesh =
      engine->CreateMeshWrapper (meshWrap, "test2", room, csVector3 (0.0f));
    csRef<CS::Mesh::iAnimatedMesh> clothingObj =
      scfQueryInterface<CS::Mesh::iAnimatedMesh> (clothingMesh->GetMeshObject ());
    clothes.Push (clothingObj);
  }

  return true;
}

bool MakeHumanTest::CreateClothingItem (const char* clothingName)
{
  character->ClearClothes ();
  // TODO: this does not work yet...
  //character->AddCloth (...);
  character->UpdateMeshFactory ();

  // Place an instance of the clothing factory in the scene
  csRef<iMeshObjectFactory> meshFactory =
    scfQueryInterface<iMeshObjectFactory> (character->GetClothMesh (0));
  csRef<iMeshFactoryWrapper> meshWrap = 
    engine->CreateMeshFactory (meshFactory, clothingName, true);
  csRef<iMeshWrapper> clothingMesh =
    engine->CreateMeshWrapper (meshWrap, "test2", room, csVector3 (0.0f));
  csRef<CS::Mesh::iAnimatedMesh> clothingObj =  
    scfQueryInterface<CS::Mesh::iAnimatedMesh> (clothingMesh->GetMeshObject ());
  clothes.Push (clothingObj);

  return true;
}

void MakeHumanTest::TestMacroExpression ()
{
  const char* expname = "macro_realsmile";
  printf ("\nTEST: setting macro-expression '%s'\n", expname);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, 1.0))
    ReportError ("Could not apply morph target '%s'", expname);
}

void MakeHumanTest::TestMicroExpression1 ()
{
  // Expression 'smile'
  printf ("\nTEST: setting micro-expression 'smile'\n");
  const char* expname = "mouth-corner-puller";
  float weight = 0.645833;
  printf ("TEST: setting expression '%s'\n\n", expname);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
}

void MakeHumanTest::TestMicroExpression2 ()
{
  // Expression 'anger'
  printf ("\nTEST: setting macro-expression 'anger'\n");
  const char* expname = "eyebrows-right-extern-up";
  float weight = 0.187500;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eyebrows-left-down";
  weight = 1.0;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "nose-right-elevation";
  weight = 0.635417;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "nose-compression";
  weight = 0.854167;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "nose-left-dilatation";
  weight =  0.437500;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eyebrows-left-extern-up";
  weight = 0.281250;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eyebrows-right-down";
  weight =  1.0;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "nose-left-elevation";
  weight = 0.572917;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "nose-right-dilatation";
  weight = 0.458333;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "mouth-part-later";
  weight = 1.000000;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
}

void MakeHumanTest::TestMicroExpression3 ()
{
  // Expression 'cry'
  printf ("\nTEST: setting macro-expression 'cry'\n");
  const char* expname = "eyebrows-right-inner-up";
  float weight = 0.635417;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eye-left-slit";
  weight = 0.822917;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eye-right-slit";
  weight = 0.822917;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "eyebrows-left-inner-up";
  weight = 0.864583;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
  expname = "mouth-depression-retraction";
  weight =  0.583333;
  printf ("TEST: setting expression '%s' weight %f\n", expname, weight);
  if (!CS::Mesh::AnimatedMeshTools::ApplyMorphTarget (animeshFactory, expname, weight))
    ReportError ("Could not apply morph target '%s'", expname);
}

//******* Utility function ********

void MakeHumanTest::SaveSprite (const char* filename)
{
  csRef<iDocumentSystem> xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
  csRef<iDocument> doc = xml->CreateDocument ();
  csRef<iDocumentNode> root = doc->CreateRoot ();

  iMeshFactoryWrapper* meshfactwrap = engine->FindMeshFactory (MODEL_NAME);
  iMeshObjectFactory*  meshfact = meshfactwrap->GetMeshObjectFactory ();

  // Create a tag for the file type
  csRef<iDocumentNode> typeNode = root->CreateNodeBefore (CS_NODE_ELEMENT, 0);
  typeNode->SetValue ("library");

  // Create the tag for the MeshObj
  csRef<iDocumentNode> factNode = typeNode->CreateNodeBefore (CS_NODE_ELEMENT, 0);
  factNode->SetValue ("meshfact");

  // Add the mesh's name to the MeshObj tag
  const char* name = meshfactwrap->QueryObject ()->GetName ();
  if (name && *name)
    factNode->SetAttribute ("name", name);

  csRef<iFactory> factory = 
    scfQueryInterface<iFactory> (meshfact->GetMeshObjectType ());

  const char* pluginname = factory->QueryClassID ();

  if (!(pluginname && *pluginname)) return;

  csRef<iDocumentNode> pluginNode = factNode->CreateNodeBefore (CS_NODE_ELEMENT, 0);
  pluginNode->SetValue ("plugin");

  // Add the plugin tag
  char loadername[128] = "";
  csReplaceAll (loadername, pluginname, ".object.", ".loader.factory.",
    sizeof (loadername));

  pluginNode->CreateNodeBefore (CS_NODE_TEXT)->SetValue (loadername);
  csRef<iPluginManager> plugin_mgr = 
    csQueryRegistry<iPluginManager> (GetObjectRegistry ());

  char savername[128] = "";

  csReplaceAll (savername, pluginname, ".object.", ".saver.factory.",
    sizeof (savername));

  // Invoke the iSaverPlugin::WriteDown
  csRef<iSaverPlugin> saver =  csLoadPluginCheck<iSaverPlugin> (
    plugin_mgr, savername);
  if (saver) 
    saver->WriteDown (meshfact, factNode, 0/*ssource*/);

  csRef<iVFS> vfs;
  vfs = csQueryRegistry<iVFS>(GetObjectRegistry ());

  scfString str;
  doc->Write (&str);
  vfs->WriteFile (filename, str.GetData (), str.Length ());
}

bool MakeHumanTest::SetupAnimatedMesh ()
{
  // Print some additional information
  animeshFactory = character->GetMeshFactory ();
  csPrintf ("\nCharacter vertex count: %i\n", animeshFactory->GetVertexCount ());

  CS::Animation::iSkeletonFactory* skeleton = animeshFactory->GetSkeletonFactory ();
  if (skeleton)
  {
    // Print the skeleton structure
    printf ("%s", skeleton->Description ().GetData ());

    // Compute the height of the skeleton
    csQuaternion rotation;
    csVector3 offset;

    csVector3 feet (0.0f);
    CS::Animation::BoneID bone = skeleton->FindBone ("Toe_L");
    skeleton->GetTransformAbsSpace (bone, rotation, offset);
    feet += offset;

    bone = skeleton->FindBone ("Toe_R");
    skeleton->GetTransformAbsSpace (bone, rotation, offset);
    feet += offset;
    feet *= 0.5f;

    csVector3 eyes (0.0f);
    bone = skeleton->FindBone ("Eye_L");
    skeleton->GetTransformAbsSpace (bone, rotation, offset);
    eyes += offset;

    bone = skeleton->FindBone ("Eye_R");
    skeleton->GetTransformAbsSpace (bone, rotation, offset);
    eyes += offset;
    eyes *= 0.5f;

    printf ("\nfeet position: %s\n", feet.Description ().GetData ());
    printf ("\nskeleton height: %f\n", (eyes - feet).Norm ());

    // Create and setup an animation tree with a single 'debug' animation node
    csRef<iMeshObjectFactory> meshObject =
      scfQueryInterface<iMeshObjectFactory> (animeshFactory);
    printf ("body height: %f\n\n", meshObject->GetObjectModel ()->GetObjectBoundingBox ().GetSize ()[1]);

    // Create a 'debug' animation node for the animesh
    csRef<CS::Animation::iSkeletonDebugNodeManager> debugManager = 
      csQueryRegistry<CS::Animation::iSkeletonDebugNodeManager> (GetObjectRegistry ());
    if (!debugManager) return ReportError ("Failed to locate iSkeletonDebugNodeManager plugin!");

    csRef<CS::Animation::iSkeletonFactory> skelFact =
      animeshFactory->GetSkeletonFactory ();

    csRef<CS::Animation::iSkeletonAnimPacketFactory> animPacketFactory =
      skelFact->GetAnimationPacket ();

    if (!animPacketFactory)
    {
      csRef<CS::Animation::iSkeletonManager> skeletonManager = 
	csQueryRegistryOrLoad<CS::Animation::iSkeletonManager>
	(object_reg, "crystalspace.skeletalanimation");
      if (!skeletonManager)
	ReportWarning ("Could not find skeleton manager."
		       "Importing animesh skeletons and animations won't be possible");

      animPacketFactory = skeletonManager->CreateAnimPacketFactory ("test_packet");
      skelFact->SetAnimationPacket (animPacketFactory);
    }

    debugNodeFactory = debugManager->CreateAnimNodeFactory ("debug");
    debugNodeFactory->SetDebugModes (CS::Animation::DEBUG_2DLINES);
//  debugNodeFactory->SetDebugModes (CS::Animation::DEBUG_BBOXES);
    debugNodeFactory->SetRandomColor (true);
    animPacketFactory->SetAnimationRoot (debugNodeFactory);
  }

  // Create the mesh
  csRef<iMeshObjectFactory> meshFactory =
    scfQueryInterface<iMeshObjectFactory> (animeshFactory);

  csRef<iMeshFactoryWrapper> meshFactWrapper = 
    engine->CreateMeshFactory (meshFactory, "", true);

  csRef<iMeshWrapper> avatarMesh =
    engine->CreateMeshWrapper (meshFactWrapper, "test", room, csVector3 (0.0f));

  if (skeleton)
  {
    // Find a reference to the 'debug' animation node
    animesh = scfQueryInterface<CS::Mesh::iAnimatedMesh> (avatarMesh->GetMeshObject ());

    CS::Animation::iSkeletonAnimNode* rootNode =
      animesh->GetSkeleton ()->GetAnimationPacket ()->GetAnimationRoot ();

    debugNode = scfQueryInterface<CS::Animation::iSkeletonDebugNode> 
      (rootNode->FindNode ("debug"));
    if (!debugNode)
      ReportWarning ("Could not find the debug node");
  }

  CS::Debug::VisualDebuggerHelper::DebugTransform (GetObjectRegistry (), csOrthoTransform (), true);

  ResetScene ();
  return true;
}

bool MakeHumanTest::CreateRoom ()
{
  // Default behavior from DemoApplication for the creation of the scene
  if (!DemoApplication::CreateRoom ())
    return false;

  csRef<iLight> light;
  iLightList* ll = room->GetLights ();

  light = engine->CreateLight (0, csVector3 (-4, 8, -5), 15, csColor (1, 1, 1));
  ll->Add (light);
  light = engine->CreateLight (0, csVector3 (4, 8, 5), 15, csColor (1, 1, 1));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (-4, 13, -5), 15, csColor (1, 1, 1));
  ll->Add (light);
  light = engine->CreateLight (0, csVector3 (4, 13, 5), 15, csColor (1, 1, 1));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (-4, 18, -5), 15, csColor (1, 1, 1));
  ll->Add (light);
  light = engine->CreateLight (0, csVector3 (4, 18, 5), 15, csColor (1, 1, 1));
  ll->Add (light);

  return true;
}

void MakeHumanTest::ResetScene ()
{
  if (!animesh)
    return;

  // Reset the position of the animesh
  csRef<iMeshObject> animeshObject = scfQueryInterface<iMeshObject> (animesh);
  animeshObject->GetMeshWrapper ()->QuerySceneNode ()->GetMovable ()->SetTransform
    (csOrthoTransform (csMatrix3 (), csVector3 (0.0f)));
  animeshObject->GetMeshWrapper ()->QuerySceneNode ()->GetMovable ()->UpdateMove ();

  // Reset the position of the camera
  csRef<iMeshObjectFactory> meshObject =
    scfQueryInterface<iMeshObjectFactory> (character->GetMeshFactory ());
  float height = meshObject->GetObjectModel ()->GetObjectBoundingBox ().GetSize ()[1];

  view->GetCamera ()->SetTransform
    (csOrthoTransform (csMatrix3 (),
		       csVector3 (0.0f, height * 0.5f, -height)));
}

//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

int main (int argc, char* argv[])
{
  return MakeHumanTest ().Main (argc, argv);
}
