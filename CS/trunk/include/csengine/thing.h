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
#include "csobject/pobject.h"
#include "csengine/bspbbox.h"
#include "csengine/rview.h"
#include "csengine/movable.h"
#include "csengine/tranman.h"
#include "csengine/bsp.h"
#include "csutil/flags.h"
#include "csutil/cscolor.h"
#include "csutil/csvector.h"
#include "ithing.h"
#include "imovable.h"
#include "ipolmesh.h"
#include "iviscull.h"

class csSector;
class csEngine;
class csStatLight;
class csMaterialWrapper;
class csMaterialList;
class csPolygon3D;
class csPolygonTree;
struct iGraphics3D;
class Dumper;

/**
 * This structure keeps the indices of the vertices which
 * define the bounding box of a csThing. It is calculated
 * by CreateBoundingBox() and stored with the csThing.<p>
 *
 * The following are six polygons which describe the faces
 * of the bounding box (clock-wise vertex order):<br>
 * <ul>
 *   <li>i2,i1,i3,i4
 *   <li>i6,i2,i4,i8
 *   <li>i5,i6,i8,i7
 *   <li>i1,i5,i7,i3
 *   <li>i1,i2,i6,i5
 *   <li>i7,i8,i4,i3
 * </ul>
 */
struct csThingBBox
{
  int i1, i2, i3, i4, i5, i6, i7, i8;
};

/**
 * If CS_ENTITY_CONVEX is set then this entity is convex (what did
 * you expect :-)
 * This means the 3D engine can do various optimizations.
 * If you set 'convex' to true the center vertex will also be calculated.
 * It is unset by default (@@@ should be calculated).
 */
#define CS_ENTITY_CONVEX 1

/**
 * If CS_ENTITY_MOVEABLE is set then this entity can move.
 * This is used by several optimizations in the 3D Engine.
 * This flag is false by default (it is assumed that most Things
 * will not move).
 */
#define CS_ENTITY_MOVEABLE 2

/**
 * If CS_ENTITY_DETAIL is set then this entity is a detail
 * object. A detail object is treated as a single object by
 * the engine. The engine can do several optimizations on this.
 * In general you should use this flag for small and detailed
 * objects. Detail objects are not included in BSP or octrees.
 */
#define CS_ENTITY_DETAIL 4

/**
 * If CS_ENTITY_CAMERA is set then this entity will be always
 * be centerer around the same spot relative to the camera. This
 * is useful for skyboxes or skydomes.
 */
#define CS_ENTITY_CAMERA 8

/**
 * If CS_ENTITY_ZFILL is set then this thing will be rendered with
 * ZFILL instead of fully using the Z-buffer. This is useful for
 * things that make the outer walls of a sector.
 */
#define CS_ENTITY_ZFILL 16

/**
 * If CS_ENTITY_INVISIBLE is set then this thing will not be rendered.
 * It will still cast shadows and be present otherwise. Use the
 * CS_ENTITY_NOSHADOWS flag to disable shadows.
 */
#define CS_ENTITY_INVISIBLE 32

/**
 * If CS_ENTITY_NOSHADOWS is set then this thing will not cast
 * shadows. Lighting will still be calculated for it though. Use the
 * CS_ENTITY_NOLIGHTING flag to disable that.
 */
#define CS_ENTITY_NOSHADOWS 64

/**
 * If CS_ENTITY_NOLIGHTING is set then this thing will not be lit.
 * It may still cast shadows though. Use the CS_ENTITY_NOSHADOWS flag
 * to disable that.
 */
#define CS_ENTITY_NOLIGHTING 128

/**
 * If CS_ENTITY_VISTREE is set then an octree will be calculated for the
 * polygons in this thing.
 */
#define CS_ENTITY_VISTREE 256

/**
 * If CS_ENTITY_BACK2FRONT is set then all objects with the same
 * render order as this one and which also have this flag set will
 * be rendered in roughly back to front order. All objects with
 * the same render order but which do not have this flag set will
 * be rendered later. This flag is important if you want to have
 * alpha transparency rendered correctly.
 */
#define CS_ENTITY_BACK2FRONT 512

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
 *
 * Things can also be used for volumetric fog. In that case
 * the Thing must be convex.
 */
class csThing : public csPObject
{
  friend class csMovable;
  friend class Dumper;

private:
  /// Number of vertices
  int num_vertices;
  /// Maximal number of vertices
  int max_vertices;

