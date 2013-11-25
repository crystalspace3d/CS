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
#include "csver.h"
#include "character.h"

#include "ivideo/txtmgr.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

MakeHumanCharacter::MakeHumanCharacter (MakeHumanManager* manager)
  : scfImplementationType (this), manager (manager),
  proxy (), rig(), generateExpressions (true), generateSkeleton (true),
  updateMode (MH_UPDATE_FULL)
{
  // Create the animated mesh factory
  csRef<iMeshObjectFactory> factory = manager->animeshType->NewFactory ();
  animeshFactory = scfQueryInterfaceSafe<CS::Mesh::iAnimatedMeshFactory> (factory);

  SetNeutral ();
}

MakeHumanCharacter::~MakeHumanCharacter ()
{
  mappingBuffer.DeleteAll ();
}

void MakeHumanCharacter::SetExpressionGeneration (bool generate)
{
  generateExpressions = generate;
}

bool MakeHumanCharacter::GetExpressionGeneration () const
{
  return generateExpressions;
}

void MakeHumanCharacter::SetSkeletonGeneration (bool generate)
{
  generateSkeleton = generate;
}

bool MakeHumanCharacter::GetSkeletonGeneration () const
{
  return generateSkeleton;
}

void MakeHumanCharacter::SetUpdateMode (MakeHumanUpdateMode mode)
{
  updateMode = mode;
}

MakeHumanUpdateMode MakeHumanCharacter::GetUpdateMode () const
{
  return updateMode;
}

void MakeHumanCharacter::SetProxy (const char* proxy)
{
  this->proxy = proxy;
}

void MakeHumanCharacter::SetRig (const char* rig)
{
  this->rig = rig;
}

void MakeHumanCharacter::ClearClothes ()
{
  clothes.DeleteAll ();
  clothesNames.DeleteAll ();
}

size_t MakeHumanCharacter::GetClothCount () const
{
  return clothes.GetSize ();
}

iAnimatedMeshFactory* MakeHumanCharacter::GetClothMesh (size_t index) const
{
  return clothes[index];
}

iAnimatedMeshFactory* MakeHumanCharacter::GetMeshFactory () const
{
  return animeshFactory;
}

