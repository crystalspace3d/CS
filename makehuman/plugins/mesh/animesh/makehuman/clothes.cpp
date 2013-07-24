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
 * MakeHuman clothes
 *-------------------------------------------------------------------------*/

csPtr<CS::Mesh::iAnimatedMeshFactory> MakehumanCharacter::GenerateCloth
 (const char* clothingName)
{
  // Get full path of clothing proxy file
  csString proxyPath (CLOTHES_MH_PATH);
  proxyPath.Append (clothingName).Append ("/");
  csString proxyFile (proxyPath);
  proxyFile.Append (clothingName).Append (".mhclo");

  // Create a new proxy
  printf ("\nCreating clothing proxy '%s'\n", clothingName);
  ProxyData* cloItem = new ProxyData (clothingName);

  // Create animesh factory from model proxy
  // TODO: this won't work if a proxy has been defined for the character
  cloItem->factory = CreateProxyMesh (clothingName, proxyFile.GetData (),
                                      nullptr, true, *cloItem);
  if (!cloItem->factory)
  {
    ReportError ("Creating animesh factory from Makehuman clothing proxy '%s' KO!",
                 proxyFile.GetData ());
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  return cloItem->factory;
}

bool MakehumanCharacter::GenerateClothes ()
{
  // Clear the cloth array
  clothes.DeleteAll ();

  // Treat all clothes referenced in the loaded Makehuman model
  for (size_t index = 0; index < human.clothesNames.GetSize (); index++)
  {
    // Load and adapt clothing item to the human model
    csString cloname = human.clothesNames[index];
    csRef<CS::Mesh::iAnimatedMeshFactory> cloth = GenerateCloth (cloname);
    if (cloth) clothes.Push (cloth);
  }

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
