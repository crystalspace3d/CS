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

void MakeHumanCharacter::Clear ()
{
  //SetNeutral ();
  parameters.DeleteAll ();
  africanValue = asianValue = 0.0f;
  breastSizeValue = 0.0f;

  skinFile = "";
  proxyFilename = "";
  clothesNames.DeleteAll ();
  clothes.DeleteAll ();
}

void MakeHumanCharacter::SetNeutral ()
{
  // TODO: manage also the fast update mode

  parameters.DeleteAll ();
  parameters.Put ("neutral", 1.f);
  parameters.Put ("caucasian", 1.f);
  parameters.Put ("female", .5f);
  parameters.Put ("male", .5f);
  parameters.Put ("young", 1.f);
  parameters.Put ("firmness0", 0.5f);
  parameters.Put ("firmness1", 0.5f);
  parameters.Put ("averageTone", 1.f);
  parameters.Put ("averageWeight", 1.f);

  africanValue = asianValue = 0.0f;
  breastSizeValue = 0.0f;
}

void MakeHumanCharacter::SetParameter (const char* category, const char* parameter, float value)
{
  if (updateMode == MH_UPDATE_FULL)
    SetParameterInternal (category, parameter, value);

  else
  {
    // Compute the delta value to be applied
    float delta = value - GetParameter (category, parameter);
    if (fabs (delta) > SMALL_EPSILON)
    {
      // Create the mesh factory if not yet made
      if (!coords.GetSize ())
      {
	SetParameterInternal (category, parameter, value);
	UpdateMeshFactory ();
	return;
      }

      // Apply the morph targets activated by the parameter
      // TODO: check if a boundary point is crossed
      csRefArray<CS::Mesh::iMakeHumanMorphTarget> targets;
      bool boundary = GetParameterTargets (category, parameter, targets);
      ApplyTargets (targets, boundary, delta);

      // Update the value of the parameter
      SetParameterInternal (category, parameter, value);
    }
  }
}

void MakeHumanCharacter::SetParameterInternal (const char* category, const char* parameter, float value)
{
  // Translate the macro parameters into lower level variables
  if (!strcmp (parameter, "gender"))
  {
    parameters.PutUnique ("male", value);
    parameters.PutUnique ("female", 1.0f - value);

    float breastValue = (1.0f - value) * breastSizeValue;
    parameters.PutUnique ("cup1", -csMin (breastValue, 0.0f));
    parameters.PutUnique ("cup2", csMax (breastValue, 0.0f));

    return;
  }

  if (!strcmp (parameter, "age"))
  {
    value = value * 2.f - 1.f;

    if (value > SMALL_EPSILON)
    {
      parameters.PutUnique ("old", value);
      parameters.DeleteAll ("child");
      parameters.PutUnique ("young", 1.0f - value);
    }

    else if (value < -SMALL_EPSILON)
    {
      parameters.DeleteAll ("old");
      parameters.PutUnique ("child", -value);
      parameters.PutUnique ("young", 1.0f + value);
    }

    else
    {
      parameters.DeleteAll ("old");
      parameters.DeleteAll ("child");
      parameters.PutUnique ("young", 1.0f);
    }

    return;
  }

  if (!strcmp (parameter, "african")
      || !strcmp (parameter, "asian"))
  {
    if (!strcmp (parameter, "african"))
      africanValue = value;
    else
      asianValue = value;

    float african = africanValue;
    float asian = asianValue;

    float ethnics = 0.0f;
    if (african > SMALL_EPSILON) ethnics += 1.0f;
    if (asian > SMALL_EPSILON) ethnics += 1.0f;

    float neutral = 1.0f - (african + asian) / ethnics;

    if (ethnics > 1.5f)
    {
      african /= ethnics;
      asian /= ethnics;
    }

    parameters.PutUnique ("african", african);
    parameters.PutUnique ("asian", asian);
    parameters.PutUnique ("neutral", neutral);
    parameters.PutUnique ("caucasian", neutral);

    return;
  }

  if (!strcmp (parameter, "tone"))
  {
    value = value * 2.f - 1.f;

    if (value > SMALL_EPSILON)
    {
      parameters.PutUnique ("muscle", value);
      parameters.DeleteAll ("flaccid");
      parameters.PutUnique ("averageTone", 1.f - value);
    }

    else if (value < -SMALL_EPSILON)
    {
      parameters.DeleteAll ("muscle");
      parameters.PutUnique ("flaccid", -value);
      parameters.PutUnique ("averageTone", 1.f + value);
    }

    else
    {
      parameters.DeleteAll ("muscle");
      parameters.DeleteAll ("flaccid");
      parameters.PutUnique ("averageTone", 1.f);
    }

    return;
  }

  if (!strcmp (parameter, "weight"))
  {
    value = value * 2.f - 1.f;

    if (value > SMALL_EPSILON)
    {
      parameters.PutUnique ("heavy", value);
      parameters.DeleteAll ("light");
      parameters.PutUnique ("averageWeight", 1.f - value);
    }

    else if (value < -SMALL_EPSILON)
    {
      parameters.DeleteAll ("heavy");
      parameters.PutUnique ("light", -value);
      parameters.PutUnique ("averageWeight", 1.f + value);
    }

    else
    {
      parameters.DeleteAll ("heavy");
      parameters.DeleteAll ("light");
      parameters.PutUnique ("averageWeight", 1.f);
    }

    return;
  }

  if (!strcmp (parameter, "breastFirmness"))
  {
    parameters.PutUnique ("firmness0", 1.f - value);
    parameters.PutUnique ("firmness1", value);
    return;
  }

  if (!strcmp (parameter, "breastSize"))
  {
    breastSizeValue = value;
    value *= parameters.Get ("female", 0.5f);
    parameters.PutUnique ("cup1", -csMin (value, 0.0f));
    parameters.PutUnique ("cup2", csMax (value, 0.0f));
    return;
  }

  // Generic update of the parameter
  const MHParameter* mhparameter = manager->FindParameter (category, parameter);
  if (!mhparameter)
  {
    ReportError ("The parameter %s doesn't exist", CS::Quote::Single (csString ().Format ("%s:%s", category, parameter)));
    return;
  }

  if (fabs (value - mhparameter->neutral) < SMALL_EPSILON)
    parameters.DeleteAll (parameter);

  else
    parameters.PutUnique (parameter, value);
}

