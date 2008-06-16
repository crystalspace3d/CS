/*
    Copyright (C) 2005 by Jorrit Tyberghein

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
#include "csqint.h"
#include "csqsqrt.h"

#include "csgeom/box.h"
#include "csgeom/bsptree.h"
#include "csgeom/frustum.h"
#include "csgeom/math.h"
#include "csgeom/math3d.h"
#include "csgeom/sphere.h"
#include "csgeom/trimesh.h"
#include "csgfx/normalmaptools.h"
#include "csgfx/renderbuffer.h"
#include "cstool/rviewclipper.h"
#include "csutil/csendian.h"
#include "csutil/csmd5.h"
#include "csutil/memfile.h"
#include "csutil/sysfunc.h"
#include "csutil/scfarray.h"
#include "iengine/camera.h"
#include "iengine/engine.h"
#include "iengine/light.h"
#include "iengine/material.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/rview.h"
#include "iengine/sector.h"
#include "iengine/shadows.h"
#include "igeom/clip2d.h"
#include "iutil/cache.h"
#include "iutil/databuff.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "iutil/strset.h"
#include "iutil/verbositymanager.h"
#include "iutil/cmdline.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/material.h"
#include "ivideo/rendermesh.h"
#include "cstool/vertexcompress.h"
#include "cstool/normalcalc.h"
#include "cstool/primitives.h"

#include "instmesh.h"



CS_IMPLEMENT_PLUGIN

CS_PLUGIN_NAMESPACE_BEGIN(InstMesh)
{

CS_LEAKGUARD_IMPLEMENT (csInstmeshMeshObject);
CS_LEAKGUARD_IMPLEMENT (csInstmeshMeshObject::RenderBufferAccessor);
CS_LEAKGUARD_IMPLEMENT (csInstmeshMeshObjectFactory);

csInstmeshMeshObject::csInstmeshMeshObject (csInstmeshMeshObjectFactory* factory) :
        scfImplementationType (this),
	pseudoDynInfo (29, 32),
	affecting_lights (29, 32)
{
  renderBufferAccessor.AttachNew (new RenderBufferAccessor (this));
  csInstmeshMeshObject::factory = factory;
  vc = factory->vc;
  logparent = 0;
  initialized = false;
  cur_movablenr = -1;
  material = 0;
  MixMode = 0;
  lit_fact_colors = 0;
  num_lit_fact_colors = 0;
  static_fact_colors = 0;
  do_lighting = true;
  do_manual_colors = false;
  base_color.red = 0;
  base_color.green = 0;
  base_color.blue = 0;
  current_lod = 1;
  current_features = 0;
  changenr = 0;
  do_shadows = true;
  do_shadow_rec = false;
  lighting_dirty = true;
  lighting_full_dirty = true;
  shadow_caps = false;

  dynamic_ambient_version = 0;

  bufferHolder.AttachNew (new csRenderBufferHolder);

  g3d = csQueryRegistry<iGraphics3D> (factory->object_reg);
  mesh_vertices_dirty_flag = true;
  mesh_texels_dirty_flag = true;
  mesh_normals_dirty_flag = true;
  mesh_colors_dirty_flag = true;
  mesh_triangle_dirty_flag = true;
  mesh_tangents_dirty_flag = true;

  object_bbox_valid = false;
}

csInstmeshMeshObject::~csInstmeshMeshObject ()
{
  delete[] lit_fact_colors;
  delete[] static_fact_colors;

  ClearPseudoDynLights ();
}

size_t csInstmeshMeshObject::max_instance_id = 0;

void csInstmeshMeshObject::CalculateInstanceArrays ()
{
  mesh_vertices_dirty_flag = true;
  mesh_texels_dirty_flag = true;
  mesh_normals_dirty_flag = true;
  mesh_colors_dirty_flag = true;
  mesh_triangle_dirty_flag = true;
  mesh_tangents_dirty_flag = true;

  vertex_buffer = 0;
  texel_buffer = 0;
  normal_buffer = 0;
  color_buffer = 0;
  index_buffer = 0;
  binormal_buffer = 0;
  tangent_buffer = 0;

  object_bbox_valid = false; // @@@ Think again? Do while managing instances!

  size_t fact_vt_len = factory->fact_vertices.GetSize ();
  size_t vt_len = fact_vt_len * instances.GetSize ();
  csVector3* fact_vertices = factory->fact_vertices.GetArray ();
  mesh_vertices.SetMinimalCapacity (vt_len);
  mesh_vertices.SetSize (0);
  csVector2* fact_texels = factory->fact_texels.GetArray ();
  mesh_texels.SetMinimalCapacity (vt_len);
  mesh_texels.SetSize (0);
  csVector3* fact_normals = factory->fact_normals.GetArray ();
  mesh_normals.SetMinimalCapacity (vt_len);
  mesh_normals.SetSize (0);
  csColor4* fact_colors = factory->fact_colors.GetArray ();
  mesh_colors.SetMinimalCapacity (vt_len);
  mesh_colors.SetSize (0);

  size_t fact_tri_len = factory->fact_triangles.GetSize ();
  size_t tri_len = fact_tri_len * instances.GetSize ();
  csTriangle* fact_triangles = factory->fact_triangles.GetArray ();
  mesh_triangles.SetMinimalCapacity (tri_len);
  mesh_triangles.SetSize (0);

  size_t i, idx;
  for (i = 0 ; i < instances.GetSize () ; i++)
  {
    // @@@ Do more optimal with array copy for texels and colors?
    const csReversibleTransform& tr = instances[i].transform;
    for (idx = 0 ; idx < fact_vt_len ; idx++)
    {
      mesh_vertices.Push (tr.This2Other (fact_vertices[idx]));
      mesh_texels.Push (fact_texels[idx]);
      mesh_normals.Push (tr.This2OtherRelative (fact_normals[idx]));
      mesh_colors.Push (fact_colors[idx]);
    }
    int mult = (int)(i * fact_vt_len);
    for (idx = 0 ; idx < fact_tri_len ; idx++)
    {
      csTriangle tri = fact_triangles[idx];
      tri.a += mult;
      tri.b += mult;
      tri.c += mult;
      mesh_triangles.Push (tri);
    }
  }
}

iMeshObjectFactory* csInstmeshMeshObject::GetFactory () const
{
  return (iMeshObjectFactory*)factory;
}

size_t csInstmeshMeshObject::AddInstance (const csReversibleTransform& trans)
{
  csInstance inst;
  inst.transform = trans;
  ++max_instance_id;
  inst.id = max_instance_id;
  size_t idx = instances.Push (inst);
  instances_hash.Put (max_instance_id, idx);
  initialized = false;
  changenr++;
  ShapeChanged ();
  return max_instance_id;
}

void csInstmeshMeshObject::UpdateInstancesHash ()
{
  instances_hash.Empty ();
  size_t i;
  for (i = 0 ; i < instances.GetSize () ; i++)
    instances_hash.Put (instances[i].id, i);
}

void csInstmeshMeshObject::RemoveInstance (size_t id)
{
  size_t idx = instances_hash.Get (id, csArrayItemNotFound);
  if (idx == csArrayItemNotFound) return;
  instances.DeleteIndexFast (idx);
  UpdateInstancesHash ();
  initialized = false;
  changenr++;
  ShapeChanged ();
}

void csInstmeshMeshObject::RemoveAllInstances ()
{
  instances.Empty ();
  instances_hash.Empty ();
  initialized = false;
  changenr++;
  ShapeChanged ();
}

void csInstmeshMeshObject::UpdateInstanceGeometry (size_t instance_idx)
{
  if (initialized)
  {
    csVector3* fact_vertices = factory->fact_vertices.GetArray ();
    csVector3* fact_normals = factory->fact_normals.GetArray ();
    size_t fact_vt_len = factory->fact_vertices.GetSize ();
    size_t v0_id = instance_idx * fact_vt_len;

    for (size_t i = 0; i <  fact_vt_len; i++)
    {
      mesh_vertices[v0_id + i] = 
        instances[instance_idx].transform.This2Other (fact_vertices[i]);
      mesh_normals[v0_id + i] = 
        instances[instance_idx].transform.This2OtherRelative (
	    fact_normals[i]);;
    }
  }
  mesh_vertices_dirty_flag = true;
  mesh_normals_dirty_flag = true;
  changenr++;
}

void csInstmeshMeshObject::MoveInstance (size_t id,
    const csReversibleTransform& trans)
{
  size_t idx = instances_hash.Get (id, csArrayItemNotFound);
  if (idx == csArrayItemNotFound) return;
  instances[idx].transform = trans;
  UpdateInstanceGeometry (idx);
  lighting_dirty = true;
  // Don't set lighting_full_dirty to true since we only moved a single
  // instance.
  instances[idx].lighting_dirty = true;

  if (object_bbox_valid)
  {
    // Check if we have to update the bounding box.
    csBox3 fact_box = factory->GetFactoryBox ();
    fact_box.SetCenter (trans.GetOrigin ());
    // @@@ We ignore the transform here. Assuming that in general
    // the instances will be small so there is little chance for
    // error here.
    if (!object_bbox.Contains (fact_box))
    {
      object_bbox += fact_box;
      ShapeChanged ();
    }
  }
}

const csReversibleTransform& csInstmeshMeshObject::GetInstanceTransform (
    size_t id)
{
  size_t idx = instances_hash.Get (id, csArrayItemNotFound);
  if (idx != csArrayItemNotFound)
    return instances[idx].transform;
  static csReversibleTransform dummy;
  return dummy;
}

void csInstmeshMeshObject::ClearPseudoDynLights ()
{
  csHash<csShadowArray*, csPtrKey<iLight> >::GlobalIterator it (
    pseudoDynInfo.GetIterator ());
  while (it.HasNext ())
  {
    csShadowArray* arr = it.Next ();
    delete arr;
  }
}

void csInstmeshMeshObject::CheckLitColors ()
{
  if (do_manual_colors) return;
  size_t numcol = factory->GetVertexCount () * instances.GetSize ();
  if (numcol != num_lit_fact_colors)
  {
    ClearPseudoDynLights ();

    num_lit_fact_colors = numcol;
    delete[] lit_fact_colors;
    lit_fact_colors = new csColor4 [num_lit_fact_colors];
    delete[] static_fact_colors;
    static_fact_colors = new csColor4 [num_lit_fact_colors];
  }
}

void csInstmeshMeshObject::InitializeDefault (bool clear)
{
  SetupObject ();

  if (!do_shadow_rec) return;
  if (do_manual_colors) return;

  // Set all colors to ambient light.
  size_t i;
  CheckLitColors ();
  if (clear)
  {
    //csColor amb;
    //factory->engine->GetAmbientLight (amb);
    for (i = 0 ; i < num_lit_fact_colors ; i++)
    {
      lit_fact_colors[i].Set (0, 0, 0);
      static_fact_colors[i].Set (0, 0, 0);
    }
  }
  lighting_dirty = true;
  lighting_full_dirty = true;
}

void csInstmeshMeshObject::CalculateBBoxRadius ()
{
  object_bbox_valid = true;
  if (instances.GetSize () == 0)
  {
    object_bbox.Set (0, 0, 0, 0, 0, 0);
    radius = 0.0f;
    return;
  }
  const csBox3& fact_box = factory->GetFactoryBox ();
  object_bbox = fact_box;
  csVector3 pos = instances[0].transform.GetOrigin ();
  object_bbox.SetCenter (pos);
  float max_sqradius = pos * pos;
  size_t i;
  for (i = 1 ; i < instances.GetSize () ; i++)
  {
    csBox3 transformed_box = fact_box;
    pos = instances[i].transform.GetOrigin ();
    transformed_box.SetCenter (pos);
    object_bbox += transformed_box;
    float sqradius = pos * pos;
    if (sqradius > max_sqradius) max_sqradius = sqradius;
  }

  radius = csQsqrt (max_sqradius);
}

float csInstmeshMeshObject::GetRadius ()
{
  SetupObject ();
  if (!object_bbox_valid) CalculateBBoxRadius ();
  return radius;
}

const csBox3& csInstmeshMeshObject::GetObjectBoundingBox ()
{
  SetupObject ();
  if (!object_bbox_valid) CalculateBBoxRadius ();
  return object_bbox;
}

void csInstmeshMeshObject::SetObjectBoundingBox (const csBox3& bbox)
{
  SetupObject ();
  object_bbox_valid = true;
  object_bbox = bbox;
}

char* csInstmeshMeshObject::GenerateCacheName ()
{
  csMemFile mf;
  mf.Write ("instmesh", 8);
  uint32 l;
  l = csLittleEndian::Convert ((uint32)factory->GetVertexCount ());
  mf.Write ((char*)&l, 4);
  l = csLittleEndian::Convert ((uint32)factory->GetTriangleCount ());
  mf.Write ((char*)&l, 4);

  if (logparent)
  {
    if (logparent->QueryObject ()->GetName ())
      mf.Write (logparent->QueryObject ()->GetName (),
        strlen (logparent->QueryObject ()->GetName ()));
    iMovable* movable = logparent->GetMovable ();
    iSector* sect = movable->GetSectors ()->Get (0);
    if (sect && sect->QueryObject ()->GetName ())
      mf.Write (sect->QueryObject ()->GetName (),
        strlen (sect->QueryObject ()->GetName ()));
  }

  csMD5::Digest digest = csMD5::Encode (mf.GetData (), mf.GetSize ());
  csString hex(digest.HexString());
  return hex.Detach();
}

const char CachedLightingMagic[] = "GmL1";
const int CachedLightingMagicSize = sizeof (CachedLightingMagic);

bool csInstmeshMeshObject::ReadFromCache (iCacheManager* cache_mgr)
{
  if (!do_shadow_rec) return true;
  SetupObject ();
  lighting_dirty = true;
  lighting_full_dirty = true;
  char* cachename = GenerateCacheName ();
  cache_mgr->SetCurrentScope (cachename);
  delete[] cachename;

  bool rc = false;
  csRef<iDataBuffer> db = cache_mgr->ReadCache ("genmesh_lm", 0, (uint32)~0);
  if (db)
  {
    csMemFile mf ((const char*)(db->GetData ()), db->GetSize ());
    char magic[CachedLightingMagicSize];
    if (mf.Read (magic, CachedLightingMagicSize - 1) != 4) goto stop;
    magic[CachedLightingMagicSize - 1] = 0;
    if (strcmp (magic, CachedLightingMagic) == 0)
    {
      size_t v;
      for (v = 0; v < num_lit_fact_colors; v++)
      {
	csColor4& c = static_fact_colors[v];
	uint8 b;
	if (mf.Read ((char*)&b, sizeof (b)) != sizeof (b)) goto stop;
	c.red = (float)b / (float)CS_NORMAL_LIGHT_LEVEL;
	if (mf.Read ((char*)&b, sizeof (b)) != sizeof (b)) goto stop;
	c.green = (float)b / (float)CS_NORMAL_LIGHT_LEVEL;
	if (mf.Read ((char*)&b, sizeof (b)) != sizeof (b)) goto stop;
	c.blue = (float)b / (float)CS_NORMAL_LIGHT_LEVEL;
      }

      uint8 c;
      if (mf.Read ((char*)&c, sizeof (c)) != sizeof (c)) goto stop;
      while (c != 0)
      {
	char lid[16];
	if (mf.Read (lid, 16) != 16) goto stop;
	iLight *l = factory->engine->FindLightID (lid);
	if (!l) goto stop;
	l->AddAffectedLightingInfo ((iLightingInfo*)this);

	csShadowArray* shadowArr = new csShadowArray();
	float* intensities = new float[num_lit_fact_colors];
	shadowArr->shadowmap = intensities;
	for (size_t n = 0; n < num_lit_fact_colors; n++)
	{
          uint8 b;
          if (mf.Read ((char*)&b, sizeof (b)) != sizeof (b))
          {
            delete shadowArr;
            goto stop;
          }
          intensities[n] = (float)b / (float)CS_NORMAL_LIGHT_LEVEL;
	}
	pseudoDynInfo.Put (l, shadowArr);

        if (mf.Read ((char*)&c, sizeof (c)) != sizeof (c)) goto stop;
      }
      rc = true;
    }
  }

stop:
  cache_mgr->SetCurrentScope (0);
  return rc;
}

bool csInstmeshMeshObject::WriteToCache (iCacheManager* cache_mgr)
{
  if (!do_shadow_rec) return true;
  char* cachename = GenerateCacheName ();
  cache_mgr->SetCurrentScope (cachename);
  delete[] cachename;

  bool rc = false;
  csMemFile mf;
  mf.Write (CachedLightingMagic, CachedLightingMagicSize - 1);
  for (size_t v = 0; v < num_lit_fact_colors; v++)
  {
    const csColor4& c = static_fact_colors[v];
    int i; uint8 b;

    i = csQint (c.red * (float)CS_NORMAL_LIGHT_LEVEL);
    if (i < 0) i = 0; if (i > 255) i = 255; b = i;
    mf.Write ((char*)&b, sizeof (b));

    i = csQint (c.green * (float)CS_NORMAL_LIGHT_LEVEL);
    if (i < 0) i = 0; if (i > 255) i = 255; b = i;
    mf.Write ((char*)&b, sizeof (b));

    i = csQint (c.blue * (float)CS_NORMAL_LIGHT_LEVEL);
    if (i < 0) i = 0; if (i > 255) i = 255; b = i;
    mf.Write ((char*)&b, sizeof (b));
  }
  uint8 c = 1;

  csHash<csShadowArray*, csPtrKey<iLight> >::GlobalIterator pdlIt (
    pseudoDynInfo.GetIterator ());
  while (pdlIt.HasNext ())
  {
    mf.Write ((char*)&c, sizeof (c));

    csPtrKey<iLight> l;
    csShadowArray* shadowArr = pdlIt.Next (l);
    const char* lid = l->GetLightID ();
    mf.Write ((char*)lid, 16);

    float* intensities = shadowArr->shadowmap;
    for (size_t n = 0; n < num_lit_fact_colors; n++)
    {
      int i; uint8 b;
      i = csQint (intensities[n] * (float)CS_NORMAL_LIGHT_LEVEL);
      if (i < 0) i = 0; if (i > 255) i = 255; b = i;
      mf.Write ((char*)&b, sizeof (b));
    }
  }
  c = 0;
  mf.Write ((char*)&c, sizeof (c));


  rc = cache_mgr->CacheData ((void*)(mf.GetData ()), mf.GetSize (),
    "genmesh_lm", 0, (uint32)~0);
  cache_mgr->SetCurrentScope (0);
  return rc;
}

void csInstmeshMeshObject::PrepareLighting ()
{
}

void csInstmeshMeshObject::LightChanged (iLight*)
{
  lighting_dirty = true;
  lighting_full_dirty = true;
}

void csInstmeshMeshObject::LightDisconnect (iLight* light)
{
  affecting_lights.Delete (light);
  lighting_dirty = true;
  lighting_full_dirty = true;
}

void csInstmeshMeshObject::DisconnectAllLights ()
{
  csSet<csPtrKey<iLight> >::GlobalIterator it = affecting_lights.
      	GetIterator ();
  while (it.HasNext ())
  {
    iLight* l = (iLight*)it.Next ();
    l->RemoveAffectedLightingInfo ((iLightingInfo*)this);
  }
  affecting_lights.Empty ();
  lighting_dirty = true;
  lighting_full_dirty = true;
}

#define SHADOW_CAST_BACKFACE

void csInstmeshMeshObject::AppendShadows (iMovable* movable,
    iShadowBlockList* shadows, const csVector3& origin)
{
  if (!do_shadows) return;
  size_t tri_num = factory->GetTriangleCount ();
  const csVector3* vt = factory->GetVertices ();
  size_t vt_num = factory->GetVertexCount ();
  const csVector3* vt_world, * vt_array_to_delete;
  size_t i;
  if (movable->IsFullTransformIdentity ())
  {
    vt_array_to_delete = 0;
    vt_world = vt;
  }
  else
  {
    vt_array_to_delete = new csVector3 [vt_num];
    vt_world = vt_array_to_delete;
    csReversibleTransform movtrans = movable->GetFullTransform ();
//@@@ FIXME
//    for (i = 0 ; i < vt_num ; i++)
//      vt_world[i] = movtrans.This2Other (vt[i]);
  }

  iShadowBlock *list = shadows->NewShadowBlock ((int)tri_num);
  csFrustum *frust;
  bool cw = true;                   //@@@ Use mirroring parameter here!
  const csTriangle* tri = factory->GetTriangles ();
  for (i = 0 ; i < tri_num ; i++, tri++)
  {
    csPlane3 pl (vt_world[tri->c], vt_world[tri->b], vt_world[tri->a]);
    //if (pl.VisibleFromPoint (origin) != cw) continue;
    float clas = pl.Classify (origin);
    if (ABS (clas) < EPSILON) continue;
#ifdef SHADOW_CAST_BACKFACE
    if ((clas < 0) == cw) continue;
#else
    if ((clas <= 0) != cw) continue;
#endif

    // Let the casted shadow appear with a tiny tiny offset...
    const csVector3 offs = csVector3 (pl.norm) * csVector3 (EPSILON);
    pl.DD += (origin + offs) * pl.norm;
#ifndef SHADOW_CAST_BACKFACE
    pl.Invert ();
#endif
    frust = list->AddShadow (origin, 0, 3, pl);
#ifdef SHADOW_CAST_BACKFACE
    frust->GetVertex (0).Set (vt_world[tri->c] - origin);
    frust->GetVertex (1).Set (vt_world[tri->b] - origin);
    frust->GetVertex (2).Set (vt_world[tri->a] - origin);
#else
    frust->GetVertex (0).Set (vt_world[tri->a] - origin);
    frust->GetVertex (1).Set (vt_world[tri->b] - origin);
    frust->GetVertex (2).Set (vt_world[tri->c] - origin);
#endif
  }

  delete[] vt_array_to_delete;
}

bool csInstmeshMeshObject::SetMaterialWrapper (iMaterialWrapper* mat)
{
  material = mat;
  iMaterialWrapper* mater = material;
  if (!mater) mater = factory->GetMaterialWrapper ();
  material_needs_visit = mater->IsVisitRequired ();
  return true;
}

void csInstmeshMeshObject::SetupShaderVariableContext ()
{
  uint bufferMask = (uint)CS_BUFFER_ALL_MASK;
  bufferHolder->SetAccessor (renderBufferAccessor, bufferMask);
}
  
void csInstmeshMeshObject::SetupObject ()
{
  if (!initialized)
  {
    initialized = true;
    CalculateInstanceArrays ();
    delete[] lit_fact_colors;
    lit_fact_colors = 0;
    delete[] static_fact_colors;
    static_fact_colors = 0;
    if (!do_manual_colors)
    {
      num_lit_fact_colors = factory->fact_vertices.GetSize ()
	* instances.GetSize ();
      lit_fact_colors = new csColor4 [num_lit_fact_colors];
      size_t i;
      for (i = 0 ; i <  num_lit_fact_colors; i++)
        lit_fact_colors[i].Set (0, 0, 0);
      lighting_dirty = true;
      lighting_full_dirty = true;
      static_fact_colors = new csColor4 [num_lit_fact_colors];
      for (i = 0 ; i <  num_lit_fact_colors; i++)
        //static_fact_colors[i] = base_color;	// Initialize to base color.
        static_fact_colors[i].Set (0, 0, 0);
    }
    iMaterialWrapper* mater = material;
    if (!mater) mater = factory->GetMaterialWrapper ();
    CS_ASSERT (mater != 0);
    material_needs_visit = mater->IsVisitRequired ();

    SetupShaderVariableContext ();
  }
}

#define VERTEX_OFFSET       (10.0f * SMALL_EPSILON)

/*
  Lighting w/o local shadows:
  - Contribution from all affecting lights is calculated and summed up
    at runtime.
  Lighting with local shadows:
  - Contribution from static lights is calculated, summed and stored.
  - For every static pseudo-dynamic lights, the intensity of contribution
    is stored.
  - At runtime, the static lighting colors are copied to the actual used
    colors, the intensities of the pseudo-dynamic lights are multiplied
    with the actual colors of that lights and added as well, and finally,
    dynamic lighst are calculated.
*/
void csInstmeshMeshObject::CastShadows (iMovable* movable, iFrustumView* fview)
{
  SetupObject ();

  if (do_manual_colors) return;
  if (!do_lighting) return;

  iBase* b = (iBase *)fview->GetUserdata ();
  csRef<iLightingProcessInfo> lpi = scfQueryInterface<iLightingProcessInfo> (b);
  CS_ASSERT (lpi != 0);

  iLight* li = lpi->GetLight ();
  bool dyn = lpi->IsDynamic ();

  if (!dyn)
  {
    if (!do_shadow_rec || li->GetDynamicType () == CS_LIGHT_DYNAMICTYPE_PSEUDO)
    {
      li->AddAffectedLightingInfo ((iLightingInfo*)this);
      if (li->GetDynamicType () != CS_LIGHT_DYNAMICTYPE_PSEUDO)
        affecting_lights.Add (li);
    }
  }
  else
  {
    if (!affecting_lights.In (li))
    {
      li->AddAffectedLightingInfo ((iLightingInfo*)this);
      affecting_lights.Add (li);
    }
    if (do_shadow_rec) return;
  }

  if (!do_shadow_rec) return;

  csReversibleTransform o2w (movable->GetFullTransform ());

  csFrustum *light_frustum = fview->GetFrustumContext ()->GetLightFrustum ();
  iShadowBlockList* shadows = fview->GetFrustumContext ()->GetShadows ();
  iShadowIterator* shadowIt = shadows->GetShadowIterator ();

  const csVector3* normals = factory->GetNormals ();
  const csVector3* vertices = factory->GetVertices ();
  csColor4* colors = static_fact_colors;
  // Compute light position in object coordinates
  csVector3 wor_light_pos = li->GetMovable ()->GetFullPosition ();
  csVector3 obj_light_pos = o2w.Other2This (wor_light_pos);

  bool pseudoDyn = li->GetDynamicType () == CS_LIGHT_DYNAMICTYPE_PSEUDO;
  csShadowArray* shadowArr = 0;
  if (pseudoDyn)
  {
    shadowArr = new csShadowArray ();
    pseudoDynInfo.Put (li, shadowArr);
    shadowArr->shadowmap = new float[factory->GetVertexCount ()];
    memset(shadowArr->shadowmap, 0, factory->GetVertexCount() * sizeof(float));
  }

  csColor light_color = li->GetColor () * (256. / CS_NORMAL_LIGHT_LEVEL);

  csColor col;
  size_t i;
  for (i = 0 ; i < factory->GetVertexCount () ; i++)
  {
    const csVector3& normal = normals[i];
#ifdef SHADOW_CAST_BACKFACE
    csVector3 v = o2w.This2Other (vertices[i]) - wor_light_pos;
#else
    /*
      A small fraction of the normal is added to prevent unwanted
      self-shadowing (due small inaccuracies, the tri(s) this vertex
      lies on may shadow it.)
     */
    csVector3 v = o2w.This2Other (vertices[i] + (normal * VERTEX_OFFSET)) -
      wor_light_pos;
    /*csVector3 vN (v); vN.Normalize();
    v -= (vN * 0.1f);*/
#endif

    if (!light_frustum->Contains (v))
    {
      continue;
    }
    
    float vrt_sq_dist = csSquaredDist::PointPoint (obj_light_pos,
      vertices[i]);
    if (vrt_sq_dist >= csSquare (li->GetCutoffDistance ())) continue;
    
    bool inShadow = false;
    shadowIt->Reset ();
    while (shadowIt->HasNext ())
    {
      csFrustum* shadowFrust = shadowIt->Next ();
      if (shadowFrust->Contains (v))
      {
	inShadow = true;
	break;
      }
    }
    if (inShadow) continue;

    float in_vrt_dist =
      (vrt_sq_dist >= SMALL_EPSILON) ? csQisqrt (vrt_sq_dist) : 1.0f;

    float cosinus;
    if (vrt_sq_dist < SMALL_EPSILON) cosinus = 1;
    else cosinus = (obj_light_pos - vertices[i]) * normal;
    // because the vector from the object center to the light center
    // in object space is equal to the position of the light

    if (cosinus > 0)
    {
      if (vrt_sq_dist >= SMALL_EPSILON) cosinus *= in_vrt_dist;
      float bright = li->GetBrightnessAtDistance (csQsqrt (vrt_sq_dist));
      if (cosinus < 1) bright *= cosinus;
      if (pseudoDyn)
      {
	// Pseudo-dynamic
	if (bright > 2.0f) bright = 2.0f; // @@@ clamp here?
	shadowArr->shadowmap[i] = bright;
      }
      else
      {
	col = light_color * bright;
	colors[i] += col;
      }
    }
  }
}