  /// Vertices in world space.
  csVector3* wor_verts;
  /// Vertices in object space.
  csVector3* obj_verts;
  /// Vertices in camera space.
  csVector3* cam_verts;
  /// Camera space vertices.
  csTransformedSet cam_verts_set;

  /// The array of polygons forming the outside of the set
  csPolygonArray polygons;

  /// The array of curves forming the outside of the set
  csCurvesArray curves;

  /// Optional oriented bounding box.
  csThingBBox* bbox;

  /// Bounding box in object space.
  csBox3 obj_bbox;

  /// If true then the bounding box in object space is valid.
  bool obj_bbox_valid;

  /// Radius of object in object space.
  csVector3 obj_radius;

  /**
   * Light frame number. Using this number one can see if gouraud shaded
   * vertices have been initialized already.
   */
  long light_frame_number;

  /**
   * Draw the given array of polygons in the current csPolygonSet. This
   * function is called by subclasses of csPolygonSet (csSector and
   * csThing currently). This version uses iGraphics3D->DrawPolygonMesh()
   * for more efficient rendering. WARNING! This version only works for
   * lightmapped polygons right now.
   */
  void DrawPolygonArrayDPM (csPolygonInt** polygon, int num, csRenderView* rview,
  	bool use_z_buf);

  /// Fog information.
  csFog fog;

private:
  /**
   * Vector with visibility objects. Only useful if this thing has a
   * static tree and thus can do visibility testing.
   */
  csVector visobjects;

  /// Engine handle.
  csEngine* engine;

  /// If convex, this holds the index to the center vertex.
  int center_idx;

  /// Pointer to the Thing Template which it derived from.
  csThing* ParentTemplate;

  /**
   * If this variable is not NULL then it is a BSP or octree for this
   * thing.
   */
  csPolygonTree* static_tree;

  /**
   * Utility function to be called whenever obj changes which updates
   * object to world transform in all of the curves
   */
  void UpdateCurveTransform ();

  /// Internal draw function.
  void DrawInt (csRenderView& rview);

  /// Bounding box for polygon trees.
  csPolyTreeBBox tree_bbox;

  /// If true this thing is visible.
  bool is_visible;

  /// If true this thing is a 'sky' object.
  bool is_sky;

  /// If true this thing is a 'template' object.
  bool is_template;

  /**
   * Update this thing in the polygon trees.
   */
  void UpdateInPolygonTrees ();

  /// Position in the world.
  csMovable movable;

protected:
  /// Move this thing to the specified sector. Can be called multiple times.
  virtual void MoveToSector (csSector* s);

  /// Remove this thing from all sectors it is in (but not from the engine).
  virtual void RemoveFromSectors ();

  /**
   * Update transformations after the thing has moved
   * (through updating the movable instance).
   * This MUST be done after you change the movable otherwise
   * some of the internal data structures will not be updated
   * correctly. This function is called by movable.UpdateMove().
   */
  virtual void UpdateMove ();

public:
  /// Set of flags
  csFlags flags;

public:

//@@@@@@@@@@@@@@@@ CHECK OUT PRIVATE/PUBLIC/PROTECTED OF THIS CLASS
//@@@@@@@@@@@@@@@@ ORDER FUNCTIONS IN THIS CLASS ON FUNCTIONALITY

//@@@@@@@@@@@@@@@@@@
  /**
   * Draw the given array of polygons in the current csPolygonSet. This
   * function is called by subclasses of csPolygonSet (csSector and
   * csThing currently).
   */
  static void DrawPolygonArray (csPolygonInt** polygon, int num, csRenderView* rview,
  	bool use_z_buf);
//@@@@@@@@@@@@@@@@@@
  /**
   * Test a number of polygons against the c-buffer and insert them to the
   * c-buffer if visible and also add them to a queue.
   * If 'pvs' is true then the PVS is used (polygon->IsVisible()).
   */
  static void* TestQueuePolygonArray (csPolygonInt** polygon, int num, csRenderView* d,
  	csPolygon2DQueue* poly_queue, bool pvs);
//@@@@@@@@@@@@@@@@@@@
  /**
   * Draw one 3D/2D polygon combination. The 2D polygon is the transformed
   * and clipped version of the 3D polygon.
   */
  static void DrawOnePolygon (csPolygon3D* p, csPolygon2D* poly, csRenderView* d,
	bool use_z_buf);
//@@@@@@@@@@@@
  /**
   * This function is called by the BSP tree traversal routine
   * to test polygons against the C buffer and add them to a queue if needed.
   */
  static void* TestQueuePolygons (csThing*, csPolygonInt** polygon,
  	int num, bool same_plane, void* data);
//@@@@@@@@@@@
  /**
   * Draw a number of polygons from a queue (used with C buffer processing).
   */
  void DrawPolygonsFromQueue (csPolygon2DQueue* queue, csRenderView* rview);
//@@@@@@@@@@@@
  /**
   * This function is called by the BSP tree traversal routine
   * to draw a number of polygons.
   */
  static void* DrawPolygons (csThing*, csPolygonInt** polygon,
  	int num, bool same_plane, void* data);