float MakeHumanCharacter::GetParameter (const char* category, const char* parameter) const
{
  // Translate the lower level variables into macro parameters
  if (!strcmp (parameter, "gender"))
    return parameters.Get ("male", 0.5f);

  if (!strcmp (parameter, "age"))
  {
    const float* value = parameters.GetElementPointer ("old");
    if (value && *value > SMALL_EPSILON) return .5f + *value * .5f;

    value = parameters.GetElementPointer ("child");
    if (value && *value > SMALL_EPSILON) return .5f - *value * .5f;

    return .5f;
  }

  if (!strcmp (parameter, "african"))
    return africanValue;

  if (!strcmp (parameter, "asian"))
    return asianValue;

  if (!strcmp (parameter, "tone"))
  {
    const float* value = parameters.GetElementPointer ("muscle");
    if (value && *value > SMALL_EPSILON) return .5f + *value * .5f;

    value = parameters.GetElementPointer ("flaccid");
    if (value && *value > SMALL_EPSILON) return .5f - *value * .5f;

    return .5f;
  }

  if (!strcmp (parameter, "weight"))
  {
    const float* value = parameters.GetElementPointer ("heavy");
    if (value && *value > SMALL_EPSILON) return .5f + *value * .5f;

    value = parameters.GetElementPointer ("light");
    if (value && *value > SMALL_EPSILON) return .5f - *value * .5f;

    return .5f;
  }

  if (!strcmp (parameter, "breastFirmness"))
    return parameters.Get ("firmness1", 0.0f);

  if (!strcmp (parameter, "breastSize"))
    return breastSizeValue;

  // Generic access to the parameter
  const MHParameter* mhparameter = manager->FindParameter (category, parameter);
  if (!mhparameter)
  {
    ReportError ("The parameter %s doesn't exist", CS::Quote::Single (parameter));
    return 0.0f;
  }

  return parameters.Get (parameter, mhparameter->neutral);
}