bool MakeHumanCharacter::UpdateMeshFactory ()
{
  // Clear all previous data
  mappingBuffer.DeleteAll ();
  basicMesh.DeleteAll ();
  morphedMesh.DeleteAll ();
  basicMorph.DeleteAll ();
  mhJoints.DeleteAll ();
  microExpressions.DeleteAll ();
  csCoords.DeleteAll ();
  csTexcoords.DeleteAll ();
  csNormals.DeleteAll ();
  clothes.DeleteAll ();

  // Copy the base buffers from the manager
  coords = manager->coords;
  texcoords = manager->texcoords;
  normals = manager->normals;

  // Generate the list of target names and weights
  csPrintf ("%s\n", Description ().GetData ());
  csArray<Target> targets;
  ExpandTargets (targets);

  // Parse all offset buffers of the model targets
  for (size_t i = 0; i < targets.GetSize (); i++)
    manager->ParseMakeHumanTargetFile (&targets[i]);

  // Apply morph targets to the buffer of MakeHuman vertex coordinates
  if (!ApplyTargetsToModel (targets))
    return ReportError ("Problem while applying morph targets to MakeHuman mesh buffer");

  csArray<Target> macroExpressions;
  if (generateExpressions)
  {
    // Parse macro-expressions of MakeHuman model
    if (!GenerateMacroExpressions (macroExpressions))
      ReportError ("Problem while generating model's macro-expressions");

    // Generate micro-expressions of MakeHuman model
    if (!GenerateMicroExpressions (microExpressions))
      ReportError ("Problem while generating model's micro-expressions");
  }

   // Generate CS mesh buffers
  csDirtyAccessArray<Submesh> csSubmeshes;
  if (!GenerateMeshBuffers (coords, texcoords, normals,
			    manager->faceGroups, false, mhJoints, csSubmeshes, mappingBuffer))
    return ReportError ("Generating mesh buffers for MakeHuman model KO!");

  // Create mesh for human model
  printf ("\nCreating Crystal Space model:\n");
  printf ("  - number of vertices of MakeHuman model: %i\n", (int)coords.GetSize ());
  printf ("  - number of vertices of Crystal Space model: %i\n", (int)csCoords.GetSize ());

  // Create mesh texture
  csString textureName;
  csString texturePath;
  if (skinFile.IsEmpty ())
  {
    textureName = DEFAULT_SKIN;
    texturePath = SKIN_PATH;
    texturePath += DEFAULT_SKIN;
  }

  else
  {
    textureName = skinFile;
    texturePath = skinFile;
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
    //printf ("WARNING: setting default normals to CS mesh\n");
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
  //animeshFactory->ComputeTangents ();

  animeshFactory->SetSkeletonFactory (nullptr);
  animeshFactory->Invalidate ();

  // Find the name of the rig
  csString rigName = rig;
  if (rigName.IsEmpty ()) rigName = DEFAULT_RIG;

  if (generateSkeleton)
  {
    // Create a skeleton by parsing MakeHuman rig file
    // and update bone positions according to mesh modifications
    if (!CreateSkeleton (modelName, rigName, mhJoints))
      ReportError ("Problem while updating animesh skeleton");

    // Remove the bones that are not influencing any vertex
    CS::Mesh::AnimatedMeshTools::CleanSkeleton (animeshFactory);
  }

  // If no proxy is defined
  if ((!proxy || strcmp (proxy, "") == 0)
      && (!proxyFilename || strcmp (proxyFilename, "") == 0))
  {
    if (generateExpressions)
    {
      // Add the facial macro-expressions to the animesh factory
      if (!AddExpressionsToModel (animeshFactory, mappingBuffer, "macro_", macroExpressions))
	ReportError ("Could not add macro-expressions to the animesh factory");

      // Add the facial micro-expressions to the animesh factory
      if (!AddExpressionsToModel (animeshFactory, mappingBuffer, "", microExpressions))
	ReportError ("Could not add micro-expressions to the animesh factory");
    }
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
      // Use proxy referenced in MakeHuman model file (mhm)
      path = proxyFilename;
      csString tmp = proxyFilename;
      size_t start = tmp.FindLast ('/') + 1;
      size_t end = tmp.FindLast ('.');
      name = tmp.Slice (start, end - start);
    }

    // Create a new proxy
    printf ("\nCreating proxy '%s' for human model\n", name.GetData ());
    ProxyData* proxyModel = new ProxyData (name);

    // Create animesh factory from model proxy
    // TODO: re-use the current animesh factory
    animeshFactory = CreateProxyMesh (name.GetData (), path.GetData (),
				      texturePath, false, *proxyModel);
    if (!animeshFactory)
      return ReportError ("Creating animesh factory from MakeHuman model proxy %s KO!",
			  CS::Quote::Single (proxyFilename.GetData ()));

    if (generateSkeleton)
    {
      // Create a skeleton by parsing MakeHuman rig file
      // and update bone positions according to mesh modifications
      if (!CreateSkeleton (modelName, rigName, mhJoints, proxyModel))
	ReportError ("Problem while updating proxy skeleton");

      // Remove the bones that are not influencing any vertex
      CS::Mesh::AnimatedMeshTools::CleanSkeleton (animeshFactory);
    }

    if (generateExpressions)
    {
      // Generate the facial micro-expressions of the MakeHuman model proxy
      csArray<Target> proxyExpressions;
      if (!GenerateProxyMicroExpressions (*proxyModel, microExpressions, proxyExpressions))
	ReportError ("Problem while generating micro-expressions of proxy model");

      // Add the facial micro-expressions to the animesh factory
      if (!AddExpressionsToModel (animeshFactory, proxyModel->mappingBuffer, "", proxyExpressions))
	ReportError ("Could not add micro-expressions to the animesh factory of proxy model");
    }

    // Update the animesh factory
    animeshFactory->Invalidate ();
  }

  // Generate the clothes
  GenerateClothes ();

  return true;
}

/*-------------------------------------------------------------------------*
 * MakeHuman model parser (.mhm)
 *-------------------------------------------------------------------------*/

bool MakeHumanCharacter::Save (const char* filename) const
{
  csRef<iFile> file = manager->vfs->Open (filename, VFS_FILE_WRITE);
  if (!file.IsValid ())
    return ReportError ("Could not open file %s", filename);

  csString txt;
  txt.Format ("# Written by Crystal Space %i.%i.%i\n",
	      CS_VERSION_NUM_MAJOR, CS_VERSION_NUM_MINOR, CS_VERSION_NUM_RELEASE);
  txt += "version 1.0.0\n";
  txt += "tags model\n";

  txt += csString ().Format ("gender %f\n", GetParameter ("macro", "gender"));
  txt += csString ().Format ("age %f\n", GetParameter ("macro", "age"));
  txt += csString ().Format ("muscle %f\n", GetParameter ("macro", "tone"));
  txt += csString ().Format ("weight %f\n", GetParameter ("macro", "weight"));
  txt += csString ().Format ("african %f\n", GetParameter ("macro", "african"));
  txt += csString ().Format ("asian %f\n", GetParameter ("macro", "asian"));
  txt += csString ().Format ("height %f\n", GetParameter ("macro", "height"));
  txt += csString ().Format ("genitals %f\n", GetParameter ("gender", "genitals"));
  txt += csString ().Format ("buttocks %f\n", GetParameter ("torso", "buttocks"));
  txt += csString ().Format ("breastPosition %f\n", GetParameter ("gender", "breastPosition"));
  txt += csString ().Format ("breastPoint %f\n", GetParameter ("gender", "breastPoint"));
  txt += csString ().Format ("breastFirmness %f\n", GetParameter ("gender", "breastFirmness"));
  txt += csString ().Format ("breastSize %f\n", GetParameter ("gender", "breastSize"));
  txt += csString ().Format ("stomach %f\n", GetParameter ("torso", "stomach"));
  txt += csString ().Format ("pelvisTone %f\n", GetParameter ("torso", "pelvisTone"));
  txt += csString ().Format ("breastDistance %f\n", GetParameter ("gender", "breastDistance"));

  for (csHash<float, csString>::ConstGlobalIterator it = parameters.GetIterator (); it.HasNext (); )
  {
    csString parameter;
    float value = it.Next (parameter);

    // Ignore the internal parameters that have no description
    if (!manager->parameters[parameter]) continue;
    if (parameter == "african" || parameter == "asian") continue;

    csString category;
    manager->FindParameterCategory (parameter, category);
    txt += csString ().Format ("%s %s %f\n", category.GetData (), parameter.GetData (), value);
  }

  csString skinTexture = skinFile;
  skinTexture.ReplaceAll (SKIN_PATH, "");
  if (skinTexture.IsEmpty ()) skinTexture = DEFAULT_SKIN;
  txt += csString ().Format ("skinTexture %s\n", skinTexture.GetData ());

  // TODO: proxy + rig + clothes + redundant parameters + case sensitivity problems

  size_t size = file->Write (txt.GetData (), txt.Length ());
  return size == txt.Length ();
}

