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
#include "csutil/scfstringarray.h"
#include "iutil/stringarray.h"

#include "character.h"
#include "makehuman.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

SCF_IMPLEMENT_FACTORY (MakeHumanManager);

MakeHumanManager::MakeHumanManager (iBase* parent)
  : scfImplementationType (this, parent)
{
}

MakeHumanManager::~MakeHumanManager ()
{
}

bool MakeHumanManager::Initialize (iObjectRegistry* objectRegistry)
{
  // Find references to the engine objects
  this->objectRegistry = objectRegistry;

  engine = csQueryRegistry<iEngine> (objectRegistry);
  if (!engine) return ReportError ( "Failed to locate 3D engine!");

  g3d = csQueryRegistry<iGraphics3D> (objectRegistry);
  if (!g3d) return ReportError ( "Failed to locate 3D renderer!");

  vfs = csQueryRegistry<iVFS> (objectRegistry);
  if (!vfs) return ReportError ( "Failed to locate Virtual File System!");

  animeshType = csLoadPluginCheck<iMeshObjectType>
    (objectRegistry, "crystalspace.mesh.object.animesh", false);
  if (!animeshType) return ReportError ( "Could not load the animesh object plugin!");

  // Create the labels of the categories
  // TODO: parse that from a configuration file instead
  csStringArray array;

  array.Push ("female");
  array.Push ("male");
  categoryLabels.Put ("gender", array);

  array.DeleteAll ();
  array.Push ("child");
  array.Push ("young");
  array.Push ("old");
  categoryLabels.Put ("age", array);

  array.DeleteAll ();
  array.Push ("african");
  array.Push ("asian");
  array.Push ("neutral");
  categoryLabels.Put ("ethnic", array);

  array.DeleteAll ();
  array.Push ("african");
  array.Push ("asian");
  array.Push ("caucasian");
  categoryLabels.Put ("ethnic2", array);

  array.DeleteAll ();
  array.Push ("muscle");
  array.Push ("flaccid");
  array.Push ("averageTone");
  categoryLabels.Put ("tone", array);

  array.DeleteAll ();
  array.Push ("heavy");
  array.Push ("light");
  array.Push ("averageWeight");
  categoryLabels.Put ("weight", array);

  array.DeleteAll ();
  array.Push ("cup1");
  array.Push ("cup2");
  categoryLabels.Put ("breastSize", array);

  array.DeleteAll ();
  array.Push ("firmness0");
  array.Push ("firmness1");
  categoryLabels.Put ("breastFirmness", array);

  // Create the categories of parameters
  // TODO: parse that from a configuration file instead
  MHCategory* category = &categories.Put ("macro", MHCategory ());

  category->AddSubCategory ("main", "");
  globalPatterns.Push ("macrodetails/universal-${gender}-${age}-${tone}-${weight}");
  globalPatterns.Push ("macrodetails/${ethnic}-${gender}-${age}");
  category->AddParameter ("gender", "", "");
  category->AddParameter ("age", "", "");
  //category->AddParameter ("caucasian", "", "");
  category->AddParameter ("african", "", "");
  category->AddParameter ("asian", "", "");
  category->AddParameter ("weight", "", "");
  category->AddParameter ("tone", "", "");
  category->AddParameter ("height", "macrodetails/universal-stature-${value}", "dwarf", "giant");

  category = &categories.Put ("torso", MHCategory ());

  category->AddSubCategory ("main", "");
  category->AddParameter ("pelvisTone", "details/${gender}-${age}-pelvis-tone${value}", "1", "2");
  category->AddParameter ("stomach", "details/${gender}-${age}-${tone}-${weight}-stomach${value}", "1", "2");
  category->AddParameter ("buttocks", "details/${gender}-${age}-nates${value}", "1", "2");

  category = &categories.Put ("gender", MHCategory ());

  category->AddSubCategory ("main", "");
  category->AddParameter ("genitals", "details/genitals_${gender}_${value}_${age}", "feminine", "masculine");
  globalPatterns.Push ("breast/female-${age}-${tone}-${weight}-${breastSize}-${breastFirmness}");
  category->AddParameter ("breastSize", "", "");
  category->AddParameter ("breastFirmness", "", "");
  category->AddParameter ("breastPosition", "breast/breast-${value}", "down", "up");
  category->AddParameter ("breastDistance", "breast/breast-dist-${value}", "min", "max");
  category->AddParameter ("breastPoint", "breast/breast-point-${value}", "min", "max");

  category = &categories.Put ("face", MHCategory ());

  category->AddSubCategory ("neck", "neck/${ethnic2}/${gender}_${age}/${value}");
  category->AddParameter ("neck-scale-depth-less", "neck-scale-depth-more");
  category->AddParameter ("neck-scale-horiz-less", "neck-scale-horiz-more");

  category->AddSubCategory ("cheek", "cheek/${ethnic2}/${gender}_${age}/${value}");
  category->AddParameter ("l-cheek-in", "l-cheek-out");
  category->AddParameter ("l-cheek-bones-out", "l-cheek-bones-in");
  category->AddParameter ("r-cheek-in", "r-cheek-out");
  category->AddParameter ("r-cheek-bones-out", "r-cheek-bones-in");

  category->AddSubCategory ("head-shape", "head/${ethnic}/${gender}_${age}/${value}");
  category->AddParameter ("", "head-oval");
  category->AddParameter ("", "head-round");
  category->AddParameter ("", "head-rectangular");

  // Build the list of references to the parameters
  for (csHash<MHCategory, csString>::GlobalIterator it = categories.GetIterator (); it.HasNext (); )
  {
    MHCategory& category = it.Next ();
    for (csHash<MHParameter, csString>::GlobalIterator rit = category.parameters.GetIterator (); rit.HasNext (); )
    {
      csString name;
      MHParameter& parameter = rit.Next (name);
      parameters.Put (name, &parameter);
    }
  }

  /*
    TODO: cfg file or options for:
      - paths
      - scale
      - if generate expressions
      - if generate clothes
      - if clean skeleton
      - cache size
  */

  // Parse the object file describing the MakeHuman neutral model
  if (!ParseObjectFile (MESH_DATA_FILE, coords, texcoords, normals, faceGroups))
    return ReportError ("Failed parsing of base object file '%s'", MESH_DATA_FILE);

  return true;
}

