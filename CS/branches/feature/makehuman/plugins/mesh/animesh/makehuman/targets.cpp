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
#include "targets.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

/*-------------------------------------------------------------------------*
 * MakeHumanMorphTarget
 *-------------------------------------------------------------------------*/

MakeHumanMorphTarget::MakeHumanMorphTarget ()
  : scfImplementationType (this), scale (1.0f), direction (MH_DIRECTION_BOTH)
{

}

const char* MakeHumanMorphTarget::GetName () const
{
  return name;
}

const csArray<csVector3>& MakeHumanMorphTarget::GetOffsets () const
{
  return offsets;
}

const csArray<size_t>& MakeHumanMorphTarget::GetIndices () const
{
  return indices;
}

float MakeHumanMorphTarget::GetScale () const
{
  return scale;
}

MakeHumanMorphTargetDirection MakeHumanMorphTarget::GetDirection () const
{
  return direction;
}

/*-------------------------------------------------------------------------*
 * MakeHuman morph targets parser (.target)
 *-------------------------------------------------------------------------*/
bool MakeHumanManager::ParseMakeHumanTargetFile
  (const char* filename, csArray<csVector3>& offsets, csArray<size_t>& indices)
{
  // Check if the target buffers are already in the cache
  TargetBuffer* buffer = targetBuffers.GetElementPointer (filename);
  if (buffer)
  {
    offsets = buffer->offsets;
    indices = buffer->indices;
    return true;
  }

  // Open the MakeHuman target file
  csRef<iFile> file = OpenFile (filename, TARGETS_PATH);
  if (!file)
    return ReportError ("Could not open file %s", filename);
  if (file->GetSize () == 0)
    return true;

  // Parse the MakeHuman target file
  int mhxIndex;
  float offsetX, offsetY, offsetZ;
  char line[256];

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!ParseLine (file, line, 255)) 
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed MakeHuman target file");
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
        ReportError ("Wrong number of element in MakeHuman target file (line %s)",
		CS::Quote::Single (line));
	continue;
      }

      // Parse the index of mhx vertex
      if (sscanf (words[0], "%i", &mhxIndex) != 1 || mhxIndex >= (int) coords.GetSize ())
      {
/*
        ReportError ("Wrong index element in MakeHuman target file %s (%s)",
		CS::Quote::Single (filename), CS::Quote::Single (words[0]));
*/
        continue;
      }

      // Parse X component of offset
      if (sscanf (words[1], "%f", &offsetX) != 1)
        return ReportError ("Wrong X element in MakeHuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[1]));

      // Parse Y component of offset
      if (sscanf (words[2], "%f", &offsetY) != 1)
        return ReportError ("Wrong Y element in MakeHuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[2]));

      // Parse Z component of offset
      if (sscanf (words[3], "%f", &offsetZ) != 1)
        return ReportError ("Wrong Z element in MakeHuman target file %s (%s)",
		       CS::Quote::Single (filename), CS::Quote::Single (words[3]));

      // Copy the parsed offset into the buffers (adapting the Z axis)
      offsets.Push (csVector3 (offsetX, offsetY, -offsetZ));
      indices.Push (mhxIndex);
    }
  }

  // Cache the target buffers
  {
    TargetBuffer& buffer = targetBuffers.Put (filename, TargetBuffer ());
    buffer.offsets = offsets;
    buffer.indices = indices;
    // TODO: don't let the cache grow too high
  }

  return true;
}

bool MakeHumanCharacter::ApplyTargetsToModel (const csArray<Target>& targets)
{
  // Make a backup of neutral MakeHuman mesh and init basic offsets buffer 'basicMorph'
  // TODO: not needed if not generating expressions
  basicMesh.DeleteAll ();
  basicMorph.DeleteAll ();
  for (size_t i = 0; i < coords.GetSize (); i++)
  {
    basicMesh.Push (coords[i]);
    basicMorph.Push (csVector3 (0.0f));
  }

  // Apply MakeHuman targets to neutral mesh buffer
  //printf ("\nApplying morphtargets:\n");
  for (size_t ti = 0; ti < targets.GetSize (); ti++)
  {
    // Get morph target
    Target target = targets[ti];

    // Skip negligible deformations
    if (fabs (target.weight) < EPSILON || !target.offsets.GetSize ())
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
      isBasicProp = (ethnic == "neutral" || ethnic == "african" || ethnic == "asian") &&
	(gender == "female" || gender == "male") &&
	(age == "child" || age == "young" || age == "old");
    }

    // Check that the size of the offset and index buffers are the same
    if (target.offsets.GetSize () != target.indices.GetSize ())
      return ReportError ("Error while applying morph targets: wrong number of offsets and vertices");

    // Apply target offsets to MakeHuman mesh buffer
    for (size_t i = 0; i < target.offsets.GetSize (); i++)
    {
      // Add offsets of all model properties to basic mesh
      coords[target.indices[i]] += target.weight * target.offsets[i];

      // Cumulate offsets of basic properties (gender/ethnic/age)
      if (isBasicProp)
        basicMorph[target.indices[i]] += target.weight * target.offsets[i];
    }
    
    //printf ("  '%s.target' with weight %.4f\n", target.name.GetData (), target.weight);
  }

  // Make a backup of morphed MakeHuman model
  morphedMesh.DeleteAll ();
  for (size_t i = 0; i < coords.GetSize (); i++)
    morphedMesh.Push (coords[i]);

  // Notice that basic MakeHuman model 'base.obj' doesn't define vertex normals;
  // consequently, array 'normals' is empty and should not be updated after morphing

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
