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
#include "ivideo/txtmgr.h"
#include "character.h"

#define DEFAULT_GROUP_NAME "default-dummy-group"

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

/*-------------------------------------------------------------------------*
 * MakeHuman mesh object parser (.obj)
 *-------------------------------------------------------------------------*/

bool MakehumanManager::ParseObjectFile (const char* filename,
					csDirtyAccessArray<FaceGroup>& faceGroups)
{
  printf ("Parsing object file '%s'... ", filename);

  // Open mesh data file
  csRef<iFile> file = OpenFile (filename, "/lib/makehuman/");
  if (!file)
    return ReportError ("Could not open file %s", filename);

  // Clear data buffers
  coords.DeleteAll ();
  texcoords.DeleteAll ();
  normals.DeleteAll ();

  // Parse the mesh data file
  char line[256];
  size_t numWords, numIndices;
  int vIdx, uvIdx, vnIdx;
  csString curMaterialName;
  VertBuf facegroup;
  csString vertIdx;
  FaceGroup fg (DEFAULT_GROUP_NAME);  

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!ParseLine (file, line, 255)) 
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed object data file '%s'", filename);
    }
    else
    {
      // Split line into words, using space character as separator
      csStringArray words;
      numWords = words.SplitString (csString (line).Trim (), " ", csStringArray::delimIgnore);

      // Vertex coordinates
      if (strcmp (words[0], "v") == 0)
      {
        csVector3 vertex;

        if (numWords != 4)
          return ReportError ("Wrong element in object file: not valid number of vertex coordinates");

        for (size_t i = 0; i < 3; i++)
          if (sscanf (words[i+1], "%f", &vertex[i]) != 1)
            return ReportError ("Wrong element in object file: not a valid vertex coordinate");

        coords.Push (vertex);
      }

      // Texture coordinates
      else if (strcmp (words[0], "vt") == 0)
      {
        csVector2 texco;

        if (numWords != 3)
          return ReportError ("Wrong element in object file: not valid number of texture coordinates");

        for (size_t i = 0; i < 2; i++)
          if (sscanf (words[i+1], "%f", &texco[i]) != 1)
            return ReportError ("Wrong element in object file: not a valid texture coordinate");

        texcoords.Push (texco);
      }

      // Vertex normals
      else if (strcmp (words[0], "vn") == 0)
      {
        csVector3 normal;

        if (numWords != 4)
          return ReportError ("Wrong element in object file: not valid number of normal coordinates");

        for (size_t i = 0; i < 3; i++)
          if (sscanf (words[i+1], "%f", &normal[i]) != 1)
            return ReportError ("Wrong element in object file: not a valid normal coordinate");

        normals.Push (normal);
      }

      // Face
      else if (strcmp (words[0], "f") == 0)
      {
        if ((numWords != 4) && (numWords != 5))
          return ReportError ("Wrong element in object file: not a valid number of face vertices");

        fg.faceCount++;

        for (size_t i = 1; i < numWords; i++)
        {
          // Split vertex data
          csStringArray indices;
          numIndices = indices.SplitString (words[i], "/", csStringArray::delimIgnore);
          
          if (numIndices == 0)
            return ReportError ("Wrong element in object file: not enough indices per face vertex");

          // Parse vertex index
          if (sscanf (indices[0], "%i", &vIdx) != 1)
            return ReportError ("Wrong element in object file: invalid index of face vertex");

          fg.vIndices.Push ((size_t) --vIdx);   // -1 because obj is 1 based list

          // Parse index of uv coordinates
          if (numIndices > 1)
          {
            if (sscanf (indices[1], "%i", &uvIdx) != 1)
              return ReportError ("Wrong element in object file: invalid index of face uv coordinates");

            fg.uvIndices.Push ((size_t) --uvIdx);   // -1 because obj is 1 based list
          }

          // Parse index of normal
          if (numIndices > 2)
          {
            if (sscanf (indices[2], "%i", &vnIdx) != 1)
              return ReportError ("Wrong element in object file: invalid index of vertex normal");

            fg.vnIndices.Push ((size_t) --vnIdx);   // -1 because obj is 1 based list
          }
        }

        // Check if feace group is composed of triangles
        if (numWords == 4)
        {
          // Fill buffers with default value (size_t) ~0
          fg.vIndices.Push ((size_t) ~0);
          if (fg.uvIndices.GetSize () > 0)
            fg.uvIndices.Push ((size_t) ~0);
          if (fg.vnIndices.GetSize () > 0)
            fg.vnIndices.Push ((size_t) ~0);
        }
      }

      // Face group
      else if (strcmp (words[0], "g") == 0)
      {
        if (numWords != 2)
          return ReportError ("Wrong element in object file: missing group name");

        if (fg.faceCount > 0)
        {
          // Copy parsed data of previous face group
          faceGroups.Push (fg);

          // Reset buffers and counter of faces
          fg.vIndices.Empty ();
          fg.uvIndices.Empty ();
          fg.vnIndices.Empty ();
          fg.faceCount = 0;
        }

        // Get name of the new face group
        fg.groupName.Replace (csString (words[1]));
      }

      // Material name
      else if (strcmp (words[0], "usemtl") == 0)
      {
        if (numWords != 2)
          return ReportError ("Wrong element in object file: missing material name");

        fg.materialName = words[1];
        // TODO: parse material file
      }
    }
  }

  // Copy data of last parsed face group
  if (fg.faceCount > 0)
  {
    // Copy parsed data of last face group
    faceGroups.Push (fg);

    // Reset buffers and counter of faces
    fg.vIndices.DeleteAll ();
    fg.uvIndices.DeleteAll ();
    fg.vnIndices.DeleteAll ();
  }

  printf ("done!\n");
  return true;
}

