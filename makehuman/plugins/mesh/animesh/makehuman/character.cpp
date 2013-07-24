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
#include "character.h"

#include "ivideo/txtmgr.h"

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

MakehumanModel::MakehumanModel ()
{
}

void MakehumanModel::Reset ()
{
  props.DeleteAll ();
  measures.DeleteAll ();
  skinFile = "";
  proxyFilename = "";
  clothesNames.DeleteAll ();
}

void MakehumanModel::SetNeutral ()
{
  Reset ();

  props.Put ("weight", 0.5f);
  props.Put ("gender", 0.5f);
  props.Put ("age", 0.5f);
  props.Put ("height", 0.5f);
  props.Put ("asian", 0.333333f);
  props.Put ("african", 0.333333f);
  props.Put ("muscle", 0.5f);
  props.Put ("caucasian", 0.333333f);
  props.Put ("breastFirmness", 0.5f);
  props.Put ("breastSize", 0.5f);
}

MakehumanCharacter::MakehumanCharacter (MakehumanManager* manager)
  : scfImplementationType (this), manager (manager),
  proxy (), rig()
{
  // Create the animated mesh factory
  csRef<iMeshObjectFactory> factory = manager->animeshType->NewFactory ();
  animeshFactory = scfQueryInterfaceSafe<CS::Mesh::iAnimatedMeshFactory> (factory);

  // Copy the base buffers from the manager
  coords = manager->coords;;
  texcoords = manager->texcoords;;
  normals = manager->normals;;
}

MakehumanCharacter::~MakehumanCharacter ()
{
  mappingBuffer.DeleteAll ();
}

iAnimatedMeshFactory* MakehumanCharacter::GetMeshFactory () const
{
  return animeshFactory;
}

