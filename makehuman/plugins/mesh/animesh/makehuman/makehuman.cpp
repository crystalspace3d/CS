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
#include "csutil/xmltiny.h"
#include "iutil/document.h"
#include "iutil/stringarray.h"

#include "character.h"
#include "makehuman.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

SCF_IMPLEMENT_FACTORY (MakeHumanManager);

static const char* msgid = "crystalspace.mesh.animesh.makehuman";

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
  if (!engine) return ReportError ("Failed to locate 3D engine!");

  g3d = csQueryRegistry<iGraphics3D> (objectRegistry);
  if (!g3d) return ReportError ("Failed to locate 3D renderer!");

  vfs = csQueryRegistry<iVFS> (objectRegistry);
  if (!vfs) return ReportError ("Failed to locate Virtual File System!");

  synldr = csQueryRegistry<iSyntaxService> (objectRegistry);
  if (!synldr) ReportError ("Failed to locate the syntax service!");

  animeshType = csLoadPluginCheck<iMeshObjectType>
    (objectRegistry, "crystalspace.mesh.object.animesh", false);
  if (!animeshType) return ReportError ("Could not load the animesh object plugin!");

  // Create the labels of the categories
  // TODO: parse that from the configuration file instead
  csStringArray array;

  array.Push ("female");
  array.Push ("male");
  categoryLabels.PutUnique ("gender", array);

  array.DeleteAll ();
  array.Push ("child");
  array.Push ("young");
  array.Push ("old");
  categoryLabels.PutUnique ("age", array);

  array.DeleteAll ();
  array.Push ("african");
  array.Push ("asian");
  array.Push ("neutral");
  categoryLabels.PutUnique ("ethnic", array);

  array.DeleteAll ();
  array.Push ("african");
  array.Push ("asian");
  array.Push ("caucasian");
  categoryLabels.PutUnique ("ethnic2", array);

  array.DeleteAll ();
  array.Push ("muscle");
  array.Push ("flaccid");
  array.Push ("averageTone");
  categoryLabels.PutUnique ("tone", array);

  array.DeleteAll ();
  array.Push ("heavy");
  array.Push ("light");
  array.Push ("averageWeight");
  categoryLabels.PutUnique ("weight", array);

  array.DeleteAll ();
  array.Push ("cup1");
  array.Push ("cup2");
  categoryLabels.PutUnique ("breastSize", array);

  array.DeleteAll ();
  array.Push ("firmness0");
  array.Push ("firmness1");
  categoryLabels.PutUnique ("breastFirmness", array);

  // Parse the categories of parameters from the configuration rules file
  InitTokenTable (xmltokens);
  if (!ParseConfigurationRules (CONFIGURATION_RULES_FILE))
    return false;

  // Build the list of references to the parameters
  for (csHash<MHCategory, csString>::GlobalIterator it = categories.GetIterator (); it.HasNext (); )
  {
    MHCategory& category = it.Next ();
    for (csHash<MHParameter, csString>::GlobalIterator rit = category.parameters.GetIterator (); rit.HasNext (); )
    {
      csString name;
      MHParameter& parameter = rit.Next (name);
      parameters.PutUnique (name, &parameter);
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
/*
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
*/
  for (csHash<MHCategory, csString>::ConstGlobalIterator it = categories.GetIterator (); it.HasNext (); )
  {
    csString name;
    const MHCategory& mhcategory = it.Next (name);

    if (mhcategory.parameters.Contains (parameter))
    {
      category = name;
      return true;
    }
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

  for (size_t i = 0; i < categoriesOrder.GetSize (); i++)
    names->Push (categoriesOrder.Get (i));

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

bool MakeHumanManager::ParseConfigurationRules (const char* filename)
{
  csRef<iFile> file = vfs->Open (filename, VFS_FILE_READ);
  if (!file)
  {
    ReportWarning ("Error opening the configuration rules file %s", CS::Quote::Single (filename));
    return false;
  }

  csRef<iDocumentSystem> documentSystem;
  documentSystem.AttachNew (new csTinyDocumentSystem ());
  csRef<iDocument> document = documentSystem->CreateDocument ();
  const char* error = document->Parse (file, false);
  if (error != 0)
  {
    ReportWarning ("Could not parse the configuration rules file %s: %s",
		   CS::Quote::Single (filename), error);
    return false;
  }

  csRef<iDocumentNode> root = document->GetRoot ()->GetNode ("rules");
  if (!root)
  {
    ReportWarning ("The configuration rules file %s is empty",
		   CS::Quote::Single (filename));
    return false;
  }

  csRef<iDocumentNodeIterator> it = root->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_CATEGORY:
      ParseCategory (child);
      break;

    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }

  return true;
}

bool MakeHumanManager::ParseCategory (iDocumentNode* node)
{
  const char* name = node->GetAttributeValue ("name");
  if (!name)
  {
    synldr->ReportError (msgid, node, "No name specified for the category");
    return false;
  }

#ifdef CS_DEBUG
  if (categories.Contains (name))
  {
    ReportError ("The category %s already exists", CS::Quote::Single (name));
    return false;
  }
#endif

  MHCategory* category = &categories.PutUnique (name, MHCategory ());
  categoriesOrder.Push (name);

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_SUBCATEGORY:
      ParseSubCategory (child, category);
      break;

    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }

  return true;
}

bool MakeHumanManager::ParseSubCategory (iDocumentNode* node, MHCategory* category)
{
  const char* name = node->GetAttributeValue ("name");
  if (!name)
  {
    synldr->ReportError (msgid, node, "No name specified for the sub-category");
    return false;
  }

  csString pattern = node->GetAttributeValue ("pattern");
  category->AddSubCategory (name, pattern);

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_GLOBAL:
    {
      csString pattern = child->GetAttributeValue ("pattern");
      globalPatterns.Push (pattern);
      break;
    }

    case XMLTOKEN_PARAMETER:
    {
      const char* name = child->GetAttributeValue ("name");
      const char* pattern = child->GetAttributeValue ("pattern");
      csString left = child->GetAttributeValue ("left");
      csString right = child->GetAttributeValue ("right");
      category->AddParameter (name, pattern, left, right);
      break;
    }

    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }

  return true;
}

