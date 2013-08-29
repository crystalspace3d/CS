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
  proxy (), rig(), generateExpressions (true)
{
  // Create the animated mesh factory
  csRef<iMeshObjectFactory> factory = manager->animeshType->NewFactory ();
  animeshFactory = scfQueryInterfaceSafe<CS::Mesh::iAnimatedMeshFactory> (factory);
}

MakehumanCharacter::~MakehumanCharacter ()
{
  mappingBuffer.DeleteAll ();
}

void MakehumanCharacter::SetExpressionGeneration (bool generate)
{
  generateExpressions = generate;
}

bool MakehumanCharacter::GetExpressionGeneration () const
{
  return generateExpressions;
}

iAnimatedMeshFactory* MakehumanCharacter::GetMeshFactory () const
{
  return animeshFactory;
}

bool MakehumanCharacter::UpdateMeshFactory ()
{
  // Clear all previous data
  modelVals.DeleteAll ();
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

  // Process the property values of the model
  if (!ProcessModelProperties (human, &modelVals))
    return ReportError ("Problem while processing Makehuman model properties");
  PrintModelProperties (modelVals);

  // Generate the list of target names and weights
  csArray<Target> targets;
  GenerateTargetsWeights (modelVals, targets);

  // Parse all offset buffers of the model targets
  for (size_t i = 0; i < targets.GetSize (); i++)
  {
    if (!manager->ParseMakehumanTargetFile (targets[i].path, targets[i].offsets, targets[i].indices))
      return ReportError ("Parsing target file '%s' KO!", targets[i].path.GetData ());
  }

  // Apply morph targets to the buffer of Makehuman vertex coordinates
  if (!ApplyTargetsToModel (targets))
    return ReportError ("Problem while applying morph targets to Makehuman mesh buffer");

  csArray<Target> macroExpressions;
  if (generateExpressions)
  {
    // Parse macro-expressions of Makehuman model
    if (!GenerateMacroExpressions (modelVals, macroExpressions))
      ReportError ("Problem while generating model's macro-expressions");

    // Generate micro-expressions of Makehuman model
    if (!GenerateMicroExpressions (modelVals, microExpressions))
      ReportError ("Problem while generating model's micro-expressions");
  }

   // Generate CS mesh buffers
  csDirtyAccessArray<Submesh> csSubmeshes;
  if (!GenerateMeshBuffers (coords, texcoords, normals,
			    manager->faceGroups, false, mhJoints, csSubmeshes, mappingBuffer))
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

  animeshFactory->SetSkeletonFactory (nullptr);
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
    animeshFactory = CreateProxyMesh (name.GetData (), path.GetData (),
				      texturePath, false, *proxyModel);
    if (!animeshFactory)
      return ReportError ("Creating animesh factory from Makehuman model proxy %s KO!",
			  CS::Quote::Single (human.proxyFilename.GetData ()));

    // Create a skeleton by parsing Makehuman rig file
    // and update bone positions according to mesh modifications
    if (!CreateSkeleton (modelName, rigName, mhJoints, proxyModel))
      ReportError ("Problem while updating proxy skeleton");

    // Remove the bones that are not influencing any vertex
    CS::Mesh::AnimatedMeshTools::CleanSkeleton (animeshFactory);

    if (generateExpressions)
    {
      // Generate the facial micro-expressions of the Makehuman model proxy
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

float MakehumanCharacter::GetProperty (const char* property) const
{
  return human.props.Get (property, 0.0f);
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
  while (!file->AtEOF () && count < 3 && manager->ParseLine (file, line, 255))
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

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
