/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein

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

#ifndef __CS_THING_H__
#define __CS_THING_H__

#include "csgeom/transfrm.h"
#include "csgeom/objmodel.h"
#include "parrays.h"
#include "csutil/csobject.h"
#include "csutil/nobjvec.h"
#include "csutil/util.h"
#include "csutil/flags.h"
#include "csutil/cscolor.h"
#include "csutil/array.h"
#include "csutil/refarr.h"
#include "csutil/garray.h"
#include "csutil/blockallocator.h"
#include "csutil/parray.h"
#include "iengine/mesh.h"
#include "iengine/rview.h"
#include "iengine/shadcast.h"
#include "imesh/thing/thing.h"
#include "imesh/thing/polygon.h"
#include "imesh/object.h"
#include "imesh/lighting.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "iutil/config.h"
#include "iutil/dbghelp.h"
#include "lghtmap.h"

#include "csgeom/subrec2.h"
#include "csgfx/memimage.h"
#include "ivideo/txtmgr.h"

class csThing;
class csThingStatic;
class csThingObjectType;
class csPolygon3D;
class csPolygon2D;
class csPoly2DPool;
class csLightPatchPool;
class csPolyTexLightMap;
struct iShadowBlockList;
struct csVisObjInfo;
struct iGraphics3D;
struct iRenderView;
struct iMovable;
struct iFrustumView;
struct iMaterialWrapper;
struct iPolygonBuffer;

/**
 * A structure used to replace materials.
 */
struct RepMaterial
{
  iMaterialWrapper* old_mat;
  iMaterialWrapper* new_mat;
  RepMaterial (iMaterialWrapper* o, iMaterialWrapper* n) :
  	old_mat (o), new_mat (n) { }
};

/**
 * A helper class for iPolygonMesh implementations used by csThing.
 */
class PolyMeshHelper : public iPolygonMesh
{
public:
  /**
   * Make a polygon mesh helper which will accept polygons which match
   * with the given flag (one of CS_POLY_COLLDET or CS_POLY_VISCULL).
   */
  PolyMeshHelper (uint32 flag) :
  	polygons (0), vertices (0), poly_flag (flag) { }
  virtual ~PolyMeshHelper () { Cleanup (); }

  void Setup ();
  void SetThing (csThingStatic* thing);

  virtual int GetVertexCount ()
  {
    Setup ();
    return num_verts;
  }
  virtual csVector3* GetVertices ()
  {
    Setup ();
    return vertices;
  }
  virtual int GetPolygonCount ()
  {
    Setup ();
    return num_poly;
  }
  virtual csMeshedPolygon* GetPolygons ()
  {
    Setup ();
    return polygons;
  }
  virtual void Cleanup ();
  
  virtual csFlags& GetFlags () { return flags;  }
  virtual uint32 GetChangeNumber() const { return 0; }

private:
  csThingStatic* thing;
  uint32 static_data_nr;	// To see if the static thing has changed.
  csMeshedPolygon* polygons;	// Array of polygons.
  csVector3* vertices;		// Array of vertices (points to obj_verts).
  int num_poly;			// Total number of polygons.
  int num_verts;		// Total number of vertices.
  uint32 poly_flag;		// Polygons must match with this flag.
  csFlags flags;
};

/**
 * The static data for a thing.
 */
class csThingStatic : public iThingFactoryState, public iMeshObjectFactory
#ifdef CS_USE_NEW_RENDERER
		      ,public iRenderBufferSource