void csInstmeshMeshObject::UpdateLightingOne (
  const csReversibleTransform& trans, iLight* li)
{
  const csVector3* normals = mesh_normals.GetArray ();
  csColor4* colors = lit_fact_colors;
  // Compute light position in object coordinates
  csVector3 wor_light_pos = li->GetMovable ()->GetFullPosition ();
  const csColor& fixed_light_color = li->GetColor ();
  float sq_cutoff_distance = csSquare (li->GetCutoffDistance ());

  size_t fact_vt_len = factory->fact_vertices.GetSize ();

  size_t idx;
  for (idx = 0 ; idx < instances.GetSize () ; idx++)
  {
    if (!(lighting_full_dirty || instances[idx].lighting_dirty))
    {
      instances[idx].lighting_dirty = false;
      continue;
    }
    instances[idx].lighting_dirty = false;

    csReversibleTransform transform = trans * instances[idx].transform;
    csVector3 obj_light_pos = transform.Other2This (wor_light_pos);
    float obj_sq_dist = obj_light_pos * obj_light_pos;
    if (obj_sq_dist >= sq_cutoff_distance) continue;
    float in_obj_dist =
      (obj_sq_dist >= SMALL_EPSILON) ? csQisqrt (obj_sq_dist) : 1.0f;

    csColor light_color = fixed_light_color * (256. / CS_NORMAL_LIGHT_LEVEL)
        * li->GetBrightnessAtDistance (csQsqrt (obj_sq_dist));
    if (light_color.red < EPSILON && light_color.green < EPSILON
  	  && light_color.blue < EPSILON)
      continue;

    size_t v0_id = idx * fact_vt_len;

    csColor col;
    size_t i;
    if (obj_sq_dist < SMALL_EPSILON)
    {
      for (i = v0_id ; i < v0_id + fact_vt_len ; i++)
        colors[i] += light_color;
    }
    else
    {
      obj_light_pos *= in_obj_dist;
      for (i = v0_id ; i < v0_id + fact_vt_len ; i++)
      {
	// @@@ normals[i] is transformed already! We might need to undo that transform
	// in a relative way. Think!
        float cosinus = obj_light_pos * normals[i];

        if (cosinus > 0)
        {
          col = light_color;
          if (cosinus < 1) col *= cosinus;
          colors[i] += col;
        }
      }
    }
  }
}

