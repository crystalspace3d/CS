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

#define INFLUENCES_PER_VERTEX 4

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

/*-------------------------------------------------------------------------*
 * MakeHuman rig parser (.rig)
 *-------------------------------------------------------------------------*/

bool MakehumanCharacter::CalculateBaricenter (const VertBuf& facegroup, csVector3& center)
{
  if (facegroup.vertexCount == 0)
    return false;

  size_t mhIndex;
  center.Set (0.0f);
  for (size_t i = 0; i < facegroup.vertexCount; i++)
  {
    mhIndex = facegroup.vertices[i];
    center += morphedMesh[mhIndex];
  }

  center /= (float) facegroup.vertexCount;
  return true;
} 

bool MakehumanCharacter::ParseJointDefinition (csString line, 
					       const csHash<VertBuf, csString>& mhJoints,
					       csHash<csVector3, csString>& jointPos)
{
  // Split line
  csStringArray stmp;
  size_t numVals = stmp.SplitString (line.Trim (), " ", csStringArray::delimIgnore);
  if (numVals < 2)
    return ReportError ("Invalid definition of joint location\n");

  // Parse definition of joint location
  // There are 7 different types of location definitions:
  // voffset, offset, joint, vertex, position, line, front
  int mhIndex;
  csString name (stmp[0]);
  csString type (stmp[1]);
  csVector3 pos;

  if (type.Compare ("voffset"))
  {
    if (numVals < 6)
      return ReportError ("Invalid definition of joint location ('voffset')\n");

    // Parse one vertex index and one offset
    csVector3 offset;
    if (sscanf (stmp[2], "%i", &mhIndex) != 1)
      return ReportError ("Invalid vertex index in definition of joint location\n");

    for (int i=0; i<3; i++)
    {
      if (sscanf (stmp[i + 3], "%f", &(offset[i])) != 1)
        return ReportError ("Invalid offset in definition of joint location\n");
    }

    pos = morphedMesh[mhIndex] + offset;
  }
  else if (type.Compare ("offset"))
  {
    if (numVals < 6)
      return ReportError ("Invalid definition of joint location ('offset')\n");

    // Parse one joint name and one offset
    csVector3 offset;
    csString jointName (stmp[2]);
    if (!jointPos.Contains (jointName))
      return ReportError ("Error while calculating joint location: unknown joint '%s'",
                     jointName.GetData ());

    for (int i=0; i<3; i++)
    {
      if (sscanf (stmp[i + 3], "%f", &(offset[i])) != 1)
        return ReportError ("Invalid offset in definition of joint location\n");
    }

    pos = *(jointPos[ jointName ]) + offset;    
  }
  else if (type.Compare ("joint"))
  {
    if (numVals < 3)
      return ReportError ("Invalid definition of joint location ('joint')\n");

    csString jointName (stmp[2]);
    if (!mhJoints.Contains (jointName))
      return ReportError ("Error while calculating joint location: unknown joint '%s'",
                     jointName.GetData ());

    VertBuf emptyJoint;
    VertBuf joint = mhJoints.Get (jointName, emptyJoint);
    if (!CalculateBaricenter (joint, pos))
      return ReportError ("Problem while calculating the baricenter of head joint %s", jointName.GetData ());
  }
  else if (type.Compare ("vertex"))
  {
    if (numVals < 3)
      return ReportError ("Invalid definition of joint location ('vertex')\n");

    // Parse one vertex index
    if (sscanf (stmp[2], "%i", &mhIndex) != 1)
      return ReportError ("Invalid vertex index in definition of joint location\n");

    pos = morphedMesh[mhIndex];
  }
  else if (type.Compare ("position"))
  {
    if (numVals < 5)
      return ReportError ("Invalid definition of joint location ('position')\n");

    for (size_t i=2; i<5; i++)
      if (!jointPos.Contains (csString (stmp[i])))
        return ReportError ("Error while calculating joint location: unknown joint '%s'", stmp[i]);

    csVector3 x = *(jointPos[ csString (stmp[2]) ]);
    csVector3 y = *(jointPos[ csString (stmp[3]) ]);
    csVector3 z = *(jointPos[ csString (stmp[4]) ]);
    pos = csVector3 (x[0], y[1], z[2]);
  }
  else if (type.Compare ("line"))
  {
    if (numVals < 6)
      return ReportError ("Invalid definition of joint location ('line')\n");

    float k1, k2;
    csVector3 p1, p2;

    if (sscanf (stmp[2], "%f", &(k1)) != 1)
      return ReportError ("Invalid value in definition of joint location\n");

    if (sscanf (stmp[4], "%f", &(k2)) != 1)
      return ReportError ("Invalid value in definition of joint location\n");

    if (!jointPos.Contains (csString (stmp[3])) or
        !jointPos.Contains (csString (stmp[5])))
      return ReportError ("Error while calculating joint location: unknown joint");
    
    p1 = *(jointPos[ csString (stmp[3]) ]);
    p2 = *(jointPos[ csString (stmp[5]) ]);
    pos = k1 * p1 + k2 * p2;
  }
  else if (type.Compare ("front"))
  {
    if (numVals < 6)
      return ReportError ("Invalid definition of joint location ('front')\n");

    csVector3 raw, head, tail, offset, vec, vraw, rvec, nloc;
    float vec2, x;

    for (size_t i=2; i<5; i++)
      if (!jointPos.Contains (csString (stmp[i])))
        return ReportError ("Error while calculating joint location: unknown joint '%s'", stmp[i]);

    raw  = *(jointPos[ csString (stmp[2]) ]);
    head = *(jointPos[ csString (stmp[3]) ]);
    tail = *(jointPos[ csString (stmp[4]) ]);

    csString tmp = csString (stmp[5]).Trim ();
    csString vals = tmp.Slice (1, tmp.Length () - 1);
    csStringArray offs (vals, ",", csStringArray::delimIgnore);

    for (int i=0; i<3; i++)
    {
      if (sscanf (offs[i], "%f", &(offset[i])) != 1)
        return ReportError ("Invalid offset in definition of joint location\n");
    }

    vec = tail - head;
    vec2 = vec * vec;
    vraw = raw - head;
    x = (vec * vraw) / vec2;
    rvec = x * vec;
    pos = head + rvec + offset;
  }
  else
    return ReportError ("WARNING: unknown joint type '%s' in Makehuman rig file.\n", type.GetData ());

  // Add joint name and its position to the hashing table
  jointPos.Put (name, pos);

  return true;
}

