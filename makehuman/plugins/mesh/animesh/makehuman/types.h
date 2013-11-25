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
#ifndef __MAKEHUMAN_TYPES_H__
#define __MAKEHUMAN_TYPES_H__

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

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
// path to the basic MakeHuman model file (3D mesh object)
#define MESH_DATA_FILE      "/lib/makehuman/data/3dobjs/base.obj"
// path to the configuration rules file
#define CONFIGURATION_RULES_FILE "/lib/makehuman/rules.xml"
//#define CONFIGURATION_RULES_FILE "/lib/makehuman/rules_pure_python.xml"
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

struct MHParameter
{
  //csString name;
  csString pattern;
  csString left;
  csString right;
  float neutral;
  //float min, max;
  //float scale;

  MHParameter (const char* pattern, const char* left, const char* right)
  : pattern (pattern), left (left), right (right), neutral (0.0f)
    {}
};

struct MHSubCategory
{
  csString name;
  csString pattern;
  csStringArray parameters;
  //float min, max;

  MHSubCategory (const char* name, const char* pattern)
  : name (name), pattern (pattern) {}
};

struct MHCategory
{
  csArray<MHSubCategory> subCategories;
  csHash<MHParameter, csString> parameters;

  void AddSubCategory (const char* subCategory, const char* pattern);
  void AddParameter (const char* name, const char* left, const char* right); 
  void AddParameter (const char* name, const char* pattern, const char* left, const char* right);
  void AddParameter (const char* left, const char* right);
};

/// DATA STRUCTURES FOR MAKEHUMAN MORPH TARGETS

// Morph target data
struct Target
{
  csString name;     // name of the morph target
  csString path;     // path to the morph target file
  float weight;      // weight associated to the morph target
  csArray<csVector3> offsets;  // offsets buffer of morph target
  csArray<size_t> indices;  // indices of the vertices that are morphed

  Target ()
  : weight (1.0f)
  {}

  Target (const char* name)
  : name (name), weight (1.0f)
  {}

  Target (const char* name, float weight)
  : name (name), weight (weight)
  {}

  Target (const char* name, const char* path, float weight)
  : name (name), path (path), weight (weight)
  {}
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

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)

#endif // __MAKEHUMAN_TYPES_H__