/*
Rules for color calculation:
  EAmb = Static Engine Ambient
  SAmb = Dynamic Sector Ambient
  BC   = Base Color (base_color)
  FC   = Color Array from factory
  SC   = Static Color Array (static_fact_colors)
  LC   = Colors calculated from all relevant lights
  LDC  = Colors calculated from dynamic lights only
  C    = Final Color Array (lit_fact_colors)

  sr   = do_shadow_rec flag
  l    = lighting flag
  mc   = manual colors flag

  sr   mc   l    formula
  ----------------------
  *    1    *    C[i] = FC[i]
  *    0    0    C[i] = BC+FC[i]
  1    0    1    C[i] = BC+SC[i]+EAmb+SAmb+FC[i]+LDC[i]
  0    0    1    C[i] = BC+LC[i]+EAmb+SAmb+FC[i]
*/

void csInstmeshMeshObject::UpdateLighting (
  const csSafeCopyArray<csLightInfluence>& lights,
  iMovable* movable)
{
  size_t i;
  if (cur_movablenr != movable->GetUpdateNumber ())
  {
    lighting_dirty = true;
    lighting_full_dirty = true;
    cur_movablenr = movable->GetUpdateNumber ();
  }

  if (factory->DoFullBright ())
  {
    size_t numcol = factory->GetVertexCount () * instances.GetSize ();
    lighting_dirty = false;
    lighting_full_dirty = false;
    for (i = 0 ; i < numcol ; i++)
    {
      lit_fact_colors[i].Set (1, 1, 1);
    }
    return;
  }

  if (do_manual_colors) return;

  const csColor4* colors_ptr = mesh_colors.GetArray ();

  if (do_lighting)
  {
    if (!lighting_dirty)
    {
      iSector* sect = movable->GetSectors ()->Get (0);
      if (dynamic_ambient_version == sect->GetDynamicAmbientVersion ())
        return;
      dynamic_ambient_version = sect->GetDynamicAmbientVersion ();
    }
    lighting_dirty = false;
    mesh_colors_dirty_flag = true;

    csColor4 col;
    if (factory->engine)
    {
      factory->engine->GetAmbientLight (col);
      col += base_color;
      iSector* sect = movable->GetSectors ()->Get (0);
      if (sect)
        col += sect->GetDynamicAmbientLight ();
    }
    else
    {
      col = base_color;
    }

    if (lighting_full_dirty)
    {
      size_t numcol = factory->GetVertexCount () * instances.GetSize ();
      for (i = 0 ; i < numcol ; i++)
        lit_fact_colors[i] = col + static_fact_colors[i] + colors_ptr[i];
    }
    else
    {
      size_t fact_vt_len = factory->fact_vertices.GetSize ();
      size_t idx;
      for (idx = 0 ; idx < instances.GetSize () ; idx++)
	if (instances[idx].lighting_dirty)
	{
          size_t v0_id = idx * fact_vt_len;
          for (i = v0_id ; i < v0_id + fact_vt_len ; i++)
            lit_fact_colors[i] = col + static_fact_colors[i] + colors_ptr[i];
	}
    }

    if (do_shadow_rec)
    {
      csReversibleTransform trans = movable->GetFullTransform ();
      csSet<csPtrKey<iLight> >::GlobalIterator it = affecting_lights.
      	GetIterator ();
      while (it.HasNext ())
      {
        iLight* l = (iLight*)it.Next ();
        UpdateLightingOne (trans, l);
      }
      csHash<csShadowArray*, csPtrKey<iLight> >::GlobalIterator pdlIt =
        pseudoDynInfo.GetIterator ();
      while (pdlIt.HasNext ())
      {
        csPtrKey<iLight> l;
        csShadowArray* shadowArr = pdlIt.Next (l);
        csColor c = l->GetColor ();
        if (c.red > EPSILON || c.green > EPSILON || c.blue > EPSILON)
        {
          c = c * (256. / CS_NORMAL_LIGHT_LEVEL);
          float* intensities = shadowArr->shadowmap;
          for (size_t i = 0; i < num_lit_fact_colors; i++)
          {
            lit_fact_colors[i] += c * intensities[i];
          }
        }
      }
    }
    else
    {
      // Do the lighting.
      csReversibleTransform trans = movable->GetFullTransform ();
      // the object center in world coordinates. "0" because the object
      // center in object space is obviously at (0,0,0).
      size_t num_lights = lights.GetSize ();
      for (size_t l = 0 ; l < num_lights ; l++)
      {
        iLight* li = lights[l].light;
        if (!li)
          continue;

        li->AddAffectedLightingInfo ((iLightingInfo*)this);
        affecting_lights.Add (li);
        UpdateLightingOne (trans, li);
      }
    }
    // @@@ Try to avoid this loop!
    // Clamp all vertex colors to 2.
    if (lighting_full_dirty)
    {
      size_t numcol = factory->GetVertexCount () * instances.GetSize ();
      for (i = 0 ; i < numcol ; i++)
        lit_fact_colors[i].Clamp (2., 2., 2.);
    }
    else
    {
      size_t fact_vt_len = factory->fact_vertices.GetSize ();
      size_t idx;
      for (idx = 0 ; idx < instances.GetSize () ; idx++)
	if (instances[idx].lighting_dirty)
	{
          size_t v0_id = idx * fact_vt_len;
          for (i = v0_id ; i < v0_id + fact_vt_len ; i++)
            lit_fact_colors[i].Clamp (2., 2., 2.);
	}
    }

    lighting_full_dirty = false;
  }
  else
  {
    if (!lighting_dirty)
      return;
    lighting_dirty = false;
    lighting_full_dirty = false;
    mesh_colors_dirty_flag = true;

    size_t numcol = factory->GetVertexCount () * instances.GetSize ();
    for (i = 0 ; i < numcol ; i++)
    {
      lit_fact_colors[i] = base_color + colors_ptr[i];
      lit_fact_colors[i].Clamp (2., 2., 2.);
    }
  }
}

