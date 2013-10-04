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
#include "imesh/animesh.h"
#include "imesh/makehuman.h"
#include "imesh/object.h"
#include "imesh/skeleton2.h"
#include "iutil/comp.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"
#include "ivaria/reporter.h"
#include "ivideo/rndbuf.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

using namespace CS::Mesh;

/*******************************************************************
 ******** Data structures used by MakeHumanManager class ************
 *******************************************************************/

/// FILE NAMES AND VFS PATHS FOR MAKEHUMAN DATA

// TODO: cfg options instead of macros

// name of a saved MakeHuman model in MODELS_PATH folder
#define MODEL_NAME          "test"
// Default skin
//#define DEFAULT_SKIN "brown-female-young-2-bald.png"
#define DEFAULT_SKIN "texture.png"
// default MakeHuman rig file (here we use the 'simple' rig)
#define DEFAULT_RIG "simple"
// name of default proxy model
#define PROXY_NAME          "rorkimaru"
// default MakeHuman rig file used for proxy models
#define PROXY_RIG_NAME      "rigid"
// path of the basic MakeHuman model file (3D mesh object)
#define MESH_DATA_FILE      "/lib/makehuman/data/3dobjs/base.obj"
// path of the MakeHuman folders
#define RIG_PATH            "/lib/makehuman/data/rigs/"
//#define SKIN_PATH           "/lib/makehuman/data/skins/"
#define SKIN_PATH           "/lib/makehuman/data/textures/"
#define TEXTURE_PATH        "/lib/makehuman/data/textures/"
#define PROXY_PATH          "/lib/makehuman/data/proxymeshes/"
#define TARGETS_PATH        "/lib/makehuman/data/targets/"
#define EXPRESSIONS_PATH    "/lib/makehuman/data/targets/expression/"
#define LANDMARKS_PATH      "/lib/makehuman/data/landmarks/"
#define CLOTHES_MH_PATH     "/lib/makehuman/data/clothes/"

/// DATA STRUCTURES USED TO PARSE MAKEHUMAN MODELS

struct VertBuf
{
  csArray<size_t> vertices;  // array of vertex indices
};

struct MappingVertex
{
  size_t   csIdx;     // index of Crystal Space vertex
  size_t   uvIdx;     // index of MakeHuman uv coordinates
  csString material;  // material name

  MappingVertex (size_t cs, size_t uv, csString mat)
  : csIdx (cs), uvIdx (uv), material (mat)
  {}
};

struct FaceGroup
{
  csString groupName;     // name of MakeHuman facegroup (i.e. a submesh or a joint)
  csString materialName;  // name of the material
  size_t faceCount;       // number of faces in the facegroup
  csDirtyAccessArray<size_t> vIndices;   // array of vertex indices
  csDirtyAccessArray<size_t> uvIndices;  // array of UV indices
  csDirtyAccessArray<size_t> vnIndices;  // array of normal indices

  FaceGroup (const char* name)
  : groupName (name), faceCount (0)
  {}
};

struct Submesh
{
  csString subName;       // name of submesh
  csString materialName;  // name of submesh material
  size_t faceCount;       // number of faces in the submesh
  csDirtyAccessArray<csTriangle> triangles;  // array of triangles composing the submesh

  Submesh (const char* name, const char* mat)
  : subName (name), materialName (mat), faceCount (0)
  {}
};

struct BoneInfluences
{
  csArray<CS::Animation::BoneID> bones;  // bones influencing a vertex
  csArray<float> influences;             // weights associated with the bones
};

struct VertexInfluence
{
  size_t vertex;   // index of a vertex influenced by a bone
  float weight;    // weight associated with this vertex

  VertexInfluence (size_t v, float w)
  : vertex (v), weight (w)
  {}
};

struct Bone
{
  csString parent; // name of parent bone
  csString head;   // name of head joint
  csString tail;   // name of tail joint
  float roll;      // roll angle of the bone (in radians)
  csArray<VertexInfluence> weights;  // all vertices influenced by this bone 
                                     // and their weights

  Bone (csString parentName, csString headName, csString tailName, float rollAngle)
  : parent (parentName), head (headName), tail (tailName), roll (rollAngle)
  {}
};

/// DATA STRUCTURES FOR MAKEHUMAN MODEL PROPERTIES

// Basic human model properties
struct MakeHumanModel
{
  // Model properties:
  csHash<float, csString> properties;
  // Measures:
  csHash<float, csString> measures;
  // Texture (full vfs path):
  csString skinFile;
  // Model proxy file (full vfs path):
  csString proxyFilename;
  // List of clothes:
  csArray<csString> clothesNames;

  MakeHumanModel (/*const char* modelName*/);

  void Reset ();
  void SetNeutral ();
};

/// DATA STRUCTURES FOR MAKEHUMAN MORPH TARGETS