  /**
   * Callback that is called whenever a movable that we're interested in
   * changes.
   */
  static void MovableListener (iMovable* movable, int action, void* userdata);

  /**
   * Current light frame number. This is used for seeing
   * if gouraud shading should be recalculated for this thing.
   * If there is a mismatch between the frame number of this set
   * and the global number then the gouraud shading is not up-to-date.
   */
  static long current_light_frame_number;

  /**
   * How many times are we busy drawing this thing (recursive).
   * This is an important variable as it indicates to
   * 'new_transformation' which set of camera vertices it should
   * use.
   */
  int draw_busy;

  /**
   * Tesselation parameter:
   * Center of thing to determine distance from
   */
  csVector3 curves_center;
  /// scale param (the larger this param it, the more the curves are tesselated).
  float curves_scale;  

  /// Curve vertices. 
  csVector3* curve_vertices;
  /// Texture coords of curve vertices
  csVector2* curve_texels;

  /// Number of vertices.
  int num_curve_vertices;
  /// Maximum number of vertices.
  int max_curve_vertices;


public:
  /**
   * Create an empty thing.
   */
  csThing (csEngine*, bool is_sky = false, bool is_template = false);

  /// Destructor.
  virtual ~csThing ();

  /**
   * Prepare all polygons for use. This function MUST be called
   * AFTER the texture manager has been prepared. This function
   * is normally called by csEngine::Prepare() so you only need
   * to worry about this function when you add sectors or things
   * later.
   */
  void Prepare (csSector* sector);

  /// Just add a new vertex to the thing.
  int AddVertex (const csVector3& v) { return AddVertex (v.x, v.y, v.z); }

  /// Just add a new vertex to the thing.
  int AddVertex (float x, float y, float z);

  /**
   * Add a vertex but first check if there is already
   * a vertex close to the wanted position. In that case
   * don't add a new vertex but return the index of the old one.
   * Note that this function is not very efficient. If you plan
   * to add a lot of vertices you should just use AddVertex()
   * and call CompressVertices() later.
   */
  int AddVertexSmart (const csVector3& v) { return AddVertexSmart (v.x, v.y, v.z); }

  /**
   * Add a vertex but first check if there is already
   * a vertex close to the wanted position. In that case
   * don't add a new vertex but return the index of the old one.
   * Note that this function is not very efficient. If you plan
   * to add a lot of vertices you should just use AddVertex()
   * and call CompressVertices() later.
   */
  int AddVertexSmart (float x, float y, float z);

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
  void CompressVertices ();

  /// Return the world space vector for the vertex.
  csVector3& Vwor (int idx) { return wor_verts[idx]; }

  /// Return the object space vector for the vertex.
  csVector3& Vobj (int idx) { return obj_verts[idx]; }

  /**
   * Return the camera space vector for the vertex.
   * Make sure you recently called CamUpdate(). Otherwise it is
   * possible that this coordinate will not be valid.
   */
  csVector3& Vcam (int idx) { return cam_verts[idx]; }

  /// Return the number of vertices.
  int GetNumVertices () { return num_vertices; }

  /// Add a polygon to this thing.
  void AddPolygon (csPolygonInt* spoly);

  /// Create a new polygon in this thing and add it.
  csPolygon3D* NewPolygon (csMaterialWrapper* material);

  /// Get the number of polygons in this thing.
  int GetNumPolygons ()
  { return polygons.Length (); }

  /// Get a csPolygonInt with the index.
  csPolygonInt* GetPolygonInt (int idx);

  /// Get the specified polygon from this set.
  csPolygon3D *GetPolygon3D (int idx)
  { return polygons.Get (idx); }

  /// Get the named polygon from this set.
  csPolygon3D *GetPolygon3D (const char* name);

  /// Get the entire array of polygons.
  csPolygonArray& GetPolygonArray () { return polygons; }

  /// Add a curve to this thing.
  void AddCurve (csCurve* curve);