#endif
{
public:
  csRef<csThingObjectType> thing_type;
  /// Pointer to logical parent.
  iBase* logparent;

  /// Set of flags
  csFlags flags;

  /// Number of vertices
  int num_vertices;
  /// Maximal number of vertices
  int max_vertices;
  /// Vertices in object space.
  csVector3* obj_verts;  
  /// Normals in object space
  csVector3* obj_normals;

  /// Smooth flag
  bool smoothed;

  /// Bounding box in object space.
  csBox3 obj_bbox;

  /// If true then the bounding box in object space is valid.
  bool obj_bbox_valid;

  /// Radius of object in object space.
  csVector3 obj_radius;
  /// Full radius of object in object space.
  float max_obj_radius;

  /// Static polys which share the same material
  struct csStaticPolyGroup
  {
    iMaterialWrapper* material;
    csArray<int> polys;

    int numLitPolys;
    int totalLumels;
  };

  struct StaticSuperLM;

  /**
   * Static polys which share the same material and fit on the same SLM 
   * template.
   */
  struct csStaticLitPolyGroup : public csStaticPolyGroup
  {
    csArray<csSubRect2*> slmSubrects;
    csArray<csRect> lmRects;
    StaticSuperLM* staticSLM;
  };

  /// SLM template
  struct StaticSuperLM
  {
    int width, height;
    csSubRectangles2* rects;
    int freeLumels;

    StaticSuperLM (int w, int h) : width(w), height(h)    
    {
      rects = 0;
      freeLumels = width * height;
    }
    ~StaticSuperLM()
    {
      delete rects;
    }

    csSubRectangles2* GetRects ()
    {
      if (rects == 0)
      {
	rects = new csSubRectangles2 (csRect (0, 0, width, height));
      }
      return rects;
    }
  };

  /// The array of static polygon data (csPolygon3DStatic).
  csPolygonStaticArray static_polygons;
  csPDelArray<csStaticLitPolyGroup> litPolys;
  csPDelArray<csStaticPolyGroup> unlitPolys;
  csArray<StaticSuperLM*> superLMs;

  /**
   * Polygon indices to polygons that contain portals (optimization).
   * csPolygon3DStatic will make sure to call AddPortalPolygon() and
   * RemovePortalPolygon() when appropriate.
   */
  csArray<int> portal_polygons;

  /// If true then this thing has been prepared (Prepare() function).
  bool prepared;
  bool lmprepared;

  /**
   * This field describes how the light hitting polygons of this thing is
   * affected by the angle by which the beam hits the polygon. If this value is
   * equal to -1 (default) then the global csPolyTexture::cfg_cosinus_factor
   * will be used.
   */
  float cosinus_factor;

#ifdef CS_USE_NEW_RENDERER
  csRef<iGraphics3D> r3d;

  csRef<iRenderBuffer> vertex_buffer;
  csRef<iRenderBuffer> texel_buffer;
  csRef<iRenderBuffer> normal_buffer;
  csRef<iRenderBuffer> color_buffer;
  csRef<iRenderBuffer> index_buffer;

  csStringID vertex_name, texel_name, normal_name, color_name, index_name;
#endif


public:
  csThingStatic (iBase* parent, csThingObjectType* thing_type);
  virtual ~csThingStatic ();

  /**
   * When a portal is added/removed the portal list must be updated.
   * Prepare() does this automatically But this function is useful in
   * cases where you don't want the overhead of a full prepare.
   */
  void UpdatePortalList ();

  /**
   * Prepare the thing for use. This function MUST be called
   * AFTER the texture manager has been prepared. This function
   * is normally called by csEngine::Prepare() so you only need
   * to worry about this function when you add sectors or things
   * later.
   */
  void Prepare ();
  /**
   * Sets up a layout of lightmaps on a super lightmap that all instances
   * of this factory can reuse.
   */
  void PrepareLMLayout ();
  /// Do the actual distribution.
  void DistributePolyLMs (const csStaticPolyGroup& inputPolys,
    csPDelArray<csStaticLitPolyGroup>& outputPolys, 
    csStaticPolyGroup* rejectedPolys);
  /// Delete LM distro information. Needed when adding/removing polys.
  void UnprepareLMLayout ();

  /// Calculates the interpolated normals of all vertices
  void CalculateNormals ();

  /**
   * Get the static data number.
   */
  uint32 GetStaticDataNumber () const
  {
    return scfiObjectModel.GetShapeNumber ();
  }

  /// Get the specified polygon from this set.
  csPolygon3DStatic *GetPolygon3DStatic (int idx)
  { return static_polygons.Get (idx); }

  /// Clone this static data in a seperate instance.
  csPtr<csThingStatic> Clone ();

  /**
   * Get the bounding box in object space for this polygon set.
   * This is calculated based on the oriented bounding box.
   */
  void GetBoundingBox (csBox3& box);

  /**
   * Get the radius in object space for this polygon set.
   */
  void GetRadius (csVector3& rad, csVector3& cent);

  //----------------------------------------------------------------------
  // Vertex handling functions
  //----------------------------------------------------------------------

  /// Just add a new vertex to the thing.
  int AddVertex (const csVector3& v) { return AddVertex (v.x, v.y, v.z); }

  /// Just add a new vertex to the thing.
  int AddVertex (float x, float y, float z);

  virtual int CreateVertex (const csVector3 &iVertex)
  { return AddVertex (iVertex.x, iVertex.y, iVertex.z); }

  /**
   * Compress the vertex table so that all nearly identical vertices
   * are compressed. The polygons in the set are automatically adapted.
   * This function can be called at any time in the creation of the object
   * and it can be called multiple time but it normally only makes sense
   * to call this function after you have finished adding all polygons
   * and all vertices.<p>
   * Note that calling this function will make the camera vertex array
   * invalid.
   */
  virtual void CompressVertices ();

  /**
   * Optimize the vertex table so that all unused vertices are deleted.
   * Note that calling this function will make the camera vertex array
   * invalid.
   */
  void RemoveUnusedVertices ();

  /// Change a vertex.
  virtual void SetVertex (int idx, const csVector3& vt);

  /// Delete a vertex.
  virtual void DeleteVertex (int idx);

  /// Delete a range of vertices.
  virtual void DeleteVertices (int from, int to);

  /// Return the object space vector for the vertex.
  const csVector3& Vobj (int idx) const { return obj_verts[idx]; }

  /// Return the number of vertices.
  virtual int GetVertexCount () const { return num_vertices; }

  virtual const csVector3 &GetVertex (int i) const
  { return obj_verts[i]; }
  virtual const csVector3* GetVertices () const
  { return obj_verts; }

  /// Add a polygon to this thing.
  void AddPolygon (csPolygon3DStatic* spoly);

  /**
   * Intersect object-space segment with polygons of this set. Return
   * polygon index it intersects with (or -1) and the intersection point
   * in object coordinates.<p>
   *
   * If 'pr' != 0 it will also return a value between 0 and 1
   * indicating where on the 'start'-'end' vector the intersection
   * happened.<p>
   *
   * If only_portals == true then only portals are checked.
   */
  int IntersectSegmentIndex (
    const csVector3 &start, const csVector3 &end,
    csVector3 &isect,
    float *pr,
    bool only_portals);

  SCF_DECLARE_IBASE;

  virtual void* GetPrivateObject () { return (void*)this; }
  virtual int GetPolygonCount () { return static_polygons.Length (); }
  virtual iPolygon3DStatic *GetPolygon (int idx);
  virtual iPolygon3DStatic *GetPolygon (const char* name);
  virtual iPolygon3DStatic *CreatePolygon (const char *iName);
  virtual int FindPolygonIndex (iPolygon3DStatic* polygon) const;
  virtual void RemovePolygon (int idx);
  virtual void RemovePolygons ();

  virtual int GetPortalCount () const { return portal_polygons.Length (); }
  virtual iPortal* GetPortal (int idx) const;
  virtual iPolygon3DStatic* GetPortalPolygon (int idx) const;

  virtual csFlags& GetFlags () { return flags; }

  virtual iPolygon3DStatic* IntersectSegment (const csVector3& start,
	const csVector3& end, csVector3& isect,
	float* pr = 0, bool only_portals = false);

  virtual void SetSmoothingFlag (bool smoothing) { smoothed = smoothing; }
  virtual bool GetSmoothingFlag () { return smoothed; }
  virtual csVector3* GetNormals () { return obj_normals; }
    
  virtual float GetCosinusFactor () const { return cosinus_factor; }
  virtual void SetCosinusFactor (float c) { cosinus_factor = c; }

  //-------------------- iMeshObjectFactory interface implementation ---------

  virtual csPtr<iMeshObject> NewInstance ();
  virtual void HardTransform (const csReversibleTransform& t);
  virtual bool SupportsHardTransform () const { return true; }
  virtual void SetLogicalParent (iBase* lp) { logparent = lp; }
  virtual iBase* GetLogicalParent () const { return logparent; }

  //-------------------- iPolygonMesh interface implementation ---------------
  struct PolyMesh : public PolyMeshHelper
  {
    SCF_DECLARE_IBASE;
    PolyMesh ();
  } scfiPolygonMesh;

  //------------------- Lower detail iPolygonMesh implementation ---------------
  struct PolyMeshLOD : public PolyMeshHelper
  {
    PolyMeshLOD ();
    // @@@ Not embedded because we can't have two iPolygonMesh implementations
    // in csThing.
    SCF_DECLARE_IBASE;
  } scfiPolygonMeshLOD;

  //------------------------- iObjectModel implementation ----------------
  class ObjectModel : public csObjectModel
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThingStatic);
    virtual void GetObjectBoundingBox (csBox3& bbox,
    	int /*type = CS_BBOX_NORMAL*/)
    {
      scfParent->GetBoundingBox (bbox);
    }
    virtual void GetRadius (csVector3& rad, csVector3& cent)
    {
      scfParent->GetRadius (rad, cent);
    }
  } scfiObjectModel;
  friend class ObjectModel;

