/*
    Copyright (C) 2000 by Jorrit Tyberghein
  
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

#ifndef SOLIDBSP_H
#define SOLIDBSP_H

#include "csgeom/math2d.h"
#include "csgeom/box.h"
#include "csgeom/polyedge.h"

class csPoly2DEdges;
class csSolidBsp;
class csSolidBspNodePool;

/**
 * A solid BSP node.
 */
class csSolidBspNode
{
  friend class csSolidBsp;
  friend class csSolidBspNodePool;

private:
  /// Left child.
  csSolidBspNode* left;
  /// Right child.
  csSolidBspNode* right;
  /// The splitter plane dividing 'left' and 'right'.
  csPlane2 splitter;
  /// Center point on original edge that was using for the splitter plane.
  csVector2 split_center;
  /// True if this node is solid.
  bool solid;

private:
  /// Make an empty node.
  csSolidBspNode () : left (NULL), right (NULL), solid (false) { }

  /**
   * Destroy this node.
   */
  ~csSolidBspNode ();
};

/**
 * A pool for solid BSP nodes. This pool is a bit special.
 * You can free entire sub-trees in one go. If you then later
 * request a new node from the pool then it will take that sub-tree
 * extract the two children, put them in the pool as two new free
 * sub-trees and return the resulting empty node.
 */
class csSolidBspNodePool
{
private:
  struct PoolObj
  {
    PoolObj* next;
    csSolidBspNode* node;
  };
  /// List of allocated nodes.
  PoolObj* alloced;
  /// List of previously allocated, but now unused polygons.
  PoolObj* freed;

public:
  /// Create an empty pool.
  csSolidBspNodePool () : alloced (NULL), freed (NULL) { }

  /// Destroy pool and all objects in the pool.
  ~csSolidBspNodePool ();

  /**
   * Allocate a new object in the pool.
   */
  csSolidBspNode* Alloc ();

  /**
   * Free an object and put it back in the pool.
   * Note that it is only legal to free objects which were allocated
   * from the pool.
   */
  void Free (csSolidBspNode* node);

  /// Dump some information about this pool.
  void Dump ();
};

/**
 * This 2D BSP is a special type of solid BSP designed specifically
 * for 2D visibility culling and shadow collection on a face.
 * The individual polygons inserted in this tree are irrelevant. The
 * only thing that matters is wether or not a node is solid (i.e.
 * completely covered) or not. Nodes that are solid and contain several
 * sub-nodes with other polygons/nodes can be removed by the BSP tree
 * at any time so you cannot depend on the polygons being there later.
 */
class csSolidBsp
{
private:
  /// The root of the tree.
  csSolidBspNode* root;

  /// A pool of nodes.
  static csSolidBspNodePool node_pool;
  /// A pool of polygons.
  static csPoly2DEdgesPool poly_pool;

private:
  /// Insert a polygon in the node.
  bool InsertPolygon (csSolidBspNode* node, csPoly2DEdges* poly);

  /// Test a polygon in the node.
  bool TestPolygon (csSolidBspNode* node, csPoly2DEdges* poly);

public:
  /**
   * Create an empty tree.
   */
  csSolidBsp ();

  /**
   * Destroy the whole tree.
   */
  virtual ~csSolidBsp ();

  /**
   * Make tree empty.
   */
  void MakeEmpty ();

  /**
   * Is the tree full?
   */
  bool IsFull () { return false; }

  /**
   * Insert a polygon into the quad-tree.
   * Return true if the tree was modified (i.e. if parts of the
   * polygon were visible.
   */
  bool InsertPolygon (csVector2* verts, int num_verts);

  /**
   * Test for polygon visibility with the quad-tree.
   * Return true if polygon is visible.
   */
  bool TestPolygon (csVector2* verts, int num_verts);
};

#endif /*SOLIDBSP_H*/