bool MakeHumanCharacter::Load (const char* filename)
{
  // Clear the state of this character
  SetNeutral ();
  // TODO: take care of the FAST UPDATE mode (eg use SetActivatedParameters)

  // Open the model file
  //csRef<iFile> file = manager->OpenFile (filename, "");
  csRef<iFile> file = manager->vfs->Open (filename, VFS_FILE_READ);
  if (!file)
    return ReportError ("Could not open file %s", filename);

  // Parse the model file
  printf ("\nParsing MakeHuman model file: '%s'\n\n", filename);
  char line[256];
  float value;

  // Skip the three first lines of the file
  // TODO: treat 3rd line tags of MakeHuman model file
  uint count = 0;
  while (!file->AtEOF () && count < 3 && manager->ParseLine (file, line, 255))
    count++;

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255))
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed MakeHuman model file");
    }
    else
    {
      csStringArray words;
      size_t numVals = words.SplitString (csString (line).Trim (), " ", csStringArray::delimIgnore);

      // Parse a MakeHuman property name
      csString parameter (words[0]);

      // Parse property type
      if (parameter == "skinTexture")
      {
        if (numVals < 2)
          return ReportError ("Wrong element in MakeHuman model file");

        // Parse texture filename
        csString texstr (words[1]);
        printf ("MakeHuman model skin texture: '%s'\n", 
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
            ReportWarning ("Could not find the texture file '%s' of the MakeHuman model!", texPath.GetData ());
	    skinFile = SKIN_PATH;
	    skinFile += DEFAULT_SKIN;
	  }
        }

        else skinFile = texPath;
      }
      else if (parameter == "clothes")
      {
        if (numVals < 3)
          return ReportError ("Wrong element in MakeHuman model file");

        // Parse clothing name
        csString clotmp (words[1]);
        csString clostr = clotmp.Slice (0, clotmp.FindLast ('.'));
        clothesNames.Push (clostr);
      }
      else if (parameter == "proxy")
      {
        if (numVals < 2)
          return ReportError ("Wrong element in MakeHuman model file");

        // Parse the proxy filename
        proxyFilename = csString ("/lib/makehuman/").Append (words[1]);

        size_t start = proxyFilename.FindLast ('/') + 1;
        size_t end = proxyFilename.FindLast ('.');
        printf ("MakeHuman model proxy: '%s'\n", 
               proxyFilename.Slice (start, end - start).GetData ());
      }

      else if (numVals == 3)
      {
        // Parse the category and parameter name and value
	csString category = csString (words[0]);
        parameter = csString (words[1]);
        if (sscanf (words[2], "%f", &value) != 1)
	{
	  ReportError ("Could not parse the value of the parameter %s",
		       CS::Quote::Single (parameter));
	  continue;
	}

	// Fix the parameter name
	parameter[0] = csUnicodeTransform::MapToLower (parameter[0]);

	SetParameter (category, parameter, value);
      }

      else
      {
        if (numVals < 2)
          return ReportError ("Wrong element in MakeHuman model file");

        // Parse property value
        if (sscanf (words[1], "%f", &value) != 1)
        {
	  ReportError ("Could not parse the value of the parameter %s",
		       CS::Quote::Single (parameter));
          continue;
        }

	// Fix the parameter name
	parameter[0] = csUnicodeTransform::MapToLower (parameter[0]);
	if (parameter == "muscle") parameter = "tone";

	csString category;
	if (!manager->FindParameterCategory (parameter, category))
	{
	  ReportError ("Could not find the category of the parameter %s",
		       CS::Quote::Single (parameter));
	  continue;
	}

	SetParameter (category, parameter, value);
      }
    }
  }

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