bool MakehumanCharacter::UpdateMeshFactory ()
{
  // Process property values of model
  ModelTargets modelVals;
  if (!ProcessModelProperties (human, &modelVals))
    return ReportError ("Problem while processing Makehuman model properties");
  PrintModelProperties (modelVals);

  // Generate a list of target names and weights
  csArray<Target> targets;
  if (!GenerateTargetsWeights (modelVals, &targets))
    return ReportError ("Problem while generating targets and weights of Makehuman model");

  // Parse all offset buffers of the model targets
  for (size_t i=0; i<targets.GetSize (); i++)
  {
    if (!ParseMakehumanTargetFile (targets[i].path, targets[i].offsets))
      return ReportError ("Parsing target file '%s' KO!", targets[i].path.GetData ());
  }

  // Apply morph targets to the buffer of Makehuman vertex coordinates
  if (!ApplyTargetsToModel (targets))
    return ReportError ("Problem while applying morph targets to Makehuman mesh buffer");

  // Parse macro-expressions of Makehuman model
  csArray<Target> macroExpressions;
  if (!GenerateMacroExpressions (modelVals, macroExpressions))
    ReportError ("Problem while generating model's macro-expressions");

  // Generate micro-expressions of Makehuman model
  if (!GenerateMicroExpressions (modelVals, microExpressions))
    ReportError ("Problem while generating model's micro-expressions");

   // Generate CS mesh buffers
  csDirtyAccessArray<Submesh> csSubmeshes;
  if (!GenerateMeshBuffers (manager->faceGroups, false, mhJoints, csSubmeshes, mappingBuffer))
    return ReportError ("Generating mesh buffers for Makehuman model KO!");

  // Create mesh for human model
  printf ("\nCreating Crystal Space model:\n");
  printf ("  - number of vertices of Makehuman model: %i\n", (int)coords.GetSize ());
  printf ("  - number of vertices of Crystal Space model: %i\n", (int)csCoords.GetSize ());

  // Create mesh texture
  csString textureName;
  csString texturePath;
  if (human.skinFile.IsEmpty ())
  {
    textureName = DEFAULT_SKIN;
    texturePath = SKIN_PATH;
    texturePath += DEFAULT_SKIN;
  }

  else
  {
    textureName = human.skinFile;
    texturePath = human.skinFile;
  }

  size_t idx1 = textureName.FindLast ('/') + 1;
  size_t idx2 = textureName.FindLast ('.');
  textureName.Replace (textureName.Slice (idx1, idx2 - idx1));

  csColor* transp = new csColor (0.0f, 0.0f, 0.0f);   // TODO: define correctly this parameter
  iTextureWrapper* texture = 
    manager->engine->CreateTexture (textureName.GetData (), texturePath.GetData (), transp, CS_TEXTURE_2D);
  if (!texture) return ReportError ("Cannot load skin texture %s!",
				    CS::Quote::Single (texturePath.GetData ()));
  texture->Register (manager->g3d->GetTextureManager ());

  // TODO: Define material of animesh factory
  /*
    csRef<iMaterialWrapper> material = 
    engine->CreateMaterial (csSubmeshes[0].materialName.GetData (), texture);
    if (!material)
    {
    ReportError ("Couldn't create material %s!", CS::Quote::Single (csSubmeshes[0].materialName.GetData ()));
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
    }
    animeshFactory->SetMaterialWrapper (material);
  */

  // Initialize the render buffers
  csRef<iRenderBuffer> vertbuf;
  csRef<iRenderBuffer> texbuf;
  csRef<iRenderBuffer> normalbuf;

  // Copy parsed data into buffer of vertex coordinates
  size_t vertexCount = csCoords.GetSize ();
  if (vertexCount > 0)
  {
    vertbuf = csRenderBuffer::CreateRenderBuffer (vertexCount,
                                                  CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    vertbuf->CopyInto (csCoords.GetArray (), vertexCount);
    animeshFactory->SetVertices (vertbuf);
  }

  // Copy parsed data into buffer of texture coordinates
  size_t texCount = csTexcoords.GetSize ();
  if (texCount > 0)
  {
    texbuf = csRenderBuffer::CreateRenderBuffer (texCount,
						 CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 2);
    texbuf->CopyInto (csTexcoords.GetArray (), texCount);
    animeshFactory->SetTexCoords (texbuf);
  }

  // Copy parsed data into buffer of vertex coordinates
  if (normals.GetSize () > 0)
  {
    size_t normalCount = csNormals.GetSize ();
    normalbuf = csRenderBuffer::CreateRenderBuffer (normalCount,
                                                    CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    normalbuf->CopyInto (csNormals.GetArray (), normalCount);
    animeshFactory->SetNormals (normalbuf);
  }
  else
  {
    // TODO: Calculate mesh normals
    printf ("WARNING: setting default normals to CS mesh\n");
    //animeshFactory->ComputeNormals ();

    //*******************************
    // TODO: SUPPRESS TEMPORARY CODE
    size_t normalCount = csNormals.GetSize ();
    normalbuf = csRenderBuffer::CreateRenderBuffer
      (normalCount, CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    normalbuf->CopyInto (csNormals.GetArray (), normalCount);
    animeshFactory->SetNormals (normalbuf);
    //*******************************
  }

  // Copy parsed data into submesh buffers
  for (size_t sub = 0; sub < csSubmeshes.GetSize (); sub++)
  {
    if (csSubmeshes[sub].subName.StartsWith ("helper-"))
      continue;

    size_t faceCount = csSubmeshes[sub].triangles.GetSize ();
    if (faceCount > 0)
    {
      csRef<iRenderBuffer> subbuf = csRenderBuffer::CreateIndexRenderBuffer
	(3 * faceCount, CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT, 0, vertexCount - 1);
      csRenderBufferLock<int> indices (subbuf);
      for (size_t t = 0; t < faceCount; t++)
      {
        csTriangle tri (csSubmeshes[sub].triangles[t]);
        *(indices++) = tri[0];
        *(indices++) = tri[1];
        *(indices++) = tri[2];
      }

      if (subbuf)
      {
        CS::Mesh::iAnimatedMeshSubMeshFactory* smf = 
          animeshFactory->CreateSubMesh (subbuf, csSubmeshes[sub].subName.GetData (), true);
        
        // TODO: Create submesh texture and material
        csRef<iMaterialWrapper> material = 
          manager->engine->CreateMaterial (csSubmeshes[sub].materialName.GetData (), texture);
        if (!material)
          return ReportError ("Couldn't create material %s!", 
				       CS::Quote::Single (csSubmeshes[sub].materialName.GetData ()));

        smf->SetMaterial (material);
      }
    }
  }

  // Calculate mesh tangents
  animeshFactory->ComputeTangents ();

  animeshFactory->Invalidate ();

  // Find the name of the rig
  csString rigName = rig;
  if (rigName.IsEmpty ()) rigName = DEFAULT_RIG;

  // Create a skeleton by parsing Makehuman rig file
  // and update bone positions according to mesh modifications
  if (!CreateSkeleton (modelName, rigName, mhJoints))
    ReportError ("Problem while updating animesh skeleton");

  // Remove the bones that are not influencing any vertex
  CS::Mesh::AnimatedMeshTools::CleanSkeleton (animeshFactory);

  // If no proxy is defined
  if ((!proxy || strcmp (proxy, "") == 0)
      && (!human.proxyFilename || strcmp (human.proxyFilename, "") == 0))
  {
    // Add the facial macro-expressions to the animesh factory
    if (!AddExpressionsToModel (animeshFactory, mappingBuffer, "macro_", macroExpressions))
      ReportError ("Could not add macro-expressions to the animesh factory");

    // Add the facial micro-expressions to the animesh factory
    if (!AddExpressionsToModel (animeshFactory, mappingBuffer, "", microExpressions))
      ReportError ("Could not add micro-expressions to the animesh factory");
  }

  // If a proxy is defined
  else
  {
    // Get the proxy name and file path
    csString name;
    csString path;

    if (proxy)
    {
      // Use proxy given as parameter
      name = proxy;
      path = csString (PROXY_PATH).Append (proxy);
      path.Append ("/").Append (proxy).Append (".proxy");
    }

    else
    {
      // Use proxy referenced in Makehuman model file (mhm)
      path = human.proxyFilename;
      csString tmp = human.proxyFilename;
      size_t start = tmp.FindLast ('/') + 1;
      size_t end = tmp.FindLast ('.');
      name = tmp.Slice (start, end - start);
    }

    // Create a new proxy
    printf ("\nCreating proxy '%s' for human model\n", name.GetData ());
    ProxyData* proxyModel = new ProxyData (name);

    // Create animesh factory from model proxy
    // TODO: re-use the current animesh factory
    animeshFactory =
      CreateProxyMesh (name.GetData (), path.GetData (),
		       human.skinFile, false, *proxyModel);
    if (!animeshFactory)
      return ReportError ("Creating animesh factory from Makehuman model proxy %s KO!",
			  CS::Quote::Single (human.proxyFilename.GetData ()));

    // Create a skeleton by parsing Makehuman rig file
    // and update bone positions according to mesh modifications
    if (!CreateSkeleton (modelName, rigName, mhJoints, proxyModel))
      ReportError ("Problem while updating proxy skeleton");

    // Remove the bones that are not influencing any vertex
    CS::Mesh::AnimatedMeshTools::CleanSkeleton (animeshFactory);

    // Generate the facial micro-expressions of the Makehuman model proxy
    csArray<Target> proxyExpressions;
    if (!GenerateProxyMicroExpressions (*proxyModel, microExpressions, proxyExpressions))
      ReportError ("Problem while generating micro-expressions of proxy model");

    // Add the facial micro-expressions to the animesh factory
    if (!AddExpressionsToModel (animeshFactory, proxyModel->mappingBuffer, "", proxyExpressions))
      ReportError ("Could not add micro-expressions to the animesh factory of proxy model");

    // Update the animesh factory
    animeshFactory->Invalidate ();
  }

  // Generate the clothes
  GenerateClothes ();

  return true;
}

void MakehumanCharacter::Clear ()
{
  human.Reset ();
  clothes.DeleteAll ();
}

void MakehumanCharacter::SetNeutral ()
{
  human.SetNeutral ();
}

bool MakehumanCharacter::Parse (const char* filename)
{
  Clear ();
  return ParseMakehumanModelFile (filename, &human);
}

void MakehumanCharacter::SetProxy (const char* proxy)
{
  this->proxy = proxy;
}

void MakehumanCharacter::SetRig (const char* rig)
{
  this->rig = rig;
}

void MakehumanCharacter::SetMeasure (const char* measure, float value)
{
  human.measures.PutUnique (measure, value);
}

void MakehumanCharacter::SetProperty (const char* property, float value)
{
  human.props.PutUnique (property, value);
}

void MakehumanCharacter::ClearClothes ()
{
  clothes.DeleteAll ();
  human.clothesNames.DeleteAll ();
}

size_t MakehumanCharacter::GetClothCount () const
{
  return clothes.GetSize ();
}

iAnimatedMeshFactory* MakehumanCharacter::GetClothMesh (size_t index) const
{
  return clothes[index];
}

/*-------------------------------------------------------------------------*
 * MakeHuman model parser (.mhm)
 *-------------------------------------------------------------------------*/

bool MakehumanCharacter::ParseMakehumanModelFile (const char* filename, MakehumanModel* human)
{
  // Open model file
  csRef<iFile> file = manager->OpenFile (filename, "");
  if (!file)
    return ReportError ( "Could not open file %s", filename);

  // Parse model file
  printf ("\nParsing Makehuman model file: '%s'\n\n", filename);
  char line[256];
  float val;

  // Skip the three first lines of the file
  // TODO: treat 3rd line tags of Makehuman model file
  uint count = 0;
  while (!file->AtEOF () and count < 3 and manager->ParseLine (file, line, 255))
    count++;

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255))
    {
      if (!file->AtEOF ())
        return ReportError ( "Malformed Makehuman model file");
    }
    else
    {
      csStringArray words;
      size_t numVals = words.SplitString (csString (line).Trim (), " ", csStringArray::delimIgnore);

      // Parse a Makehuman property name
      csString mhprop (words[0]);

      // Parse property type
      if (strcmp (mhprop.GetData (), "skinTexture") == 0)
      {
        if (numVals < 2)
          return ReportError ("Wrong element in Makehuman model file");

        // Parse texture filename
        csString texstr (words[1]);
        printf ("Makehuman model skin texture: '%s'\n", 
               texstr.Slice (0, texstr.FindLast ('.')).GetData ());

        // Get path of model texture
	// TODO: texture or skin path?
        //csString texPath (TEXTURE_PATH);
        csString texPath (SKIN_PATH);
        texPath.Append (texstr);
        if (!manager->vfs->Exists (texPath.GetData ()))
        {
          texPath.Replace (SKIN_PATH);
          texPath.Append (texstr);
          if (!manager->vfs->Exists (texPath.GetData ()))
	  {
            ReportWarning ("Could not find the texture file '%s' of the Makehuman model!", texPath.GetData ());
	    human->skinFile = SKIN_PATH;
	    human->skinFile += DEFAULT_SKIN;
	  }
        }

        else human->skinFile = texPath;
      }
      else if (strcmp (mhprop.GetData (), "clothes") == 0)
      {
        if (numVals < 3)
          return ReportError ("Wrong element in Makehuman model file");

        // Parse clothing name
        csString clotmp (words[1]);
        csString clostr = clotmp.Slice (0, clotmp.FindLast ('.'));
        human->clothesNames.Push (clostr);
      }
      else if (strcmp (mhprop.GetData (), "measure") == 0)
      {
        if (numVals < 3)
          return ReportError ( "Wrong element in Makehuman model file");

        // Parse measure name and value
        mhprop = csString (words[1]);
        if (sscanf (words[2], "%f", &val) != 1)
          return ReportError ( "Wrong element in Makehuman model file");

        human->measures.PutUnique (mhprop, val);
      }
      else if (strcmp (mhprop.GetData (), "proxy") == 0)
      {
        if (numVals < 2)
          return ReportError ( "Wrong element in Makehuman model file");

        // Parse the proxy filename
        human->proxyFilename = csString ("/lib/makehuman/").Append (words[1]);

        size_t start = human->proxyFilename.FindLast ('/') + 1;
        size_t end = human->proxyFilename.FindLast ('.');
        printf ("Makehuman model proxy: '%s'\n", 
               human->proxyFilename.Slice (start, end - start).GetData ());
      }
      else
      {
        if (numVals < 2)
          return ReportError ( "Wrong element in Makehuman model file");

        // Parse property value
        if (sscanf (words[1], "%f", &val) != 1)
        {
          // TODO: treat 2nd property word (ex: "face nose") in Makehuman model file
          continue;
        }

        human->props.PutUnique (mhprop, val);
      }
    }
  }

  return true;
}

