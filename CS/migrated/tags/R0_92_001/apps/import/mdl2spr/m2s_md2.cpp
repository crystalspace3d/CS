/*
  Crystal Space Quake MDL/MD2 convertor
  Copyright (C) 1998 by Nathaniel Saint Martin <noote@bigfoot.com>
  Significant overhaul by Eric Sunshine <sunshine@sunshineco.com> in Feb 2000

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cssysdef.h"
#include "m2s_md2.h"
#include "cssys/csendian.h"

// This class was stolen from John Burkhardt's MD2 converter
// (see CS/libs/csparser/impexp/md2.cpp).
// helper class to manage the vertex mappings.  This class takes in
// index pairs--one indexes into an array of xyz coordinates, the other
// indexes into an array of texture coordinates--and remaps them into
// an array of combined xyz+texture coordinate vertices.  The final
// size of the combined vertex array will be equal to the number of
// unique xyz/texture coordinate index pairs used in the original array.
// This class is needed because the Quake2 MD2 file format uses
// indices into two separate coordinate arrays, one for xyz coordinates, one
// for texture coordinates.  Since CS uses a combined format, where every
// vertex contains xyz+texture coordinates bound together, we must build
// a set of vertices differently than the quake2 format.

Md2::md2vertexset::~md2vertexset()
{
  if (xyzcoordindices) delete [] xyzcoordindices;
  if (texcoordindices) delete [] texcoordindices;
}

// go from xyz_index and texcoord_index to a single cs vertex_index
short Md2::md2vertexset::get_csvertexindex(short xyzindex, short texindex)
{
  if (numindices == 0)
  {
    xyzcoordindices = new short[1]; texcoordindices = new short[1];
    xyzcoordindices[0] = xyzindex; texcoordindices[0] = texindex;
    return numindices++;
  }
  int csindex;
  for (csindex=0; csindex < numindices; csindex++)
  {
    if ((xyzcoordindices[csindex] == xyzindex) &&
       (texcoordindices[csindex] == texindex))
       return csindex;
  }
  // no match found, make another spot
  short *newxyzindices = new short[numindices+1];
  short *newtexindices = new short[numindices+1];
  for (csindex=0; csindex < numindices; csindex++)
  {
    newxyzindices[csindex] = xyzcoordindices[csindex];
    newtexindices[csindex] = texcoordindices[csindex];
  }
  delete [] xyzcoordindices;
  delete [] texcoordindices;
  newxyzindices[numindices] = xyzindex;
  newtexindices[numindices] = texindex;
  xyzcoordindices = newxyzindices;
  texcoordindices = newtexindices;
  return numindices++;
}

// go from a single cs vertex_index to the xyz,texcoord index pair
void Md2::md2vertexset::get_md2vertexmap(
  short csindex, short &xyzindex, short &texindex) const
{
  xyzindex = xyzcoordindices[csindex];
  texindex = texcoordindices[csindex];
}

bool Md2::IsFileMD2Model(const char* mdlfile)
{
  return superclass::CheckMagic(mdlfile, "IDP2");
}

Md2::Md2()
{
  Clear();
}

Md2::Md2(const char* mdlfile)
{
  Clear();
  ReadMDLFile(mdlfile);
}

Md2::~Md2()
{
  // @@@ FIXME: Unimplemented :-)
}

void Md2::Clear()
{
  nbskins = 0;
  skinheight = skinwidth = 0;
  skins = NULL;
  nbframes = 0;
  frames = NULL;
  nbtriangles = 0;
  triangles = NULL;
  nbvertices = 0;
  vertices = NULL;
  skinheight = skinwidth = 0;
}

bool Md2::ReadMDLFile(const char* mdlfile)
{
  FILE *f;
  int i, j;

  clearError();

  if (mdlfile == NULL || strlen(mdlfile) == 0)
    return setError("MD2 filename is NULL");

  if ((f = fopen(mdlfile, "rb")) == NULL)
    return setError("Cannot find mdl file");

  // read mdl header
  magic_t magic;
  if (fread(&magic, sizeof(magic_t), 1, f) != 1)
    return setError("Cannot read magic header", f);

  // check if is a correct version
  if (strncmp(magic.id, "IDP2", 4) != 0)
    return setError("Invalid mdl magic", f);

  // check if is a correct version
  magic.version = convert_endian (magic.version);
  if (magic.version != 8)
    return setError("Invalid mdl version", f);

  // read mdl header
  md2_t header;
  if (fread(&header, sizeof(md2_t), 1, f) != 1)
    return setError("Cannot read mdl header", f);
  header.skinwidth = convert_endian ((long)header.skinwidth);
  header.skinheight = convert_endian ((long)header.skinheight);
  header.framesize = convert_endian ((long)header.framesize);
  header.numskins = convert_endian ((long)header.numskins);
  header.numxyz = convert_endian ((long)header.numxyz);
  header.numverts = convert_endian ((long)header.numverts);
  header.numtris = convert_endian ((long)header.numtris);
  header.numglcmds = convert_endian ((long)header.numglcmds);
  header.numframes = convert_endian ((long)header.numframes);
  header.ofsskins = convert_endian ((long)header.ofsskins);
  header.ofsverts = convert_endian ((long)header.ofsverts);
  header.ofstris = convert_endian ((long)header.ofstris);
  header.ofsframes = convert_endian ((long)header.ofsframes);
  header.ofsglcmds = convert_endian ((long)header.ofsglcmds);
  header.ofsend = convert_endian ((long)header.ofsend);

  // skins ops
  fseek(f, header.ofsskins, SEEK_SET);
  skinheight = header.skinheight;
  skinwidth = header.skinwidth;
  nbskins = header.numskins;
  skins = new skin2_t[nbskins];
  memset(skins, 0, sizeof(skin2_t) * nbskins);
  for (i = 0; i < nbskins; i++)
  {
    memset(skins[i].name, 0, MD2_SKIN_NAME_MAX + 1);
    if (fread(skins[i].name, MD2_SKIN_NAME_MAX, 1, f) != 1)
      return setError("Error then reading mdl file", f);
  }

  // vertices ops
  fseek(f, header.ofsverts, SEEK_SET);
  nbvertices = header.numverts;
  vertices = new vertice2_t[nbvertices];
  memset(vertices, 0, sizeof(vertice2_t) * nbvertices);

  // read all vertices
  for (i = 0; i < nbvertices; i++)
  {
    if (fread(&vertices[i], sizeof(vertice2_t), 1, f) != 1)
      return setError("Error then reading mdl file", f);
    vertices[i].s = convert_endian (vertices[i].s);
    vertices[i].t = convert_endian (vertices[i].t);
  }

  // triangles ops
  fseek(f, header.ofstris, SEEK_SET);
  nbtriangles = header.numtris;
  triangle2_t* qtriangles = new triangle2_t[nbtriangles];
  memset(qtriangles, 0, sizeof(triangle2_t) * nbtriangles);
  // read all triangles
  for (i = 0; i < nbtriangles; i++)
  {
    if (fread(&qtriangles[i], sizeof(triangle2_t), 1, f) != 1)
      return setError("Error then reading mdl file", f);
    qtriangles[i].xyz[0] = convert_endian (qtriangles[i].xyz[0]);
    qtriangles[i].xyz[1] = convert_endian (qtriangles[i].xyz[1]);
    qtriangles[i].xyz[2] = convert_endian (qtriangles[i].xyz[2]);
    qtriangles[i].vertice[0] = convert_endian (qtriangles[i].vertice[0]);
    qtriangles[i].vertice[1] = convert_endian (qtriangles[i].vertice[1]);
    qtriangles[i].vertice[2] = convert_endian (qtriangles[i].vertice[2]);
  }

  // frames ops
  nbframes = header.numframes;
  frames = new frame2_t [nbframes];
  memset(frames, 0, sizeof(frame2_t) * nbframes);
  nbxyz = header.numxyz;
  // check all frames
  for (i = 0; i < nbframes; i++)
  {
    if (fread(&frames[i].scale, sizeof(vec3_t), 1, f) != 1)
      return setError("Error then reading mdl file", f);
    frames[i].scale.x = convert_endian (frames[i].scale.x);
    frames[i].scale.y = convert_endian (frames[i].scale.y);
    frames[i].scale.z = convert_endian (frames[i].scale.z);

    if (fread(&frames[i].translate, sizeof(vec3_t), 1, f) != 1)
      return setError("Error then reading mdl file", f);
    frames[i].translate.x = convert_endian (frames[i].translate.x);
    frames[i].translate.y = convert_endian (frames[i].translate.y);
    frames[i].translate.z = convert_endian (frames[i].translate.z);

    // name of frame
    memset(frames[i].name, 0, MD2_FRAME_NAME_MAX + 1);
    if (fread(frames[i].name, MD2_FRAME_NAME_MAX, 1, f) != 1)
      return setError("Error then reading mdl file", f);

    // frame vertices
    frames[i].trivert = new trivertx_t[nbxyz];
    if (fread(frames[i].trivert, sizeof(trivertx_t) * nbxyz, 1, f) != 1)
      return setError("Error then reading mdl file", f);
  }

  fclose(f);

  // This conversion was mostly stolen from John Burkhardt's MD2 converter
  // (see CS/libs/csparser/impexp/md2.cpp).
  // next we read in the triangle connectivity data.  This data describes
  // each triangle as three indices, referring to three numbered vertices.
  // This data is, like the skin texture coords, independent of frame number.
  // There are actually two set of indices in the original quake file;
  // one indexes into the xyz coordinate table, the other indexes into
  // the uv texture coordinate table.  Since CS uses only one table, not
  // two, we must merge the xyz and uv texture coordinates into one table.
  // this means generating a unique CS vertex for each (xyz, uv) pair
  // of indices in the original quake file.  The md2vertexset named
  // 'modelvertices' handles
  // all the bookkeeping, allocating new vertices as it needs to
  triangles = new short[nbtriangles * 3];
  short* pt = triangles;
  for (i = 0; i < nbtriangles; i++)
  {
    for (j = 0; j < 3; j++, pt++)
    {
      short xyzindex = qtriangles[i].xyz[j];
      short texindex = qtriangles[i].vertice[j];
      *pt = modelvertices.get_csvertexindex(xyzindex,texindex);
    }
  }

  return true;
}

void Md2::dumpstats(FILE* s) const
{
  fprintf(s, "\nQuake2 Model (MD2) Statistics:\n");
  fprintf(s, "Skins:       %d\n", nbskins);
  fprintf(s, "Frames:      %d\n", nbframes);
  fprintf(s, "Triangles:   %d\n", nbtriangles);
  fprintf(s, "XYZ points:  %d\n", nbxyz);
  fprintf(s, "UV points:   %d\n", nbvertices);
  fprintf(s, "CS vertices: %d\n", modelvertices.vertexindexcount());
}

bool Md2::WriteSPR(const char* spritename, float scaleMdl, int delayMdl,
  float positionMdlX, float positionMdlY, float positionMdlZ,
  bool actionNamingMdl, bool /*resizeSkin*/, int maxFrames) const
{
  FILE *f;
  char *spritefilename;
  int i = 0, j = 0, k = 0;

  if (spritename == NULL || strlen(spritename) == 0)
  {
    fprintf(stderr, "Unable to save: NULL sprite name\n");
    return false;
  }

  spritefilename = new char [strlen(spritename) + 5];
  strcpy(spritefilename, spritename);
  strcat(spritefilename, ".spr");

  char const* skinname = "skin";

  // generate skin texture
  if (nbskins > 0)
    skinname = skins[0].name;
  else
    fprintf(stderr,
      "Warning: no skin named in this model: using `%s' by default\n",
      skinname);

  if ((f = fopen(spritefilename, "w")) == NULL)
  {
    fprintf(stderr, "Cannot open sprite file %s for writing\n", spritename);
    return false;
  }

  // begin hard work now
  fprintf(f, "MESHOBJ '%s' (\n", spritename);
  fprintf(f, "\tPLUGIN ('crystalspace.mesh.loader.factory.sprite.3d')\n");
  fprintf(f, "\tPARAMS (\n");

  fprintf(f, "\t\tMATERIAL ('%s')\n", skinname);

  // extract only n frames if maxFrames was specified
  int outFrames = nbframes;
  if (maxFrames!=-1)
	outFrames = maxFrames;

  printf("Generate Frames\n");
  int const klim = modelvertices.vertexindexcount();
  for (i = 0; i < outFrames; i++)
  {
    fprintf(f, "\t\tFRAME '%s' (", frames[i].name);
    for (k = 0; k < klim; k++)
    {
      short xyzindex, texindex;
      modelvertices.get_md2vertexmap(k, xyzindex, texindex);
      trivertx_t const& trivert = frames[i].trivert[xyzindex];
      vertice2_t const& texvert = vertices[texindex];

      // it seem y and z are switched
      float x = (float) trivert.packedposition[0];
      float z = (float) trivert.packedposition[1];
      float y = (float) trivert.packedposition[2];
      float u = (float) texvert.s;
      float v = (float) texvert.t;
      
      x = ((x * frames[i].scale.x) + frames[i].translate.x) * scaleMdl;
      y = ((y * frames[i].scale.z) + frames[i].translate.z) * scaleMdl;
      z = ((z * frames[i].scale.y) + frames[i].translate.y) * scaleMdl;

      x += positionMdlX;
      y += positionMdlY;
      z += positionMdlZ;

      u = u / (float) skinwidth;
      v = v / (float) skinheight;
      
      fprintf(f, " V (%.3f,%.3f,%.3f:%.2f,%.2f)", x, y, z, u, v);
    }
    fprintf(f, " )\n");
  }

  if (actionNamingMdl)
  {
    printf("Generate Actions\n");
    for (i = 0; i < outFrames; i++)
    {
      if (!isdigit(frames[i].name[strlen(frames[i].name)-1]))
      {
        fprintf(f, "\t\tACTION '%s' ( F ('%s', %d) )\n",
	  frames[i].name, frames[i].name, delayMdl);
      }
      else
      {
        char name_action[64];
        memset(name_action, 0, 64);

        int base_action;
        for (j = strlen(frames[i].name)-1; j > 1; j--)
          if (!isdigit(frames[i].name[j]))
            break;
        base_action=j + 1;
        strncpy(name_action, frames[i].name, base_action);
        
        fprintf(f, "\t\tACTION '%s' (", name_action);

        fprintf(f, " F ('%s', %d)", frames[i].name, delayMdl);
        for (j=i + 1; j < outFrames; j++, i++)
        {
          char toy[64], toy2[64];
          bool scrash = false;

          strcpy(toy, name_action);
          strcat(toy, "%s");
          sscanf(frames[j].name, toy, &toy2);
          
          for (k = 0; k < (int)strlen(toy2); k++)
          {
            if (!isdigit(toy2[k]))
            {
              scrash = true;
              break;
            }
          }
          if (scrash)
	    break;
          
          if (strncmp(name_action, frames[j].name, base_action) == 0)
            fprintf(f, " F ('%s', %d)", frames[j].name, delayMdl);
          else
	    break;
        }
        fprintf(f, " )\n");
      }
    }
  }

  printf("Generate Triangles\n");
  short const* tri = triangles;
  for (i = 0; i < nbtriangles; i++, tri += 3)
    fprintf(f, "\t\tTRIANGLE (%d,%d,%d)\n", *tri, *(tri + 1), *(tri + 2));

  fprintf(f, "\t)\n)\n");
  fclose(f);
  return true;
}