csRenderMesh** csInstmeshMeshObject::GetRenderMeshes (
	int& n, iRenderView* rview, 
	iMovable* movable, uint32 frustum_mask)
{
  CheckLitColors ();
  SetupObject ();


  n = 0;

  if (mesh_triangles.GetSize () > 0)
  {
    iCamera* camera = rview->GetCamera ();

    int clip_portal, clip_plane, clip_z_plane;
    CS::RenderViewClipper::CalculateClipSettings (rview->GetRenderContext (),
	frustum_mask, clip_portal, clip_plane, clip_z_plane);

    lighting_movable = movable;

    if (!do_manual_colors && !do_shadow_rec && factory->light_mgr)
    {
      // Remember relevant lights for later.
      scfArrayWrap<iLightInfluenceArray, csSafeCopyArray<csLightInfluence> > 
        relevantLightsWrap (relevant_lights); //Yes, know, its on the stack...

      factory->light_mgr->GetRelevantLights (logparent, &relevantLightsWrap, -1);
    }

    const csReversibleTransform o2wt = movable->GetFullTransform ();
    const csVector3& wo = o2wt.GetOrigin ();

    // Array still needed?@@@
    {
      renderMeshes.SetSize (1);

      iMaterialWrapper* mater = material;
      if (!mater) mater = factory->GetMaterialWrapper ();
      if (!mater)
      {
        csPrintf ("INTERNAL ERROR: mesh used without material!\n");
        return 0;
      }

      if (mater->IsVisitRequired ()) mater->Visit ();

      bool rmCreated;
      csRenderMesh*& meshPtr = rmHolder.GetUnusedMesh (rmCreated,
        rview->GetCurrentFrameNumber ());

      meshPtr->mixmode = CS_MIXMODE_ALPHATEST_ENABLE;
      meshPtr->clip_portal = clip_portal;
      meshPtr->clip_plane = clip_plane;
      meshPtr->clip_z_plane = clip_z_plane;
      meshPtr->do_mirror = camera->IsMirrored ();
      meshPtr->meshtype = CS_MESHTYPE_TRIANGLES;
      meshPtr->indexstart = 0;
      meshPtr->indexend = (unsigned int)(mesh_triangles.GetSize () * 3);
      meshPtr->material = mater;
      CS_ASSERT (mater != 0);
      meshPtr->worldspace_origin = wo;
      meshPtr->buffers = bufferHolder;
      meshPtr->geometryInstance = (void*)factory;
      meshPtr->object2world = o2wt;

      renderMeshes[0] = meshPtr;
    }

    n = (int)renderMeshes.GetSize ();
    return renderMeshes.GetArray ();
  }
  else 
    return 0;
}