bool MakehumanCharacter::ParseBoneDefinition (csString line,
					      csHash<Bone, csString>& bones,
					      csArray<csString>& boneOrder)
{
  // Extract bone name
  csStringArray stmp;
  size_t numVals = stmp.SplitString (line.Trim (), " ", csStringArray::delimIgnore);
  csString boneName = csString (stmp[0]);
  if (numVals < 4)
    return ReportError ("Invalid definition of bone '%s'\n", boneName.GetData ());

  // Extract bone data
  csString head = csString (stmp[1]);
  csString tail = csString (stmp[2]);
  csString parent = strcmp (stmp[4],"-")==0 ? csString ("") : csString (stmp[4]);
  float roll;
  if (sscanf (stmp[3], "%f", &roll) != 1)
    return ReportError ("Invalid roll angle in definition of bone '%s'\n", boneName.GetData ());

  // TODO: parse Makehuman bone options (p.e. '-nc')

  // Add data into bones array
  Bone boneData (parent, head, tail, roll);
  bones.Put (boneName, boneData);
  boneOrder.Push (boneName);

  return true;
}

bool MakehumanCharacter::ParseBoneWeights (csString line, 
					   csArray<VertexInfluence>& boneWeights)
{
  // Parse bone influence
  csStringArray words;
  size_t numVals = words.SplitString (line.Trim (), " ", csStringArray::delimIgnore);
          
  if (numVals != 2)
    return ReportError ("Wrong element in object file: invalid definition of bone influence");

  // Parse vertex index
  int mhIndex;
  if (sscanf (words[0], "%i", &(mhIndex)) != 1)
    return ReportError ("Invalid vertex index in definition of bone influence\n");

  // Parse influence value
  float bw;
  if (sscanf (words[1], "%f", &(bw)) != 1)
    return ReportError ("Invalid weight in definition of bone influence\n");

  // Save parsed influence weight
  VertexInfluence vi (mhIndex, bw);
  boneWeights.Push (vi);

  return true;
}

