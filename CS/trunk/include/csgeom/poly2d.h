/*
    Copyright (C) 1998 by Jorrit Tyberghein
  
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

#ifndef POLY2D_H
#define POLY2D_H

#include "csgeom/math2d.h"
#include "csgeom/box.h"

class csClipper;
class Dumper;

/**
 * The following class represents a general 2D polygon with
 * a bounding box.
 */
class csPoly2D
{
  friend class Dumper;

protected:
  /// The 2D vertices.
  csVector2* vertices;
  ///
  int num_vertices;
  ///
  int max_vertices;

  /// A 2D bounding box that is maintained automatically.
  csBox2 bbox;

public:
  /**
   * Make a new empty polygon.
   */
  csPoly2D (int start_size = 10);

  /// Copy constructor.
  csPoly2D (csPoly2D& copy);

  /// Destructor.
  virtual ~csPoly2D ();

  /**
   * Initialize the polygon to empty.
   */
  void MakeEmpty ();

  /**
   * Get the number of vertices.
   */
  int GetNumVertices () { return num_vertices; }

  /**
   * Get the array with all vertices.
   */
  csVector2* GetVertices () { return vertices; }

  /**
   * Get the specified vertex.
   */
  csVector2* GetVertex (int i) 
  {
    if (i<0 || i>=num_vertices) return NULL;
    return &vertices[i];
  }

  /**
   * Get the specified vertex.
   */
  csVector2& operator[] (int i)
  {
    return vertices[i];
  }

  /**
   * Get the first vertex.
   */
  csVector2* GetFirst ()
  { if (num_vertices<=0) return NULL;  else return vertices; }

  /**
   * Get the last vertex.
   */
  csVector2* GetLast ()
  { if (num_vertices<=0) return NULL;  else return &vertices[num_vertices-1]; }

  /**
   * Test if this vector is inside the polygon.
   */
  bool In (const csVector2& v);

  /**
   * Test if a vector is inside the given polygon.
   */
  static bool In (csVector2* poly, int num_poly, const csVector2& v);

  /**
   * Make room for at least the specified number of vertices.
   */
  void MakeRoom (int new_max);

  /**
   * Set the number of vertices.
   */
  void SetNumVertices (int n) { MakeRoom (n); num_vertices = n; }

  /**
   * Add a vertex (2D) to the polygon.
   * Return index of added vertex.
   */
  int AddVertex (const csVector2& v) { return AddVertex (v.x, v.y); }

  /**
   * Add a vertex (2D) to the polygon.
   * Return index of added vertex.
   */
  int AddVertex (float x, float y);

  /**
   * Set all polygon vertices at once.
   * Note! This doesn't update the bounding box!
   */
  void SetVertices (csVector2 *v, int num)
  { memcpy (vertices, v, (num_vertices = num) * sizeof (csVector2)); }

  /// Update the bounding box (useful after SetVertices).
  void UpdateBoundingBox ();

  /// Get the bounding box (in 2D space) for this polygon.
  csBox2& GetBoundingBox () { return bbox; }

  /**
   * Clipping routines. They return false if the resulting polygon is not
   * visible for some reason.
   * Note that these routines must not be called if the polygon is not visible.
   * These routines will not check that.
   * Note that these routines will put the resulting clipped 2D polygon
   * in place of the original 2D polygon.
   */
  bool ClipAgainst (csClipper* view);

  /**
   * Intersect this polygon with a given plane and return the
   * two resulting polygons in left and right. This version is
   * robust. If one of the edges of this polygon happens to be
   * on the same plane as 'plane' then the edge will go to the
   * polygon which already has most edges. i.e. you will not
   * get degenerate polygons.
   */
  void Intersect (const csPlane2& plane, csPoly2D* left, csPoly2D* right);
};

/**
 * This factory is responsible for creating csPoly2D objects or subclasses
 * of csPoly2D. To create a new factory which can create subclasses of csPoly2D
 * you should create a subclass of this factory.
 */
class csPoly2DFactory
{
public:
  /// A shared factory that you can use.
  static csPoly2DFactory* SharedFactory();

  /// Create a poly2d.
  virtual csPoly2D* Create () { CHK (csPoly2D* p = new csPoly2D ()); return p; }
};

#endif /*POLY2D_H*/
