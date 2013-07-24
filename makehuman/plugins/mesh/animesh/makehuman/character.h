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
#ifndef __MAKEHUMAN_CHARACTER_H__
#define __MAKEHUMAN_CHARACTER_H__

#include "makehuman.h"

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

using namespace CS::Mesh;

class MakehumanCharacter
  : public scfImplementation1<MakehumanCharacter, CS::Mesh::iMakehumanCharacter>
{
public:
  MakehumanCharacter (MakehumanManager* manager);
  ~MakehumanCharacter ();

  //-- iMakehumanCharacter
  virtual iAnimatedMeshFactory* GetMeshFactory () const;
  virtual bool UpdateMeshFactory ();

  virtual void Clear ();
  virtual void SetNeutral ();

  virtual bool Parse (const char* filename);

  virtual void SetProxy (const char* proxy);
  virtual void SetRig (const char* rig);

  virtual void SetMeasure (const char* measure, float value);
  virtual void SetProperty (const char* property, float value);

  virtual void ClearClothes ();
  virtual size_t GetClothCount () const;
  virtual iAnimatedMeshFactory* GetClothMesh (size_t index) const;

private:
  csRef<MakehumanManager> manager;
  csRef<iAnimatedMeshFactory> animeshFactory;

  csString proxy;
  csString rig;

  /// Model variables
  csString modelName;
  MakehumanModel human;            // array of model properties

  /// Backup of Makehuman buffers (used for proxy processing)
  csArray<VertBuf> mappingBuffer;   // index correspondence between mhx and cs vertices 
                                    // of the model
  csDirtyAccessArray<csVector3> basicMesh;  // array of vertex coordinates of 
                                            // neutral Makehuman model
  csDirtyAccessArray<csVector3> morphedMesh;   // array of vertex coordinates of
                                               // morphed Makehuman model
  csArray<csVector3> basicMorph;    // total offsets corresponding to basic properties
                                    // (gender/ethnic/age) of Makehuman model
  csHash<VertBuf, csString> mhJoints;   // list of Makehuman joints used to define
                                        // bone positions (parsed from file 'base.obj')
  csArray<Target> microExpressions; // generated micro-expressions of Makehuman model
  
  /// Temporary Makehuman data (parsed from a Makehuman object file)
  csDirtyAccessArray<csVector3> coords;     // array of vertex coordinates
  csDirtyAccessArray<csVector2> texcoords;  // array of texture coordinates
  csDirtyAccessArray<csVector3> normals;    // array of vertex normals

  /// Temporary Crystal Space mesh buffers (corresponding to parsed Makehuman object)
  csDirtyAccessArray<csVector3> csCoords;     // array of CS vertex coordinates
  csDirtyAccessArray<csVector2> csTexcoords;  // array of CS texture coordinates
  csDirtyAccessArray<csVector3> csNormals;    // array of CS vertex normals

  /// Clothes
  csRefArray<CS::Mesh::iAnimatedMeshFactory> clothes;

  /**************************************************************************/

  /// Utility methods

  bool ReportError (const char* msg, ...) const;
  bool ReportWarning (const char* msg, ...) const;

  void PrintModelProperties (const ModelTargets& modelVals);

  /// MakeHuman model parser (.mhm)

  /**
   * Parse a Makehuman model file and copy the basic property values in 'human'.
   * \param filename  Name of Makehuman model file (with extension '.mhm')
   * \param human  Generated property values of given Makehuman model
   *               (only relevant if the method returns true)
   * \Return true if property values were successfully generated
   */
  bool ParseMakehumanModelFile (const char* filename, MakehumanModel* human);

  /**
   * Generate target names and associated weights from given Makehuman model 
   * properties.
   * \param human  Property values of a Makehuman model
   * \param modelVals  Generated list of target tags and weights for each
   *                   model property (only relevant if the method returns true)
   * \Return true if model properties were successfully generated
   */
  bool ProcessModelProperties (const MakehumanModel human,  ModelTargets* modelVals);

  /**
   * Build target list containing full path of Makehuman target files and their 
   * corresponding weights.
   * \param modelVals  List of target tags and weights for each model property
   * \param targets  Generated list of Makehuman target filenames and their 
   *                 associated weights (only relevant if the method returns true)
   * \Return true if targets were successfully generated
   */
  bool GenerateTargetsWeights (const ModelTargets modelVals, 
                               csArray<Target>* targets);

  /// MakeHuman target parser (.target)

  /**
   * Parse a Makehuman target file into a buffer defining a displacement 
   * for each CS vertex.
   * \param filename  Path of a Makehuman target file (i.e. filepath composed of 
   *                  the morph target name and the extension '.target':
   *                  '<vfs_path>/<target_name>.target')
   * \param offsetsBuffer  Offsets buffer of parsed Makehuman target
   * \Return true if success
   */
  bool ParseMakehumanTargetFile (const char* filename,
                                 csRef<iRenderBuffer>& offsetsBuffer);

  /**
   * Apply morph targets to Makehuman mesh buffer
   * \param targets  Array of Makehuman morph targets
   * \Return true if targets have been successfully applied to model buffer
   *
   * Call ParseObjectFile() on basic Makehuman model file before morphing 
   * the parsed mesh.
   */
  bool ApplyTargetsToModel (const csArray<Target>& targets);

  /// MakeHuman mesh object parser (.obj)

  /**
   * Parse a Makehuman object file. 
   * \param filename  Path of a Wavefront OBJ file (with extension '.obj')
   * \param faceGroups  Parsed face groups of Makehuman object
   * \Return true if the object file was successfully parsed
   */
  //bool ParseObjectFile (const char* filename,
  //                    csDirtyAccessArray<FaceGroup>& faceGroups);

  /**
   * Generate a mapping buffer between Makehuman and Crystal Space vertices,
   * and define Makehuman joints and Crystal Space submeshes.
   * \param faceGroups  Parsed Makehuman face groups
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \param mhJoints  Generated list of Makehuman joints (i.e. face groups used 
   *                  to define bone positions)
   * \param csSubmeshes  Generated Crystal Space submeshes (corresponding 
   *                     to Makehuman submeshes)
   * \param mappingBuf  Generated mapping buffer between Makehuman and 
   *                    Crystal Space vertices
   * \Return true if success
   *
   * Call ParseObjectFile() before generating Crystal Space buffers
   */
  bool GenerateMeshBuffers (const csDirtyAccessArray<FaceGroup>& faceGroups,
                            const bool doubleSided,
                            csHash<VertBuf, csString>& mhJoints,
                            csDirtyAccessArray<Submesh>& csSubmeshes,
                            csArray<VertBuf>& mappingBuf);

  /**
   * Create an animated mesh factory from parsed Makehuman object,
   * i.e. define a mesh object into internal CS data structures, and return it.
   * \param textureFile  VFS path of the image file used as texture for object
   * \param csSubmeshes  Array of Crystal Space submeshes
   * \Return an animesh factory corresponding to parsed Makehuman object
   *
   * Call GenerateMeshBuffers() before creating the Crystal Space animesh
   */
  csPtr<CS::Mesh::iAnimatedMeshFactory> CreateAnimatedMesh 
          (const char* textureFile, const csDirtyAccessArray<Submesh>& csSubmeshes);


  /// MakeHuman rig parser (.rig)

  /**
   * Calculate sum baricenter of a facegroup (i.e. a set of vertices).
   * \param facegroup  Face group composed of Makehuman vertex indices
   * \param center  Calculated position of facegroup baricenter
   * \Return true if baricenter has been successfully calculated
   */
  bool CalculateBaricenter (const VertBuf& facegroup, csVector3& center);

  /**
   * Parse a line defining a new Makehuman joint, evaluate its position
   * by using Makehuman model, and add it to table 'jointPos'.
   * \param line  String defining the position of a Makehuman joint
   * \param mhJoints  Table of joints, composed of Makehuman vertex indices
   *                  (parsed from model object file)
   * \param jointPos  Table of calculated joint positions
   * \Return true if the new joint has been successfully parsed and its
   *  position evaluated
   */
  bool ParseJointDefinition (csString line, 
                             const csHash<VertBuf, csString>& mhJoints,
                             csHash<csVector3, csString>& jointPos);

  /**
   * Parse a Makehuman definition of bone. 
   * \param line  String defining a Makehuman bone
   * \param bones  Table of parsed bone data
   * \param boneOrder  Order of parsed bones
   * \Return true if the new bone has been successfully parsed
   */
  bool ParseBoneDefinition (csString line,
                            csHash<Bone, csString>& bones,
                            csArray<csString>& boneOrder);

  /**
   * Parse a Makehuman bone influence and copy parsed data into table 
   * 'boneWeights'.
   * \param line  String defining the Makehuman bone weight of a vertex
   * \param boneWeights  Table of parsed weights of current bone
   * \Return true if the weight has been successfully parsed and
   *  added to table boneWeights
   */
  bool ParseBoneWeights (csString line, 
                         csArray<VertexInfluence>& boneWeights);

  /**
   * Parse joints and bones defined in a Makehuman rig file.
   * \param filename  Name of Makehuman rig file
   * \param mhJoints  Table of joints, composed of Makehuman vertex indices
   * \param jointPos  Table of parsed joint positions
   * \param bones  Table of parsed bone data
   * \param boneOrder  Order of parsed bones
   * \Return true if rig file has been successfully parsed and
   *  added to tables jointPos, bones and boneWeights
   */
  bool ParseRigFile (const char* filename,
                     const csHash<VertBuf, csString>& mhJoints,
                     csHash<csVector3, csString>& jointPos,
                     csHash<Bone, csString>& bones,
                     csArray<csString>& boneOrder);

  /**
   * Create a skeleton for an animesh factory and define its bones
   * \param skelName  Name of skeleton
   * \param bones  Table of parsed bone data
   * \param boneOrder  Order of parsed bones
   * \param amfact  Animated mesh factory of the model
   * \Return true if the animesh skeleton was successfully created
   */
  bool CreateSkelFact (const char* skelName, 
                       const csHash<Bone, csString>& bones,
                       const csArray<csString>& boneOrder,
                       CS::Mesh::iAnimatedMeshFactory* amfact);

  /**
   * Set the bone positions of an animated mesh factory
   * \param mhJoints  Table of rig joints (i.e. Makehuman vertex indices)
   * \param jointPos  Table of parsed joint positions
   * \param bones  Table of parsed bone data
   * \param moveOrigin  Indicates if model's origin should be moved to have 
   *                    its feets centered on world coordinate system's origin
   * \param amfact  Animated mesh factory of the model
   * \Return true if bone positions have been correctly set on the 
   *  given animesh factory
   *
   * A skeleton and its bones should have been previously created by
   * calling CreateSkelFact() on the animesh factory.
   */
  bool SetBoneLocations (const csHash<VertBuf, csString>& mhJoints,
                         const csHash<csVector3, csString>& jointPos,
                         const csHash<Bone, csString>& bones,
                         const bool moveOrigin,
                         CS::Mesh::iAnimatedMeshFactory* amfact);

  /**
   * Set the bone influences of an animated mesh factory.
   * \param boneWeights  Table of bone weights
   * \param amfact  Animated mesh factory of the model
   * \Return true if bone influences have been correctly set on the 
   *  animesh factory
   *
   * A skeleton and its bones should have been previously created by
   * calling CreateSkelFact() on the animesh factory.
   */
  bool SetBoneInfluences (const csHash<Bone, csString>& bones,
                          CS::Mesh::iAnimatedMeshFactory* amfact);

  /**
   * Set the bone influences of an animated mesh factory.
   * \param boneWeights  Table of bone weights
   * \param proxy  Parsed data of Makehuman proxy model
   * \Return true if bone influences have been correctly set on the 
   *  animesh factory of proxy model
   *
   * The proxy model must have been generated with CreateProxyMesh();
   * ans its skeleton and bones should have been previously created by calling
   * CreateSkelFact() on the animesh factory associated with proxy model.
   */
  bool SetBoneInfluences (const csHash<Bone, csString>& bones,
                          ProxyData* proxy);

  /**
   * Create skeleton of the morphed human model, according to Makehuman standards.
   * If a proxy model is provided, the skeleton is added to its animesh factory;
   * otherwise, it is added to the fully detailed model.
   * \param modelName  Name of human model
   * \param rigName  Name of model rig (without extension '.rig')
   * \param mhJoints  Table of joints, composed of Makehuman vertex indices
   * \param proxy  Parsed data of Makehuman proxy model
   * \Return true if model rig has been successfully created
   *
   * The animesh factory of Makehuman model must have been created previously
   * with CreateAnimatedMesh().
   */
  bool CreateSkeleton (const char* modelName,
                       const char* rigName,
                       const csHash<VertBuf, csString>& mhJoints,
                       ProxyData* proxy = nullptr);


  /// MakeHuman expression parser (.target)

  /**
   * Create morph targets for Makehuman facial macro-expressions.
   * The expression names are generated by parsing Makehuman expression
   * files from subfolders of '/lib/makehuman/data/targets/expression/'.
   * The expression names are prefixed by 'macro_'.
   * Notice that these macro-expressions are only adapted to 
   * 100% Caucasian models.
   * \param modelVals  Parsed property values of Makehuman model
   * \param macroExpr  Array of parsed Makehuman expression targets
   * \Return true if expressions have been successfully parsed
   *
   * These macro-expressions are deprecated in new Makehuman releases
   * (last compatible version: Makehuman 1.0 alpha 7).
   */
  bool GenerateMacroExpressions (const ModelTargets& modelVals,
                                 csArray<Target>& macroExpr);

  /**
   * Parse Makehuman landmarks, used to generate micro-expressions
   * \param bodypart  Part of human body corresponding to the name of
   *                  a Makehuman landmarks file (without extension '.lmk')
   * \param landmarks  Array of parsed landmarks (i.e. vertex indices)
   * \Return true if landmarks have been successfully parsed
   */
  bool ParseLandmarks (const char* bodypart, csArray<size_t>& landmarks);

  /**
   * Calculate offsets of a Makehuman micro-expression by adapting them
   * to human model.
   */
  bool WarpMicroExpression (const csArray<csVector3>& xverts,
                            double s2[], double w[],
                            Target& expr);

  /**
   * Parse all micro-expressions defined in Makehuman subfolders of 
   * 'makehuman/data/targets/expression/units/' and cumulate them 
   * into a resulting offset buffer per micro-expression. 
   * The choice of folders and weights is done by using basic model 
   * properties (ethnics/gender/age).
   * \param modelVals  Property values of Makehuman model
   * \param microExpr  Array of parsed Makehuman expression targets
   * \Return true if all micro-expressions offsets have been parsed successfully
   */
  bool ParseMicroExpressions (const ModelTargets& modelVals,
                              csArray<Target>& microExpr);

  /**
   * Generate morph targets for Makehuman facial micro-expressions.
   * The expression names are generated by using the parsed Makehuman
   * expression files in subfolders of 'makehuman/data/targets/expression/units/'.
   * \param modelVals  Property values of Makehuman model
   * \param microExpr  Array of parsed Makehuman expression targets
   * \Return true if expressions have been successfully generated
   */
  bool GenerateMicroExpressions (const ModelTargets& modelVals,
                                 csArray<Target>& microExpr);

  /**
   * Convert Makehuman expression targets and add them to an animesh factory.
   * \param amfact  Animated mesh factory of the model
   * \param mapBuf
   * \param prefix  Prefix used to name the new expressions
   * \param mhExpressions  Array of Makehuman expression targets
   * \Return true if expressions have been successfully added to model factory
   */
  bool AddExpressionsToModel (CS::Mesh::iAnimatedMeshFactory* amfact,
                              const csArray<VertBuf>& mapBuf,
                              const char* prefix,
                              const csArray<Target>& mhExpressions);


  /// MakeHuman proxy parser (.proxy and .mhclo)

  /**
   * Parse a proxy scale component (along X, Y or Z axis) and save it into 
   * proxy data structure.
   * \param words  Array of strings to parse
   * \param proxy  Parsed data of proxy object
   * \Return true if scale component has been successfully parsed
   */
  bool ParseProxyScale (csStringArray& words, ProxyData& proxy);

  /**
   * Parse a proxy vertex, i.e. data used to adapt the position of a proxy
   * vertex to an underlying human mesh.
   * \param words  Array of strings to parse
   * \param proxy  Parsed data of proxy object
   * \Return true if proxy vert has been successfully parsed
   */
  bool ParseProxyVert (csStringArray& words, ProxyData& proxy);

  /**
   * Parse a proxy object from a Makehuman proxy file. Such a proxy defines
   * how a mesh object should be adapted to a human model.
   * \param proxyFile  Path of a Makehuman proxy file (extensions .proxy or .mhclo)
   * \param proxy  Parsed data of Makehuman proxy
   * \Return true if proxy file has been successfully parsed
   */
  bool ParseProxyFile (const char* proxyFile, ProxyData& proxy);

  /**
   * Adapt a neutral proxy object to loaded human model.
   * \param proxy  Parsed data of Makehuman proxy
   * \Return true if proxy has been successfully adjusted to the 
   *  model factory
   *
   * Since this method adapts a proxy to a human model, a model should 
   * have been previously created with method GenerateModel() before calling 
   * this one.
   */
  bool AdaptProxyToModel (ProxyData& proxy);

  /**
   * Create a proxy object by parsing a Makehuman proxy file, adapt it to
   * loaded human model and generate the corresponding animesh factory.
   * \param proxyName  Name of a Makehuman proxy
   * \param proxyFile  Path of a Makehuman proxy file (extensions .proxy or .mhclo)
   * \param texture  Texture used on generated animesh factory (with extension .png); 
   *                 if it is not defined (nullptr), the texture referenced in parsed
   *                 proxy data is used
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \param proxy  Generated data of proxy object
   * \Return true if an animated mesh factory corresponding to Makehuman proxy object
   *  has been successfully generated; nullptr otherwise
   */
  csPtr<CS::Mesh::iAnimatedMeshFactory> CreateProxyMesh 
    (const char* proxyName, const char* proxyFile,
     const char* texture, const bool doubleSided, ProxyData& proxy);

  /**
   * Generate expression targets for a Makehuman proxy model.
   * \param proxy  Parsed Makehuman proxy model
   * \param modelExpr  Array of expression targets for Makehuman model
   * \param proxyExpr  Generated expression targets for Makehuman proxy model
   * \Return true if expressions have been correctly generated for human
   *  proxy model
   *
   * Since this method adapts the expression of a human model to a proxy model, 
   * the model should have been previously created with method GenerateModel() 
   * before calling this one.
   */
  bool GenerateProxyMicroExpressions (const ProxyData& proxy, 
                                      const csArray<Target>& modelExpr,
                                      csArray<Target>& proxyExpr);

  csPtr<CS::Mesh::iAnimatedMeshFactory> GenerateCloth (const char* clothingName);
  bool GenerateClothes ();

};

}
CS_PLUGIN_NAMESPACE_END (Makehuman)

#endif // __MAKEHUMAN_CHARACTER_H__
