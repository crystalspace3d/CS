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
#ifndef __MAKEHUMAN_MANAGER_H__
#define __MAKEHUMAN_MANAGER_H__

#include "csgeom/tri.h"
#include "csgfx/renderbuffer.h"
#include "cstool/animeshtools.h"
#include "cstool/rbuflock.h"
#include "csutil/array.h"
#include "csutil/csstring.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/hash.h"
#include "csutil/refarr.h"
#include "csutil/scf_implementation.h"
#include "csutil/stringarray.h"
#include "iengine/engine.h"
#include "imap/services.h"
#include "imesh/animesh.h"
#include "imesh/makehuman.h"
#include "imesh/object.h"
#include "imesh/skeleton2.h"
#include "iutil/comp.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"
#include "ivaria/reporter.h"
#include "ivideo/rndbuf.h"

#include "types.h"

struct iDocumentNode;

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

using namespace CS::Mesh;

class MakeHumanManager : public scfImplementation2<MakeHumanManager,
  CS::Mesh::iMakeHumanManager, iComponent>
{
  friend class MakeHumanCharacter;

public:
  MakeHumanManager (iBase* parent);
  ~MakeHumanManager ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry* objectRegistry);

  //-- iMakeHumanManager
  virtual csPtr<iMakeHumanCharacter> CreateCharacter ();

  virtual csPtr<iStringArray> GetProxies () const;
  virtual csPtr<iStringArray> GetRigs () const;

  virtual csPtr<iStringArray> GetCategories () const;
  virtual csPtr<iStringArray> GetSubCategories (const char* category) const;
  virtual csPtr<iStringArray> GetParameters (const char* category, const char* subCategory) const;
  virtual csPtr<iStringArray> GetParameters (const char* category) const;

  /**
   * Generate an animesh factory for a MakeHuman proxy model, corresponding
   * to a loaded MakeHuman model. By priority order, the proxy will be determined by:
   * - the proxy referenced in MakeHuman model file (mhm),
   * - the proxy name provided as parameter,
   * - the default proxy PROXY_NAME
   * \param proxyName  Name of a MakeHuman proxy (defined in PROXY_PATH folder)
   * \param rigName  Name of a MakeHuman rig file (without extension '.rig')
   * \Return the animesh factory corresponding to a proxy of the loaded human model
   *  if success; NULL otherwise
   */
/*
  virtual csPtr<CS::Mesh::iAnimatedMeshFactory> GenerateModel 
    (const char* factoryName, const char* filename = "",
     const char* proxy = "", const char* rig = "");
*/
  /**
   * Load a clothing item as an animesh factory and adapt it to the loaded model.
   * \param clothingName  Name of a MakeHuman clothing item located 
   *                      in MakeHuman 'clothes' folder
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \Return the animesh factory corresponding to MakeHuman clothing item if succeeded;
   *  NULL otherwise
   *
   * Since this method adapts clothes to a MakeHuman model, such a model should 
   * have been previously loaded with method GenerateModel().
   */
/*
  virtual csPtr<CS::Mesh::iAnimatedMeshFactory> GenerateClothingItem
    (const char* clothingName, const bool doubleSided);
*/
  /**
   * Load all clothes of current MakeHuman model as animesh factories
   * and adapt them to the loaded model.
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \param clothesFacts  Array of generated animesh factories, one for each clothing 
   *                      item of the model (only relevant if the method returns true)
   * \param clothesNames  Array of clothes names corresponding to the entries of 
   *                      clothesFacts
   * \Return true if clothes were successfully generated
   *
   * Since this method loads clothes referenced in a MakeHuman model file, 
   * a MakeHuman model should have been previously loaded with
   * method GenerateModel() before calling this one.
   */
/*
   virtual bool GenerateClothes (const bool doubleSided,
				csRefArray<CS::Mesh::iAnimatedMeshFactory>& clothesFacts,
				csArray<csString>& clothesNames);
*/

private:
  /// Environment variables
  iObjectRegistry* objectRegistry;
  csRef<iEngine> engine;
  csRef<iGraphics3D> g3d;
  csRef<iVFS> vfs;
  csRef<iSyntaxService> synldr;
  csRef<iMeshObjectType> animeshType;

  /// Base model
  csDirtyAccessArray<FaceGroup> faceGroups;

  /// Temporary MakeHuman data (parsed from a MakeHuman object file)
  csDirtyAccessArray<csVector3> coords;     // array of vertex coordinates
  csDirtyAccessArray<csVector2> texcoords;  // array of texture coordinates
  csDirtyAccessArray<csVector3> normals;    // array of vertex normals

  // Cache of target data
  struct TargetBuffer
  {
    csArray<csVector3> offsets;
    csArray<size_t> indices;
  };
  csHash<TargetBuffer, csString> targetBuffers;

  // Description of the categories of parameters
  csHash<MHCategory, csString> categories;
  csStringArray categoriesOrder;
  csHash<MHParameter*, csString> parameters;
  csHash<csStringArray, csString> categoryLabels;
  csStringArray globalPatterns;

  /**************************************************************************/

  /// Utility methods

  bool ReportError (const char* msg, ...) const;
  bool ReportWarning (const char* msg, ...) const;
  bool ReportInfo (const char* msg, ...) const;

  csPtr<iFile> OpenFile (const char* filename, const char* vfsPath);
  bool ParseLine (iFile* file, char* buf, size_t nbytes);
  bool ParseWord (const char* txt, char* buf, size_t& start);

  const MHParameter* FindParameter (const char* category, const char* parameter) const;
  bool FindParameterCategory (const char* parameter, csString& category) const;

  /// MakeHuman plugin configuration rules

  csStringHash xmltokens;
#define CS_TOKEN_ITEM_FILE \
  "plugins/mesh/animesh/makehuman/makehuman.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE

  bool ParseConfigurationRules (const char* filename);
  bool ParseCategory (iDocumentNode* node);
  bool ParseSubCategory (iDocumentNode* node, MHCategory* category);

  /// MakeHuman mesh object parser (.obj)

  /**
   * Parse a MakeHuman object file. 
   * \param filename  Path of a Wavefront OBJ file (with extension '.obj')
   * \param faceGroups  Parsed face groups of MakeHuman object
   * \Return true if the object file was successfully parsed
   */
  bool ParseObjectFile (const char* filename,
			csDirtyAccessArray<csVector3>& coords,
			csDirtyAccessArray<csVector2>& texcoords,
			csDirtyAccessArray<csVector3>& normals,
                        csDirtyAccessArray<FaceGroup>& faceGroups);

  /// MakeHuman morph target parser (.target)

  /**
   * Parse a MakeHuman target file into a buffer defining a displacement 
   * for each CS vertex.
   * \param filename  Path of a MakeHuman target file (i.e. filepath composed of 
   *                  the morph target name and the extension '.target':
   *                  '<vfs_path>/<target_name>.target')
   * \param offsetsBuffer  Offsets buffer of parsed MakeHuman target
   * \Return true if success
   */
  bool ParseMakeHumanTargetFile (Target* target);
  bool ParseMakeHumanTargetFile
    (Target* target, csArray<csVector3>& offsets, csArray<size_t>& indices);
  bool ParseMakeHumanTargetFile
    (const char* filename, csArray<csVector3>& offsets, csArray<size_t>& indices);
  bool ParseMakeHumanTargetFile
    (iFile* file, csArray<csVector3>& offsets, csArray<size_t>& indices);

};

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)

#endif // __MAKEHUMAN_MANAGER_H__