bool MakehumanCharacter::GenerateMeshBuffers (const csDirtyAccessArray<FaceGroup>& faceGroups,
					      const bool doubleSided,
					      csHash<VertBuf, csString>& mhJoints,
					      csDirtyAccessArray<Submesh>& csSubmeshes,
					      csArray<VertBuf>& mappingBuf)
{
  size_t mhIndex, uvIndex;
  csTriangle tri;
  bool triangle;
  
  // Clear data buffers
  csCoords.DeleteAll ();
  csTexcoords.DeleteAll ();
  csNormals.DeleteAll ();
  mhJoints.DeleteAll ();
  csSubmeshes.DeleteAll ();
  mappingBuf.DeleteAll ();

  // Init Makehuman mapping buffer (list defining a list of MappingVertex 
  // for each Makehuman vertex (i.e. the corresponding CS vertex, 
  // the index of Makehuman uv coordinates and its material)
  size_t vertexCount = coords.GetSize ();
  csArray< csArray<MappingVertex> > mapBuf; 
  mapBuf.SetSize (vertexCount);
  size_t csIndex = 0;

  // Treat all Makehuman face groups
  printf ("face group count %zu\n", faceGroups.GetSize());
  for (size_t fgIdx = 0; fgIdx < faceGroups.GetSize (); fgIdx++)
  {
    //printf ("face group %s\n", faceGroups[fgIdx].groupName.GetData());
    VertBuf joint;

    // Get the face group vertices
    csDirtyAccessArray<size_t> faceVerts = faceGroups[fgIdx].vIndices;
    csDirtyAccessArray<size_t> faceUVs   = faceGroups[fgIdx].uvIndices;
    csDirtyAccessArray<size_t> faceNorms = faceGroups[fgIdx].vnIndices;

    // Check if the face group defines uv coordinates and normals
    bool hasUV = faceUVs.GetSize () != 0;
    bool hasMeshNorm = normals.GetSize () != 0;     // normals from mesh definition ('vn')
    bool hasFaceNorm = faceNorms.GetSize () != 0;   // normals from facegroup definitions ('f')

    // Check if the face group is a joint definition
    bool jointDef = faceGroups[fgIdx].groupName.StartsWith ("joint-");

    // Initialize an index buffer for CS submesh
    size_t indexCount = faceVerts.GetSize ();
    size_t faceCount  = indexCount / 4;
    Submesh csSubmesh (faceGroups[fgIdx].groupName.GetData (), 
                       faceGroups[fgIdx].materialName.GetData ());

    // Treat all mesh faces of the face group
    for (size_t indexF = 0; indexF < faceCount; indexF++)
    {
      csArray<size_t> csFace;
      triangle = false;

      // Check if all Makehuman face vertices are defined in CS
      for (size_t i = 0; i < 4; i++)
      {
        mhIndex = faceVerts[4*indexF + i];

        // Check if Makehuman faces are triangles
        if (i == 3 && mhIndex == (size_t) ~0)
        {
          triangle = true;
          break;
        }

        // Get vertex coordinates
        csVector3 co = coords[mhIndex];
        co[2] *= -1.0f;          // adapt sign of Z component to CS

        // Get vertex normal
        csVector3 normal (0, 0,-1);  // define default normal
        if (hasFaceNorm)
        {
          normal = normals[ faceNorms[4*indexF+i] ];
          normal[2] *= -1.0f;      // adapt sign of Z component to CS
        }
        else if (hasMeshNorm)
        {
          normal = normals[ mhIndex ];
          normal[2] *= -1.0f;      // adapt sign of Z component to CS
        }
        
        if (jointDef)
          joint.vertices.Push (mhIndex);

        if (hasUV)
        {
          // Get UV coordinates of vertex
          uvIndex = faceUVs[4*indexF + i];
          csVector2 uv = texcoords[uvIndex];

          bool vertexFound = false;
          for (size_t mappedI = 0; mappedI < mapBuf[mhIndex].GetSize (); mappedI++)
          {
            MappingVertex mappedV = mapBuf[mhIndex][mappedI];
            bool testNorm = hasMeshNorm ? true : hasFaceNorm ? 
              (normal == csNormals[mappedV.csIdx]) : true;

            if (faceGroups[fgIdx].materialName == mappedV.material &&
                fabs (uv[0] - texcoords[mappedV.uvIdx][0]) < EPSILON &&
                fabs (uv[1] - texcoords[mappedV.uvIdx][1]) < EPSILON &&
                testNorm)
            {
              // Makehuman vertex is defined in CS with the same 
              // [material / UV coordinates / normal (if defined)]
              // => don't do anything
              vertexFound = true;
              if (!jointDef)
                csFace.Push (mappedV.csIdx);
              break;
            }
          }
        
          if (!vertexFound)
          {
            // Makehuman vertex is either not defined in CS or defined in CS with 
            // different [material / UV coordinates / normal (if defined)]
            // => create a new CS vertex
            MappingVertex mapv (csIndex, uvIndex, faceGroups[fgIdx].materialName);
            mapBuf[mhIndex].Push (mapv);
            csCoords.Push (co);
            uv[1] = 1.0f - uv[1];    // adapt UV coordinates to CS
            csTexcoords.Push (uv);
            csNormals.Push (normal);
            if (!jointDef)
              csFace.Push (csIndex);
            csIndex++;

            if (doubleSided)
            {
              MappingVertex mapv2 (csIndex, uvIndex, faceGroups[fgIdx].materialName);
              mapBuf[mhIndex].Push (mapv2);
              csCoords.Push (co);
              csTexcoords.Push (uv);
              csNormals.Push (-1.0f * normal);
              csIndex++;
            }
          }
        }
        else
        {
          // Without UV coordinates, there is a direct mapping between
          // Makehuman and CS vertices
          csCoords.Push (co);
          csNormals.Push (normal);
          if (!jointDef)
            csFace.Push (csIndex);
          csIndex++;

          if (doubleSided)
          {
            csCoords.Push (co);
            csNormals.Push (-1.0f * normal);
            csIndex++;
          }
        }
      }

      if (!jointDef)
      {
        // Copy face data into index buffer of CS submesh.
        // Makehuman faces have 3 or 4 vertices while the ones of Crystal Space 
        // are always triangles; a triangulation might thus be needed

        // first CS face respecting order of indices: [2, 1, 0]
        tri = csTriangle ((int) csFace[2], (int) csFace[1], (int) csFace[0]);
        csSubmesh.triangles.Push (tri);

        if (doubleSided)
        {
          // add back side of the face: [0, 1, 2]
          tri = csTriangle ((int) csFace[0] + 1, (int) csFace[1] + 1, (int) csFace[2] + 1);
          csSubmesh.triangles.Push (tri);
        }

        // second CS face respecting order of indices: [3, 2, 0]
        if (!triangle)
        {
          tri = csTriangle ((int) csFace[3], (int) csFace[2], (int) csFace[0]);
          csSubmesh.triangles.Push (tri);

          if (doubleSided)
          {
            // add back side of the second face: [3, 0, 2]
            tri = csTriangle ((int) csFace[3] + 1, (int) csFace[0] + 1, (int) csFace[2] + 1);
            csSubmesh.triangles.Push (tri);
          }
        }
      }
      
    }

    if (jointDef)
    {
      // Get joint name (slice prefix "joint-" from Makehuman group name)
      csString name (faceGroups[fgIdx].groupName.Slice (6));

      // Add parsed face group to the array of Makehuman joints
      mhJoints.Put (name, joint);
    }
    else
      // Add parsed submesh to the array of CS submeshes
      csSubmeshes.Push (csSubmesh);
  }

  // Copy mapping buffer between Makehuman and CS vertices; only keep what is useful 
  // for the parsing of Makehuman targets, rigs and clothes
  for (size_t i = 0; i < mapBuf.GetSize (); i++)
  {
    VertBuf buf;
    for (size_t vi = 0; vi < mapBuf[i].GetSize (); vi++)
    {
      MappingVertex vmap = mapBuf[i][vi];
      buf.vertices.Push (vmap.csIdx);
    }
    mappingBuf.Push (buf);
  }

  return true;
}