// Morph target data
struct Target
{
  csString name;     // name of the morph target
  csString path;     // path of the morph target file
  float weight;      // weight associated to the morph target
  csArray<csVector3> offsets;  // offsets buffer of morph target
  csArray<size_t> indices;  // indices of the vertices that are morphed

  Target ()
  : weight (0.0)
  {}

  Target (const char* n)
  : name (n), weight (1.0)
  {}

  Target (const char* n, const char* p, float val)
  : name (n), path (p), weight (val)
  {}
};

// Generated human model properties
// (in terms of morph targets, i.e. name tags and weights)
struct ModelTargets {
  csArray<Target> gender;
  csArray<Target> age;
  csArray<Target> ethnics;
  csArray<Target> weight;
  csArray<Target> muscle;
  csArray<Target> height;
  csArray<Target> genitals;
  csArray<Target> buttocks;
  csArray<Target> stomach;
  csArray<Target> breastFirmness;
  csArray<Target> breastSize;
  csArray<Target> breastPosition;
  csArray<Target> breastDistance;
  csArray<Target> breastTaper;
  csArray<Target> pelvisTone;
  csArray<Target> measures;

  void DeleteAll ()
  {
    gender.DeleteAll ();
    age.DeleteAll ();
    ethnics.DeleteAll ();
    weight.DeleteAll ();
    muscle.DeleteAll ();
    height.DeleteAll ();
    genitals.DeleteAll ();
    buttocks.DeleteAll ();
    stomach.DeleteAll ();
    breastFirmness.DeleteAll ();
    breastSize.DeleteAll ();
    breastPosition.DeleteAll ();
    breastDistance.DeleteAll ();
    breastTaper.DeleteAll ();
    pelvisTone.DeleteAll ();
    measures.DeleteAll ();    
  }
};

/// DATA STRUCTURES FOR MAKEHUMAN PROXIES (i.e. MODELS, RIGS, CLOTHES)

// Vertex data of a proxy
struct ProxyVert
{
  int v[3];     // vertex indices
  float w[3];   // weights
  float d[3];   // offsets

  ProxyVert ()
  {
    v[0] = v[1] = v[2] = -1;
    w[0] = w[1] = w[2] = 0.0f;
    d[0] = d[1] = d[2] = 0.0f;
  }
};

// Scale data of a proxy
struct ProxyScale
{
  int v1, v2;  // vertex indices
  float den;   // denominator

  ProxyScale ()
  : v1(-1), v2(-1), den(0.0f)
  {}
};

// Data of a proxy object
struct ProxyData
{
  // MakeHuman name of the proxy object 
  csString name;
  // animated mesh factory of proxy object
  csRef<CS::Mesh::iAnimatedMeshFactory> factory;
  // correspondence between MH and CS vertices of the proxy
  csArray<VertBuf> mappingBuffer;
  // object filename
  csString objFile;
  // texture filename
  csString texFile;
  // mask filename
  csString maskFile;
  // proxy scale
  ProxyScale proxyScale[3];
  // proxy buffers
  csArray<ProxyVert> proxyVerts;
  csDirtyAccessArray<csVector2> proxyUVs;
  csDirtyAccessArray<csTriangle> proxyFaces;

  ProxyData (const char* pName)
  : name (pName), factory (nullptr)
  {}

  ~ProxyData ()
  {
    mappingBuffer.DeleteAll();
    proxyVerts.DeleteAll();
  }
};

/************************************************************************
 *********************** MakeHumanManager class **********************
 ************************************************************************/

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
  virtual csPtr<iStringArray> GetMeasures () const;
  virtual csPtr<iStringArray> GetProperties () const;

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

  csRef<iMeshObjectType> animeshType;

  /// Base model
  csDirtyAccessArray<FaceGroup> faceGroups;

  /// Temporary MakeHuman data (parsed from a MakeHuman object file)
  csDirtyAccessArray<csVector3> coords;     // array of vertex coordinates
  csDirtyAccessArray<csVector2> texcoords;  // array of texture coordinates
  csDirtyAccessArray<csVector3> normals;    // array of vertex normals

  struct TargetBuffer
  {
    csArray<csVector3> offsets;
    csArray<size_t> indices;
  };
  csHash<TargetBuffer, csString> targetBuffers;

  /**************************************************************************/

  /// Utility methods

  bool ReportError (const char* msg, ...) const;
  bool ReportWarning (const char* msg, ...) const;

  csPtr<iFile> OpenFile (const char* filename, const char* vfsPath);
  bool ParseLine (iFile* file, char* buf, size_t nbytes);
  bool ParseWord (const char* txt, char* buf, size_t& start);

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
  bool ParseMakeHumanTargetFile
    (const char* filename, csArray<csVector3>& offsets, csArray<size_t>& indices);

};

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)

#endif // __MAKEHUMAN_MANAGER_H__