#ifdef CS_USE_NEW_RENDERER
  iRenderBuffer *GetRenderBuffer (csStringID name);

  void FillRenderMeshes (csDirtyAccessArray<csRenderMesh*>& rmeshes,
    const csArray<RepMaterial>& repMaterials);
#endif

  virtual iObjectModel* GetObjectModel () { return &scfiObjectModel; }
};

/**
 * A Thing is a set of polygons. A thing can be used for the
 * outside of a sector or else to augment the sector with
 * features that are difficult to describe with convex sectors alone.<p>
 *
 * Every polygon in the set has a visible and an invisible face;
 * if the vertices of the polygon are ordered clockwise then the
 * polygon is visible. Using this feature it is possible to define
 * two kinds of things: in one kind the polygons are oriented
 * such that they are visible from within the hull. In other words,
 * the polygons form a sort of container or room where the camera
 * can be located. This kind of thing can be used for the outside
 * walls of a sector. In another kind the polygons are
 * oriented such that they are visible from the outside.
 */
class csThing : public iBase
{
  friend class PolyMeshHelper;

private:
  /// Static data for this thing.
  csRef<csThingStatic> static_data;

  /// ID for this thing (will be >0).
  unsigned int thing_id;
  /// Last used ID.
  static int last_thing_id;
  /// Current visibility number.
  uint32 current_visnr;