void csInstmeshMeshObject::GetRadius (float& rad, csVector3& cent)
{
  rad = GetRadius ();
  cent = object_bbox.GetCenter ();
}

bool csInstmeshMeshObject::HitBeamOutline (const csVector3& start,
  const csVector3& end, csVector3& isect, float* pr)
{
  // This is now closer to an outline hitting method. It will
  // return as soon as it touches any triangle in the mesh, and
  // will be a bit faster than its more accurate cousin (below).

  csSegment3 seg (start, end);
  size_t i, max = factory->GetTriangleCount();
  const csTriangle *tr = factory->GetTriangles();
  const csVector3 *vrt = factory->GetVertices ();
  for (i = 0 ; i < max ; i++)
  {
    if (csIntersect3::SegmentTriangle (seg, vrt[tr[i].a], vrt[tr[i].b],
        vrt[tr[i].c], isect))
    {
      if (pr) *pr = csQsqrt (csSquaredDist::PointPoint (start, isect) /
        csSquaredDist::PointPoint (start, end));

      return true;
    }
  }
  return false;
}

bool csInstmeshMeshObject::HitBeamObject (const csVector3& start,
  const csVector3& end, csVector3& isect, float *pr, int* polygon_idx,
  iMaterialWrapper** material)
{
  if (polygon_idx) *polygon_idx = -1;
  // This is the slow version. Use for an accurate hit on the object.
  // It will cycle through every triangle in the mesh serching for the
  // closest intersection. Slower, but returns the closest hit.
  // Usage is optional.

  csSegment3 seg (start, end);
  size_t i, max = factory->GetTriangleCount();
  float tot_dist = csSquaredDist::PointPoint (start, end);
  float dist, temp;
  float itot_dist = 1 / tot_dist;
  dist = temp = tot_dist;
  const csVector3 *vrt = factory->GetVertices ();
  csVector3 tmp;
  const csTriangle *tr = factory->GetTriangles();
  for (i = 0 ; i < max ; i++)
  {
    if (csIntersect3::SegmentTriangle (seg, vrt[tr[i].a], vrt[tr[i].b],
        vrt[tr[i].c], tmp))
    {
      temp = csSquaredDist::PointPoint (start, tmp);
      if (temp < dist)
      {
        isect = tmp;
	dist = temp;
	if (polygon_idx) *polygon_idx = (int)i;
      }
    }
  }
  if (pr) *pr = csQsqrt (dist * itot_dist);
  if (dist >= tot_dist)
    return false;

  if (material)
  {
    // @@@ Submeshes not yet supported!
    //const csPDelArray<csInstmeshSubMesh>& sm = subMeshes.GetSize () == 0
    	//? factory->GetSubMeshes ()
	//: subMeshes;
    //if (sm.GetSize () == 0)
    //{
      *material = csInstmeshMeshObject::material;
      if (!*material) *material = factory->GetMaterialWrapper ();
    //}
  }

  return true;
}

