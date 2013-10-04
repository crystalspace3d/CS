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

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

/*-------------------------------------------------------------------------*
 * MakeHuman proxy parser (.proxy and .mhclo)
 *-------------------------------------------------------------------------*/

bool MakeHumanCharacter::ParseProxyScale (csStringArray& words, ProxyData& proxy)
{
  int mhxIndex;
  float floatVal;
  size_t numWords = words.GetSize ();
  if (numWords != 5)
    return ReportError ("Wrong scaling values in proxy '%s'", proxy.name.GetData ());

  // Get scaling axis
  char key = words[1][0];
  int component = -1;
  if (key == 'x')
    component = 0;
  else if (key == 'y')
    component = 1;
  else if (key == 'z')
    component = 2;

  // Parse scale values: "v1 v2 den"
  // where: 
  //    vi  --> mhx index of human model vertex
  //    den --> scaling factor
  if (sscanf (words[2], "%i", &mhxIndex) != 1)
    return ReportError ("Wrong scale definition in proxy %s", proxy.name.GetData ());

  // take the first found CS vertex since only its coordinates are used
  proxy.proxyScale[component].v1 = mhxIndex;
  
  if (sscanf (words[3], "%i", &mhxIndex) != 1)
    return ReportError ("Wrong scale definition in proxy %s", proxy.name.GetData ());
    
  // take the first found CS vertex since only its coordinates are used
  proxy.proxyScale[component].v2 = mhxIndex;
    
  if (sscanf (words[4], "%f", &floatVal) != 1)
    return ReportError ("Wrong scale definition in proxy %s", proxy.name.GetData ());
  
  proxy.proxyScale[component].den = floatVal;

  return true;
}

bool MakeHumanCharacter::ParseProxyVert (csStringArray& words, ProxyData& proxy)
{
  int mhxIndex;
  float floatVal;

  // Parse the definition of a proxy vertex: "v1"  or  "v1 v2 v3 w1 w2 w3 d1 d2 d3"
  // where: 
  //    vi --> mhx index of human model vertex
  //    wi --> weight
  //    di --> offset

  // Check if the definition is limited to one vertex reference.
  size_t numWords = words.GetSize ();
  if (numWords == 1)
  {
    if (sscanf (words[0], "%i", &mhxIndex) != 1)
      return ReportError ("Wrong vertex index in proxy '%s'", proxy.name.GetData ());
        
    // Create new proxy vertex definition
    ProxyVert pvert;
    pvert.v[0] = mhxIndex;
    proxy.proxyVerts.Push (pvert);
        
    return true;
  }

  if (numWords != 9)
    return ReportError ("Wrong number of values in proxy '%s'", proxy.name.GetData ());

  // Create new proxy vertex definition
  ProxyVert pvert;

  // Parse three vertex indices: v1 v2 v3
  for (int i=0 ; i<3 ; i++)
  {
    if (sscanf (words[i], "%i", &mhxIndex) != 1)
      return ReportError ("Wrong vertex index in proxy '%s'", proxy.name.GetData ());

    pvert.v[i] = mhxIndex;
  }

  // Parse three weights: w1 w2 w3
  for (int i=0 ; i<3 ; i++)
  {
    if (sscanf (words[3+i], "%f", &floatVal) != 1)
      return ReportError ("Wrong weight in clothing proxy '%s'", proxy.name.GetData ());

    pvert.w[i] = floatVal;
  }

  // Parse three offsets: d1 d2 d3
  for (int i=0 ; i<3 ; i++)
  {
    if (sscanf (words[6+i], "%f", &floatVal) != 1)
      return ReportError ("Wrong offset in proxy '%s'", proxy.name.GetData ());
    
    pvert.d[i] = floatVal;
  }

  // Save parsed vertex in buffer
  proxy.proxyVerts.Push (pvert);

  return true;
}