bool MakehumanCharacter::ParseRigFile (const char* filename, 
				       const csHash<VertBuf, csString>& mhJoints,
				       csHash<csVector3, csString>& jointPos, 
				       csHash<Bone, csString>& bones,
				       csArray<csString>& boneOrder)
{
  // Open rig file
  csRef<iFile> file = manager->OpenFile (filename, RIG_PATH);
  if (!file)
    return ReportError ("Could not open file %s", filename);

  // Parse rig file
  char line[256];
  csString curBone;
  bool doJoints = false;
  bool doBones = false;
  bool doWeights = false;

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255)) 
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed mesh data file", filename);
    }
    else
    {
      csString csline (line);
      if (csline.Trim ().IsEmpty ())
        continue;

      if (csline.StartsWith ("# locations"))
      {
        doJoints = true;
        doBones = false;
        doWeights = false;
        continue;
      }
      else if (csline.StartsWith ("# bones"))
      {
        doBones = true;
        doJoints = false;
        doWeights = false;
        continue;
      }
      else if (csline.StartsWith ("# weights"))
      {
        doJoints = false;
        doBones = false;

        // Search for current bone among parsed bones
        curBone = csline.Slice (10);
        if (!bones.Contains (curBone))
        {
          //printf ("\nWarning: unknown bone '%s' in definition of bone weights\n", curBone.GetData ());
          doWeights = false;
          continue;
        }

        doWeights = true;
        continue;
      }

      if (doJoints)
      {
        // Parse the joints section: each line defines a new joint;
        // calculate joint location and add it to the joints location data structure
        if (!ParseJointDefinition (csline, mhJoints, jointPos))
          ReportError ("Problem while parsing definition of joints in rig file");
      }
      else if (doBones)
      {
        // Parse the bones section: each line defines a new bone;
        // calculate bone location and add it to the bones location data structure
        if (!ParseBoneDefinition (csline, bones, boneOrder))
          ReportError ("Problem while parsing definition of bone in rig file");
      }
      else if (doWeights)
      {
        // Parse the weights section: each line defines a new weight
        ParseBoneWeights (csline, bones[curBone]->weights);
      }
    }
  }

  return true;
}

bool MakehumanCharacter::CreateSkelFact (const char* skelName, 
					 const csHash<Bone, csString>& bones,
					 const csArray<csString>& boneOrder,
					 CS::Mesh::iAnimatedMeshFactory* amfact)
{
  // Create a new skeleton factory
  csRef<CS::Animation::iSkeletonFactory> skelFact;
  csRef<CS::Animation::iSkeletonManager> skelManager = 
    csQueryRegistryOrLoad<CS::Animation::iSkeletonManager> (manager->objectRegistry,
                                                            "crystalspace.skeletalanimation");
  if (!skelManager)
    return ReportError ("Error while querying the skeleton manager");

  skelFact = skelManager->CreateSkeletonFactory (skelName);
  if (!skelFact)
    return ReportError ("Could not create skeleton %s.", skelName);

  amfact->SetSkeletonFactory (skelFact);

  // Set the number of bone influences per vertex
  amfact->SetBoneInfluencesPerVertex (INFLUENCES_PER_VERTEX);

  // Create the new bones
  csString boneName;
  for (size_t i = 0; i < boneOrder.GetSize (); i++)
  {
    CS::Animation::BoneID newBone;
    boneName = boneOrder[i];
    const Bone* bone = bones.GetElementPointer (boneName);

    if (bone->parent.Compare (""))
    {
      // Create root bone
      newBone = skelFact->CreateBone ();
    }
    else
    {
      // Get bone parent
      CS::Animation::BoneID parent = skelFact->FindBone (bone->parent.GetData ());
      if (parent == CS::Animation::InvalidBoneID)
      {
        ReportError ("Invalid parent bone '%s' in definition of bones", bone->parent.GetData ());
        return false;
      }

      // Create child bone
      newBone = skelFact->CreateBone (parent);
    }

    if (newBone == CS::Animation::InvalidBoneID)
    {
      ReportError ("Invalid bone '%s' in definition of bones", boneName.GetData ());
      return false;
    }

    // Set bone name
    skelFact->SetBoneName (newBone, boneName.GetData ());
  }

  return true;
}

