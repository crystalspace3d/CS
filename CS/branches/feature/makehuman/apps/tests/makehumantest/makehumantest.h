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
#ifndef __MAKEHUMANTEST_H__
#define __MAKEHUMANTEST_H__

#include "cstool/demoapplication.h"
#include "imesh/animesh.h"
#include "imesh/animnode/debug.h"
#include "imesh/makehuman.h"

class MakehumanTest : public CS::Utility::DemoApplication
{
private:

  /// Debug nodes
  csRef<CS::Animation::iSkeletonDebugNode> debugNode;
  csRef<CS::Animation::iSkeletonDebugNodeFactory> debugNodeFactory;

  /// Model variables
  csRef<CS::Mesh::iMakehumanManager> makehumanManager;     // Makehuman generator
  csRef<CS::Mesh::iMakehumanCharacter> character;
  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory;  // model factory
  csRef<CS::Mesh::iAnimatedMesh> animesh;                // model mesh
  csRefArray<CS::Mesh::iAnimatedMeshFactory> clothesFactories;  // clothes factories
  csRefArray<CS::Mesh::iAnimatedMesh> clothes;                  // clothes meshes

  void ResetScene ();

  /**
   * Load a Makehuman model as a CS animated mesh factory
   * (with adapted expressions and skeleton) and place it in current scene.
   * \param modelName  Name of a Makehuman model file (without extension '.mhm')
   *                   located in Makehuman 'models' folder
   * \param proxyName  Name of a Makehuman proxy (without extension '.proxy')
   *                   located in Makehuman 'data/proxymeshes' folder
   * \param rigName  Name of a Makehuman rig file (without extension '.rig')
   *                   located in Makehuman 'data/rigs' folder
   */
  bool CreateModel (const char* factoryName, const char* filename = "",
		    const char* proxy = "", const char* rig = "");

  bool CreateCustomModel ();
  bool SetupAnimatedMesh ();

  void TestTargetAccess (const char* property, bool testOffsets = false);

  /**
   * Load clothes of current Makehuman model as animated mesh factories,
   * adapt them to loaded model and correctly place them in the scene,
   * on human model.
   *
   * Since this method loads clothes referenced in a Makehuman 
   * model file, a Makehuman model should have been previously loaded
   * with methods CreateModel() or CreateProxyModel().
   * \param doubleSided  Indicates if generated meshes should be double sided
   */
  bool CreateClothes ();

  /**
   * Load a clothing item as an animated mesh factory,
   * adapt it to loaded model and correctly place it in the scene,
   * on human model.
   * \param clothingName  Name of a Makehuman clothing item (without extension)
   *                      located in Makehuman 'data/clothes' folder
   * \param doubleSided  Indicates if generated mesh should be double sided
   */
  bool CreateClothingItem (const char* clothingName);

  void TestMacroExpression ();
  void TestMicroExpression1 ();
  void TestMicroExpression2 ();
  void TestMicroExpression3 ();

  void SaveSprite (const char* filename);

public:
  MakehumanTest ();
  ~MakehumanTest ();

  //-- csApplicationFramework
  bool OnInitialize (int argc, char* argv[]);
  bool Application ();

  //-- CS::Utility::DemoApplication
  void Frame ();
  bool OnKeyboard (iEvent &event);
  bool CreateRoom ();
};

#endif // __MAKEHUMANTEST_H__