bool MakeHumanCharacter::ParseProxyFile (const char* proxyFile, ProxyData& proxy)
{
  // Open the resource file
  csRef<iFile> file = manager->OpenFile (proxyFile, "/lib/makehuman/");
  if (!file)
    return ReportError ("Could not open file %s", proxyFile);

  // Reset proxy buffers
  proxy.proxyVerts.DeleteAll ();
  proxy.proxyUVs.DeleteAll ();
  proxy.proxyFaces.DeleteAll ();

  // Parse mapping buffer file
  char line[256];
  size_t numWords;
  size_t vertIndex = -1;
  size_t uvIndex = -1;
  size_t faceIndex = -1;
  bool doVerts = false;
  bool doUVs = false;
  bool doFaces = false;

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255)) 
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed proxy file %s", proxyFile);
    }
    else
    {
      // Split line into separated words
      csStringArray words;
      numWords = words.SplitString (csString (line).Trim (), " ", csStringArray::delimIgnore);
      if (numWords == 0)
        continue;

      if (csString (words[0]).Compare ("#"))
      {
        if (numWords <= 1)
          return ReportError ("Wrong keyword element in proxy file %s", proxyFile);

        // Check if this line begins the definition of proxy vertices
        if (csString (words[1]).Compare ("verts"))
        {
          doVerts = true;
          doUVs = false;
          doFaces = false;
        }
        // Check if this line begins the definition of UV coordinates
        if (csString (words[1]).Compare ("texVerts"))
        {
          doUVs = true;
          doVerts = false;
          doFaces = false;
        }
        // Check if this line begins the definition of proxy faces
        if (csString (words[1]).Compare ("texFaces"))
        {
          doFaces = true;
          doVerts = false;
          doUVs = false;
        }
        // Check if this line defines texture
        else if (csString (words[1]).Compare ("obj_file"))
        {
          proxy.objFile = csString (words[2]);
        }
        // Check if this line defines texture
        else if (csString (words[1]).Compare ("texture"))
        {
          proxy.texFile = csString (words[2]);
        }
        // Check if this line defines mask
        else if (csString (words[1]).Compare ("mask"))
        {
          proxy.maskFile = csString (words[2]);
          // TODO: use mask in combination with z depth
        }
        // Check if this line defines material
        else if (csString (words[1]).Compare ("material"))
        {
          // TODO: parse material; implement ParseProxyMaterial ()
        }
        else if (csString (words[1]).Compare ("z_depth"))
        {
          // TODO: parse/define proxy order
        }
        // Check if the line is a scale definition
        else if (csString (words[1]).Slice (1).Compare ("_scale"))
        {
          if (!ParseProxyScale (words, proxy))
            return ReportError ("Wrong scale definition in proxy file %s", proxyFile);
        }

        continue;
      }

      if (doVerts)
      {
        vertIndex++;

        if (!ParseProxyVert (words, proxy))
          return ReportError ("Wrong vertex definition in proxy file %s", proxyFile);
      }
      else if (doUVs)
      {
        uvIndex++;
        // UNUSED DATA
        // TODO: parse texVerts section; implement ParseProxyUVs ()
      }
      else if (doFaces)
      {
        faceIndex++;
        // UNUSED DATA
        // TODO: parse texFaces section; implement ParseProxyFaces ()
      }
    }
  }

  return true;
}