bool MakeHumanCharacter::GetParameterTargets
(const char* category, const char* parameter, csRefArray<iMakeHumanMorphTarget>& targets)
{
  if (!strcmp (parameter, "gender"))
  {
    // Save the current value of the parameter
    float gender = parameters.Get ("male", 0.5f);

    // 'male' targets
    parameters.PutUnique ("male", 1.0f);
    parameters.PutUnique ("female", .0f);

    csArray<Target> localTargets;
    ExpandParameterTargets (localTargets, "gender");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    // 'female' targets
    parameters.PutUnique ("male", 0.f);
    parameters.PutUnique ("female", 1.0f);

    ExpandParameterTargets (localTargets, "gender");
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

    // 'breastSize' targets
    if (fabs (breastSizeValue) > SMALL_EPSILON)
    {
      float breastValue = breastSizeValue;
      parameters.PutUnique ("cup1", -csMin (breastValue, 0.0f));
      parameters.PutUnique ("cup2", csMax (breastValue, 0.0f));

      ExpandParameterTargets (localTargets, "breastSize");
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);
    }

    // Put back the initial parameter values
    parameters.PutUnique ("male", gender);
    parameters.PutUnique ("female", 1.0f - gender);

    float breastValue = (1.0f - gender) * breastSizeValue;
    parameters.PutUnique ("cup1", -csMin (breastValue, 0.0f));
    parameters.PutUnique ("cup2", csMax (breastValue, 0.0f));

    return false;
  }

  if (!strcmp (parameter, "age"))
  {
    bool result;

    // Save the current value of the parameter
    float old = parameters.Get ("old", 0.f);
    float child = parameters.Get ("child", 0.f);
    float young = parameters.Get ("young", 0.f);

    if (old > SMALL_EPSILON)
    {
      parameters.PutUnique ("old", 1.0f);
      parameters.PutUnique ("child", .0f);
      parameters.PutUnique ("young", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "age");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else if (child > SMALL_EPSILON)
    {
      parameters.PutUnique ("old", .0f);
      parameters.PutUnique ("child", 1.0f);
      parameters.PutUnique ("young", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "age");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else
    {
      // 'old' targets
      parameters.PutUnique ("old", 1.0f);
      parameters.PutUnique ("child", .0f);
      parameters.PutUnique ("young", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "age");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

      // 'child' targets
      parameters.PutUnique ("old", .0f);
      parameters.PutUnique ("child", 1.0f);
      parameters.PutUnique ("young", -1.0f);

      ExpandParameterTargets (localTargets, "age");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

      result = true;
    }

    // Put back the initial parameter values
    parameters.PutUnique ("old", old);
    parameters.PutUnique ("child", child);
    parameters.PutUnique ("young", young);

    return result;
  }

  // TODO: ethnics is not perfect yet
  if (!strcmp (parameter, "african"))
  {
    float african = 1.0f;
    float asian = asianValue;

    float ethnics = 1.0f;
    if (asianValue > SMALL_EPSILON) ethnics += 1.0f;

    float neutral = -1.f;

    if (ethnics > 1.5f)
    {
      african /= ethnics;
      asian = -asian / ethnics;
      neutral = (asian - 1.f) / ethnics;
    }

    parameters.PutUnique ("african", african);
    parameters.PutUnique ("asian", asian);
    parameters.PutUnique ("neutral", neutral);
    parameters.PutUnique ("caucasian", neutral);

    csArray<Target> localTargets;
    ExpandParameterTargets (localTargets, "ethnic");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    ExpandParameterTargets (localTargets, "ethnic2");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    // Put back the initial parameter values
    SetParameterInternal ("macro", "african", africanValue);

    return false;
  }


  if (!strcmp (parameter, "asian"))
  {
    float african = africanValue;
    float asian = 1.0f;

    float ethnics = 1.0f;
    if (africanValue > SMALL_EPSILON) ethnics += 1.0f;

    float neutral = -1.f;

    if (ethnics > 1.5f)
    {
      african = -african / ethnics;
      asian /= ethnics;
      neutral = (african - 1.f) / ethnics;
    }

    parameters.PutUnique ("african", african);
    parameters.PutUnique ("asian", asian);
    parameters.PutUnique ("neutral", neutral);
    parameters.PutUnique ("caucasian", neutral);

    csArray<Target> localTargets;
    ExpandParameterTargets (localTargets, "ethnic");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    ExpandParameterTargets (localTargets, "ethnic2");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    // Put back the initial parameter values
    SetParameterInternal ("macro", "asian", asianValue);

    return false;
  }

  if (!strcmp (parameter, "tone"))
  {
    bool result;

    // Save the current value of the parameter
    float muscle = parameters.Get ("muscle", 0.f);
    float flaccid = parameters.Get ("flaccid", 0.f);
    float averageTone = parameters.Get ("averageTone", 0.f);

    if (muscle > SMALL_EPSILON)
    {
      parameters.PutUnique ("muscle", 1.0f);
      parameters.PutUnique ("flaccid", .0f);
      parameters.PutUnique ("averageTone", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "tone");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else if (flaccid > SMALL_EPSILON)
    {
      parameters.PutUnique ("muscle", .0f);
      parameters.PutUnique ("flaccid", 1.0f);
      parameters.PutUnique ("averageTone", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "tone");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else
    {
      // 'muscle' targets
      parameters.PutUnique ("muscle", 1.0f);
      parameters.PutUnique ("flaccid", .0f);
      parameters.PutUnique ("averageTone", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "tone");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

      // 'flaccid' targets
      parameters.PutUnique ("muscle", .0f);
      parameters.PutUnique ("flaccid", 1.0f);
      parameters.PutUnique ("averageTone", -1.0f);

      ExpandParameterTargets (localTargets, "tone");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

      result = true;
    }

    // Put back the initial parameter values
    parameters.PutUnique ("muscle", muscle);
    parameters.PutUnique ("flaccid", flaccid);
    parameters.PutUnique ("averageTone", averageTone);

    return result;
  }

  if (!strcmp (parameter, "weight"))
  {
    bool result;

    // Save the current value of the parameter
    float heavy = parameters.Get ("heavy", 0.f);
    float light = parameters.Get ("light", 0.f);
    float averageWeight = parameters.Get ("averageWeight", 0.f);

    if (heavy > SMALL_EPSILON)
    {
      parameters.PutUnique ("heavy", 1.0f);
      parameters.PutUnique ("light", .0f);
      parameters.PutUnique ("averageWeight", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "weight");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else if (light > SMALL_EPSILON)
    {
      parameters.PutUnique ("heavy", .0f);
      parameters.PutUnique ("light", 1.0f);
      parameters.PutUnique ("averageWeight", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "weight");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else
    {
      // 'heavy' targets
      parameters.PutUnique ("heavy", 1.0f);
      parameters.PutUnique ("light", .0f);
      parameters.PutUnique ("averageWeight", -1.0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "weight");
      ConvertTargets (targets, localTargets, 2.0f, MH_DIRECTION_UP);

      // 'light' targets
      parameters.PutUnique ("heavy", .0f);
      parameters.PutUnique ("light", 1.0f);
      parameters.PutUnique ("averageWeight", -1.0f);

      ExpandParameterTargets (localTargets, "weight");
      ConvertTargets (targets, localTargets, -2.0f, MH_DIRECTION_DOWN);

      result = true;
    }

    // Put back the initial parameter values
    parameters.PutUnique ("heavy", heavy);
    parameters.PutUnique ("light", light);
    parameters.PutUnique ("averageWeight", averageWeight);

    return result;
  }

  if (!strcmp (parameter, "breastFirmness"))
  {
    // Save the current value of the parameter
    float firmness1 = parameters.Get ("firmness1", 0.5f);

    // 'positive firmness' targets
    parameters.PutUnique ("firmness0", .0f);
    parameters.PutUnique ("firmness1", 1.0f);

    csArray<Target> localTargets;
    ExpandParameterTargets (localTargets, "breastFirmness");
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

    // 'negative firmness' targets
    parameters.PutUnique ("firmness0", 1.0f);
    parameters.PutUnique ("firmness1", .0f);

    ExpandParameterTargets (localTargets, "breastFirmness");
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

    // Put back the initial parameter values
    parameters.PutUnique ("firmness0", 1.0f - firmness1);
    parameters.PutUnique ("firmness1", firmness1);

    return false;
  }

  if (!strcmp (parameter, "breastSize"))
  {
    bool result;

    // Save the current value of the parameter
    float cup1 = parameters.Get ("cup1", 0.f);
    float cup2 = parameters.Get ("cup2", 0.f);
    float female = parameters.Get ("female", 0.5f);

    if (cup1 > SMALL_EPSILON)
    {
      parameters.PutUnique ("cup1", female);
      parameters.PutUnique ("cup2", .0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "breastSize");
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else if (cup2 > SMALL_EPSILON)
    {
      parameters.PutUnique ("cup1", .0f);
      parameters.PutUnique ("cup2", female);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "breastSize");
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);

      result = false;
    }

    else
    {
      // 'cup1' targets
      parameters.PutUnique ("cup1", female);
      parameters.PutUnique ("cup2", .0f);

      csArray<Target> localTargets;
      ExpandParameterTargets (localTargets, "breastSize");
      ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

      // 'cup2' targets
      parameters.PutUnique ("cup1", .0f);
      parameters.PutUnique ("cup2", female);

      ExpandParameterTargets (localTargets, "breastSize");
      ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

      result = true;
    }

    // Put back the initial parameter values
    parameters.PutUnique ("cup1", cup1);
    parameters.PutUnique ("cup2", cup2);

    return result;
  }

#ifdef CS_DEBUG
  // TODO: use FindParameter
  if (!manager->parameters[parameter])
    return ReportError ("The parameter %s doesn't exist", CS::Quote::Single (parameter));
#endif

  const MHParameter* parameterData = manager->FindParameter (category, parameter);
  if (!manager->parameters[parameter])
    return ReportError ("The parameter %s doesn't exist", CS::Quote::Single (parameter));

  float value = parameters.Get (parameter, parameterData->neutral);

  if (parameterData->left.IsEmpty ()
      || parameterData->right.IsEmpty ())
  {
    csArray<Target> localTargets;
    ExpandTargets (localTargets, parameterData, parameter, 1.0f);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);
    return false;
  }

  if (value > parameterData->neutral + SMALL_EPSILON)
  {
    csArray<Target> localTargets;
    ExpandTargets (localTargets, parameterData, parameter, 1.0f);
    ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_BOTH);
    return false;
  }

  if (value < parameterData->neutral - SMALL_EPSILON)
  {
    csArray<Target> localTargets;
    ExpandTargets (localTargets, parameterData, parameter, 1.0f);
    ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_BOTH);
    return false;
  }

  csArray<Target> localTargets;
  localTargets.Push (Target (parameterData->pattern));
  ExpandTargets (localTargets, parameterData->right, 1.0f);
  ExpandGlobalTargets (localTargets);
  ConvertTargets (targets, localTargets, 1.0f, MH_DIRECTION_UP);

  localTargets.Push (Target (parameterData->pattern));
  ExpandTargets (localTargets, parameterData->left, 1.0f);
  ExpandGlobalTargets (localTargets);
  ConvertTargets (targets, localTargets, -1.0f, MH_DIRECTION_DOWN);

  return true;
}

void MakeHumanCharacter::ExpandGlobalTargets (csArray<Target>& targets) const
{
  ExpandTargets (targets, "gender");
  ExpandTargets (targets, "age");
  ExpandTargets (targets, "ethnic");
  ExpandTargets (targets, "ethnic2");
  ExpandTargets (targets, "tone");
  ExpandTargets (targets, "weight");
  ExpandTargets (targets, "breastFirmness");
  ExpandTargets (targets, "breastSize");
}

void MakeHumanCharacter::ExpandParameterTargets (csArray<Target>& targets, const char* parameter) const
{
  csString pattern = "${";
  pattern += parameter;
  pattern += "}";

  // Expand the global patterns
  for (size_t i = 0; i < manager->globalPatterns.GetSize (); i++)
  {
    csString globalPattern = manager->globalPatterns.Get (i);
    if (globalPattern.Find (pattern) == (size_t) -1)
      continue;

    csArray<Target> localTargets;
    localTargets.Push (Target (globalPattern));
    ExpandGlobalTargets (localTargets);
    targets.Merge (localTargets);
  }

  // Expand the patterns of all active parameters
  for (csHash<float, csString>::ConstGlobalIterator it = parameters.GetIterator (); it.HasNext (); )
  {
    csString parameter;
    it.Next (parameter);

    // Ignore the internal parameters that have no description
    if (!manager->parameters[parameter]) continue;

    // TODO: use FindParameter
    const MHParameter* parameterData = *manager->parameters[parameter];
    if (parameterData->pattern.Find (pattern) == (size_t) -1)
      continue;

    csArray<Target> localTargets;
    ExpandTargets (localTargets, parameterData, parameter, 1.0f);
    targets.Merge (localTargets);
  }
}

void MakeHumanCharacter::ExpandTargets (csArray<Target>& targets) const
{
  // Expand the global patterns
  for (size_t i = 0; i < manager->globalPatterns.GetSize (); i++)
  {
    csArray<Target> localTargets;
    localTargets.Push (manager->globalPatterns.Get (i));
    ExpandGlobalTargets (localTargets);
    targets.Merge (localTargets);
  }

  // Expand the patterns of all active parameters
  for (csHash<float, csString>::ConstGlobalIterator it = parameters.GetIterator (); it.HasNext (); )
  {
    csString parameter;
    float value = it.Next (parameter);

    // Ignore the internal parameters that have no description
    if (!manager->parameters[parameter]) continue;

    // TODO: use FindParameter
    const MHParameter* parameterData = *manager->parameters[parameter];
    if (parameterData->pattern == "") continue;

    csArray<Target> localTargets;
    ExpandTargets (localTargets, parameterData, parameter, value);
    targets.Merge (localTargets);
  }

  // Print targets
  printf ("\nMakeHuman targets stack of the character:\n");
  for (size_t index = 0; index < targets.GetSize (); index++)
    printf ("%8.2f%% '%s'\n", targets[index].weight * 100.f, targets[index].name.GetData ());
  printf ("\n");
}

void MakeHumanCharacter::ExpandTargets (csArray<Target>& targets, const char* pattern) const
{
  CS_ASSERT (manager->categoryLabels.GetElementPointer (pattern));
  ExpandTargets (targets, pattern, *manager->categoryLabels.GetElementPointer (pattern));
}

void MakeHumanCharacter::ExpandTargets (csArray<Target>& targets, const char* _pattern, const csStringArray& values) const
{
  csString pattern = "${";
  pattern += _pattern;
  pattern += "}";

  csArray<Target> results;
  for (size_t i = 0; i < targets.GetSize (); i++)
  {
    Target& target = targets.Get (i);

    // Don't do anything if the pattern is not present
    if (target.name.Find (pattern) == (size_t) -1)
    {
      results.Push (target);
      continue;
    }
    
    // Expand the pattern
    for (size_t j = 0; j < values.GetSize (); j++)
    {
      float weight = target.weight * parameters.Get (values[j], 0.0f);
      if (fabs (weight) < SMALL_EPSILON)
	continue;

      if (!strcmp (values[j], "averageTone")
	  || !strcmp (values[j], "averageWeight"))
      {
	Target newTarget (target.name, weight);
	newTarget.name.ReplaceAll (csString ().Format ("-%s", pattern.GetData ()), "");
	results.Push (newTarget);
	continue;
      }

      Target newTarget (target.name, weight);
      newTarget.name.ReplaceAll (pattern, values[j]);
      results.Push (newTarget);
    }
  }

  targets = results;
}

void MakeHumanCharacter::ExpandTargets (csArray<Target>& targets, const char* parameter, float weight) const
{
  const char* pattern = "${value}";

  for (int i = targets.GetSize () - 1; i >= 0; i--)
  {
    Target& target = targets.Get (i);
    target.name.ReplaceAll (pattern, parameter);
    target.weight *= weight;

    if (fabs (target.weight) < SMALL_EPSILON)
      targets.DeleteIndex (i);
  }
}

void MakeHumanCharacter::ExpandTargets (csArray<Target>& targets, const MHParameter* parameterData, const char* parameter, float weight) const
{
  if (weight > parameterData->neutral + SMALL_EPSILON)
  {
    if (parameterData->right == "") return;

    targets.Push (Target (parameterData->pattern));
    ExpandTargets (targets, parameterData->right, weight);
  }
  else if (weight < parameterData->neutral - SMALL_EPSILON)
  {
    if (parameterData->left == "") return;

    targets.Push (Target (parameterData->pattern));
    ExpandTargets (targets, parameterData->left, -weight);
  }
  else return;

  ExpandGlobalTargets (targets);
}

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

    manager->ParseMakeHumanTargetFile (&localTargets[i], target->offsets, target->indices);

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

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
