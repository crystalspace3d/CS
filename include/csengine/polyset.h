/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein
  
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

#ifndef POLYSET_H
#define POLYSET_H

#include "csutil/scf.h"
#include "csutil/csvector.h"
#include "csgeom/math3d.h"
#include "csgeom/math2d.h" // texel coords
#include "csobject/csobject.h"
#include "csengine/tranman.h"
#include "csengine/arrays.h"
#include "csutil/cscolor.h"
#include "igraph3d.h"
#include "ipolygon.h"

class csPolygonInt;
class csSector;
class csWorld;
class csCamera;
class csTextureHandle;
class CLights;
class csPolygon2D;
class csPolygon2DQueue;
class csCollider;
class csBspTree;
class Dumper;
class csRenderView;
class csFrustrumList;
class csCurve;
struct iPolygonSet;

/**
 * This structure keeps the indices of the vertices which
 * define the bounding box of a csPolygonSet. It is calculated
 * by CreateBoundingBox() and stored with the csPolygonSet.<p>
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
struct csPolygonSetBBox
{
  int i1, i2, i3, i4, i5, i6, i7, i8;
};

/**
 * A PolygonSet class is a set of polygons (amazing, isn't it :-)
 * A PolygonSet describes a set of polygons that form a convex and
 * (probably) closed hull. All polygons in a set share vertices
 * from the same pool.<p>
 *
 * Every polygon in the set has a visible and an invisible face;
 * if the vertices of the polygon are ordered clockwise then the
 * polygon is visible. Using this feature it is possible to define
 * two kinds of PolygonSets: in one kind the polygons are oriented
 * such that they are visible from within the hull. In other words,
 * the polygons form a sort of container or room where the camera
 * can be located. We call this kind of PolygonSet a csSector (a
 * subclass of PolygonSet). In another kind the polygons are
 * oriented such that they are visible from the outside. We call
 * this kind of PolygonSet a Thing (another subclass of PolygonSet).<p>
 *
 * Things and sectors have many similarities. That's why the
 * PolygonSet class was created: to exploit these similarities.
 * However, there are some important differences between csThings and
 * csSectors:<p>
 * <ul>
 * <li> Currently, only things can move. This means that the object
 *      space coordinates of a sector are ALWAYS equal to the world
 *      space coordinates. It would be possible to allow moveable
 *      sectors but I don't how this should be integrated into an
 *      easy model of the world.
 * <li> Things do not require portals but can use them. 
 * </ul>
 */
class csPolygonSet : public csObject, public iPolygonSet
{
  friend class Dumper;

protected:
  /**
   * PolygonSets are linked either in a csWorld object or in another
   * PolygonSet (Thing in csSector for example).
   */
  csPolygonSet* next;
  /// Parent object (sector or world) in which this object is linked.
  csObject* parent;

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

  /// csSector where this polyset belongs (pointer to 'this' if it is a sector).
  csSector* sector;

  /// Optional oriented bounding box.
  csPolygonSetBBox* bbox;

  /**
   * Light frame number. Using this number one can see if gouraud shaded
   * vertices have been initialized already.
   */
  long light_frame_number;

  /**
   * Draw one 3D/2D polygon combination. The 2D polygon is the transformed
   * and clipped version of the 3D polygon.
   */
  void DrawOnePolygon (csPolygon3D* p, csPolygon2D* poly, csRenderView* d,
	bool use_z_buf);
  /**
   * Draw the given array of polygons in the current csPolygonSet. This
   * function is called by subclasses of csPolygonSet (csSector and
   * csThing currently).
   */
  void DrawPolygonArray (csPolygonInt** polygon, int num, csRenderView* rview,
  	bool use_z_buf);

  /**
   * Test a number of polygons against the c-buffer and insert them to the
   * c-buffer if visible and also add them to a queue.
   * If 'pvs' is true then the PVS is used (polygon->IsVisible()).
   */
  void* TestQueuePolygonArray (csPolygonInt** polygon, int num, csRenderView* d,
  	csPolygon2DQueue* poly_queue, bool pvs);

private:
  /// Fog information.
  csFog fog;

public:
  /**
   * Current light frame number. This is used for seeing
   * if gouraud shading should be recalculated for this polygonset.
   * If there is a mismatch between the frame number of this set
   * and the global number then the gouraud shading is not up-to-date.
   */
  static long current_light_frame_number;