//--------------------------- MHCategory ---------------------------

void MHCategory::AddSubCategory (const char* subCategory, const char* pattern)
{
#ifdef CS_DEBUG
  for (size_t i = 0; i < subCategories.GetSize (); i++)
    if (subCategories[i].name == subCategory)
    {
      csPrintf ("ERROR: The sub-category %s already exists\n", CS::Quote::Single (subCategory));
      return;
    }
#endif

  subCategories.Push (MHSubCategory (subCategory, pattern));
}

void MHCategory::AddParameter (const char* name, const char* left, const char* right)
{
  if (!name)
  {
    AddParameter (left, right);
    return;
  }

  MHSubCategory& subCategory = subCategories[subCategories.GetSize () - 1];

  subCategory.parameters.Push (name);
  parameters.PutUnique (name, MHParameter (subCategory.pattern, left, right));
}

void MHCategory::AddParameter (const char* name, const char* pattern,
			       const char* left, const char* right)
{
  if (!pattern)
  {
    AddParameter (name, left, right);
    return;
  }

  subCategories[subCategories.GetSize () - 1].parameters.Push (name);
  parameters.PutUnique (name, MHParameter (pattern, left, right));
}

void MHCategory::AddParameter (const char* left, const char* right)
{
  MHSubCategory& subCategory = subCategories[subCategories.GetSize () - 1];

  csString name = subCategory.name;
  name += subCategory.parameters.GetSize () + 1;

  subCategory.parameters.Push (name);
  parameters.PutUnique (name, MHParameter (subCategory.pattern, left, right));
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
