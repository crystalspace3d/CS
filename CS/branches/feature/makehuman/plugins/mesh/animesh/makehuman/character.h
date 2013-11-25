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

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

using namespace CS::Mesh;

class MakeHumanCharacter
  : public scfImplementation1<MakeHumanCharacter, CS::Mesh::iMakeHumanCharacter>
{
public:
  MakeHumanCharacter (MakeHumanManager* manager);
  ~MakeHumanCharacter ();

  //-- iMakeHumanCharacter
  virtual csString Description () const;

  virtual void SetExpressionGeneration (bool generate);
  virtual bool GetExpressionGeneration () const;

  virtual void SetSkeletonGeneration (bool generate);
  virtual bool GetSkeletonGeneration () const;

  virtual void SetUpdateMode (MakeHumanUpdateMode mode);
  virtual MakeHumanUpdateMode GetUpdateMode () const;

  virtual void Clear ();
  virtual void SetNeutral ();

  virtual bool Save (const char* filename) const;
  virtual bool Load (const char* filename);

  virtual void SetProxy (const char* proxy);
  virtual void SetRig (const char* rig);

  virtual iAnimatedMeshFactory* GetMeshFactory () const;
  virtual bool UpdateMeshFactory ();

  virtual void SetParameter (const char* category, const char* parameter, float value);
  virtual void SetParameterInternal (const char* category, const char* parameter, float value);
  virtual float GetParameter (const char* category, const char* parameter) const;

  virtual bool GetParameterTargets
    (const char* category, const char* parameter, csRefArray<iMakeHumanMorphTarget>& targets);

  virtual void ClearClothes ();
  virtual size_t GetClothCount () const;
  virtual iAnimatedMeshFactory* GetClothMesh (size_t index) const;

private:
  void ApplyTargets (csRefArray<iMakeHumanMorphTarget>& targets, bool boundary, float delta);

private:
  csRef<MakeHumanManager> manager;
  csRef<iAnimatedMeshFactory> animeshFactory;

  csString proxy;
  csString rig;

  bool generateExpressions;
  bool generateSkeleton;
  MakeHumanUpdateMode updateMode;

  csHash<float, csString> parameters;
  float africanValue, asianValue;
  float breastSizeValue;

  // Texture (full vfs path):
  csString skinFile;
  // Model proxy file (full vfs path):
  csString proxyFilename;
  // List of clothes:
  csArray<csString> clothesNames;

  /// Model variables
  csString modelName;

  /// Backup of MakeHuman buffers (used for proxy processing)
  csArray<VertBuf> mappingBuffer;   // index correspondence between mhx and cs vertices 
                                    // of the model
  csDirtyAccessArray<csVector3> basicMesh;  // array of vertex coordinates of 
                                            // neutral MakeHuman model
  csDirtyAccessArray<csVector3> morphedMesh;   // array of vertex coordinates of
                                               // morphed MakeHuman model
  csArray<csVector3> basicMorph;    // total offsets corresponding to basic properties
                                    // (gender/ethnic/age) of MakeHuman model
  csHash<VertBuf, csString> mhJoints;   // list of MakeHuman joints used to define
                                        // bone positions (parsed from file 'base.obj')
  csArray<Target> microExpressions; // generated micro-expressions of MakeHuman model
  
  /// Temporary MakeHuman data (parsed from a MakeHuman object file)
  csDirtyAccessArray<csVector3> coords;     // array of vertex coordinates
  csDirtyAccessArray<csVector2> texcoords;  // array of texture coordinates
  csDirtyAccessArray<csVector3> normals;    // array of vertex normals

  /// Temporary Crystal Space mesh buffers (corresponding to parsed MakeHuman object)
  csDirtyAccessArray<csVector3> csCoords;     // array of CS vertex coordinates
  csDirtyAccessArray<csVector2> csTexcoords;  // array of CS texture coordinates
  csDirtyAccessArray<csVector3> csNormals;    // array of CS vertex normals

  /// Clothes
  csRefArray<CS::Mesh::iAnimatedMeshFactory> clothes;

  /**************************************************************************/

  /// Utility methods

  bool ReportError (const char* msg, ...) const;
  bool ReportWarning (const char* msg, ...) const;

  // Expansion of target lists
  void ExpandGlobalTargets (csArray<Target>& targets) const;
  void ExpandParameterTargets (csArray<Target>& targets, const char* parameter) const;
  void ExpandTargets (csArray<Target>& targets) const;
  void ExpandTargets (csArray<Target>& targets, const char* pattern) const;
  void ExpandTargets (csArray<Target>& targets, const char* _pattern, const csStringArray& values) const;
  void ExpandTargets (csArray<Target>& targets, const char* parameter, float weight) const;
  void ExpandTargets (csArray<Target>& targets, const MHParameter* parameterData, const char* parameter, float weight) const;

  /**
   * Convert a list of internal 'Target' data structures into the public interface
   * iMakeHumanMorphTarget.
   */
  void ConvertTargets (csRefArray<iMakeHumanMorphTarget>& targets,
		       csArray<Target>& localTargets,
		       float scale,
		       MakeHumanMorphTargetDirection direction);

  /// MakeHuman morph target management

  /**
   * Apply morph targets to MakeHuman mesh buffer
   * \param targets  Array of MakeHuman morph targets
   * \Return true if targets have been successfully applied to model buffer
   *
   * Call ParseObjectFile() on basic MakeHuman model file before morphing 
   * the parsed mesh.
   */
  bool ApplyTargetsToModel (const csArray<Target>& targets);

  /// MakeHuman mesh object parser (.obj)

  /**
   * Generate a mapping buffer between MakeHuman and Crystal Space vertices,
   * and define MakeHuman joints and Crystal Space submeshes.
   * \param faceGroups  Parsed MakeHuman face groups
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \param mhJoints  Generated list of MakeHuman joints (i.e. face groups used 
   *                  to define bone positions)
   * \param csSubmeshes  Generated Crystal Space submeshes (corresponding 
   *                     to MakeHuman submeshes)
   * \param mappingBuf  Generated mapping buffer between MakeHuman and 
   *                    Crystal Space vertices
   * \Return true if success
   *
   * Call ParseObjectFile() before generating Crystal Space buffers
   */
  bool GenerateMeshBuffers (csDirtyAccessArray<csVector3>& coords,
			    csDirtyAccessArray<csVector2>& texcoords,
			    csDirtyAccessArray<csVector3>& normals,
			    const csDirtyAccessArray<FaceGroup>& faceGroups,
                            const bool doubleSided,
                            csHash<VertBuf, csString>& mhJoints,
                            csDirtyAccessArray<Submesh>& csSubmeshes,
                            csArray<VertBuf>& mappingBuf);

  /**
   * Create an animated mesh factory from parsed MakeHuman object,
   * i.e. define a mesh object into internal CS data structures, and return it.
   * \param textureFile  VFS path of the image file used as texture for object
   * \param csSubmeshes  Array of Crystal Space submeshes
   * \Return an animesh factory corresponding to parsed MakeHuman object
   *
   * Call GenerateMeshBuffers() before creating the Crystal Space animesh
   */
  csPtr<CS::Mesh::iAnimatedMeshFactory> CreateAnimatedMesh 
          (csDirtyAccessArray<csVector3>& coords,
	   csDirtyAccessArray<csVector2>& texcoords,
	   csDirtyAccessArray<csVector3>& normals,
	   const csDirtyAccessArray<Submesh>& csSubmeshes,
	   const char* textureFile);


  /// MakeHuman rig parser (.rig)

  /**
   * Calculate sum baricenter of a facegroup (i.e. a set of vertices).
   * \param facegroup  Face group composed of MakeHuman vertex indices
   * \param center  Calculated position of facegroup baricenter
   * \Return true if baricenter has been successfully calculated
   */
  bool CalculateBaricenter (const VertBuf& facegroup, csVector3& center);

  /**
   * Parse a line defining a new MakeHuman joint, evaluate its position
   * by using MakeHuman model, and add it to table 'jointPos'.
   * \param line  String defining the position of a MakeHuman joint
   * \param mhJoints  Table of joints, composed of MakeHuman vertex indices
   *                  (parsed from model object file)
   * \param jointPos  Table of calculated joint positions
   * \Return true if the new joint has been successfully parsed and its
   *  position evaluated
   */
  bool ParseJointDefinition (csString line, 
                             const csHash<VertBuf, csString>& mhJoints,
                             csHash<csVector3, csString>& jointPos);

  /**
   * Parse a MakeHuman definition of bone. 
   * \param line  String defining a MakeHuman bone
   * \param bones  Table of parsed bone data
   * \param boneOrder  Order of parsed bones
   * \Return true if the new bone has been successfully parsed
   */
  bool ParseBoneDefinition (csString line,
                            csHash<Bone, csString>& bones,
                            csArray<csString>& boneOrder);

  /**
   * Parse a MakeHuman bone influence and copy parsed data into table 
   * 'boneWeights'.
   * \param line  String defining the MakeHuman bone weight of a vertex
   * \param boneWeights  Table of parsed weights of current bone
   * \Return true if the weight has been successfully parsed and
   *  added to table boneWeights
   */
  bool ParseBoneWeights (csString line, 
                         csArray<VertexInfluence>& boneWeights);

  /**
   * Parse joints and bones defined in a MakeHuman rig file.
   * \param filename  Name of MakeHuman rig file
   * \param mhJoints  Table of joints, composed of MakeHuman vertex indices
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
   * \param mhJoints  Table of rig joints (i.e. MakeHuman vertex indices)
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
   * \param proxy  Parsed data of MakeHuman proxy model
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
   * Create skeleton of the morphed human model, according to MakeHuman standards.
   * If a proxy model is provided, the skeleton is added to its animesh factory;
   * otherwise, it is added to the fully detailed model.
   * \param modelName  Name of human model
   * \param rigName  Name of model rig (without extension '.rig')
   * \param mhJoints  Table of joints, composed of MakeHuman vertex indices
   * \param proxy  Parsed data of MakeHuman proxy model
   * \Return true if model rig has been successfully created
   *
   * The animesh factory of MakeHuman model must have been created previously
   * with CreateAnimatedMesh().
   */
  bool CreateSkeleton (const char* modelName,
                       const char* rigName,
                       const csHash<VertBuf, csString>& mhJoints,
                       ProxyData* proxy = nullptr);


  /// MakeHuman expression parser (.target)

  /**
   * Create morph targets for MakeHuman facial macro-expressions.
   * The expression names are generated by parsing MakeHuman expression
   * files from subfolders of '/lib/makehuman/data/targets/expression/'.
   * The expression names are prefixed by 'macro_'.
   * Notice that these macro-expressions are only adapted to 
   * 100% Caucasian models.
   * \param macroExpr  Array of parsed MakeHuman expression targets
   * \Return true if expressions have been successfully parsed
   *
   * These macro-expressions are deprecated in new MakeHuman releases
   * (last compatible version: MakeHuman 1.0 alpha 7).
   */
  bool GenerateMacroExpressions (csArray<Target>& macroExpr);

  /**
   * Parse MakeHuman landmarks, used to generate micro-expressions
   * \param bodypart  Part of human body corresponding to the name of
   *                  a MakeHuman landmarks file (without extension '.lmk')
   * \param landmarks  Array of parsed landmarks (i.e. vertex indices)
   * \Return true if landmarks have been successfully parsed
   */
  bool ParseLandmarks (const char* bodypart, csArray<size_t>& landmarks);

  /**
   * Calculate offsets of a MakeHuman micro-expression by adapting them
   * to human model.
   */
  bool WarpMicroExpression (const csArray<csVector3>& xverts,
                            double s2[], double w[],
                            Target& expr);

  /**
   * Parse all micro-expressions defined in MakeHuman subfolders of 
   * 'makehuman/data/targets/expression/units/' and cumulate them 
   * into a resulting offset buffer per micro-expression. 
   * The choice of folders and weights is done by using basic model 
   * properties (ethnics/gender/age).
   * \param microExpr  Array of parsed MakeHuman expression targets
   * \Return true if all micro-expressions offsets have been parsed successfully
   */
  bool ParseMicroExpressions (csArray<Target>& microExpr);

  /**
   * Generate morph targets for MakeHuman facial micro-expressions.
   * The expression names are generated by using the parsed MakeHuman
   * expression files in subfolders of 'makehuman/data/targets/expression/units/'.
   * \param microExpr  Array of parsed MakeHuman expression targets
   * \Return true if expressions have been successfully generated
   */
  bool GenerateMicroExpressions (csArray<Target>& microExpr);

  /**
   * Convert MakeHuman expression targets and add them to an animesh factory.
   * \param amfact  Animated mesh factory of the model
   * \param mapBuf
   * \param prefix  Prefix used to name the new expressions
   * \param mhExpressions  Array of MakeHuman expression targets
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
   * Parse a proxy object from a MakeHuman proxy file. Such a proxy defines
   * how a mesh object should be adapted to a human model.
   * \param proxyFile  Path of a MakeHuman proxy file (extensions .proxy or .mhclo)
   * \param proxy  Parsed data of MakeHuman proxy
   * \Return true if proxy file has been successfully parsed
   */
  bool ParseProxyFile (const char* proxyFile, ProxyData& proxy);

  /**
   * Adapt a neutral proxy object to loaded human model.
   * \param proxy  Parsed data of MakeHuman proxy
   * \Return true if proxy has been successfully adjusted to the 
   *  model factory
   *
   * Since this method adapts a proxy to a human model, a model should 
   * have been previously created with method GenerateModel() before calling 
   * this one.
   */
  bool AdaptProxyToModel (ProxyData& proxy,
			  csDirtyAccessArray<csVector3>& coords,
			  csDirtyAccessArray<csVector2>& texcoords,
			  csDirtyAccessArray<csVector3>& normals);

  /**
   * Create a proxy object by parsing a MakeHuman proxy file, adapt it to
   * loaded human model and generate the corresponding animesh factory.
   * \param proxyName  Name of a MakeHuman proxy
   * \param proxyFile  Path of a MakeHuman proxy file (extensions .proxy or .mhclo)
   * \param texture  Texture used on generated animesh factory (with extension .png); 
   *                 if it is not defined (nullptr), the texture referenced in parsed
   *                 proxy data is used
   * \param doubleSided  Indicates if generated mesh should be double sided
   * \param proxy  Generated data of proxy object
   * \Return true if an animated mesh factory corresponding to MakeHuman proxy object
   *  has been successfully generated; nullptr otherwise
   */
  csPtr<CS::Mesh::iAnimatedMeshFactory> CreateProxyMesh 
    (const char* proxyName, const char* proxyFile,
     const char* texture, const bool doubleSided, ProxyData& proxy);

  /**
   * Generate expression targets for a MakeHuman proxy model.
   * \param proxy  Parsed MakeHuman proxy model
   * \param modelExpr  Array of expression targets for MakeHuman model
   * \param proxyExpr  Generated expression targets for MakeHuman proxy model
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
CS_PLUGIN_NAMESPACE_END (MakeHuman)

#endif // __MAKEHUMAN_CHARACTER_H__