  /// Get the number of curves in this thing.
  int GetNumCurves ()
  { return curves.Length (); }

  /// Get the specified curve from this set.
  csCurve* GetCurve (int idx)
  { return curves.Get (idx); }

  /// Get the named curve from this set.
  csCurve* GetCurve (char* name);

  /// Get the number of curve vertices.
  int GetNumCurveVertices () { return num_curve_vertices; }

  /// Get the specified curve vertex.
  csVector3& CurveVertex (int i) { return curve_vertices[i]; }

  /// Get the curve vertices.
  csVector3* GetCurveVertices () { return curve_vertices; }

  /// Get the specified curve texture coordinate (texel).
  csVector2& CurveTexel (int i) { return curve_texels[i]; }

  /// Add a curve vertex and return the index of the vertex.
  int AddCurveVertex (csVector3& v, csVector2& t);

  /**
   * Get the movable instance for this thing.
   * It is very important to call GetMovable().UpdateMove()
   * after doing any kind of modification to this movable
   * to make sure that internal data structures are
   * correctly updated.
   */
  csMovable& GetMovable () { return movable; }

  /**
   * Return true if this thing is a sky object.
   */
  bool IsSky () { return is_sky; }

  /**
   * Return true if this thing is a template.
   */
  bool IsTemplate () { return is_template; }

  /**
   * Set convexity flag of this thing. You should call this instead
   * of SetFlags (CS_ENTITY_CONVEX, CS_ENTITY_CONVEX) because this function
   * does some extra calculations.
   */
  void SetConvex (bool c);

  /**
   * If this thing is convex you can use getCenter to get the index
   * of the vertex holding the center of this thing. This center is
   * calculated by 'setConvex(true)'.
   */
  int GetCenter () { return center_idx; }

  /**
   * Do some initialization needed for visibility testing.
   * i.e. clear camera transformation.
   */
  void VisTestReset ()
  {
    tree_bbox.ClearTransform ();
  }

  /// Mark this thing as visible.
  void MarkVisible () { is_visible = true; }

  /// Mark this thing as invisible.
  void MarkInvisible () { is_visible = false; }

  /// Return if this thing is visible.
  bool IsVisible () { return is_visible; }

  /**
   * Get the static polygon tree.
   */
  csPolygonTree* GetStaticTree () { return static_tree; }

  /**
   * Call this function to generate a polygon tree for this csThing.
   * This might make drawing more efficient because
   * this thing can then be drawn using Z-fill instead of Z-buffer.
   * Also the c-buffer requires a tree of this kind.
   * If 'octree' is true this function will create an octree with mini-bsp
   * trees instead of a BSP tree alone.
   */
  void BuildStaticTree (int mode = BSP_MINIMIZE_SPLITS, bool octree = false);

  /// Get the pointer to the object to place in the polygon tree.
  csPolyTreeObject* GetPolyTreeObject ()
  {
    return &tree_bbox;
  }

  /**
   * Merge the given Thing into this one. The other polygons and
   * curves are removed from the other thing so that it is ready to
   * be removed. Warning! All Things are merged in world space
   * coordinates and not in object space as one could expect!
   */
  void Merge (csThing* other);

  /**
   * Prepare the lightmaps for all polys so that they are suitable
   * for the 3D rasterizer.
   */
  void CreateLightMaps (iGraphics3D* g3d);

  /**
   * Draw this thing given a view and transformation.
   */
  void Draw (csRenderView& rview);

  /**
   * Draw all curves in this thing given a view and transformation.
   */
  void DrawCurves (csRenderView& rview);

  /**
   * Draw this thing as a fog volume (only when fog is enabled for
   * this thing).
   */
  void DrawFoggy (csRenderView& rview);

  /**
   * Init the lightmaps for all polygons in this thing.
   */
  void InitLightMaps (bool do_cache = true);

  /**
   * Check frustum visibility on this thing.
   */
  void RealCheckFrustum (csFrustumView& lview);

  /**
   * Check frustum visibility on this thing.
   * First initialize the 2D culler cube.
   */
  void CheckFrustum (csFrustumView& lview);

  /**
   * Cache the lightmaps for all polygons in this thing.
   */
  void CacheLightMaps ();

  /**
   * Intersects world-space sphere with polygons of this set. Return
   * polygon it hits with (or NULL) and the intersection point
   * in world coordinates. It will also return the polygon with the
   * closest hit (the most nearby polygon).
   * If 'pr' != NULL it will also return the distance where the
   * intersection happened.
   */
  csPolygon3D* IntersectSphere (csVector3& center, float radius, float* pr = NULL);

