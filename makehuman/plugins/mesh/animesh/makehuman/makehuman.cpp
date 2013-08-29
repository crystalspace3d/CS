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

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

SCF_IMPLEMENT_FACTORY (MakehumanManager);

MakehumanManager::MakehumanManager (iBase* parent)
  : scfImplementationType (this, parent)
{
}

MakehumanManager::~MakehumanManager ()
{
}

bool MakehumanManager::Initialize (iObjectRegistry* objectRegistry)
{
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

  /*
    TODO: cfg file or options for:
      - paths
      - scale
      - if generate expressions
      - if generate clothes
      - if clean skeleton
      - cache size

    TODO: use a cache for the ressource files
    TODO: threaded loading of the data
  */

  // Parse the object file describing the Makehuman neutral model
  if (!ParseObjectFile (MESH_DATA_FILE, coords, texcoords, normals, faceGroups))
    return ReportError ("Failed parsing of base object file '%s'", MESH_DATA_FILE);

  return true;
}

csPtr<iMakehumanCharacter> MakehumanManager::CreateCharacter ()
{
  return new MakehumanCharacter (this);
}

csPtr<iStringArray> MakehumanManager::GetProxies () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the Makehuman data
  names->Push ("ascottk");
  //names->Push ("clear");
  names->Push ("male");
  names->Push ("rorkimaru");
  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakehumanManager::GetRigs () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the Makehuman data
  names->Push ("game");
  names->Push ("rigid");
  names->Push ("second_life");
  names->Push ("simple");
  names->Push ("xonotic");
  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakehumanManager::GetMeasures () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the Makehuman data
  names->Push ("ankle");
  names->Push ("bust");
  names->Push ("calf");
  names->Push ("frontchest");
  names->Push ("hips");
  names->Push ("lowerarmlenght");
  names->Push ("lowerlegheight");
  names->Push ("napetowaist");
  names->Push ("neckcirc");
  names->Push ("neckheight");
  names->Push ("shoulder");
  names->Push ("thighcirc");
  names->Push ("underbust");
  names->Push ("upperarm");
  names->Push ("upperarmlenght");
  names->Push ("upperlegheight");
  names->Push ("waist");
  names->Push ("waisttohip");
  names->Push ("wrist");
  return csPtr<iStringArray> (names);
}

csPtr<iStringArray> MakehumanManager::GetProperties () const
{
  csRef<iStringArray> names;
  names.AttachNew (new scfStringArray ());
  // TODO: Build this list dynamically from the Makehuman data
  names->Push ("age");
  names->Push ("ethnics");
  names->Push ("gender");
  names->Push ("weight");
  names->Push ("muscle");
  names->Push ("height");
  names->Push ("genitals");
  names->Push ("buttocks");
  names->Push ("stomach");
  names->Push ("breastFirmness");
  names->Push ("breastSize");
  names->Push ("breastPosition");
  names->Push ("breastDistance");
  names->Push ("breastTaper");
  names->Push ("pelvisTone");
  return csPtr<iStringArray> (names);
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