bool MakehumanCharacter::ProcessModelProperties (const MakehumanModel human, ModelTargets* modelVals)
{
  if (!modelVals)
    return ReportError ( "Makehuman model is not valid");

  Target val, val1, val2;
  float w;

  // ethnics
  // african: 0 is neutral; 1 is african
  // asian: 0 is neutral; 1 is asian
  float african = human.props.Get (csString ("african"), 0);  
  float asian   = human.props.Get (csString ("asian"), 0);
  float total = 0.0f;
  float factor = 1.0f;

  if (african != 0.0 and asian != 0.0)
    factor = 0.5f;

  if (african != 0.0)
  {
    val = Target ("african", nullptr, factor * african);
    modelVals->ethnics.Push (val);
    total += factor * african;
  }
  if (asian != 0.0)
  {
    val = Target ("asian", nullptr, factor * asian);
    modelVals->ethnics.Push (val);
    total += factor * asian;
  }
  if (total != 1.0)
  {
    val = Target ("neutral", nullptr, 1.0 - total);
    modelVals->ethnics.Push (val);
  }

  // gender: 0 is female; 1 is male; 0.5 is neutral
  float gender = human.props.Get (csString ("gender"), 0);
  // TODO: allow values outside of normal value bounds?
  /*
  if (gender != 1.0)
  {
    val = Target ("female", nullptr, 1.0 - gender);
    modelVals->gender.Push (val);
  }
  if (gender != 0.0)
  {
    val = Target ("male", nullptr, gender);
    modelVals->gender.Push (val);
  }
  */
  if (gender <= 1.0)
  {
    val = Target ("female", nullptr, 1.0 - gender);
    modelVals->gender.Push (val);
  }
  if (gender >= 0.0)
  {
    val = Target ("male", nullptr, gender);
    modelVals->gender.Push (val);
  }

  // age: 0 is child, 0.5 is young and 1 is old
  // (considering: 0 is 12 years old, 0.5 is 25 and 1 is 70)
  float age = human.props.Get (csString ("age"), 0);  
  w = 0;
  if (age > 0.5)
  {
    val1 = Target ("-old", nullptr, (age - 0.5) * 2.0);
    modelVals->age.Push (val1);
    w = val1.weight;
  }
  else if (age < 0.5)
  {
    val2 = Target ("-child", nullptr, (0.5 - age) * 2.0);
    modelVals->age.Push (val2);
    w = val2.weight;
  }
  if (w != 1)
  {
    val = Target ("-young", nullptr, 1.0 - w);
    modelVals->age.Push (val);
  }

  // weight: 0 for underweight, 1 for overweight
  float weight = human.props.Get (csString ("weight"), 0);  
  w = 0;
  if (weight > 0.5)
  {
    val1 = Target ("-heavy", nullptr, (weight - 0.5) * 2.0);
    modelVals->weight.Push (val1);
    w = val1.weight;
  }
  else if (weight < 0.5)
  {
    val2 = Target ("-light", nullptr, (0.5 - weight) * 2.0);
    modelVals->weight.Push (val2);
    w = val2.weight;
  }
  if (w != 1)
  {
    val = Target ("", nullptr, 1.0 - w);
    modelVals->weight.Push (val);
  }

  // muscle: 0 for flacid, 1 for muscular
  float muscle = human.props.Get (csString ("muscle"), 0);  
  w = 0;
  if (muscle > 0.5)
  {
    val1 = Target ("-muscle", nullptr, (muscle - 0.5) * 2.0);
    modelVals->muscle.Push (val1);
    w = val1.weight;
  }
  else if (muscle < 0.5)
  {
    val2 = Target ("-flaccid", nullptr, (0.5 - muscle) * 2.0);
    modelVals->muscle.Push (val2);
    w = val2.weight;
  }
  if (w != 1)
  {
    val = Target ("", nullptr, 1.0 - w);
    modelVals->muscle.Push (val);
  }

  // height: -1 for dwarf, 1 for giant
  float height = human.props.Get (csString ("height"), 0);  
  if (height > 0.0)
  {
    val = Target ("-giant", nullptr, height);
    modelVals->height.Push (val);
  }
  else if (height < 0.0)
  {
    val = Target ("-dwarf", nullptr, -height);
    modelVals->height.Push (val);
  }

  // genitals: -1 is female, 1 is male
  float genitals = human.props.Get (csString ("genitals"), 0);  
  if (genitals > 0.0)
  {
    val = Target ("-masculine", nullptr, genitals);
    modelVals->genitals.Push (val);
  }
  else if (genitals < 0.0)
  {
    val = Target ("-feminine", nullptr, -genitals);
    modelVals->genitals.Push (val);
  }

  // buttocks: -1 for round buttocks, 1 for flat
  float buttocks = human.props.Get (csString ("buttocks"), 0);  
  if (buttocks > 0.0)
  {
    val = Target ("-nates2", nullptr, buttocks);
    modelVals->buttocks.Push (val);
  }
  else if (buttocks < 0.0)
  {
    val = Target ("-nates1", nullptr, -buttocks);
    modelVals->buttocks.Push (val);
  }

  // stomach: -1 for round belly, 1 for flat
  float stomach = human.props.Get (csString ("stomach"), 0);  
  if (stomach > 0.0)
  {
    val = Target ("-stomach2", nullptr, stomach);
    modelVals->stomach.Push (val);
  }
  else if (stomach < 0.0)
  {
    val = Target ("-stomach1", nullptr, -stomach);
    modelVals->stomach.Push (val);
  }

  // breast firmness: 0 is saggy, 1 is firm
  float breastFirmness = human.props.Get (csString ("breastFirmness"), 0);  
  if (breastFirmness != 1.0)
  {
    val = Target ("-firmness0", nullptr, 1.0 - breastFirmness);
    modelVals->breastFirmness.Push (val);
  }
  if (breastFirmness != 0.0)
  {
    val = Target ("-firmness1", nullptr, breastFirmness);
    modelVals->breastFirmness.Push (val);
  }

  // breast size: -1 is flat, 1 is big
  float breastSize = human.props.Get (csString ("breastSize"), 0);  
  if (breastSize > 0.0)
  {
    val = Target ("-cup2", nullptr, breastSize);
    modelVals->breastSize.Push (val);
  }
  else if (breastSize < 0.0)
  {
    val = Target ("-cup1", nullptr, -breastSize);
    modelVals->breastSize.Push (val);
  }

  // breast position: -1 is down, 1 is up
  float breastPosition = human.props.Get (csString ("breastPosition"), 0);  
  if (breastPosition > 0.0)
  {
    val = Target ("breast-up", nullptr, breastPosition);
    modelVals->breastPosition.Push (val);
  }
  else if (breastPosition < 0.0)
  {
    val = Target ("breast-down", nullptr, -breastPosition);
    modelVals->breastPosition.Push (val);
  }

  // breast distance: -1 is minimal, 1 is maximal
  float breastDistance = human.props.Get (csString ("breastDistance"), 0);  
  if (breastDistance > 0.0)
  {
    val = Target ("breast-dist-max", nullptr, breastDistance);
    modelVals->breastDistance.Push (val);
  }
  else if (breastDistance < 0.0)
  {
    val = Target ("breast-dist-min", nullptr, -breastDistance);
    modelVals->breastDistance.Push (val);
  }

  // breast taper: -1 is minimal, 1 is maximal
  float breastTaper = human.props.Get (csString ("breastTaper"), 0);  
  if (breastTaper > 0.0)
  {
    val = Target ("breast-point-max", nullptr, breastTaper);
    modelVals->breastTaper.Push (val);
  }
  else if (breastTaper < 0.0)
  {
    val = Target ("breast-point-min", nullptr, -breastTaper);
    modelVals->breastTaper.Push (val);
  }

  // pelvis tone: -1 for fat pelvis, 1 for slim
  float pelvisTone = human.props.Get (csString ("pelvisTone"), 0);  
  if (pelvisTone > 0.0)
  {
    val = Target ("-pelvis-tone2", nullptr, pelvisTone);
    modelVals->pelvisTone.Push (val);
  }
  else if (pelvisTone < 0.0)
  {
    val = Target ("-pelvis-tone1", nullptr, -pelvisTone);
    modelVals->pelvisTone.Push (val);
  }

  // measures: -1 to decrease, 1 to increase
  csString prop;
  csHash<float, csString>::ConstGlobalIterator it;
  for (it = human.measures.GetIterator (); it.HasNext (); )
  {
    float w = it.Next (prop);
    if (w < 0)
    {
      prop.Append ("-decrease");
      val = Target (prop.GetData (), nullptr, -w);
      modelVals->measures.Push (val);
    }
    else if (w > 0)
    {
      prop.Append ("-increase");
      val = Target (prop.GetData (), nullptr, w);
      modelVals->measures.Push (val);
    }
  }

  return true;
}