size_t csInstmeshMeshObject::TriMesh::GetVertexCount ()
{
  return parent->factory->GetVertexCount ();
}

csVector3* csInstmeshMeshObject::TriMesh::GetVertices ()
{
  //@@@FIXME: data must come from mesh itself. Not factory
  return 0;
  //return scfParent->factory->GetVertices ();
}

size_t csInstmeshMeshObject::TriMesh::GetTriangleCount ()
{
  //@@@FIXME: data from mesh instead of factory
  return 0;
  //return scfParent->factory->GetTriangleCount ();
}

csTriangle* csInstmeshMeshObject::TriMesh::GetTriangles ()
{
  //@@@FIXME: data from mesh instead of factory
  return 0;
  //return scfParent->factory->GetTriangles ();
}

void csInstmeshMeshObject::PreGetBuffer (csRenderBufferHolder* holder, 
					csRenderBufferName buffer)
{
  if (!holder) return;

  if (buffer == CS_BUFFER_COLOR)
  {
    if (!do_manual_colors)
    {
      UpdateLighting (relevant_lights, lighting_movable);
    }
    if (mesh_colors_dirty_flag)
    {
      mesh_colors_dirty_flag = false;
      if (!do_manual_colors)
      {
        if (!color_buffer ||
          (color_buffer->GetSize() != (sizeof (csColor4) * 
          num_lit_fact_colors)))
        {
          // Recreate the render buffer only if the new data cannot fit inside
          // the existing buffer.
          color_buffer = csRenderBuffer::CreateRenderBuffer (
            num_lit_fact_colors, 
            do_lighting ? CS_BUF_DYNAMIC : CS_BUF_STATIC,
            CS_BUFCOMP_FLOAT, 4);
        }
	color_buffer->SetData (lit_fact_colors);
      }
      else
      {
	size_t numcol = factory->fact_vertices.GetSize () * instances.GetSize ();
        if (!color_buffer ||
          (color_buffer->GetSize() != (sizeof (csColor4) * numcol)))
        {
          // Recreate the render buffer only if the new data cannot fit inside
          // the existing buffer.
          color_buffer = csRenderBuffer::CreateRenderBuffer (
            numcol, CS_BUF_STATIC, CS_BUFCOMP_FLOAT, 4);
        }
	color_buffer->SetData (factory->GetColors ());
      }
    }
    holder->SetRenderBuffer (buffer, color_buffer);
    return;
  }
  else if (buffer == CS_BUFFER_POSITION)
  {
    if (mesh_vertices_dirty_flag)
    {
      if (!vertex_buffer)
        vertex_buffer = csRenderBuffer::CreateRenderBuffer (
          mesh_vertices.GetSize (), CS_BUF_STATIC,
          CS_BUFCOMP_FLOAT, 3);
      mesh_vertices_dirty_flag = false;
      vertex_buffer->SetData ((void*)mesh_vertices.GetArray ());
    }
    holder->SetRenderBuffer (buffer, vertex_buffer);
    return;
  }
  else if (buffer == CS_BUFFER_TEXCOORD0) 
  {
    if (mesh_texels_dirty_flag)
    {
      if (!texel_buffer)
        texel_buffer = csRenderBuffer::CreateRenderBuffer (
          mesh_texels.GetSize (), CS_BUF_STATIC,
          CS_BUFCOMP_FLOAT, 2);
      mesh_texels_dirty_flag = false;
      texel_buffer->SetData ((void*)mesh_texels.GetArray ());
    }
    holder->SetRenderBuffer (buffer, texel_buffer);
    return;
  }
  else if (buffer == CS_BUFFER_NORMAL)
  {
    if (mesh_normals_dirty_flag)
    {
      if (!normal_buffer)
        normal_buffer = csRenderBuffer::CreateRenderBuffer (
          mesh_normals.GetSize (), CS_BUF_STATIC,
          CS_BUFCOMP_FLOAT, 3);
      mesh_normals_dirty_flag = false;
      normal_buffer->SetData ((void*)mesh_normals.GetArray ());
    }
    holder->SetRenderBuffer (buffer, normal_buffer);
    return;
  }
  else if (buffer == CS_BUFFER_TANGENT || buffer == CS_BUFFER_BINORMAL) 
  {
    if (mesh_tangents_dirty_flag)
    {
      if (!tangent_buffer)
        tangent_buffer = csRenderBuffer::CreateRenderBuffer (
          mesh_vertices.GetSize (), CS_BUF_STATIC,
          CS_BUFCOMP_FLOAT, 3);
      if (!binormal_buffer)
        binormal_buffer = csRenderBuffer::CreateRenderBuffer (
          mesh_vertices.GetSize (), CS_BUF_STATIC,
          CS_BUFCOMP_FLOAT, 3);
      mesh_tangents_dirty_flag = false;

      csVector3* tangentData = new csVector3[mesh_vertices.GetSize () * 2];
      csVector3* bitangentData = tangentData + mesh_vertices.GetSize ();
      csNormalMappingTools::CalculateTangents (mesh_triangles.GetSize (), 
        mesh_triangles.GetArray (), mesh_vertices.GetSize (),
	mesh_vertices.GetArray (), mesh_normals.GetArray (), 
        mesh_texels.GetArray (), tangentData, bitangentData);

      tangent_buffer->CopyInto (tangentData, mesh_vertices.GetSize ());
      binormal_buffer->CopyInto (bitangentData, mesh_vertices.GetSize ());

      delete[] tangentData;
    }
    holder->SetRenderBuffer (buffer, (buffer == CS_BUFFER_TANGENT) ?
      tangent_buffer : binormal_buffer);
    return;
  }
  else if (buffer == CS_BUFFER_INDEX)
  {
    if (mesh_triangle_dirty_flag)
    {
      if (!index_buffer)
        index_buffer = csRenderBuffer::CreateIndexRenderBuffer (
          mesh_triangles.GetSize ()*3, CS_BUF_STATIC,
          CS_BUFCOMP_UNSIGNED_INT, 0, mesh_vertices.GetSize () - 1);
      mesh_triangle_dirty_flag = false;
      index_buffer->SetData ((void*)mesh_triangles.GetArray ());
    }
    holder->SetRenderBuffer (buffer, index_buffer);
    return;
  }
}

