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

void MakeHumanCharacter::ConvertTargets (csRefArray<iMakeHumanMorphTarget>& targets,
					 csArray<Target>& localTargets,
					 float scale,
					 MakeHumanMorphTargetDirection direction)
{
  for (size_t i = 0; i < localTargets.GetSize (); i++)
  {
    csRef<MakeHumanMorphTarget> target;
    target.AttachNew (new MakeHumanMorphTarget ());

    target->name = localTargets[i].name;
    target->scale = scale * localTargets[i].weight;
    target->direction = direction;

    manager->ParseMakeHumanTargetFile (localTargets[i].path, target->offsets, target->indices);

    // Translate the vertex indices from MakeHuman to CS
    size_t count = target->indices.GetSize ();
    for (size_t j = 0; j < count; j++)
    {
      VertBuf& mapping = mappingBuffer[target->indices[j]];

      target->indices[j] = mapping.vertices[0];

      for (size_t k = 1; k < mapping.vertices.GetSize (); k++)
      {
	target->offsets.Push (target->offsets[j]);
	target->indices.Push (mapping.vertices[k]);
      }
    }

    targets.Push (target);
  }

  localTargets.DeleteAll ();
}

bool MakeHumanCharacter::GetPropertyTargets
  (const char* property, csRefArray<iMakeHumanMorphTarget>& targets)
{
  // Copy the model values of this character
  ModelTargets values = modelVals;
  csArray<Target> localTargets;

  if (!strcmp (property, "african"))
  {
    // TODO
    return false;
  }

  if (!strcmp (property, "asian"))
  {
    // TODO
    return false;
  }

  if (!strcmp (property, "gender"))
  {
    // 'female' targets
    values.gender.DeleteAll ();
    values.gender.Push (Target ("female"));

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

    // 'male' targets
    values.gender.DeleteAll ();
    values.gender.Push (Target ("male"));

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    // TODO: Breast size and firmness targets may have a boundary point

    return false;
  }

  if (!strcmp (property, "age"))
  {
    float age = human.properties.Get (csString ("age"), 0.0f);

    if (age > 0.5f + SMALL_EPSILON)
    {
      values.age.DeleteAll ();
      values.age.Push (Target ("-old"));

      GenerateTargetsWeightsAgeGender (values, localTargets);
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      if (age < 1.0f + SMALL_EPSILON)
      {
	values.age.DeleteAll ();
	values.age.Push (Target ("-young"));

	GenerateTargetsWeightsAgeGender (values, localTargets);
	ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    if (age < 0.5f - SMALL_EPSILON)
    {
      values.age.DeleteAll ();
      values.age.Push (Target ("-child"));

      GenerateTargetsWeightsAgeGender (values, localTargets);
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      if (age > 0.0f - SMALL_EPSILON)
      {
	values.age.DeleteAll ();
	values.age.Push (Target ("-young"));

	GenerateTargetsWeightsAgeGender (values, localTargets);
	ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    values.age.DeleteAll ();
    values.age.Push (Target ("-old"));

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

    values.age.DeleteAll ();
    values.age.Push (Target ("-child"));

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

    values.age.DeleteAll ();
    values.age.Push (Target ("-young"));

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_UP);

    GenerateTargetsWeightsAgeGender (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "weight"))
  {
    float weight = human.properties.Get (csString ("weight"), 0.0f);

    if (weight > 0.5f + SMALL_EPSILON)
    {
      values.weight.DeleteAll ();
      values.weight.Push (Target ("-heavy"));

      GenerateTargetsWeightsWeightMuscle (values, localTargets);
      GenerateTargetsWeightsStomach (values, localTargets);
      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      if (weight < 1.0f + SMALL_EPSILON)
      {
	values.weight.DeleteAll ();
	values.weight.Push (Target (""));

	GenerateTargetsWeightsWeightMuscle (values, localTargets);
	GenerateTargetsWeightsStomach (values, localTargets);
	GenerateTargetsWeightsBreast (values, localTargets);
	ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    if (weight < 0.5f - SMALL_EPSILON)
    {
      values.weight.DeleteAll ();
      values.weight.Push (Target ("-light"));

      GenerateTargetsWeightsWeightMuscle (values, localTargets);
      GenerateTargetsWeightsStomach (values, localTargets);
      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      if (weight > 0.0f - SMALL_EPSILON)
      {
	values.weight.DeleteAll ();
	values.weight.Push (Target (""));

	GenerateTargetsWeightsWeightMuscle (values, localTargets);
	GenerateTargetsWeightsStomach (values, localTargets);
	GenerateTargetsWeightsBreast (values, localTargets);
	ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    values.weight.DeleteAll ();
    values.weight.Push (Target ("-heavy"));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

    values.weight.DeleteAll ();
    values.weight.Push (Target ("-light"));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

    values.weight.DeleteAll ();
    values.weight.Push (Target (""));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_UP);

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "muscle"))
  {
    float muscle = human.properties.Get (csString ("muscle"), 0.0f);

    if (muscle > 0.5f + SMALL_EPSILON)
    {
      values.muscle.DeleteAll ();
      values.muscle.Push (Target ("-muscle"));

      GenerateTargetsWeightsWeightMuscle (values, localTargets);
      GenerateTargetsWeightsStomach (values, localTargets);
      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      if (muscle < 1.0f + SMALL_EPSILON)
      {
	values.muscle.DeleteAll ();
	values.muscle.Push (Target (""));

	GenerateTargetsWeightsWeightMuscle (values, localTargets);
	GenerateTargetsWeightsStomach (values, localTargets);
	GenerateTargetsWeightsBreast (values, localTargets);
	ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    if (muscle < 0.5f - SMALL_EPSILON)
    {
      values.muscle.DeleteAll ();
      values.muscle.Push (Target ("-flaccid"));

      GenerateTargetsWeightsWeightMuscle (values, localTargets);
      GenerateTargetsWeightsStomach (values, localTargets);
      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      if (muscle > 0.0f - SMALL_EPSILON)
      {
	values.muscle.DeleteAll ();
	values.muscle.Push (Target (""));

	GenerateTargetsWeightsWeightMuscle (values, localTargets);
	GenerateTargetsWeightsStomach (values, localTargets);
	GenerateTargetsWeightsBreast (values, localTargets);
	ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);
      }

      return false;
    }

    values.muscle.DeleteAll ();
    values.muscle.Push (Target ("-muscle"));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

    values.muscle.DeleteAll ();
    values.muscle.Push (Target ("-flaccid"));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

    values.muscle.DeleteAll ();
    values.muscle.Push (Target (""));

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_UP);

    GenerateTargetsWeightsWeightMuscle (values, localTargets);
    GenerateTargetsWeightsStomach (values, localTargets);
    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "height"))
  {
    float height = human.properties.Get (csString ("height"), 0.0f);

    if (height > SMALL_EPSILON)
    {
      values.height.DeleteAll ();
      values.height.Push (Target ("-giant"));

      GenerateTargetsWeightsHeight (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (height < -SMALL_EPSILON)
    {
      values.height.DeleteAll ();
      values.height.Push (Target ("-dwarf"));

      GenerateTargetsWeightsHeight (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.height.DeleteAll ();
    values.height.Push (Target ("-giant"));

    GenerateTargetsWeightsHeight (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.height.DeleteAll ();
    values.height.Push (Target ("-dwarf"));

    GenerateTargetsWeightsHeight (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "genitals"))
  {
    float genitals = human.properties.Get (csString ("genitals"), 0.0f);

    if (genitals > SMALL_EPSILON)
    {
      values.genitals.DeleteAll ();
      values.genitals.Push (Target ("-masculine"));

      GenerateTargetsWeightsGenitals (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (genitals < -SMALL_EPSILON)
    {
      values.genitals.DeleteAll ();
      values.genitals.Push (Target ("-feminine"));

      GenerateTargetsWeightsGenitals (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.genitals.DeleteAll ();
    values.genitals.Push (Target ("-masculine"));

    GenerateTargetsWeightsGenitals (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.genitals.DeleteAll ();
    values.genitals.Push (Target ("-feminine"));

    GenerateTargetsWeightsGenitals (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "buttocks"))
  {
    float buttocks = human.properties.Get (csString ("buttocks"), 0.0f);

    if (buttocks > SMALL_EPSILON)
    {
      values.buttocks.DeleteAll ();
      values.buttocks.Push (Target ("-nates2"));

      GenerateTargetsWeightsButtocks (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (buttocks < -SMALL_EPSILON)
    {
      values.buttocks.DeleteAll ();
      values.buttocks.Push (Target ("-nates1"));

      GenerateTargetsWeightsButtocks (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.buttocks.DeleteAll ();
    values.buttocks.Push (Target ("-nates2"));

    GenerateTargetsWeightsButtocks (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.buttocks.DeleteAll ();
    values.buttocks.Push (Target ("-nates1"));

    GenerateTargetsWeightsButtocks (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "stomach"))
  {
    float stomach = human.properties.Get (csString ("stomach"), 0.0f);

    if (stomach > SMALL_EPSILON)
    {
      values.stomach.DeleteAll ();
      values.stomach.Push (Target ("-stomach2"));

      GenerateTargetsWeightsStomach (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (stomach < -SMALL_EPSILON)
    {
      values.stomach.DeleteAll ();
      values.stomach.Push (Target ("-stomach1"));

      GenerateTargetsWeightsStomach (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.stomach.DeleteAll ();
    values.stomach.Push (Target ("-stomach2"));

    GenerateTargetsWeightsStomach (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.stomach.DeleteAll ();
    values.stomach.Push (Target ("-stomach1"));

    GenerateTargetsWeightsStomach (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "pelvisTone"))
  {
    float pelvisTone = human.properties.Get (csString ("pelvisTone"), 0.0f);

    if (pelvisTone > SMALL_EPSILON)
    {
      values.pelvisTone.DeleteAll ();
      values.pelvisTone.Push (Target ("-pelvis-tone2"));

      GenerateTargetsWeightsPelvis (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (pelvisTone < -SMALL_EPSILON)
    {
      values.pelvisTone.DeleteAll ();
      values.pelvisTone.Push (Target ("-pelvis-tone1"));

      GenerateTargetsWeightsPelvis (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.pelvisTone.DeleteAll ();
    values.pelvisTone.Push (Target ("-pelvis-tone2"));

    GenerateTargetsWeightsPelvis (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.pelvisTone.DeleteAll ();
    values.pelvisTone.Push (Target ("-pelvis-tone1"));

    GenerateTargetsWeightsPelvis (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "breastFirmness"))
  {
    float breastFirmness = human.properties.Get (csString ("breastFirmness"), 0.0f);

    if (breastFirmness > SMALL_EPSILON)
    {
      values.breastFirmness.DeleteAll ();
      values.breastFirmness.Push (Target ("-firmness0"));

      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (breastFirmness < -SMALL_EPSILON)
    {
      values.breastFirmness.DeleteAll ();
      values.breastFirmness.Push (Target ("-firmness1"));

      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.breastFirmness.DeleteAll ();
    values.breastFirmness.Push (Target ("-firmness0"));

    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_UP);

    values.breastFirmness.DeleteAll ();
    values.breastFirmness.Push (Target ("-firmness1"));

    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "breastSize"))
  {
    float breastSize = human.properties.Get (csString ("breastSize"), 0.0f);

    if (breastSize > SMALL_EPSILON)
    {
      values.breastSize.DeleteAll ();
      values.breastSize.Push (Target ("-cup2"));

      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (breastSize < -SMALL_EPSILON)
    {
      values.breastSize.DeleteAll ();
      values.breastSize.Push (Target ("-cup1"));

      GenerateTargetsWeightsBreast (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.breastSize.DeleteAll ();
    values.breastSize.Push (Target ("-cup2"));

    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.breastSize.DeleteAll ();
    values.breastSize.Push (Target ("-cup1"));

    GenerateTargetsWeightsBreast (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "breastPosition"))
  {
    float breastPosition = human.properties.Get (csString ("breastPosition"), 0.0f);

    if (breastPosition > SMALL_EPSILON)
    {
      values.breastPosition.DeleteAll ();
      values.breastPosition.Push (Target ("breast-up"));

      GenerateTargetsWeightsBreastPosition (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (breastPosition < -SMALL_EPSILON)
    {
      values.breastPosition.DeleteAll ();
      values.breastPosition.Push (Target ("breast-down"));

      GenerateTargetsWeightsBreastPosition (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.breastPosition.DeleteAll ();
    values.breastPosition.Push (Target ("breast-up"));

    GenerateTargetsWeightsBreastPosition (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.breastPosition.DeleteAll ();
    values.breastPosition.Push (Target ("breast-down"));

    GenerateTargetsWeightsBreastPosition (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "breastDistance"))
  {
    float breastDistance = human.properties.Get (csString ("breastDistance"), 0.0f);

    if (breastDistance > SMALL_EPSILON)
    {
      values.breastDistance.DeleteAll ();
      values.breastDistance.Push (Target ("breast-dist-max"));

      GenerateTargetsWeightsBreastDistance (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (breastDistance < -SMALL_EPSILON)
    {
      values.breastDistance.DeleteAll ();
      values.breastDistance.Push (Target ("breast-dist-min"));

      GenerateTargetsWeightsBreastDistance (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.breastDistance.DeleteAll ();
    values.breastDistance.Push (Target ("breast-dist-max"));

    GenerateTargetsWeightsBreastDistance (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.breastDistance.DeleteAll ();
    values.breastDistance.Push (Target ("breast-dist-min"));

    GenerateTargetsWeightsBreastDistance (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  if (!strcmp (property, "breastTaper"))
  {
    float breastTaper = human.properties.Get (csString ("breastTaper"), 0.0f);

    if (breastTaper > SMALL_EPSILON)
    {
      values.breastTaper.DeleteAll ();
      values.breastTaper.Push (Target ("breast-point-max"));

      GenerateTargetsWeightsBreastTaper (values, localTargets);
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    if (breastTaper < -SMALL_EPSILON)
    {
      values.breastTaper.DeleteAll ();
      values.breastTaper.Push (Target ("breast-point-min"));

      GenerateTargetsWeightsBreastTaper (values, localTargets);
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      return false;
    }

    values.breastTaper.DeleteAll ();
    values.breastTaper.Push (Target ("breast-point-max"));

    GenerateTargetsWeightsBreastTaper (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

    values.breastTaper.DeleteAll ();
    values.breastTaper.Push (Target ("breast-point-min"));

    GenerateTargetsWeightsBreastTaper (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

    return true;
  }

  ReportError ("Unvalid property name: %s", CS::Quote::Single (property));

  return false;
}

bool MakeHumanCharacter::GetMeasureTargets
(const char* measure, csRefArray<iMakeHumanMorphTarget>& targets)
{
  // Copy the model values of this character
  ModelTargets values = modelVals;
  csArray<Target> localTargets;

  // TODO: check availability of the measure

  float value = human.measures.Get (measure, 0.0f);
  csString name = measure;

  if (value > SMALL_EPSILON)
  {
    values.measures.DeleteAll ();
    name += "-increase";
    values.measures.Push (Target (name));

    GenerateTargetsWeightsMeasure (values, localTargets);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    return false;
  }

  if (value < -SMALL_EPSILON)
  {
    values.measures.DeleteAll ();
    name += "-decrease";
    values.measures.Push (Target (name));

    GenerateTargetsWeightsMeasure (values, localTargets);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

    return false;
  }

  values.measures.DeleteAll ();
  name = measure;
  name += "-increase";
  values.measures.Push (Target (name));

  GenerateTargetsWeightsMeasure (values, localTargets);
  ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

  values.measures.DeleteAll ();
  name = measure;
  name += "-decrease";
  values.measures.Push (Target (name));

  GenerateTargetsWeightsMeasure (values, localTargets);
  ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

  return true;
}

bool MakeHumanCharacter::ProcessModelProperties (const MakeHumanModel& human, ModelTargets* modelVals)
{
  if (!modelVals)
    return ReportError ("The MakeHuman model is not valid");

  Target val, val1, val2;
  float w;

  // ethnics
  // african: 0 is neutral; 1 is african
  // asian: 0 is neutral; 1 is asian
  // TODO: use caucasian?
  float african = human.properties.Get (csString ("african"), 0.0f);
  float asian   = human.properties.Get (csString ("asian"), 0.0f);
  float total = 0.0f;
  float ethnics = 0.0f;

  if (african > SMALL_EPSILON)
    ethnics += 1.0f;
  if (asian > SMALL_EPSILON)
    ethnics += 1.0f;

  if (african > SMALL_EPSILON)
  {
    val = Target ("african", nullptr, ethnics > SMALL_EPSILON ? african / ethnics : african);
    modelVals->ethnics.Push (val);
    total += african;
  }

  if (asian > SMALL_EPSILON)
  {
    val = Target ("asian", nullptr, ethnics > SMALL_EPSILON ? asian / ethnics : asian);
    modelVals->ethnics.Push (val);
    total += asian;
  }

  if (ethnics < SMALL_EPSILON)
  {
    val = Target ("neutral", nullptr, 1.0f);
    modelVals->ethnics.Push (val);
  }

  else
  {
    total /= ethnics;

    if (total < 1.0f)
    {
      val = Target ("neutral", nullptr, 1.0f - total);
      modelVals->ethnics.Push (val);
    }
  }

  // gender: 0 is female; 1 is male; 0.5 is neutral
  float gender = human.properties.Get (csString ("gender"), 0.5f);
  //if (fabsf (1.0f - gender) > SMALL_EPSILON)
  {
    val = Target ("female", nullptr, 1.0f - gender);
    modelVals->gender.Push (val);
  }
  //if (fabsf (gender) > SMALL_EPSILON)
  {
    val = Target ("male", nullptr, gender);
    modelVals->gender.Push (val);
  }

  // age: 0 is child, 0.5 is young and 1 is old
  // (considering: 0 is 12 years old, 0.5 is 25 and 1 is 70)
  float age = human.properties.Get (csString ("age"), 0.5f);
  w = 0;
  if (age > 0.5f + SMALL_EPSILON)
  {
    val1 = Target ("-old", nullptr, (age - 0.5f) * 2.0f);
    modelVals->age.Push (val1);
    w = val1.weight;
  }
  else if (age < 0.5f - SMALL_EPSILON)
  {
    val2 = Target ("-child", nullptr, (0.5 - age) * 2.0f);
    modelVals->age.Push (val2);
    w = val2.weight;
  }
  if (w < 1.0f)
  {
    val = Target ("-young", nullptr, 1.0f - w);
    modelVals->age.Push (val);
  }

  // weight: 0 for underweight, 1 for overweight
  float weight = human.properties.Get (csString ("weight"), 0.0f);  
  w = 0;
  if (weight > 0.5f + SMALL_EPSILON)
  {
    val1 = Target ("-heavy", nullptr, (weight - 0.5f) * 2.0f);
    modelVals->weight.Push (val1);
    w = val1.weight;
  }
  else if (weight < 0.5f - SMALL_EPSILON)
  {
    val2 = Target ("-light", nullptr, (0.5 - weight) * 2.0f);
    modelVals->weight.Push (val2);
    w = val2.weight;
  }
  if (w < 1.0f)
  {
    val = Target ("", nullptr, 1.0f - w);
    modelVals->weight.Push (val);
  }

  // muscle: 0 for flacid, 1 for muscular
  float muscle = human.properties.Get (csString ("muscle"), 0.0f);  
  w = 0;
  if (muscle > 0.5f)
  {
    val1 = Target ("-muscle", nullptr, (muscle - 0.5f) * 2.0f);
    modelVals->muscle.Push (val1);
    w = val1.weight;
  }
  else if (muscle < 0.5f)
  {
    val2 = Target ("-flaccid", nullptr, (0.5 - muscle) * 2.0f);
    modelVals->muscle.Push (val2);
    w = val2.weight;
  }
  if (w < 1.0f)
  {
    val = Target ("", nullptr, 1.0f - w);
    modelVals->muscle.Push (val);
  }

  // height: -1 for dwarf, 1 for giant
  float height = human.properties.Get (csString ("height"), 0.0f);  
  if (height > SMALL_EPSILON)
  {
    val = Target ("-giant", nullptr, height);
    modelVals->height.Push (val);
  }
  else if (height < -SMALL_EPSILON)
  {
    val = Target ("-dwarf", nullptr, -height);
    modelVals->height.Push (val);
  }

  // genitals: -1 is female, 1 is male
  float genitals = human.properties.Get (csString ("genitals"), 0.0f);  
  if (genitals > SMALL_EPSILON)
  {
    val = Target ("-masculine", nullptr, genitals);
    modelVals->genitals.Push (val);
  }
  else if (genitals < -SMALL_EPSILON)
  {
    val = Target ("-feminine", nullptr, -genitals);
    modelVals->genitals.Push (val);
  }

  // buttocks: -1 for round buttocks, 1 for flat
  float buttocks = human.properties.Get (csString ("buttocks"), 0.0f);  
  if (buttocks > SMALL_EPSILON)
  {
    val = Target ("-nates2", nullptr, buttocks);
    modelVals->buttocks.Push (val);
  }
  else if (buttocks < -SMALL_EPSILON)
  {
    val = Target ("-nates1", nullptr, -buttocks);
    modelVals->buttocks.Push (val);
  }

  // stomach: -1 for round belly, 1 for flat
  float stomach = human.properties.Get (csString ("stomach"), 0.0f);  
  if (stomach > SMALL_EPSILON)
  {
    val = Target ("-stomach2", nullptr, stomach);
    modelVals->stomach.Push (val);
  }
  else if (stomach < -SMALL_EPSILON)
  {
    val = Target ("-stomach1", nullptr, -stomach);
    modelVals->stomach.Push (val);
  }

  // breast firmness: 0 is saggy, 1 is firm
  float breastFirmness = human.properties.Get (csString ("breastFirmness"), 0.0f);  
  // TODO: is this correct?
  if (fabsf (breastFirmness - 1.0f) > SMALL_EPSILON)
  {
    val = Target ("-firmness0", nullptr, 1.0f - breastFirmness);
    modelVals->breastFirmness.Push (val);
  }
  if (fabsf (breastFirmness) > SMALL_EPSILON)
  {
    val = Target ("-firmness1", nullptr, breastFirmness);
    modelVals->breastFirmness.Push (val);
  }

  // breast size: -1 is flat, 1 is big
  float breastSize = human.properties.Get (csString ("breastSize"), 0.0f);  
  if (breastSize > SMALL_EPSILON)
  {
    val = Target ("-cup2", nullptr, breastSize);
    modelVals->breastSize.Push (val);
  }
  else if (breastSize < -SMALL_EPSILON)
  {
    val = Target ("-cup1", nullptr, -breastSize);
    modelVals->breastSize.Push (val);
  }

  // breast position: -1 is down, 1 is up
  float breastPosition = human.properties.Get (csString ("breastPosition"), 0.0f);  
  if (breastPosition > SMALL_EPSILON)
  {
    val = Target ("breast-up", nullptr, breastPosition);
    modelVals->breastPosition.Push (val);
  }
  else if (breastPosition < -SMALL_EPSILON)
  {
    val = Target ("breast-down", nullptr, -breastPosition);
    modelVals->breastPosition.Push (val);
  }

  // breast distance: -1 is minimal, 1 is maximal
  float breastDistance = human.properties.Get (csString ("breastDistance"), 0.0f);  
  if (breastDistance > SMALL_EPSILON)
  {
    val = Target ("breast-dist-max", nullptr, breastDistance);
    modelVals->breastDistance.Push (val);
  }
  else if (breastDistance < -SMALL_EPSILON)
  {
    val = Target ("breast-dist-min", nullptr, -breastDistance);
    modelVals->breastDistance.Push (val);
  }

  // breast taper: -1 is minimal, 1 is maximal
  float breastTaper = human.properties.Get (csString ("breastTaper"), 0.0f);  
  if (breastTaper > SMALL_EPSILON)
  {
    val = Target ("breast-point-max", nullptr, breastTaper);
    modelVals->breastTaper.Push (val);
  }
  else if (breastTaper < -SMALL_EPSILON)
  {
    val = Target ("breast-point-min", nullptr, -breastTaper);
    modelVals->breastTaper.Push (val);
  }

  // pelvis tone: -1 for fat pelvis, 1 for slim
  float pelvisTone = human.properties.Get (csString ("pelvisTone"), 0.0f);  
  if (pelvisTone > SMALL_EPSILON)
  {
    val = Target ("-pelvis-tone2", nullptr, pelvisTone);
    modelVals->pelvisTone.Push (val);
  }
  else if (pelvisTone < -SMALL_EPSILON)
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
    if (w > SMALL_EPSILON)
    {
      prop.Append ("-increase");
      val = Target (prop.GetData (), nullptr, w);
      modelVals->measures.Push (val);
    }
    else if (w < -SMALL_EPSILON)
    {
      prop.Append ("-decrease");
      val = Target (prop.GetData (), nullptr, -w);
      modelVals->measures.Push (val);
    }
  }

  return true;
}

void MakeHumanCharacter::GenerateTargetsWeights
(const ModelTargets& modelVals, csArray<Target>& targets)
{
  csString path, name;
  Target target;

  // Ethnics targets
  for (size_t i0 = 0; i0 < modelVals.ethnics.GetSize (); i0++)
  {
    for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
    {
      for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
      {
	float weight = modelVals.ethnics[i0].weight
	  * modelVals.gender[i1].weight 
	  * modelVals.age[i2].weight;

        if (weight > EPSILON)
	{
	  name.Format ("%s-%s%s",
		       modelVals.ethnics[i0].name.GetData (),
		       modelVals.gender[i1].name.GetData (), 
		       modelVals.age[i2].name.GetData ());
	  path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
	  target = Target (name.GetData (), path.GetData (), weight);
	  targets.Push (target);
	}
      }
    }
  }

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Muscle and weight targets
      for (size_t i3 = 0; i3 < modelVals.muscle.GetSize (); i3++)
      {
        for (size_t i4 = 0; i4 < modelVals.weight.GetSize (); i4++)
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
          if ((target.weight > EPSILON) && 
              !(strcmp (modelVals.muscle[i3].name.GetData (), "") == 0 &&
                strcmp (modelVals.weight[i4].name.GetData (), "") == 0))
            targets.Push (target);

          // Stomach targets
          for (size_t i5 = 0; i5 < modelVals.stomach.GetSize (); i5++)
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
              targets.Push (target);
          }

          // Breast size and firmness targets
          if (strcmp (modelVals.gender[i1].name.GetData (), "female") == 0)
            for (size_t i5 = 0; i5 < modelVals.breastSize.GetSize (); i5++)
            {
              for (size_t i6 = 0; i6 < modelVals.breastFirmness.GetSize (); i6++)
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
                  targets.Push (target);
              }
            }
        }
      }

      // Genitals targets
      for (size_t i3 = 0; i3 < modelVals.genitals.GetSize (); i3++)
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
          targets.Push (target);
      }

      // Buttocks targets
      for (size_t i3 = 0; i3 < modelVals.buttocks.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.buttocks[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.buttocks[i3].weight);
        if (target.weight > EPSILON)
          targets.Push (target);
      }

      // Pelvis tone targets
      for (size_t i3 = 0; i3 < modelVals.pelvisTone.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.pelvisTone[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.pelvisTone[i3].weight);
        if (target.weight > EPSILON)
          targets.Push (target);
      }
    }
  }

  // Breast position targets
  for (size_t i1 = 0; i1 < modelVals.breastPosition.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastPosition[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastPosition[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }

  // Breast distance targets
  for (size_t i1 = 0; i1 < modelVals.breastDistance.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastDistance[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastDistance[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }

  // Breast taper targets
  for (size_t i1 = 0; i1 < modelVals.breastTaper.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastTaper[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastTaper[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }

  // Height targets
  for (size_t i1 = 0; i1 < modelVals.height.GetSize (); i1++)
  {
    name.Format ("universal-stature%s", modelVals.height[i1].name.GetData ());
    path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.height[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }

  // Measure targets
  for (size_t i1 = 0; i1 < modelVals.measures.GetSize (); i1++)
  {
    name.Format ("measure-%s", modelVals.measures[i1].name.GetData ());
    path.Format ("%smeasure/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.measures[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }

  // Print targets
  printf ("\nMakeHuman targets used by model:\n");
  for (size_t index = 0; index < targets.GetSize (); index++)
    printf ("%8.2f%% '%s'\n", targets[index].weight*100, targets[index].path.GetData ());
  printf ("\n");
}

void MakeHumanCharacter::GenerateTargetsWeightsEthnics
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Ethnics targets
  for (size_t i0 = 0; i0 < modelVals.ethnics.GetSize (); i0++)
  {
    for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
    {
      for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
      {
	float weight = modelVals.ethnics[i0].weight
	  * modelVals.gender[i1].weight 
	  * modelVals.age[i2].weight;

        if (weight > EPSILON)
	{
	  name.Format ("%s-%s%s",
		       modelVals.ethnics[i0].name.GetData (),
		       modelVals.gender[i1].name.GetData (), 
		       modelVals.age[i2].name.GetData ());
	  path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
	  target = Target (name.GetData (), path.GetData (), weight);
	  targets.Push (target);
	}
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsAgeGender
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  GenerateTargetsWeightsEthnics (modelVals, targets);
  GenerateTargetsWeightsWeightMuscle (modelVals, targets);
  GenerateTargetsWeightsStomach (modelVals, targets);
  GenerateTargetsWeightsBreast (modelVals, targets);
  GenerateTargetsWeightsGenitals (modelVals, targets);
  GenerateTargetsWeightsButtocks (modelVals, targets);
  GenerateTargetsWeightsPelvis (modelVals, targets);
}

void MakeHumanCharacter::GenerateTargetsWeightsWeightMuscle
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Muscle and weight targets
      for (size_t i3 = 0; i3 < modelVals.muscle.GetSize (); i3++)
      {
        for (size_t i4 = 0; i4 < modelVals.weight.GetSize (); i4++)
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
          if ((target.weight > EPSILON) && 
              !(strcmp (modelVals.muscle[i3].name.GetData (), "") == 0 &&
                strcmp (modelVals.weight[i4].name.GetData (), "") == 0))
            targets.Push (target);
	}
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsStomach
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Muscle and weight targets
      for (size_t i3 = 0; i3 < modelVals.muscle.GetSize (); i3++)
      {
        for (size_t i4 = 0; i4 < modelVals.weight.GetSize (); i4++)
        {
          // Stomach targets
          for (size_t i5 = 0; i5 < modelVals.stomach.GetSize (); i5++)
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
              targets.Push (target);
          }
	}
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsBreast
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Muscle and weight targets
      for (size_t i3 = 0; i3 < modelVals.muscle.GetSize (); i3++)
      {
        for (size_t i4 = 0; i4 < modelVals.weight.GetSize (); i4++)
        {
          // Breast size and firmness targets
          if (strcmp (modelVals.gender[i1].name.GetData (), "female") == 0)
            for (size_t i5 = 0; i5 < modelVals.breastSize.GetSize (); i5++)
            {
              for (size_t i6 = 0; i6 < modelVals.breastFirmness.GetSize (); i6++)
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
                  targets.Push (target);
              }
            }
	}
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsGenitals
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Genitals targets
      for (size_t i3 = 0; i3 < modelVals.genitals.GetSize (); i3++)
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
          targets.Push (target);
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsButtocks
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Buttocks targets
      for (size_t i3 = 0; i3 < modelVals.buttocks.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.buttocks[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.buttocks[i3].weight);
        if (target.weight > EPSILON)
          targets.Push (target);
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsPelvis
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Gender and age targets
  for (size_t i1 = 0; i1 < modelVals.gender.GetSize (); i1++)
  {
    for (size_t i2 = 0; i2 < modelVals.age.GetSize (); i2++)
    {    
      // Pelvis tone targets
      for (size_t i3 = 0; i3 < modelVals.pelvisTone.GetSize (); i3++)
      {
        name.Format ("%s%s%s",
                     modelVals.gender[i1].name.GetData (), 
                     modelVals.age[i2].name.GetData (), 
                     modelVals.pelvisTone[i3].name.GetData ());
        path.Format ("%sdetails/%s.target", TARGETS_PATH, name.GetData ());
        target = Target (name.GetData (), path.GetData (), modelVals.gender[i1].weight 
                         * modelVals.age[i2].weight * modelVals.pelvisTone[i3].weight);
        if (target.weight > EPSILON)
          targets.Push (target);
      }
    }
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsBreastPosition
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Breast position targets
  for (size_t i1 = 0; i1 < modelVals.breastPosition.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastPosition[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastPosition[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsBreastDistance
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Breast distance targets
  for (size_t i1 = 0; i1 < modelVals.breastDistance.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastDistance[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastDistance[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsBreastTaper
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Breast taper targets
  for (size_t i1 = 0; i1 < modelVals.breastTaper.GetSize (); i1++)
  {
    name.Format ("%s", modelVals.breastTaper[i1].name.GetData ());
    path.Format ("%sbreast/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.breastTaper[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsHeight
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Height targets
  for (size_t i1 = 0; i1 < modelVals.height.GetSize (); i1++)
  {
    name.Format ("universal-stature%s", modelVals.height[i1].name.GetData ());
    path.Format ("%smacrodetails/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.height[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }
}

void MakeHumanCharacter::GenerateTargetsWeightsMeasure
(const ModelTargets& modelVals, csArray<Target>& targets) const
{
  csString path, name;
  Target target;

  // Measure targets
  for (size_t i1 = 0; i1 < modelVals.measures.GetSize (); i1++)
  {
    name.Format ("measure-%s", modelVals.measures[i1].name.GetData ());
    path.Format ("%smeasure/%s.target", TARGETS_PATH, name.GetData ());
    target = Target (name.GetData (), path.GetData (), modelVals.measures[i1].weight);
    if (target.weight > EPSILON)
      targets.Push (target);
  }
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