csPtr<iMakeHumanCharacter> MakeHumanManager::CreateCharacter ()
{
  return new MakeHumanCharacter (this);
}

const MHParameter* MakeHumanManager::FindParameter (const char* category, const char* parameter) const
{
  const MHCategory* mhcategory = categories.GetElementPointer (category);
  if (!mhcategory) return nullptr;

  return mhcategory->parameters.GetElementPointer (parameter);
}

bool MakeHumanManager::FindParameterCategory (const char* parameter, csString& category) const
{
  // We actually look only in the main categories that are not defined in the MakeHuman
  // model files
  const MHCategory* mhcategory = categories.GetElementPointer ("macro");
  if (mhcategory->parameters.Contains (parameter))
  {
    category = "macro";
    return true;
  }

  mhcategory = categories.GetElementPointer ("gender");
  if (mhcategory->parameters.Contains (parameter))
  {
    category = "gender";
    return true;
  }

  mhcategory = categories.GetElementPointer ("torso");
  if (mhcategory->parameters.Contains (parameter))
  {
    category = "torso";
    return true;
  }

  return false;
}

csPtr<iStringArray> MakeHumanManager::GetProxies () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the MakeHuman data
  names->Push ("ascottk");
  //names->Push ("clear");
  names->Push ("male");
  names->Push ("rorkimaru");
  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakeHumanManager::GetRigs () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the MakeHuman data
  names->Push ("game");
  names->Push ("rigid");
  names->Push ("second_life");
  names->Push ("simple");
  names->Push ("xonotic");
  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakeHumanManager::GetCategories () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());

  for (csHash<MHCategory, csString>::ConstGlobalIterator it = categories.GetIterator (); it.HasNext (); )
  {
    csString name;
    it.Next (name);
    names->Push (name);
  }

  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakeHumanManager::GetSubCategories (const char* category) const
{
  const MHCategory* mhcategory = categories.GetElementPointer (category);
  if (!mhcategory)
  {
    ReportError ("The category %s doesn't exist", CS::Quote::Single (category));
    return csPtr<iStringArray> (nullptr);
  }

  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());

  for (size_t i = 0; i < mhcategory->subCategories.GetSize (); i++)
    names->Push (mhcategory->subCategories[i].name);

  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakeHumanManager::GetParameters (const char* category, const char* subCategory) const
{
  const MHCategory* mhcategory = categories.GetElementPointer (category);
  if (!mhcategory)
  {
    ReportError ("The category %s doesn't exist", CS::Quote::Single (category));
    return csPtr<iStringArray> (nullptr);
  }

  const MHSubCategory* mhsubCategory = nullptr;
  for (size_t i = 0; i < mhcategory->subCategories.GetSize (); i++)
    if (mhcategory->subCategories[i].name == subCategory)
    {
      mhsubCategory = &mhcategory->subCategories[i];
      break;
    }

  if (!mhsubCategory)
  {
    ReportError ("The sub-category %s doesn't exist", CS::Quote::Single (subCategory));
    return csPtr<iStringArray> (nullptr);
  }

  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());

  for (size_t i = 0; i < mhsubCategory->parameters.GetSize (); i++)
    names->Push (mhsubCategory->parameters[i]);

  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakeHumanManager::GetParameters (const char* category) const
{
  const MHCategory* mhcategory = categories.GetElementPointer (category);
  if (!mhcategory)
  {
    ReportError ("The category %s doesn't exist", CS::Quote::Single (category));
    return csPtr<iStringArray> (nullptr);
  }

  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());

  for (csHash<MHParameter, csString>::ConstGlobalIterator it = mhcategory->parameters.GetIterator (); it.HasNext (); )
  {
    csString name;
    it.Next (name);
    names->Push (name);
  }

  return csPtr<iStringArray> (names);
}

//--------------------------- MHCategory ---------------------------

void MHCategory::AddSubCategory (const char* subCategory, const char* pattern)
{
  subCategories.Push (MHSubCategory (subCategory, pattern));
}

void MHCategory::AddParameter (const char* name, const char* left, const char* right)
{
  MHSubCategory& subCategory = subCategories[subCategories.GetSize () - 1];

  subCategory.parameters.Push (name);
  parameters.Put (name, MHParameter (subCategory.pattern, left, right));
}

void MHCategory::AddParameter (const char* name, const char* pattern,
			       const char* left, const char* right)
{
  subCategories[subCategories.GetSize () - 1].parameters.Push (name);
  parameters.Put (name, MHParameter (pattern, left, right));
}

void MHCategory::AddParameter (const char* left, const char* right)
{
  MHSubCategory& subCategory = subCategories[subCategories.GetSize () - 1];

  csString name = subCategory.name;
  name += subCategory.parameters.GetSize () + 1;

  subCategory.parameters.Push (name);
  parameters.Put (name, MHParameter (subCategory.pattern, left, right));
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