bool MakehumanCharacter::GenerateTargetsWeights (const ModelTargets modelVals, 
						 csArray<Target>* targets)
{
  csString path, name;
  Target target;

  // Ethnics targets
  for (size_t i0=0; i0< modelVals.ethnics.GetSize (); i0++)
  {
    for (size_t i1=0; i1< modelVals.gender.GetSize (); i1++)
    {
      for (size_t i2=0; i2< modelVals.age.GetSize (); i2++)
      {
        name.Format ("%s-%s%s",
                     modelVals.ethnics[i0].name.GetData (),
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData ());
        path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (),
                         modelVals.ethnics[i0].weight * modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight);
        if (target.weight > EPSILON)
          targets->Push (target);
      }
    }
  }

  // Gender and age targets
  for (size_t i1=0; i1< modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2=0; i2< modelVals.age.GetSize (); i2++)
    {    
      // Muscle and weight targets
      for (size_t i3=0; i3< modelVals.muscle.GetSize (); i3++)
      {
        for (size_t i4=0; i4< modelVals.weight.GetSize (); i4++)
        {
          name.Format ("universal-%s%s%s%s",
                       modelVals.gender[i1].name.GetData (), 
                       modelVals.age[i2].name.GetData (), 
                       modelVals.muscle[i3].name.GetData (), 
                       modelVals.weight[i4].name.GetData ());
          path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
          target = Target (name.GetData (), path.GetData (), 
                           modelVals.gender[i1].weight * modelVals.age[i2].weight
                           * modelVals.muscle[i3].weight * modelVals.weight[i4].weight);
          if ((target.weight > EPSILON) and 
              !(strcmp (modelVals.muscle[i3].name.GetData (), "")==0 and 
                strcmp (modelVals.weight[i4].name.GetData (), "")==0))
            targets->Push (target);

          // Stomach targets
          for (size_t i5=0; i5< modelVals.stomach.GetSize (); i5++)
          {
            name.Format ("%s%s%s%s%s",
                         modelVals.gender[i1].name.GetData (), 
                         modelVals.age[i2].name.GetData (), 
                         modelVals.muscle[i3].name.GetData (), 
                         modelVals.weight[i4].name.GetData (),
                         modelVals.stomach[i5].name.GetData ());
            path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
            target = Target (name.GetData (), path.GetData (), 
                             modelVals.gender[i1].weight * modelVals.age[i2].weight
                             * modelVals.muscle[i3].weight * modelVals.weight[i4].weight
                             * modelVals.stomach[i5].weight);
            if (target.weight > EPSILON)
              targets->Push (target);
          }

          // Breast size and firmness targets
          if (strcmp (modelVals.gender[i1].name.GetData (), "female") == 0)
            for (size_t i5=0; i5< modelVals.breastSize.GetSize (); i5++)
            {
              for (size_t i6=0; i6< modelVals.breastFirmness.GetSize (); i6++)
              {
                name.Format ("%s%s%s%s%s%s",
                             modelVals.gender[i1].name.GetData (), 
                             modelVals.age[i2].name.GetData (), 
                             modelVals.muscle[i3].name.GetData (), 
                             modelVals.weight[i4].name.GetData (),
                             modelVals.breastSize[i5].name.GetData (), 
                             modelVals.breastFirmness[i6].name.GetData ());
                path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
                target = Target (name.GetData (), path.GetData (), 
                                 modelVals.gender[i1].weight * modelVals.age[i2].weight
                                 * modelVals.muscle[i3].weight * modelVals.weight[i4].weight
                                 * modelVals.breastSize[i5].weight 
                                 * modelVals.breastFirmness[i6].weight);
                if (target.weight > EPSILON)
                  targets->Push (target);
              }
            }

        }
      }

      // Genitals targets
      for (size_t i3=0; i3< modelVals.genitals.GetSize (); i3++)
      {
        name.Format ("genitals_%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.genitals[i3].name.GetData (), 
                     modelVals.age[i2].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        path.ReplaceAll ("-", "_");
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight *
                         modelVals.genitals[i3].weight * modelVals.age[i2].weight);
        if (target.weight > EPSILON)
          targets->Push (target);
      }

      // Buttocks targets
      for (size_t i3=0; i3< modelVals.buttocks.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.buttocks[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.buttocks[i3].weight);
        if (target.weight > EPSILON)
          targets->Push (target);
      }

      // Pelvis tone targets
      for (size_t i3=0; i3< modelVals.pelvisTone.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.pelvisTone[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.pelvisTone[i3].weight);
        if (target.weight > EPSILON)
          targets->Push (target);
      }
    }
  }

  // Breast position targets
  for (size_t i1=0; i1< modelVals.breastPosition.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastPosition[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastPosition[i1].weight);
    if (target.weight > EPSILON)
      targets->Push (target);
  }

  // Breast distance targets
  for (size_t i1=0; i1< modelVals.breastDistance.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastDistance[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastDistance[i1].weight);
    if (target.weight > EPSILON)
      targets->Push (target);
  }

  // Breast taper targets
  for (size_t i1=0; i1< modelVals.breastTaper.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastTaper[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastTaper[i1].weight);
    if (target.weight > EPSILON)
      targets->Push (target);
  }

  // Height targets
  for (size_t i1=0; i1< modelVals.height.GetSize (); i1++)
  {
    name.Format ("universal-stature%s", modelVals.height[i1].name.GetData ());
    path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.height[i1].weight);
    if (target.weight > EPSILON)
      targets->Push (target);
  }

  // Measure targets
  for (size_t i1=0; i1< modelVals.measures.GetSize (); i1++)
  {
    name.Format ("measure-%s", modelVals.measures[i1].name.GetData ());
    path.Format ("%smeasure/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.measures[i1].weight);
    if (target.weight > EPSILON)
      targets->Push (target);
  }

  // Print targets
  printf ("\nMakehuman targets used by model:\n");
  for (size_t index=0; index<targets->GetSize (); index++)
    printf ("%8.2f%% '%s'\n", (*targets)[index].weight*100, (*targets)[index].path.GetData ());
  printf ("\n");

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
