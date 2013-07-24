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

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

/*-------------------------------------------------------------------------*
 * MakeHuman targets parser (.target)
 *-------------------------------------------------------------------------*/

bool MakehumanCharacter::ParseMakehumanTargetFile (const char* filename, 
						   csRef<iRenderBuffer>& offsetsBuffer)
{
  // Get number of Makehuman model vertices
  offsetsBuffer = nullptr;
  size_t totalMHVerts = coords.GetSize ();
  if (totalMHVerts == 0)
    return ReportError ( 
                   "Error while parsing Makehuman target file: no parsed Makehuman vertices");

  // Open Makehuman target file
  csRef<iFile> file = manager->OpenFile (filename, TARGETS_PATH);
  if (!file)
    return ReportError ( "Could not open file %s", filename);
  if (file->GetSize () == 0)
    return true;

  // Initialize and lock the offset buffer
  offsetsBuffer = csRenderBuffer::CreateRenderBuffer 
    (totalMHVerts, CS_BUF_STATIC, CS_BUFCOMP_FLOAT, 3);
  csVector3* offsets = (csVector3*) offsetsBuffer->Lock (CS_BUF_LOCK_NORMAL);
  for (size_t i=0; i<totalMHVerts; i++)
    offsets[i] = csVector3 (0.0f);

  // Parse Makehuman target file
  int mhxIndex;
  float offsetX, offsetY, offsetZ;
  char line[256];

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255)) 
    {
      if (!file->AtEOF ())
        return ReportError ( "Malformed Makehuman target file");
    }
    else
    {
      csString text = line;
      if (text.StartsWith ("#")) continue;

      csStringArray words;
      size_t numVals = words.SplitString (text.Trim (), " ", csStringArray::delimIgnore);

      // TODO: allow silent skipping of unvalid lines

      if (numVals != 4)
      {
        ReportError ( "Wrong number of element in Makehuman target file (line %s)",
		CS::Quote::Single (line));
	continue;
      }

      // Parse the index of mhx vertex
      if (sscanf (words[0], "%i", &mhxIndex) != 1 || mhxIndex >= (int) totalMHVerts)
      {
/*
        ReportError ( "Wrong index element in Makehuman target file %s (%s)",
		CS::Quote::Single (filename), CS::Quote::Single (words[0]));
*/
        continue;
      }

      // Parse X component of offset
      if (sscanf (words[1], "%f", &offsetX) != 1)
        return ReportError ( "Wrong X element in Makehuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[1]));

      // Parse Y component of offset
      if (sscanf (words[2], "%f", &offsetY) != 1)
        return ReportError ( "Wrong Y element in Makehuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[2]));

      // Parse Z component of offset
      if (sscanf (words[3], "%f", &offsetZ) != 1)
        return ReportError ( "Wrong Z element in Makehuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[3]));

      // Copy the parsed offset into buffer
      offsets[mhxIndex] = csVector3 (offsetX, offsetY, offsetZ);
    }
  }

  // Unlock offsets buffer
  offsetsBuffer->Release ();

  return true;
}

bool MakehumanCharacter::ApplyTargetsToModel (const csArray<Target>& targets)
{
  // Make a backup of neutral Makehuman mesh and init basic offsets buffer 'basicMorph'
  basicMesh.DeleteAll ();
  basicMorph.DeleteAll ();
  size_t vertexMHCount = coords.GetSize ();
  for (size_t i = 0; i < vertexMHCount; i++)
  {
    basicMesh.Push (coords[i]);
    basicMorph.Push (csVector3 (0.0f));
  }

  // Apply Makehuman targets to neutral mesh buffer
  printf ("\nApplying morphtargets:\n");
  for (size_t ti = 0; ti < targets.GetSize (); ti++)
  {
    // Get morph target
    Target target = targets[ti];

    // Skip negligible deformations
    if (fabs (target.weight) < EPSILON or target.offsets == nullptr)
      continue;

    // Check if this target concerns basic model properties (gender/ethnic/age)
    csStringArray props;
    size_t numVals = props.SplitString (target.name.Trim (), "-", csStringArray::delimIgnore);
    bool isBasicProp = false;
    if (numVals == 3)
    {
      csString ethnic (props[0]);
      csString gender (props[1]);
      csString age (props[2]);
      isBasicProp = (ethnic == "neutral" or ethnic == "african" or ethnic == "asian") and
	(gender == "female" or gender == "male") and
	(age == "child" or age == "young" or age == "old");
    }

    // Lock offset buffer
    csRenderBufferLock<csVector3> offsets (target.offsets);
    if (offsets.GetSize () != vertexMHCount)
      return ReportError (
                     "Error while applying morph targets: wrong number of offsets");

    // Apply target offsets to Makehuman mesh buffer
    for (size_t i=0; i<vertexMHCount; i++)
    {
      // Add offsets of all model properties to basic mesh
      coords[i] += target.weight * offsets[i];

      // Cumulate offsets of basic properties (gender/ethnic/age)
      if (isBasicProp)
        basicMorph[i] += target.weight * offsets[i];
    }
    
    printf ("  '%s.target'  with weight %.4f\n", target.name.GetData (), target.weight);
  }

  // Make a backup of morphed Makehuman model
  morphedMesh.DeleteAll ();
  for (size_t i = 0; i < vertexMHCount; i++)
    morphedMesh.Push (coords[i]);

  // Notice that basic Makehuman model 'base.obj' doesn't define vertex normals;
  // consequently, array 'normals' is empty and should not be updated after morphing

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