  /**
   * Add polygons and vertices from the specified template. Replace the materials if they 
   * match one in the matList.
   */
  void MergeTemplate (csThing* tpl, csSector* sector, csMaterialList* matList, const char* prefix, 
	csMaterialWrapper* default_material = NULL,
  	float default_texlen = 1, bool objspace = false,
	csVector3* shift = NULL, csMatrix3* transform = NULL);

  /**
   * Add polygons and vertices from the specified thing (seen as template).
   */
  void MergeTemplate (csThing* tpl, csSector* sector, csMaterialWrapper* default_material = NULL,
  	float default_texlen = 1, bool objspace = false,
	csVector3* shift = NULL, csMatrix3* transform = NULL);

  /// Set parent template
  void SetTemplate (csThing *t)
  { ParentTemplate = t; }

  /// Query parent template
  csThing *GetTemplate () const
  { return ParentTemplate; }

  /**
   * Intersect world-space segment with polygons of this set. Return
   * polygon it intersects with (or NULL) and the intersection point
   * in world coordinates.<p>
   *
   * If 'pr' != NULL it will also return a value between 0 and 1
   * indicating where on the 'start'-'end' vector the intersection
   * happened.
   */
  csPolygon3D* IntersectSegment (const csVector3& start, 
                                       const csVector3& end, csVector3& isect,
				       float* pr = NULL);

  /**
   * Transform to the given camera if needed. This function works
   * via the transformation manager and will only transform if needed.
   */
  void UpdateTransformation (const csCamera& c)
  {
    cam_verts_set.Transform (wor_verts, num_vertices, (const csTransform&)c);
    cam_verts = cam_verts_set.GetVertexArray ()->GetVertices ();
  }

  /**
   * Translate with the given vector if needed. This function works
   * via the transformation manager and will only translate if needed.
   */
  void UpdateTransformation (const csVector3& trans)
  {
    cam_verts_set.Translate (wor_verts, num_vertices, trans);
    cam_verts = cam_verts_set.GetVertexArray ()->GetVertices ();
  }

  /// Make transformation table ready but don't do the transform yet.
  void UpdateTransformation ()
  {
    cam_verts_set.Update ();
    cam_verts_set.GetVertexArray ()->SetNumVertices (num_vertices);
    cam_verts = cam_verts_set.GetVertexArray ()->GetVertices ();
  }

  /// Make sure the camera vertices are up-to-date to the current camera frame.
  void CamUpdate ()
  {
    cam_verts_set.CheckUpdate ();
    cam_verts = cam_verts_set.GetVertexArray ()->GetVertices ();
  }

  /**
   * Do a hard transform of the object vertices.
   * This transformation and the original coordinates are not
   * remembered but the object space coordinates are directly
   * computed (world space coordinates are set to the object space
   * coordinates by this routine).
   */
  void HardTransform (const csReversibleTransform& t);

  /// Get the array of camera vertices.
  csVector3* GetCameraVertices ()
  {
    CamUpdate ();
    return cam_verts;
  }

  /// Get the engine for this thing.
  csEngine* GetEngine () { return engine; }

  /**
   * Return a list of shadow frustums which extend from
   * this polygon set. The origin is the position of the light.
   * Note that this function uses camera space coordinates and
   * thus assumes that this polygon set is transformed to the
   * origin of the light.
   */
  csFrustumList* GetShadows (csSector* sector, csVector3& origin);

  /**
   * Create an oriented bounding box (currently not oriented yet@@@)
   * for this polygon set. This function will add the vertices for the
   * bounding box to the set itself so that they will get translated
   * together with the other vertices. The indices of the vertices
   * are added to the csThingBBox structure which is returned here.
   * Note that this creation is done in object space. The newly added
   * vertices will not have been translated to world/camera space yet.<p>
   */
  void CreateBoundingBox ();

  /// Get the oriented bounding box created by CreateBoundingBox().
  csThingBBox* GetBoundingBox ()
  {
    if (!bbox) CreateBoundingBox ();
    return bbox;
  }

  /**
   * Get the bounding box in object space for this polygon set.
   * This is calculated based on the oriented bounding box.
   */
  void GetBoundingBox (csBox3& box);

  /**
   * Get the radius in object space for this polygon set.
   */
  const csVector3& GetRadius ();