csPtr<CS::Mesh::iAnimatedMeshFactory> MakehumanCharacter::CreateAnimatedMesh
(const char* textureFile, 
 const csDirtyAccessArray<Submesh>& csSubmeshes)
{
  // Create an animated mesh factory
  csRef<iMeshObjectFactory> fact = manager->animeshType->NewFactory ();
  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFact =
    scfQueryInterfaceSafe<CS::Mesh::iAnimatedMeshFactory> (fact);
  
  if (!animeshFact)
  {
    ReportError ("Could not load the animesh object plugin!");
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
  }

  // Create mesh texture
  csString texName (textureFile);
  size_t idx1 = texName.FindLast ('/') + 1;
  size_t idx2 = texName.FindLast ('.');
  texName.Replace (texName.Slice (idx1, idx2 - idx1));
  csColor* transp = new csColor (0.0f, 0.0f, 0.0f);   // TODO: define correctly this parameter
  iTextureWrapper* texture = 
    manager->engine->CreateTexture (texName.GetData (), textureFile, transp, CS_TEXTURE_2D);
  texture->Register (manager->g3d->GetTextureManager ());

  // TODO: Define material of animesh factory
  /*
    csRef<iMaterialWrapper> material = 
    engine->CreateMaterial (csSubmeshes[0].materialName.GetData (), texture);
    if (!material)
    {
    ReportError ("Couldn't create material %s!", CS::Quote::Single (csSubmeshes[0].materialName.GetData ()));
    return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
    }
    animeshFact->SetMaterialWrapper (material);
  */

  // Initialize the render buffers
  csRef<iRenderBuffer> vertbuf;
  csRef<iRenderBuffer> texbuf;
  csRef<iRenderBuffer> normalbuf;

  // Copy parsed data into buffer of vertex coordinates
  size_t vertexCount = csCoords.GetSize ();
  if (vertexCount > 0)
  {
    vertbuf = csRenderBuffer::CreateRenderBuffer (vertexCount,
                                                  CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    vertbuf->CopyInto (csCoords.GetArray (), vertexCount);
    animeshFact->SetVertices (vertbuf);
  }

  // Copy parsed data into buffer of texture coordinates
  size_t texCount = csTexcoords.GetSize ();
  if (texCount > 0)
  {
    texbuf = csRenderBuffer::CreateRenderBuffer (texCount,
						 CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 2);
    texbuf->CopyInto (csTexcoords.GetArray (), texCount);
    animeshFact->SetTexCoords (texbuf);
  }

  // Copy parsed data into buffer of vertex coordinates
  if (normals.GetSize () > 0)
  {
    size_t normalCount = csNormals.GetSize ();
    normalbuf = csRenderBuffer::CreateRenderBuffer (normalCount,
                                                    CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    normalbuf->CopyInto (csNormals.GetArray (), normalCount);
    animeshFact->SetNormals (normalbuf);
  }
  else
  {
    // TODO: Calculate mesh normals
    printf ("WARNING: setting default normals to CS mesh\n");
    //animeshFact->ComputeNormals ();

    //*******************************
    // TODO: SUPPRESS TEMPORARY CODE
    size_t normalCount = csNormals.GetSize ();
    normalbuf = csRenderBuffer::CreateRenderBuffer
      (normalCount, CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);
    normalbuf->CopyInto (csNormals.GetArray (), normalCount);
    animeshFact->SetNormals (normalbuf);
    //*******************************
  }

  // Copy parsed data into submesh buffers
  for (size_t sub = 0; sub < csSubmeshes.GetSize (); sub++)
  {
    if (csSubmeshes[sub].subName.StartsWith ("helper-"))
      continue;

    size_t faceCount = csSubmeshes[sub].triangles.GetSize ();
    if (faceCount > 0)
    {
      csRef<iRenderBuffer> subbuf = csRenderBuffer::CreateIndexRenderBuffer
	(3 * faceCount, CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT, 0, vertexCount - 1);
      csRenderBufferLock<int> indices (subbuf);
      for (size_t t = 0; t < faceCount; t++)
      {
        csTriangle tri (csSubmeshes[sub].triangles[t]);
        *(indices++) = tri[0];
        *(indices++) = tri[1];
        *(indices++) = tri[2];
      }

      if (subbuf)
      {
        CS::Mesh::iAnimatedMeshSubMeshFactory* smf = 
          animeshFact->CreateSubMesh (subbuf, csSubmeshes[sub].subName.GetData (), true);
        
        // TODO: Create submesh texture and material
        csRef<iMaterialWrapper> material = 
          manager->engine->CreateMaterial (csSubmeshes[sub].materialName.GetData (), texture);
        if (!material)
        {
          ReportError ("Could not create material %s!", 
                  CS::Quote::Single (csSubmeshes[sub].materialName.GetData ()));
          return csPtr<CS::Mesh::iAnimatedMeshFactory> (nullptr);
        }

        smf->SetMaterial (material);
      }
    }
  }

  // Calculate mesh tangents
  animeshFact->ComputeTangents ();

  animeshFact->Invalidate ();

  return animeshFact;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