  /**
   * Vertices in world space.
   * It is possible that this array is equal to obj_verts. In that
   * case this is a thing that never moves.
   */
  csVector3* wor_verts;
  /// Vertices in camera space.
  csVector3* cam_verts;
  /// Number of vertices for cam_verts.
  int num_cam_verts;

  /// Camera number for which the above camera vertices are valid.
  long cameranr;

  /**
   * This number indicates the last value of the movable number.
   * This thing can use this to check if the world space coordinates
   * need to be updated.
   */
  long movablenr;
  /**
   * The last movable used to move this object.
   */
  iMovable* cached_movable;
  /**
   * How is moving of this thing controlled? This is one of the
   * CS_THING_MOVE_... flags above.
   */
  int cfg_moving;

  /// The array of dynamic polygon data (csPolygon3D).
  csPolygonArray polygons;

  /// Optional array of materials to replace.
  csArray<RepMaterial> replace_materials;

#ifndef CS_USE_NEW_RENDERER
  /**
   * If we are a detail object then this will contain a reference to a
   * polygon buffer for the 3D renderer. This can be used by DrawPolygonMesh.
   */
  iPolygonBuffer* polybuf;
#endif // CS_USE_NEW_RENDERER
  /**
   * An array of materials that are used with the polygon buffer.
   */
  csArray<iMaterialWrapper*> polybuf_materials;
  /**
   * An array of materials that must be visited before use.
   */
  csArray<iMaterialWrapper*> materials_to_visit;

  /**
   * Bounding box in world space.
   * This is a cache for GetBoundingBox(iMovable,csBox3) which
   * will recalculate this if the movable changes (by using movablenr).
   */
  csBox3 wor_bbox;
  /// Last movable number that was used for the bounding box in world space.
  long wor_bbox_movablenr;

  /// Dynamic ambient light assigned to this thing.
  csColor dynamic_ambient;
  /**
   * Version number for dynamic/pseudo-dynamic light changes
   * and also for ambient.
   */
  uint32 light_version;

  /// Pointer to the Thing Template which it derived from.
  csThing* ParentTemplate;
  /// Pointer to logical parent.
  iBase* logparent;

  /// If true then this thing has been prepared (Prepare() function).
  bool prepared;

  /**
   * This number is compared with the static_data_nr in the static data to
   * see if static data has changed and this thing needs to updated local
   * data
   */
  int32 static_data_nr;

  float current_lod;
  uint32 current_features;

#ifdef CS_USE_NEW_RENDERER
  csDirtyAccessArray<csRenderMesh*> renderMeshes;
  csReversibleTransform tr_o2c; //@@@

  void PrepareRenderMeshes ();
  void ClearRenderMeshes ();
#endif

  /// A group of polys that share the same material.
  struct csPolyGroup
  {
    iMaterialWrapper* material;
    csArray<csPolygon3D*> polys;
  };

  /// Polys with the same material and the same SLM
  struct csLitPolyGroup : public csPolyGroup
  {
    csRefArray<iRendererLightmap> lightmaps;
    csRef<iSuperLightmap> SLM;
  };

  csPDelArray<csLitPolyGroup> litPolys;
  csPDelArray<csPolyGroup> unlitPolys;
  bool lightmapsPrepared;
  bool lightmapsDirty;

  void PrepareLMs ();
  void ClearLMs ();
  void UpdateDirtyLMs ();
public:
  /**
   * How many times are we busy drawing this thing (recursive).
   * This is an important variable as it indicates to
   * 'new_transformation' which set of camera vertices it should
   * use.
   */
  int draw_busy;

private:
  /**
   * Prepare the polygon buffer for use by DrawPolygonMesh.
   * If the polygon buffer is already made then this function will do
   * nothing.
   */
  void PreparePolygonBuffer ();

  /**
   * Invalidate a thing. This has to be called when new polygons are
   * added or removed.
   */
  void InvalidateThing ();

  /**
   * Draw the given array of polygons in the current thing.
   * This version uses iGraphics3D->DrawPolygonMesh()
   * for more efficient rendering. WARNING! This version only works for
   * lightmapped polygons right now and is far from complete.
   * 't' is the movable transform.
   */
  void DrawPolygonArrayDPM (iRenderView* rview, iMovable* movable,
  	csZBufMode zMode);

  /**
   * Draw the given array of polygons.
   */
  void DrawPolygonArray (const csReversibleTransform& t,
	iRenderView* rview, csZBufMode zMode);