//----------------------------------------------------------------------

csInstmeshMeshObjectFactory::csInstmeshMeshObjectFactory (
  iMeshObjectType *pParent, iObjectRegistry* object_reg) : 
  scfImplementationType (this, (iBase*)pParent)
{
  csInstmeshMeshObjectFactory::object_reg = object_reg;

  logparent = 0;
  instmesh_type = pParent;

  material = 0;
  light_mgr = csQueryRegistry<iLightManager> (object_reg);

  g3d = csQueryRegistry<iGraphics3D> (object_reg);

  autonormals = false;
  autonormals_compress = true;
  factory_bbox_valid = false;

  default_mixmode = 0;
  default_lighting = true;
  default_color.Set (0, 0, 0);
  default_manualcolors = false;
  default_shadowcasting = true;
  default_shadowreceiving = false;

  csRef<iEngine> eng = csQueryRegistry<iEngine> (object_reg);
  engine = eng; // We don't want a circular reference!

  vc = csQueryRegistry<iVirtualClock> (object_reg);

  csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser> (
  	object_reg);
  do_fullbright = (cmdline->GetOption ("fullbright") != 0);
}

csInstmeshMeshObjectFactory::~csInstmeshMeshObjectFactory ()
{
}

void csInstmeshMeshObjectFactory::CalculateBoundingVolumes ()
{
  if (factory_bbox_valid) return;
  factory_bbox_valid = true;
  size_t i;
  factory_bbox.StartBoundingBox (fact_vertices[0]);
  factory_radius = csQsqrt (fact_vertices[0] * fact_vertices[0]);
  for (i = 0 ; i < fact_vertices.GetSize () ; i++)
  {
    const csVector3& v = fact_vertices[i];
    factory_bbox.AddBoundingVertexSmart (v);
    float rad = csQsqrt (v * v);
    if (rad > factory_radius) factory_radius = rad;
  }
}