  /// Return true if this has fog.
  bool HasFog () { return fog.enabled; }

  /// Return fog structure.
  csFog& GetFog () { return fog; }

  /// Conveniance function to set fog to some setting.
  void SetFog (float density, const csColor& color)
  {
    fog.enabled = true;
    fog.density = density;
    fog.red = color.red;
    fog.green = color.green;
    fog.blue = color.blue;
  }

  /// Disable fog.
  void DisableFog () { fog.enabled = false; }

  /**
   * Find the minimum and maximum Z values of all vertices in
   * this polygon set (in camera space). This is used for planed fog.
   */
  void GetCameraMinMaxZ (float& minz, float& mazx);

  /**
   * Intersect this polygon set in camera space with a polygon which
   * coincides with plane Zc = <value> (a plane parallel to the view
   * plane) and return a new polygon which is the intersection (as a 2D
   * polygon with z not given). This function assumes that the
   * polygon set is convex (so it can in general not be used for polygon
   * sets which use a BSP tree) and gives unexpected results otherwise.
   * Delete the returned polygon with 'delete []' when ready.
   */
  csVector2* IntersectCameraZPlane (float z, csVector2* clipper, int num_clip, int& num_pts);

  /// Register a visibility object with this culler.
  void RegisterVisObject (iVisibilityObject* visobj);
  /// Unregister a visibility object with this culler.
  void UnregisterVisObject (iVisibilityObject* visobj);
  /**
   * Do the visibility test from a given viewpoint. This will first
   * clear the visible flag on all registered objects and then it will
   * mark all visible objects.
   */
  void VisTest (iCamera* camera);

  CSOBJTYPE;
  DECLARE_IBASE_EXT (csPObject);

  //------------------------- iThing interface --------------------------------
  struct eiThing : public iThing
  {
    DECLARE_EMBEDDED_IBASE (csThing);
 
    virtual csThing *GetPrivateObject () { return scfParent; }
    virtual const char *GetName () const { return scfParent->GetName (); }
    virtual void SetName (const char *iName) { scfParent->SetName (iName); }
    virtual void CompressVertices () { scfParent->CompressVertices(); }
    virtual int GetPolygonCount () { return scfParent->polygons.Length (); }
    virtual iPolygon3D *GetPolygon (int idx);
    virtual iPolygon3D *CreatePolygon (const char *iName);
    virtual int GetVertexCount () { return scfParent->num_vertices; }
    virtual csVector3 &GetVertex (int idx) { return scfParent->obj_verts [idx]; }
    virtual csVector3 &GetVertexW (int idx) { return scfParent->wor_verts [idx]; }
    virtual csVector3 &GetVertexC (int idx) { return scfParent->cam_verts [idx]; }
    virtual int CreateVertex (csVector3 &iVertex)
    { return scfParent->AddVertex (iVertex.x, iVertex.y, iVertex.z); }
    virtual bool CreateKey (const char *iName, const char *iValue);
    virtual iMovable* GetMovable () { return &scfParent->GetMovable ().scfiMovable; }
  } scfiThing;
  friend struct eiThing;
 
  //-------------------- iPolygonMesh interface implementation ------------------
  struct PolyMesh : public iPolygonMesh
  {
    DECLARE_EMBEDDED_IBASE (csThing);

    virtual int GetNumVertices () { return scfParent->GetNumVertices (); }
    virtual csVector3* GetVertices () { return scfParent->wor_verts; }
    virtual int GetNumPolygons ()
    {
      GetPolygons ();	// To make sure our count is ok.
      return num;
    }
    virtual csMeshedPolygon* GetPolygons ();

    PolyMesh () { polygons = NULL; }
    virtual ~PolyMesh () { delete[] polygons; }

    csMeshedPolygon* polygons;
    int num;
  } scfiPolygonMesh;
  friend struct PolyMesh;
 
  //-------------------- iVisibilityCuller interface implementation ------------------
  struct VisCull : public iVisibilityCuller
  {
    DECLARE_EMBEDDED_IBASE (csThing);
    virtual void RegisterVisObject (iVisibilityObject* visobj)
    {
      scfParent->RegisterVisObject (visobj);
    }
    virtual void UnregisterVisObject (iVisibilityObject* visobj)
    {
      scfParent->UnregisterVisObject (visobj);
    }
    virtual void VisTest (iCamera* camera)
    {
      scfParent->VisTest (camera);
    }
  } scfiVisibilityCuller;
  friend struct VisCull;
};

#endif // __CS_THING_H__