  /**
   * Draw one 3D/2D polygon combination. The 2D polygon is the transformed
   * and clipped version of the 3D polygon.
   * 't' is the movable transform.
   */
  static void DrawOnePolygon (csPolygon3D* p, csPolygon2D* poly,
	const csReversibleTransform& t,
	iRenderView* d, csZBufMode zMode, const csPlane3& camera_plane);

  /// Generate a cachename based on geometry.
  char* GenerateCacheName ();  

public:
  /**
   * Create an empty thing.
   */
  csThing (iBase* parent, csThingStatic* static_data);

  /// Destructor.
  virtual ~csThing ();

  /// Get the pointer to the static data.
  csThingStatic* GetStaticData () { return static_data; }

  //----------------------------------------------------------------------
  // Vertex handling functions
  //----------------------------------------------------------------------

  /**
   * Return the world space vector for the vertex.
   * Make sure you recently called WorUpdate(). Otherwise it is
   * possible that this coordinate will not be valid.
   */
  const csVector3& Vwor (int idx) const { return wor_verts[idx]; }

  /**
   * Return the camera space vector for the vertex.
   * Make sure you recently called UpdateTransformation(). Otherwise it is
   * possible that this coordinate will not be valid.
   */
  const csVector3& Vcam (int idx) const { return cam_verts[idx]; }

  //----------------------------------------------------------------------
  // Polygon handling functions
  //----------------------------------------------------------------------

  /// Get the number of polygons in this thing.
  int GetPolygonCount ()
  { return polygons.Length (); }

  /// Get the specified polygon from this set.
  csPolygon3DStatic *GetPolygon3DStatic (int idx)
  { return static_data->GetPolygon3DStatic (idx); }

  /// Get the specified polygon from this set.
  csPolygon3D *GetPolygon3D (int idx)
  { return polygons.Get (idx); }

  /// Get the named polygon from this set.
  csPolygon3D *GetPolygon3D (const char* name);

  /// Get the entire array of polygons.
  csPolygonArray& GetPolygonArray () { return polygons; }

  /// Find a polygon index.
  int FindPolygonIndex (iPolygon3D* polygon) const;

  /// Find a polygon index.
  int FindPolygonIndex (iPolygon3DStatic* polygon) const;

  /// Remove a single polygon.
  void RemovePolygon (int idx);

  /// Remove all polygons.
  void RemovePolygons ();

  //----------------------------------------------------------------------
  // Setup
  //----------------------------------------------------------------------

  /**
   * Prepare all polygons for use. This function MUST be called
   * AFTER the texture manager has been prepared. This function
   * is normally called by csEngine::Prepare() so you only need
   * to worry about this function when you add sectors or things
   * later.
   */
  void Prepare ();

  /** Reset the prepare flag so that this Thing can be re-prepared.
   * Among other things this will allow cached lightmaps to be
   * recalculated.
   */
  void Unprepare ();

  /**
   * Merge the given Thing into this one. The other polygons and
   * curves are removed from the other thing so that it is ready to
   * be removed. Warning! All Things are merged in world space
   * coordinates and not in object space as one could expect!
   */
  void Merge (csThing* other);

  /// Set parent template.
  void SetTemplate (csThing *t)
  { ParentTemplate = t; }

  /// Query parent template.
  csThing *GetTemplate () const
  { return ParentTemplate; }

  /// Find the real material to use if it was replaced (or 0 if not).
  iMaterialWrapper* FindRealMaterial (iMaterialWrapper* old_mat);

  void ReplaceMaterial (iMaterialWrapper* oldmat, iMaterialWrapper* newmat);
  void ClearReplacedMaterials ();

  void InvalidateMaterialHandles ();

  //----------------------------------------------------------------------
  // Bounding information
  //----------------------------------------------------------------------

  /**
   * Get the bounding box for this object given some transformation (movable).
   */
  void GetBoundingBox (iMovable* movable, csBox3& box);

  /**
   * Get a write object for a vis culling system.
   */
  iPolygonMesh* GetWriteObject ();

  //----------------------------------------------------------------------
  // Drawing
  //----------------------------------------------------------------------

  /**
   * Test if this thing is visible or not.
   */
  bool DrawTest (iRenderView* rview, iMovable* movable);

  /**
   * Draw this thing given a view and transformation.
   */
  bool Draw (iRenderView* rview, iMovable* movable, csZBufMode zMode);

  //----------------------------------------------------------------------
  // Lighting
  //----------------------------------------------------------------------

  /**
   * Init the lightmaps for all polygons in this thing.
   */
  void InitializeDefault (bool clear);

  /**
   * Read the lightmaps from the cache.
   */
  bool ReadFromCache (iCacheManager* cache_mgr);

  /**
   * Cache the lightmaps for all polygons in this thing.
   */
  bool WriteToCache (iCacheManager* cache_mgr);

  /**
   * Prepare the lightmaps for all polys so that they are suitable
   * for the 3D rasterizer.
   */
  void PrepareLighting ();