bool MakehumanCharacter::SetBoneLocations (const csHash<VertBuf, csString>& mhJoints,
					   const csHash<csVector3, csString>& jointPos,
					   const csHash<Bone, csString>& bones,
					   const bool moveOrigin,
					   CS::Mesh::iAnimatedMeshFactory* amfact)
{
  // Verify location of model's origin (i.e. mid-feet), using the MH joints definition: 
  //   origin = (0.5 * left-toe) + (0.5 * right-toe) + [0,-0.3, 0]
  // TODO: modify hard code by parsing FloorJoints from file 'makehuman/shared/mhx/rig_body_25.py'
  VertBuf vb;
  csVector3 ltoe = csVector3 (0.0f);
  VertBuf lvb = mhJoints.Get ("l-toe-1-1", vb);
  if (!CalculateBaricenter (lvb, ltoe))
    return ReportError ("Problem while calculating the baricenter of the left toe");

  csVector3 rtoe = csVector3 (0.0f);
  VertBuf rvb = mhJoints.Get ("r-toe-1-1", vb);
  if (!CalculateBaricenter (rvb, rtoe))
    return ReportError ("Problem while calculating the baricenter of the right toe");
    
  csVector3 origin = (0.5 * ltoe + 0.5 * rtoe) + csVector3 (0.0f, -0.3f, 0.0f);
 
  // The human model mesh should have its origin displaced to have feets on the ground.
  // Since proxy mesh is generated from this model, its position should not be corrected.
  if (moveOrigin && !origin.IsZero ())
  {
    // Move mesh so that its origin is at [0, 0, 0]
    csRef<iMeshObjectFactory> obFact = scfQueryInterface<iMeshObjectFactory> (amfact);
    csVector3 iorigin = csVector3 (-origin[0],-origin[1], origin[2]);         // inverse Z component
    csReversibleTransform translation = csReversibleTransform (csMatrix3 (), iorigin);
    obFact->HardTransform (translation);
  }

  // Calculate positions of the bones
  csRef<CS::Animation::iSkeletonFactory> skelFact = amfact->GetSkeletonFactory ();
  csArray< CS::Animation::BoneID > boneList = skelFact->GetBoneOrderList ();
  csArray< CS::Animation::BoneID >::Iterator it = boneList.GetIterator ();

  while (it.HasNext ())
  {
    CS::Animation::BoneID bi = it.Next ();
    if (!skelFact->HasBone (bi))
    {
      ReportError ("Bone %i is not defined in skeleton factory", (int) bi);
      continue;
    }

    const char* bname = skelFact->GetBoneName (bi);
    if (!bones.Contains (bname))
    {
      ReportError ("Error: unknown original position of bone %i", (int) bi);
      continue;
    }
      
    // Get location of bone head and tail
    const Bone* bone = bones.GetElementPointer (bname);
    if (!bone)
      return ReportError ("Error while calculating bone location: unknown bone '%s'", bname);
    if (!jointPos.Contains (bone->head))
      return ReportError ("Error while calculating bone %s location: unknown head joint '%s'",
                     bname, bone->head.GetData ());
    if (!jointPos.Contains (bone->tail))
      return ReportError ("Error while calculating bone %s location: unknown tail joint '%s'",
                     bname, bone->tail.GetData ());

    // Get bone position
    csVector3 pos0;
    csVector3 headpos = jointPos.Get (bone->head, pos0);
    csVector3 tailpos = jointPos.Get (bone->tail, pos0);
    if (!origin.IsZero ())
    {
      // Apply origin translation to the bone if it was non null
      headpos -= origin;
      tailpos -= origin;
    }

    // Get bone orientation
    headpos[2] *= -1.0f;        // inverse Z component
    tailpos[2] *= -1.0f;        // inverse Z component
    csVector3 orientation = tailpos - headpos;
    orientation.Normalize ();
    csVector3 up (0, 1, 0);
    csReversibleTransform transf;
    transf.LookAt (orientation, up);  // TODO: use bone.roll value to set bone rotation
    csMatrix3 m = transf.GetT2O ();
    csQuaternion rot;
    rot.SetMatrix (m);

    // Set new transformation of the bone
    skelFact->SetTransformAbsSpace (bi, rot, headpos);
  }

  return true;
}