bool MakeHumanCharacter::AdaptProxyToModel (ProxyData& proxy,
					    csDirtyAccessArray<csVector3>& coords,
					    csDirtyAccessArray<csVector2>& texcoords,
					    csDirtyAccessArray<csVector3>& normals)
{
  if (animeshFactory->GetVertexCount () != 0)
  {
    printf ("Adapting proxy '%s' to human model\n", proxy.name.GetData ());

    // Get vertex buffer of the human model mesh
    csRef<iRenderBuffer> modelVertBuf = animeshFactory->GetVertices ();
    csRenderBufferLock<csVector3> modelVerts (modelVertBuf, CS_BUF_LOCK_READ);

    // Calculate clothing scale
    size_t mhIndex1, mhIndex2, csIndex1, csIndex2;
    float scale[3];
    for (int i = 0; i < 3; i++)
    {
      if (proxy.proxyScale[i].v1 == -1)
      {
        scale[i] = 1.0f;
        continue;
      }
      
      // Take coordinates of the first corresponding CS vertex
      mhIndex1 = proxy.proxyScale[i].v1;
      csIndex1 = mappingBuffer[mhIndex1].vertices[0];
      mhIndex2 = proxy.proxyScale[i].v2;
      csIndex2 = mappingBuffer[mhIndex2].vertices[0];
      scale[i] = 
        fabs (modelVerts[csIndex1][i] - modelVerts[csIndex2][i]) / proxy.proxyScale[i].den;
    }
        
    // Update clothing mesh, using its associated proxy
    size_t mhIndex, csIndex;
    csVector3 v0, v1, v2;
    size_t totalVertices = (size_t) proxy.proxyVerts.GetSize ();

    for (size_t index = 0; index < totalVertices; index++)
    {
      ProxyVert pvert = proxy.proxyVerts[index];

      // Proxy vertex is defined by the coordinates of a single vertex
      if ((pvert.v[1] == -1) || (pvert.v[2] == -1))
      {
        mhIndex = pvert.v[0];
        csIndex = mappingBuffer[mhIndex].vertices[0];
        coords[index] = modelVerts[csIndex];
        continue;
      }
      
      // Proxy vertex is defined by barycentric coordinates
      csIndex = mappingBuffer[pvert.v[0]].vertices[0];
      v0 = modelVerts[csIndex];
      csIndex = mappingBuffer[pvert.v[1]].vertices[0];
      v1 = modelVerts[csIndex];
      csIndex = mappingBuffer[pvert.v[2]].vertices[0];
      v2 = modelVerts[csIndex];

      coords[index][0] =
        pvert.w[0] * v0[0] + pvert.w[1] * v1[0] + pvert.w[2] * v2[0] + pvert.d[0] * scale[0];
      coords[index][1] =
        pvert.w[0] * v0[1] + pvert.w[1] * v1[1] + pvert.w[2] * v2[1] + pvert.d[1] * scale[1];
      coords[index][2] =
        pvert.w[0] * v0[2] + pvert.w[1] * v1[2] + pvert.w[2] * v2[2] + pvert.d[2] * scale[2];
    }
  }

  return true;
}