  /// Marks the whole object as it is affected by any light.
  void MarkLightmapsDirty ();

  //----------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------

  /**
   * Check frustum visibility on this thing.
   * First initialize the 2D culler cube.
   */
  void CastShadows (iFrustumView* lview, iMovable* movable);

  /**
   * Append a list of shadow frustums which extend from
   * this thing. The origin is the position of the light.
   */
  void AppendShadows (iMovable* movable, iShadowBlockList* shadows,
  	const csVector3& origin);

  /**
   * Test a beam with this thing.
   */
  bool HitBeamOutline (const csVector3& start, const csVector3& end,
  	csVector3& isect, float* pr);

  /**
   * Test a beam with this thing.
   */
  bool HitBeamObject (const csVector3& start, const csVector3& end,
  	csVector3& isect, float* pr);

  //----------------------------------------------------------------------
  // Transformation
  //----------------------------------------------------------------------

  /**
   * Transform to the given camera if needed. This function will use
   * the camera number to avoid unneeded transformation.
   */
  void UpdateTransformation (const csTransform& c, long cam_cameranr);

  /// Make sure the world vertices are up-to-date.
  void WorUpdate ();

  /// Get the array of camera vertices.
  csVector3* GetCameraVertices (const csTransform& c, long cam_cameranr)
  {
    UpdateTransformation (c, cam_cameranr);
    return cam_verts;
  }

  //----------------------------------------------------------------------
  // Various
  //----------------------------------------------------------------------

  /**
   * Do a hardtransform. This will make a clone of the factory
   * to avoid other meshes using this factory to be hard transformed too.
   */
  void HardTransform (const csReversibleTransform& t);

  /**
   * Control how this thing will be moved.
   */
  void SetMovingOption (int opt);

  /**
   * Get the moving option.
   */
  int GetMovingOption () const { return cfg_moving; }

  /// Sets dynamic ambient light for this thing
  void SetDynamicAmbientLight(const csColor& color)
  {
      dynamic_ambient = color;
      light_version++;
      MarkLightmapsDirty ();
  }
  /// Gets dynamic ambient light for this thing
  const csColor& GetDynamicAmbientLight()
  {
      return dynamic_ambient;
  }

  /// Get light version.
  uint32 GetLightVersion() const
  { return light_version; }

  void DynamicLightChanged (iDynLight* dynlight);
  void DynamicLightDisconnect (iDynLight* dynlight);
  void StaticLightChanged (iStatLight* statlight);
  void StaticLightDisconnect (iStatLight* statlight);

  SCF_DECLARE_IBASE;

  //------------------------- iThingState interface -------------------------
  struct ThingState : public iThingState
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    virtual void* GetPrivateObject () { return (void*)scfParent; }
    virtual iThingFactoryState* GetFactory ()
    {
      return (iThingFactoryState*)(scfParent->static_data);
    }

    virtual iPolygon3D *GetPolygon (int idx);
    virtual iPolygon3D *GetPolygon (const char* name);
    virtual int FindPolygonIndex (iPolygon3D* polygon) const
    { return scfParent->FindPolygonIndex (polygon); }

    virtual iPolygon3D* GetPortalPolygon (int idx) const;

    virtual const csVector3 &GetVertexW (int i) const
    { return scfParent->wor_verts[i]; }
    virtual const csVector3* GetVerticesW () const
    { return scfParent->wor_verts; }
    virtual const csVector3 &GetVertexC (int i) const
    { return scfParent->cam_verts[i]; }
    virtual const csVector3* GetVerticesC () const
    { return scfParent->cam_verts; }

    virtual int GetMovingOption () const
    { return scfParent->GetMovingOption (); }
    virtual void SetMovingOption (int opt)
    { scfParent->SetMovingOption (opt); }

    virtual iPolygon3D* IntersectSegment (const csVector3& start,
	const csVector3& end, csVector3& isect,
	float* pr = 0, bool only_portals = false);

    /// Prepare.
    virtual void Prepare ()
    {
      scfParent->Prepare ();
      if (scfParent->static_data->flags.Check (CS_THING_FASTMESH))
        scfParent->PreparePolygonBuffer ();
    }

    /// Unprepare.
    virtual void Unprepare ()
    {
      scfParent->Unprepare ();
    }