bool MakehumanCharacter::SetBoneInfluences (const csHash<Bone, csString>& bones,
					    CS::Mesh::iAnimatedMeshFactory* amfact)
{
  // Get bone influences per mesh vertex and sort them in increasing order
  csRef<CS::Animation::iSkeletonFactory> skelFact = amfact->GetSkeletonFactory ();
  size_t numVerts = amfact->GetVertexCount ();
  csArray< BoneInfluences > weights;
  weights.SetSize (numVerts);
  csString boneName;

  csHash<Bone, csString>::ConstGlobalIterator it = bones.GetIterator ();
  while (it.HasNext ())
  {
    Bone bi = it.Next (boneName);

    CS::Animation::BoneID curBone = skelFact->FindBone (boneName.GetData ());
    if (curBone == CS::Animation::InvalidBoneID)
      continue;

    for (size_t i = 0; i < bi.weights.GetSize (); i++)
    {
      VertexInfluence vw = bi.weights[i];

      VertBuf mapv = mappingBuffer[vw.vertex];
      for (size_t j = 0; j < mapv.vertexCount; j++)
      {
        size_t csIndex = mapv.vertices[j];
        size_t index = weights[csIndex].influences.InsertSorted (vw.weight);
        weights[csIndex].bones.Insert (index, curBone);
      }

    }
  }

  // Set 4 bone influences for each mesh vertex
  size_t realPerVertex = amfact->GetBoneInfluencesPerVertex ();
  CS::Mesh::AnimatedMeshBoneInfluence* bi = amfact->GetBoneInfluences ();

  for (size_t vi = 0; vi < numVerts; vi++)
  {
    // Set the 4 biggest weights of that vertex
    size_t numWeights = weights[vi].influences.GetSize ();
    size_t minIdx = numWeights < realPerVertex ? numWeights : realPerVertex;
    for (size_t i = 0; i < minIdx; i++)
    {
      size_t idx = vi*realPerVertex + (minIdx - i - 1);   // inverse order of sorted weights
      bi[idx].bone = weights[vi].bones[i];
      bi[idx].influenceWeight = weights[vi].influences[i];
    }

    // Fill missing bone influences with null values
    for (size_t i = minIdx; i < realPerVertex; i++)
    {
      bi[vi*realPerVertex + i].bone = (CS::Animation::BoneID) 0;
      bi[vi*realPerVertex + i].influenceWeight = 0.0f;
    }
  }
  
  return true;
}

bool MakehumanCharacter::SetBoneInfluences (const csHash<Bone, csString>& bones,
					    ProxyData* proxy)
{
  // Note: Makehuman defines a specific process to generate bone weights on a proxy model.
  // It is implemented by method 'getProxyWeights ()' in file mh2proxy.py

  // Get proxy weight per vertex of proxy model
  csHash<VertexInfluence, size_t> proxyWeights;

  for (size_t pi = 0; pi < proxy->proxyVerts.GetSize (); pi++)
  {
    ProxyVert pv = proxy->proxyVerts[pi];
    for (size_t i = 0; i < 3; i++)
    {
      if (pv.w[i] == -1)
        break;

      VertexInfluence vi (pi, pv.w[i]);
      proxyWeights.Put (pv.v[i], vi);
    }
  }

  // Init buffer of CS bone influences
  csRef<CS::Animation::iSkeletonFactory> skelFact = proxy->factory->GetSkeletonFactory ();
  size_t numVerts = proxy->factory->GetVertexCount ();
  csArray< BoneInfluences > weights;
  weights.SetSize (numVerts);

  // Evaluate influence of each bone
  csString boneName;
  csHash<Bone, csString>::ConstGlobalIterator it = bones.GetIterator ();

  while (it.HasNext ())
  {
    csArray<VertexInfluence> vgroup;
    Bone bi = it.Next (boneName);
    CS::Animation::BoneID curBone = skelFact->FindBone (boneName.GetData ());
    if (curBone == CS::Animation::InvalidBoneID)
      continue;

    for (size_t i = 0; i < bi.weights.GetSize (); i++)
    {
      size_t v = bi.weights[i].vertex;
      float wt = bi.weights[i].weight;

      csHash<VertexInfluence, size_t>::Iterator viter = proxyWeights.GetIterator (v);
      while (viter.HasNext ())
      {
        VertexInfluence vi = viter.Next ();
        size_t pv = vi.vertex;
        float w = vi.weight;

        // Calculate bone weight on proxy vertex
        float pw = w * wt;
        if (pw > EPSILON)
          vgroup.Push (VertexInfluence (pv, pw));
      }
    }

    if (vgroup.IsEmpty ())
      continue;

    // Cumulate vertex influences of current bone
    size_t pv0;
    float wt0;
    csHash<float, size_t> fixedVGroup;
    for (size_t i = 0; i < vgroup.GetSize (); i++)
    {
      pv0 = vgroup[i].vertex;
      wt0 = vgroup[i].weight;

      if (fixedVGroup.Contains (pv0))
        *fixedVGroup[pv0] += wt0;
      else
        if (wt0 > EPSILON)
          fixedVGroup.Put (pv0, wt0);
    }

    // Get bone influences per proxy vertex and sort them in increasing order
    size_t vi;
    float wi;
    csHash<float, size_t>::GlobalIterator it = fixedVGroup.GetIterator ();
    while (it.HasNext ())
    {
      wi = it.Next (vi);    
      VertBuf mapv = proxy->mappingBuffer[vi];
      for (size_t j = 0; j < mapv.vertexCount; j++)
      {
        size_t csIndex = mapv.vertices[j];
        size_t index = weights[csIndex].influences.InsertSorted (wi);
        weights[csIndex].bones.Insert (index, curBone);
      }
    }
  }

  // Set 4 bone influences for each proxy vertex
  size_t realPerVertex = proxy->factory->GetBoneInfluencesPerVertex ();
  CS::Mesh::AnimatedMeshBoneInfluence* bi = proxy->factory->GetBoneInfluences ();

  for (size_t vi = 0; vi < numVerts; vi++)
  {
    // Set the 4 biggest weights of that vertex
    size_t numWeights = weights[vi].influences.GetSize ();
    size_t minIdx = numWeights < realPerVertex ? numWeights : realPerVertex;
    for (size_t i = 0; i < minIdx; i++)
    {
      size_t idx = vi*realPerVertex + (minIdx - i - 1);   // inverse order of sorted weights
      bi[idx].bone = weights[vi].bones[i];
      bi[idx].influenceWeight = weights[vi].influences[i];
    }

    // Fill missing bone influences with null values
    for (size_t i = minIdx; i < realPerVertex; i++)
    {
      bi[vi*realPerVertex + i].bone = (CS::Animation::BoneID) 0;
      bi[vi*realPerVertex + i].influenceWeight = 0.0f;
    }
  }

  return true;
}