csPtr<CS::Mesh::iAnimatedMeshFactory> MakeHumanCharacter::CreateProxyMesh 
(const char* proxyName, const char* proxyFile,
 const char* texture, const bool doubleSided,
 ProxyData& proxy)
{
  if (!coords.GetSize ())
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);

  // Parse the proxy file if there is some
  if (!ParseProxyFile (proxyFile, proxy))
  {
    ReportError ("Failed parsing proxy file '%s'", proxyFile);
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  // Check that an object file describing the MakeHuman neutral mesh of 
  // proxy object is defined
  csDirtyAccessArray<FaceGroup> faceGroups;
  csString dirPath;
  csDirtyAccessArray<csVector3> proxyCoords;
  csDirtyAccessArray<csVector2> proxyTexcoords;
  csDirtyAccessArray<csVector3> proxyNormals;

  if (!proxy.objFile.IsEmpty ())
  {
    // Get path of object file
    csString path (proxyFile);
    size_t end = path.FindLast ('/');
    dirPath = path.Slice (0, end + 1);
    csString objPath (dirPath);
    objPath.Append (proxy.objFile);

    // Parse the object file
    if (!manager->ParseObjectFile (objPath.GetData (), proxyCoords, proxyTexcoords, proxyNormals, faceGroups))   
      // Warning: this clears the buffers coords/texcoords/normals
      // TODO: change that
    {
      ReportError ("Parsing object file '%s' KO!", proxy.objFile.GetData ());
      return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
    }
  }

  else faceGroups = manager->faceGroups;

  // Copy the base buffers
  coords = proxyCoords;
  texcoords = proxyTexcoords;
  normals = proxyNormals;

  // Adapt the proxy mesh to the model
  if (!AdaptProxyToModel (proxy, coords, texcoords, normals))
  {
    ReportError ("Failed the adaptation of proxy '%s' to human model", proxyName);
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  // Generate CS mesh buffers
  csHash<VertBuf, csString> tmp;
  csDirtyAccessArray<Submesh> csSubmeshes;
  if (!GenerateMeshBuffers (coords, texcoords, normals,
			    faceGroups, doubleSided, tmp, csSubmeshes, proxy.mappingBuffer))
  {
    ReportError ("Generating mesh buffers for MakeHuman proxy KO!");
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  // Check validity of proxy and object files
  if (proxy.mappingBuffer.GetSize () != proxy.proxyVerts.GetSize ())
  {
    ReportError ("Inconsistent proxy/object files for '%s'", proxyName);
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }
  
  // Create animesh factory from parsed MakeHuman proxy object  
  if (texture)
  {
    // Use texture given as parameter (model skin)
    proxy.factory = CreateAnimatedMesh (coords, texcoords, normals, csSubmeshes, texture);
  }
  else
  {
    // Use texture file referenced by proxy file (clothing texture)
    csString textureFilePath (dirPath);
    textureFilePath.Append (proxy.texFile);
    proxy.factory = CreateAnimatedMesh (coords, texcoords, normals, csSubmeshes, textureFilePath.GetData ());
  }

  if (!proxy.factory)
  {
    ReportError ("Creating CS animesh from MakeHuman proxy '%s' KO!", proxyName);
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  printf ("  - number of vertices of MakeHuman proxy model: %i\n",
         (int) proxy.proxyVerts.GetSize ());
  printf ("  - number of vertices of Crystal Space proxy model: %i\n",
         (int) proxy.factory->GetVertexCount ());

  return proxy.factory;
}

bool MakeHumanCharacter::GenerateProxyMicroExpressions (const ProxyData& proxy, 
							const csArray<Target>& modelExpr,
							csArray<Target>& proxyExpr)
{
  // Reset expressions buffer
  proxyExpr.DeleteAll ();

  // Generate micro-expressions offsets for human model, using its associated proxy
  size_t mhIndex;
  csVector3 v0, v1, v2;
  size_t* mapping;

  size_t totalVertices = (size_t) proxy.proxyVerts.GetSize ();
  if (totalVertices == 0)
    return ReportError ("Error while generating proxy expressions: proxy vertices are not defined");

  for (size_t ei = 0; ei < modelExpr.GetSize (); ei++)
  {
    // Create new micro-expression for model proxy
    size_t proxyIndex = proxyExpr.Push (Target (modelExpr[ei].name.GetData ()));
    Target& expr = proxyExpr[proxyIndex];

    // Build a mapping buffer for the vertices that are activated by the morph target
    csHash<size_t, size_t> indexMapping;
    for (size_t i = 0; i < modelExpr[ei].indices.GetSize (); i++)
      indexMapping.Put (modelExpr[ei].indices[i], i);

    for (size_t index = 0; index < totalVertices; index++)
    {
      ProxyVert pvert = proxy.proxyVerts[index];

      // Case where the proxy offset is defined by the offset of a single model vertex
      if ((pvert.v[1] == -1) || (pvert.v[2] == -1))
      {
        mhIndex = pvert.v[0];
	mapping = indexMapping.GetElementPointer (mhIndex);
	if (mapping)
	{
	  expr.offsets.Push (modelExpr[ei].offsets[*mapping]);
	  expr.indices.Push (index);
	}

        continue;
      }

      // Case where the proxy offset is defined by barycentric coordinates of
      // the model vertices displaced by the expression offsets
      bool activated = false;

      mhIndex = pvert.v[0];
      mapping = indexMapping.GetElementPointer (mhIndex);
      if (mapping)
      {
	activated = true;
	v0 = modelExpr[ei].offsets[*mapping];
      }
      else v0 = 0.0f;

      mhIndex = pvert.v[1];
      mapping = indexMapping.GetElementPointer (mhIndex);
      if (mapping)
      {
	activated = true;
	v1 = modelExpr[ei].offsets[*mapping];
      }
      else v1 = 0.0f;

      mhIndex = pvert.v[2];
      mapping = indexMapping.GetElementPointer (mhIndex);
      if (mapping)
      {
	activated = true;
	v2 = modelExpr[ei].offsets[*mapping];
      }
      else v2 = 0.0f;

      if (!activated) continue;

      csVector3 offset (pvert.w[0] * v0[0] + pvert.w[1] * v1[0] + pvert.w[2] * v2[0],
		       pvert.w[0] * v0[1] + pvert.w[1] * v1[1] + pvert.w[2] * v2[1],
		       pvert.w[0] * v0[2] + pvert.w[1] * v1[2] + pvert.w[2] * v2[2]);
      expr.offsets.Push (offset);
      expr.indices.Push (index);
    }
  }

  return true;
}
 
}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