  /**
   * How many times are we busy drawing this polyset (recursive).
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

  /// Construct a csPolygonSet.
  csPolygonSet ();

  /**
   * Delete all contents of this polygonset (vertices,
   * polygons, curves, and BSP tree).
   */
  virtual ~csPolygonSet ();

  /**
   * Prepare all polygons for use. This function MUST be called
   * AFTER the texture manager has been prepared. This function
   * is normally called by csWorld::Prepare() so you only need
   * to worry about this function when you add sectors or things
   * later.
   */
  virtual void Prepare ();

  /// Just add a new vertex to the polygonset.
  int AddVertex (const csVector3& v) { return AddVertex (v.x, v.y, v.z); }

  /// Just add a new vertex to the polygonset.
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

  /// Add a polygon to this polygonset.
  void AddPolygon (csPolygonInt* spoly);

  /// Create a new polygon in this polygonset and add it.
  csPolygon3D* NewPolygon (csTextureHandle* texture);

  /// Get the number of polygons in this polygonset.
  int GetNumPolygons ()
  { return polygons.Length (); }

  /// Get a csPolygonInt with the index.
  csPolygonInt* GetPolygon (int idx);

  /// Get the specified polygon from this set.
  csPolygon3D *GetPolygon3D (int idx)
  { return polygons.Get (idx); }

  /// Get the named polygon from this set.
  csPolygon3D *GetPolygon3D (char* name);

  /// Get the entire array of polygons.
  csPolygonArray& GetPolygonArray () { return polygons; }

  /// Add a curve to this polygonset.
  void AddCurve (csCurve* curve);

  /// Get the number of curves in this polygonset.
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

  /// Get the specified curve texture coordinate (texel).
  csVector2& CurveTexel (int i) { return curve_texels[i]; }

  /// Add a curve vertex and return the index of the vertex.
  int AddCurveVertex (csVector3& v, csVector2& t);

  /**
   * Intersect world-space segment with polygons of this set. Return
   * polygon it intersects with (or NULL) and the intersection point
   * in world coordinates.<p>
   *
   * If 'pr' != NULL it will also return a value between 0 and 1
   * indicating where on the 'start'-'end' vector the intersection
   * happened.
   */
  virtual csPolygon3D* IntersectSegment (const csVector3& start, 
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

  /// Get the array of camera vertices.
  csVector3* GetCameraVertices ()
  {
    CamUpdate ();
    return cam_verts;
  }

  /// Get the next polygonset in its linked list.
  csPolygonSet* GetNext () { return next; }
  /// Get the parent for this polygonset.
  csObject* GetParent () { return parent; }

  /// Set the next polygonset and parent in its linked list.
  void SetNext (csObject* parent, csPolygonSet* next)
  {
    csPolygonSet::parent = parent;
    csPolygonSet::next = next;
  }

  /**
   * Set the sector that this polygonset belongs to.
   * This is either the polygonset itself if it is a sector
   * or else the sector that this thing is in.
   */
  void SetSector (csSector* sector) { csPolygonSet::sector = sector; }

  /// Return the sector that this polygonset belongs to.
  csSector* GetSector () { return sector; }

  /**
   * Return a list of shadow frustrums which extend from
   * this polygon set. The origin is the position of the light.
   * Note that this function uses camera space coordinates and
   * thus assumes that this polygon set is transformed to the
   * origin of the light.
   */
  csFrustrumList* GetShadows (csVector3& origin);

  /**
   * Create an oriented bounding box (currently not oriented yet@@@)
   * for this polygon set. This function will add the vertices for the
   * bounding box to the set itself so that they will get translated
   * together with the other vertices. The indices of the vertices
   * are added to the csPolygonSetBBox structure which is returned here.
   * Note that this creation is done in object space. The newly added
   * vertices will not have been translated to world/camera space yet.<p>
   */
  void CreateBoundingBox ();

  /// Get the oriented bounding box created by CreateBoundingBox().
  csPolygonSetBBox* GetBoundingBox () { return bbox; }

  /**
   * Get the bounding box in object space for this polygon set.
   * This is calculated based on the oriented bounding box.
   */
  void GetBoundingBox (csVector3& min_bbox, csVector3& max_bbox);

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

  CSOBJTYPE;
  DECLARE_IBASE;

  /// Same as GetName()
  virtual const char *GetObjectName () { return GetName (); }
};

#endif /*POLYSET_H*/