    virtual void ReplaceMaterial (iMaterialWrapper* oldmat,
  	iMaterialWrapper* newmat)
    {
      scfParent->ReplaceMaterial (oldmat, newmat);
    }
    virtual void ClearReplacedMaterials ()
    {
      scfParent->ClearReplacedMaterials ();
    }
  } scfiThingState;
  friend struct ThingState;

  //------------------------- iLightingInfo interface -------------------------
  /// iLightingInfo implementation.
  struct LightingInfo : public iLightingInfo
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    virtual void InitializeDefault (bool clear)
    {
      scfParent->InitializeDefault (clear);
    }
    virtual bool ReadFromCache (iCacheManager* cache_mgr)
    {
      return scfParent->ReadFromCache (cache_mgr);
    }
    virtual bool WriteToCache (iCacheManager* cache_mgr)
    {
      return scfParent->WriteToCache (cache_mgr);
    }
    virtual void PrepareLighting ()
    {
      scfParent->PrepareLighting ();
    }
    virtual void SetDynamicAmbientLight (const csColor& color)
    { scfParent->SetDynamicAmbientLight (color); }
    virtual const csColor& GetDynamicAmbientLight ()
    { return scfParent->GetDynamicAmbientLight (); }
    virtual void DynamicLightChanged (iDynLight* dynlight)
    { scfParent->DynamicLightChanged (dynlight); }
    virtual void DynamicLightDisconnect (iDynLight* dynlight)
    { scfParent->DynamicLightDisconnect (dynlight); }
    virtual void StaticLightChanged (iStatLight* statlight)
    { scfParent->StaticLightChanged (statlight); }
    virtual void StaticLightDisconnect (iStatLight* statlight)
    { scfParent->StaticLightDisconnect (statlight); }
  } scfiLightingInfo;
  friend struct LightingInfo;

  //-------------------- iPolygonMesh interface implementation ---------------
  struct PolyMesh : public PolyMeshHelper
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    PolyMesh () : PolyMeshHelper (CS_POLY_COLLDET) { }
  } scfiPolygonMesh;

  //------------------- Lower detail iPolygonMesh implementation ---------------
  struct PolyMeshLOD : public PolyMeshHelper
  {
    PolyMeshLOD ();
    // @@@ Not embedded because we can't have two iPolygonMesh implementations
    // in csThing.
    SCF_DECLARE_IBASE;
  } scfiPolygonMeshLOD;

  //-------------------- iShadowCaster interface implementation ----------
  struct ShadowCaster : public iShadowCaster
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    virtual void AppendShadows (iMovable* movable, iShadowBlockList* shadows,
    	const csVector3& origin)
    {
      scfParent->AppendShadows (movable, shadows, origin);
    }
  } scfiShadowCaster;
  friend struct ShadowCaster;

  //-------------------- iShadowReceiver interface implementation ----------
  struct ShadowReceiver : public iShadowReceiver
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    virtual void CastShadows (iMovable* movable, iFrustumView* fview)
    {
      scfParent->CastShadows (fview, movable);
    }
  } scfiShadowReceiver;
  friend struct ShadowReceiver;

  virtual csRenderMesh **GetRenderMeshes (int &num);

  //-------------------- iMeshObject interface implementation ----------
  struct MeshObject : public iMeshObject
  {
    SCF_DECLARE_EMBEDDED_IBASE (csThing);
    virtual iMeshObjectFactory* GetFactory () const;
    virtual bool DrawTest (iRenderView* rview, iMovable* movable)
    {
      return scfParent->DrawTest (rview, movable);
    }
    virtual void UpdateLighting (iLight** /*lights*/, int /*num_lights*/,
      	iMovable* /*movable*/) { }
    virtual bool Draw (iRenderView* rview, iMovable* movable,
    	csZBufMode zMode)
    {
      return scfParent->Draw (rview, movable, zMode);
    }
#ifdef CS_USE_NEW_RENDERER
    virtual bool DrawZ (iRenderView* rview, iMovable* movable,
    	csZBufMode zMode)
    {
      return false;
    }
    virtual bool DrawShadow (iRenderView* rview, iMovable* movable,
    	csZBufMode zMode, iLight *light)
    {
      return false;
    }
    virtual bool DrawLight (iRenderView* rview, iMovable* movable,
  	csZBufMode zbufMode, iLight *light)
    {
      return false;
    }
      /// The following enable/disable shadow caps for stencil shadow rendering
    virtual void EnableShadowCaps () {}
    virtual void DisableShadowCaps () {}
#endif // CS_USE_NEW_RENDERER
    virtual csRenderMesh** GetRenderMeshes (int &n)
    {
      return scfParent->GetRenderMeshes (n);
    }
    virtual void SetVisibleCallback (iMeshObjectDrawCallback* /*cb*/) { }
    virtual iMeshObjectDrawCallback* GetVisibleCallback () const
    { return 0; }
    virtual void NextFrame (csTicks /*current_time*/,const csVector3& /*pos*/) { }
    virtual void HardTransform (const csReversibleTransform& t)
    {
      scfParent->HardTransform (t);
    }
    virtual bool SupportsHardTransform () const { return true; }
    virtual bool HitBeamOutline (const csVector3& start, const csVector3& end,
      csVector3& isect, float* pr)
    {
      return scfParent->HitBeamOutline (start, end, isect, pr);
    }
    virtual bool HitBeamObject (const csVector3& start, const csVector3& end,
  	csVector3& isect, float* pr)
    {
      return scfParent->HitBeamObject (start, end, isect, pr);
    }
    virtual void SetLogicalParent (iBase* lp) { scfParent->logparent = lp; }
    virtual iBase* GetLogicalParent () const { return scfParent->logparent; }
    virtual iObjectModel* GetObjectModel ()
    {
      return scfParent->static_data->GetObjectModel ();
    }
    virtual bool SetColor (const csColor&) { return false; }
    virtual bool GetColor (csColor&) const { return false; }
    virtual bool SetMaterialWrapper (iMaterialWrapper*) { return false; }
    virtual iMaterialWrapper* GetMaterialWrapper () const { return 0; }
    virtual void InvalidateMaterialHandles ()
    {
      scfParent->InvalidateMaterialHandles ();
    }
    virtual int GetPortalCount () const
    {
      return scfParent->static_data->GetPortalCount ();
    }
    virtual iPortal* GetPortal (int idx) const
    {
      return scfParent->static_data->GetPortal (idx);
    }
  } scfiMeshObject;
  friend struct MeshObject;
};