void csInstmeshMeshObjectFactory::AddVertex (const csVector3& v,
      const csVector2& uv, const csVector3& normal,
      const csColor4& color)
{
  if (fact_vertices.GetSize () == 0)
  {
    factory_bbox.StartBoundingBox (v);
    factory_radius = csQsqrt (v * v);
  }
  else
  {
    factory_bbox.AddBoundingVertexSmart (v);
    float rad = csQsqrt (v * v);
    if (rad > factory_radius) factory_radius = rad;
  }
  fact_vertices.Push (v);
  fact_texels.Push (uv);
  fact_normals.Push (normal);
  fact_colors.Push (color);
}

void csInstmeshMeshObjectFactory::Compress ()
{
  //size_t old_num = fact_vertices.GetSize ();
  csCompressVertexInfo* vt = csVertexCompressor::Compress (
    	fact_vertices, fact_texels, fact_normals, fact_colors);
  if (vt)
  {
    //printf ("From %d to %d\n", int (old_num), int (fact_vertices.GetSize ()));
    //fflush (stdout);

    // Now we can remap the vertices in all triangles.
    size_t i;
    for (i = 0 ; i < fact_triangles.GetSize () ; i++)
    {
      fact_triangles[i].a = (int)vt[fact_triangles[i].a].new_idx;
      fact_triangles[i].b = (int)vt[fact_triangles[i].b].new_idx;
      fact_triangles[i].c = (int)vt[fact_triangles[i].c].new_idx;
    }
    delete[] vt;
  }
}

void csInstmeshMeshObjectFactory::CalculateNormals (bool compress)
{
  csNormalCalculator::CalculateNormals (
      fact_vertices, fact_triangles, fact_normals, compress);
  autonormals = true;
  autonormals_compress = compress;
}

void csInstmeshMeshObjectFactory::HardTransform (
    const csReversibleTransform& t)
{
  size_t i;
  for (i = 0 ; i < fact_vertices.GetSize () ; i++)
  {
    fact_vertices[i] = t.This2Other (fact_vertices[i]);
    fact_normals[i] = t.This2OtherRelative (fact_normals[i]);
  }
  factory_bbox_valid = false;
}

csPtr<iMeshObject> csInstmeshMeshObjectFactory::NewInstance ()
{
  csInstmeshMeshObject* cm = new csInstmeshMeshObject (this);
  cm->SetMixMode (default_mixmode);
  cm->SetLighting (default_lighting);
  cm->SetColor (default_color);
  cm->SetManualColors (default_manualcolors);
  cm->SetShadowCasting (default_shadowcasting);
  cm->SetShadowReceiving (default_shadowreceiving);

  csRef<iMeshObject> im (scfQueryInterface<iMeshObject> (cm));
  cm->DecRef ();
  return csPtr<iMeshObject> (im);
}

//----------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (csInstmeshMeshObjectType)


csInstmeshMeshObjectType::csInstmeshMeshObjectType (iBase* pParent) :
  scfImplementationType (this, pParent)
{
  do_verbose = false;
}

csInstmeshMeshObjectType::~csInstmeshMeshObjectType ()
{
}

csPtr<iMeshObjectFactory> csInstmeshMeshObjectType::NewFactory ()
{
  csRef<csInstmeshMeshObjectFactory> cm;
  cm.AttachNew (new csInstmeshMeshObjectFactory (this,
    object_reg));
  csRef<iMeshObjectFactory> ifact (
    scfQueryInterface<iMeshObjectFactory> (cm));
  return csPtr<iMeshObjectFactory> (ifact);
}

bool csInstmeshMeshObjectType::Initialize (iObjectRegistry* object_reg)
{
  csInstmeshMeshObjectType::object_reg = object_reg;

  csRef<iVerbosityManager> verbosemgr (
    csQueryRegistry<iVerbosityManager> (object_reg));
  if (verbosemgr) 
    do_verbose = verbosemgr->Enabled ("instmesh");

  return true;
}

}
CS_PLUGIN_NAMESPACE_END(InstMesh)