bool MakehumanCharacter::CreateSkeleton (const char* modelName,
					 const char* rigName,
					 const csHash<VertBuf, csString>& mhJoints,
					 ProxyData* proxy)
{
  printf ("Creating model skeleton ");

  if (!animeshFactory)
    return ReportError ("Error while creating skeleton: the animesh factory of human model is missing");

  if (proxy && !proxy->factory)
    return ReportError ("Error while creating skeleton: the animesh factory of proxy model is missing");

  // Init buffers used to parse Makehuman rig
  csHash< csVector3, csString > jointPos;
  csHash<Bone, csString> bones;
  csArray<csString> boneOrder;

  // Parse armature from rig file RIG_NAME
  csString rigPath (RIG_PATH);
  rigPath.Append (rigName).Append (".rig");
  if (!ParseRigFile (rigPath.GetData (), mhJoints, jointPos, bones, boneOrder))
    return ReportError ("Problem while parsing joints & bones from rig file '%s'", DEFAULT_RIG);

  // Define rig name, respecting format: <factory name>_<rig name>_rig
  csString skelName (modelName);
  if (proxy)
    skelName.Append ("-").Append (proxy->name);
  skelName.Append ("_").Append (rigName).Append ("_rig");
  printf ("'%s'... ", skelName.GetData ());

  // Create skeleton factory and bones
  if (proxy)
  {
    // Use factory of model proxy
    if (!CreateSkelFact (skelName.GetData (), bones, boneOrder, proxy->factory))
      return false;

    // Set bone locations
    if (!SetBoneLocations (mhJoints, jointPos, bones, false, proxy->factory))
      return ReportError ("Problem while setting bone locations");

    // Set bone influences
    if (!SetBoneInfluences (bones, proxy))
      return ReportError ("Problem while setting bone influences");

    // Update animesh factory
    proxy->factory->Invalidate ();
  }
  else
  {
    // Use model factory
    if (!CreateSkelFact (skelName.GetData (), bones, boneOrder, animeshFactory))
      return false;

    // TODO: put back
    // Set bone locations
    if (!SetBoneLocations (mhJoints, jointPos, bones, true, animeshFactory))
      return ReportError ("Problem while setting bone locations");

    // Set bone influences
    if (!SetBoneInfluences (bones, animeshFactory))
      return ReportError ("Problem while setting bone influences");

    // Update animesh factory
    animeshFactory->Invalidate ();
  }

  printf ("done!\n");
  return true;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