/**
 * Thing type. This is the plugin you have to use to create instances
 * of csThing.
 */
class csThingObjectType : public iMeshObjectType
{
public:
  iObjectRegistry* object_reg;
  bool do_verbose;	// Verbose error reporting.
  iEngine* engine;
  /**
   * csThingObjectType must keep a reference to G3D because when polygons
   * are destructed they actually refer to G3D to clear the cache.
   */
  csRef<iGraphics3D> G3D;
  /// An object pool for 2D polygons used by the rendering process.
  csPoly2DPool* render_pol2d_pool;
  /// An object pool for lightpatches.
  csLightPatchPool* lightpatch_pool;

  /**
   * Block allocators for various types of objects in thing.
   */
  csBlockAllocator<csPolygon3D> blk_polygon3d;
  csBlockAllocator<csPolygon3DStatic> blk_polygon3dstatic;
  csBlockAllocator<csPolyLightMapMapping> blk_lightmapmapping;
  csBlockAllocator<csPolyTextureMapping> blk_texturemapping;
  csBlockAllocator<csPolyTexture> blk_polytex;
  csBlockAllocator<csLightMap> blk_lightmap;

  int maxLightmapW, maxLightmapH;
public:
  SCF_DECLARE_IBASE;

  /// Constructor.
  csThingObjectType (iBase*);

  /// Destructor.
  virtual ~csThingObjectType ();

  /// Register plugin with the system driver
  virtual bool Initialize (iObjectRegistry *object_reg);
  void Clear ();

  void Warn (const char *description, ...);
  void Bug (const char *description, ...);
  void Notify (const char *description, ...);
  void Error (const char *description, ...);

  /// New Factory.
  virtual csPtr<iMeshObjectFactory> NewFactory ();

  /// Execute a debug command.
  virtual bool DebugCommand (const char* cmd);

  /// iThingEnvironment implementation.
  struct eiThingEnvironment : public iThingEnvironment
  {
    SCF_DECLARE_EMBEDDED_IBASE(csThingObjectType);
    virtual void Clear ()
    {
      scfParent->Clear ();
    }
    virtual int GetLightmapCellSize () const
    {
      return csLightMap::lightcell_size;
    }
    virtual void SetLightmapCellSize (int size)
    {
      csLightMap::SetLightCellSize (size);
    }
    virtual int GetDefaultLightmapCellSize () const
    {
      return csLightMap::default_lightmap_cell_size;
    }
  } scfiThingEnvironment;

  /// iComponent implementation.
  struct eiComponent : public iComponent
  {
    SCF_DECLARE_EMBEDDED_IBASE(csThingObjectType);
    virtual bool Initialize (iObjectRegistry* p)
    { return scfParent->Initialize(p); }
  } scfiComponent;

  /// iConfig implementation.
  struct eiConfig : public iConfig
  {
    SCF_DECLARE_EMBEDDED_IBASE(csThingObjectType);
    virtual bool GetOptionDescription (int idx, csOptionDescription *option);
    virtual bool SetOption (int id, csVariant* value);
    virtual bool GetOption (int id, csVariant* value);
  } scfiConfig;

  /// iDebugHelper implementation
  struct eiDebugHelper : public iDebugHelper
  {
    SCF_DECLARE_EMBEDDED_IBASE(csThingObjectType);
    virtual int GetSupportedTests () const
    { return 0; }
    virtual csPtr<iString> UnitTest ()
    { return 0; }
    virtual csPtr<iString> StateTest ()
    { return 0; }
    virtual csTicks Benchmark (int num_iterations)
    { return 0; }
    virtual csPtr<iString> Dump ()
    { return 0; }
    virtual void Dump (iGraphics3D* g3d)
    { }
    virtual bool DebugCommand (const char* cmd)
    { return scfParent->DebugCommand (cmd); }
  } scfiDebugHelper;
};

#endif // __CS_THING_H__
