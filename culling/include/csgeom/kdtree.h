/*
    Copyright (C) 2002 by Jorrit Tyberghein

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

#ifndef __CS_KDTREE_H__
#define __CS_KDTREE_H__

#include "csextern.h"

#include "csgeom/box.h"
#include "csgeom/sphere.h"

#include "csutil/blockallocator.h"
#include "csutil/ref.h"
#include "csutil/scfstr.h"
#include "csutil/scf_implementation.h"
#include "csutil/floatrand.h"

#include "iutil/dbghelp.h"

/**\file
 * KD-tree implementation.
 */
/**\addtogroup geom_utils
 * @{ */

struct iGraphics3D;
struct iString;

class CS_CRYSTALSPACE_EXPORT csKDTreeChildCommon
{
private:
  // Pointer back to the original object.
  void* object;

  // leaves we belong to
  csArray<void*> leafs;

public:
  // timestamp for the last time this object was visited
  uint32 timestamp;

  /**
   * Set the pointer to the black box object.
   */
  inline void SetObject (void* obj)
  {
    object = obj;
  }

  /**
   * Get the pointer to the black box object.
   */
  inline void* GetObject () const
  {
    return object;
  }

  /// Physically add a leaf to this child.
  void AddLeaf (void* leaf)
  {
    leafs.Push (leaf);
  }

  /// Physically remove a leaf from this child.
  void RemoveLeaf (int idx)
  {
    leafs.DeleteIndexFast (idx);
  }

  /// Physically remove a leaf from this child.
  void RemoveLeaf (void* leaf)
  {
    leafs.Delete (leaf);
  }

  /**
   * Replace a leaf with another one. This is more
   * efficient than doing RemoveLeaf/AddLeaf and it is
   * useful in many cases where you want to move a child
   * in the tree.
   */
  void ReplaceLeaf (void* old_leaf, void* new_leaf)
  {
    // find the leaf to replace
    size_t idx = leafs.Find (old_leaf);

    // ensure it was found
    CS_ASSERT(idx != csArrayItemNotFound);

    // replace it
    leafs[idx] = new_leaf;
  }

  /**
   * Find leaf.
   */
  int FindLeaf (void* leaf)
  {
    return static_cast<int> (leafs.Find (leaf));
  }

  // Get number of leaves this child belongs to.
  inline int GetLeafCount () const
  {
    return static_cast<int> (leafs.GetSize ());
  }

  // Get a leaf this child belongs to.
  inline void* GetLeaf (int idx) const
  {
    return leafs[idx];
  }

};

/**
 * A child in a KD-tree using bounding boxes.
 */
class CS_CRYSTALSPACE_EXPORT csKDTreeChild
  : public csBox3, public csKDTreeChildCommon
{
public:
  // used by implementation
  typedef csBox3 BoundType;

  // set user-supplied boundaries
  inline void SetBounds(BoundType const& bounds)
  {
    Set(bounds.Min(), bounds.Max());
  }

  // create a random bound for benchmarking purposes
  static BoundType RandomBound()
  {
    static csRandomFloatGen rng;
    float x = rng.Get (-50,50);
    float y = rng.Get (-50,50);
    float z = rng.Get (-50,50);
    return BoundType (x, y, z, rng.Get (x, x+7),
      rng.Get (y, y+7), rng.Get (z, z+7));
  }

  /**
   * Get the bounding box of this object.
   */
  inline csBox3 const& GetBBox () const
  {
    return *this;
  }
};

/**
 * A child in a KD-tree using bounding boxes.
 */
class CS_CRYSTALSPACE_EXPORT csKDTreeSphereChild
  : public csSphere, public csKDTreeChildCommon
{
private:
  csBox3 box;

public:
  // used by implementation
  typedef csSphere BoundType;

  // set user-supplied boundaries
  inline void SetBounds(BoundType const& bounds)
  {
    // copy from other sphere
    SetCenter (bounds.GetCenter ());
    SetRadius (bounds.GetRadius ());

    // update our bounding box
    box.Set (bounds.GetCenter () - csVector3 (bounds.GetRadius ()),
	     bounds.GetCenter () + csVector3 (bounds.GetRadius ()));
  }

  // create a random bound for benchmarking purposes
  static BoundType RandomBound()
  {
    static csRandomFloatGen rng;
    return BoundType (
      csVector3 (rng.Get (-50,50), rng.Get (-50,50), rng.Get (-50,50)),
      rng.Get (0,7));
  }


  /**
   * Get the bounding box of this object.
   */
  inline csBox3 const& GetBBox () const
  {
    return box;
  }
};

enum
{
  CS_KDTREE_AXISINVALID = -1,
  CS_KDTREE_AXISX = 0,
  CS_KDTREE_AXISY = 1,
  CS_KDTREE_AXISZ = 2
};

#define KDTREE_MAX 100000.

namespace CS
{
namespace Geometry
{
/**
 * A KD-tree.
 * A KD-tree is a binary tree that organizes 3D space.
 * This implementation is dynamic. It allows moving, adding, and
 * removing of objects which will alter the tree dynamically.
 * The main purpose of this tree is to provide for an approximate
 * front to back ordering.
 * <p>
 * The KD-tree supports delayed insertion. When objects are inserted
 * in the tree they are not immediatelly distributed over the
 * nodes but instead they remain in the main node until it is really
 * needed to distribute them. The advantage of this is that you can
 * insert/remove a lot of objects in the tree and then do the distribution
 * calculation only once. This is more efficient and it also generates
 * a better tree as more information is available then.
 */
template<class Child>
class KDTree : public scfImplementation1<KDTree<Child>, iDebugHelper>
{
public:
  // convenience typedefs
  typedef KDTree<Child> Self;
  typedef Child Child;
  typedef typename Child::BoundType BoundType;

  /**
   * A callback function for visiting a KD-tree node. If this function
   * returns true the traversal will continue. Otherwise Front2Back()
   * will stop.
   * <p>
   * This function is itself responsible for calling Distribute() on
   * the given treenode to ensure that the objects in this node
   * are properly distributed to the children. If the function doesn't
   * want or need this functionality it doesn't have to do Distribute().
   * <p>
   * If this function decides to process the given node then it is
   * also responsible for checking the timestamp of every child in this
   * node with the timestamp given to this function. If this timestamp
   * is different the child has not been processed before. This function
   * should then update the timestamp of the child. If this is not done
   * then some objects will be encountered multiple times. In some
   * cases this may not be a problem or even desired.
   * <p>
   * 'frustum_mask' can be modified by this function to reduce the number
   * of plane tests (for frustum culling) that have to occur for children
   * of this node.
   */
  typedef bool (VisitFunc)(Self* treenode, void* userdata,
	  uint32 timestamp, uint32& frustum_mask);

  /**
   * If you implement this interface then you can give that to the
   * KDtree. The KDtree can then use this to find the description of an object.
   * This can be used for debugging as the KDtree will print out that description
   * if it finds something is wrong.
   */
  struct iObjectDescriptor : public virtual iBase
  {
    SCF_INTERFACE (iObjectDescriptor, 0, 0, 1);

    virtual csPtr<iString> DescribeObject (Child* child) = 0;
  };

  /**
   * The data type for user data to be attached to the KDTree.
   * It provides no functions but makes it possible to do a direct cast
   * for performance instead of doing an scfQueryInterface.
   */
  struct iUserData : public virtual iBase
  {
    SCF_INTERFACE (iUserData, 0, 0, 1);
  };

private:
  // allocators
  csBlockAllocator<Child>* childAlloc;
  csBlockAllocator<Self>* treeAlloc;

  // childs of this branch
  // either both of those are valid (this is a branch)
  // or both of them are nullptr (this is a leaf)
  Self* child1;
  Self* child2;
  // the parent node in the tree - nullptr if this is the root
  Self* parent;

  // An optional user object for this node.
  csRef<iUserData> userobject;

  // This is used for debugging.
  csRef<iObjectDescriptor> descriptor;

  csBox3 node_bbox;             // Bbox of the node itself.

  int split_axis;               // One of CS_KDTREE_AXIS?
  float split_location;         // Where is the split?

  // Objects in this node. If this node also has children (child1
  // and child2) then the objects here have to be moved to these
  // children. The 'Distribute()' function will do that.
  Child** objects;
  int num_objects;
  int max_objects;

  // Estimate of the total number of objects in this tree including children.
  int estimate_total_objects;

  // Minimum amount of objects in this tree before we consider splitting.
  int min_split_objects;

  // Disallow Distribute().
  // If this flag > 0 it means that we cannot find a good split
  // location for the current list of objects. So in that case we don't
  // split at all and set this flag to DISALLOW_DISTRIBUTE_TIME so
  // that we will no longer attempt to distribute for a while. Whenever
  // objects are added or removed to this node this flag will be decreased
  // so that when it becomes 0 we can make a new Distribute() attempt can
  // be made. This situation should be rare though.
#define DISALLOW_DISTRIBUTE_TIME 20
  int disallow_distribute;

  // Current timestamp we are using for Front2Back(). Objects that
  // have the same timestamp are already visited during Front2Back().
  uint32 global_timestamp;

  // get tree allocator
  inline csBlockAllocator<Self>& TreeAlloc ()
  {
    if (treeAlloc == nullptr)
    {
      if (parent == nullptr)
      {
	treeAlloc = new csBlockAllocator<Self> ();
      }
      else
      {
	treeAlloc = &parent->TreeAlloc ();
      }
    }
    return *treeAlloc;
  }

  // get child allocator
  inline csBlockAllocator<Child>& ChildAlloc ()
  {
    if (childAlloc == nullptr)
    {
      if (parent == nullptr)
      {
	childAlloc = new csBlockAllocator<Child> ();
      }
      else
      {
	childAlloc = &parent->ChildAlloc ();
      }
    }
    return *childAlloc;
  }

  /// Physically add a child to this tree node.
  void AddObject (Child* obj)
  {
    // check for corrupt object storage
    CS_ASSERT (((max_objects == 0) == (objects == nullptr)));

    // check whether we have to grow the obejct storage
    if (num_objects >= max_objects)
    {
      // double the storage size, but grow it by at least 80 objects
      max_objects += csMin (max_objects+2, 80);

      // allocate new storage
      Child** new_objects = new Child* [max_objects];

      // check for allocation failure
      CS_ASSERT (new_objects);

      // copy old objects to new storage if necessary
      if (objects && num_objects > 0)
      {
	memcpy (new_objects, objects, sizeof (csKDTreeChild*) * num_objects);
      }

      // free old storage
      delete[] objects;

      // set new storage
      objects = new_objects;
    }

    // add object to storage
    objects[num_objects++] = obj;

    // update estimated object count
    estimate_total_objects++;
  }

  /**
   * Unlink an object from the kd-tree. The 'Child' instance
   * will NOT be deleted.
   */
  void UnlinkObject (Child* object)
  {
    // remove this object from all leaves it belongs to
    for (int i = object->GetLeafCount () - 1 ; i >= 0 ; i--)
    {
      // get one of the leaves
      Self* leaf = static_cast<Self*> (object->GetLeaf (i));

      // check it actually knows about the object
      int idx = leaf->FindObject (object);

      // ensure the object is known to the leaf
      CS_ASSERT (idx != -1);

      // remove the object from the leaf
      leaf->RemoveObject (idx);

      // reduce distribution block on leaf
      if (leaf->disallow_distribute > 0)
	leaf->disallow_distribute--;

      // remove the leaf from the object
      object->RemoveLeaf (i);
    }
  }

  /// Physically remove a child from this tree node.
  void RemoveObject (int idx)
  {
    // check for out-of-bounds
    CS_ASSERT (idx >= 0 && idx < num_objects);

    // move objets in storage if necessary
    if (idx != num_objects - 1)
    {
      memmove (&objects[idx], &objects[idx+1],
	  sizeof (csKDTreeChild*) * (num_objects-idx-1));
    }

    // update object counts
    estimate_total_objects--;
    num_objects--;
  }

  /// Find an object. Returns -1 if not found.
  int FindObject (Child* obj)
  {
    // iterate over all objects looking for ours
    for (int i = 0 ; i < num_objects ; i++)
    {
      // check whether we found it
      if (objects[i] == obj)
      {
	// return it's index
	return i;
      }
    }

    // couldn't find it, return an error
    return -1;
  }

  /**
   * Add an object to this kd-tree node.
   */
  void AddObjectInt (Child* obj)
  {
    // decrease distribution block if there is one
    if (disallow_distribute > 0)
      disallow_distribute--;

    // add us as leaf for the object
    obj->AddLeaf (this);

    // physically add object to our tree
    AddObject (obj);
  }

  /**
   * Find the best split position for a given axis. This will
   * return a good position depending on tree balancing (i.e. try
   * to have as many objects left as right) and also minimizing the
   * number of objects that are cut. It will return a quality
   * value which is 0 for very bad and positive for good. It will
   * also return the location to split in the 'split_loc' value.
   * If this function returns a negative quality this means the
   * split should not be performed at all.
   */
  long FindBestSplitLocation (int axis, float& split_loc)
  {
    // If we have only two objects we use the middle of the
    // empty space between the two if there is any.
    if (num_objects == 2)
    {
      // get the object bounidng boxes to attempt a split
      const csBox3& bbox0 = objects[0]->GetBBox ();
      const csBox3& bbox1 = objects[1]->GetBBox ();

      // check whether they objects are separable
      // test whether the first object is left of the second one
      // (small threshold to avoid a bad split location)
      float max0 = bbox0.Max (axis);
      float min1 = bbox1.Min (axis);
      if (max0 < min1-.01)
      {
	// split in the middle of the empty space
	split_loc = max0 + (min1-max0) * 0.5;

	// validate split
	CS_ASSERT (split_loc > max0 && split_loc < min1);

	// perfect seperation is a good split
	return 10.0;
      }

      // test whether the first object is right of the second one
      // (small threshold to avoid a bad split location)
      float min0 = bbox0.Min (axis);
      float max1 = bbox1.Max (axis);
      if (max1 < min0-.01)
      {
	// split in the middle of the empty space
	split_loc = max1 + (min0-max1) * 0.5;

	// validate split
	CS_ASSERT (split_loc > max1 && split_loc < min0);

	// perfect separation is a good split
	return 10.0;
      }

      // the objects cannot be separated, so there's no point in splitting
      return -1.0;
    }

    // Find minimum and maximum value along the axis.
    // allocate arrays to hold object bounds
    CS_ALLOC_STACK_ARRAY_FALLBACK (float, objectsMin, num_objects, 50000);
    CS_ALLOC_STACK_ARRAY_FALLBACK (float, objectsMax, num_objects, 50000);

    // initialize minimum and maximum
    float mina =  KDTREE_MAX;
    float maxa = -KDTREE_MAX;

    // iterate over all objects filling the arrays and updating the overall bounds
    for (int i = 0 ; i < num_objects ; i++)
    {
      // get object bounding box
      const csBox3& bbox = objects[i]->GetBBox ();

      // get minimum and maximum
      float mi = bbox.Min (axis);
      float ma = bbox.Max (axis);

      // set object bounds
      objectsMin[i] = mi;
      objectsMax[i] = ma;

      // update overall bounds
      if (mi < mina) mina = mi;
      if (ma > maxa) maxa = ma;
    }

    // clamp the overall bounds to the node box as they may exceed it
    // due to objects belonging to multiple nodes
    if (mina < node_bbox.Min (axis)) mina = node_bbox.Min (axis);
    if (maxa > node_bbox.Max (axis)) maxa = node_bbox.Max (axis);

    // reject the split if the interval is too small
    if (fabs (mina - maxa) < 0.0001f) return -1.0f;

    // attempt to find a split
    // intialize best quality
    long best_qual = -2;

    // try a few different splits
    for (int attempt = 0 ; attempt < 5 ; attempt++)
    {
      // we'll try to find a split in the middle of our bounds
      float a = (mina + maxa) / 2.0f;

      // categorize all objects into left and right ones,
      // so we can evaluate the split quality
      int left = 0;
      int right = 0;
      for (int i = 0 ; i < num_objects ; i++)
      {
	// check whether this object would go into the left ndoe
	if (objectsMax[i] < a-.0001) left++;

	// check whether this object would go into the right node
	else if (objectsMin[i] > a+.0001) right++;
      }

      // evaluate the split quality
      int qual;

      // if either side of the split is empty this is a bad split
      // which we shouldn't take
      if (left == 0 || right == 0)
      {
	qual = -1.0;
      }
      else
      {
	// resulting quality is based on how many objects could be
	// seperated and how balanced they are and is strictly positive
	qual = 3 * long(left + right) - long(ABS (left - right));
      }

      // update our best quality and split location if this is the best split
      // so far
      if (qual > best_qual)
      {
	best_qual = qual;
	split_loc = a;
      }

      // if the split was leftish, try a split more to the left
      if (left <= right) maxa = a;
      // else try one more to the right
      else mina = a;
    }

    // return our best split quality
    return best_qual;
  }

  /**
   * If this node is a leaf then we will split the objects currently
   * in this leaf according to the pre-filled in split_axis
   * and split_location.
   */
  void DistributeLeafObjects ()
  {
    // ensure we have a valid split axis
    CS_ASSERT (split_axis >= CS_KDTREE_AXISX && split_axis <= CS_KDTREE_AXISZ);

    // go over all objects and add them to the according child(s)
    for (int i = 0 ; i < num_objects ; i++)
    {
      // get object bounding box so we can categorize it
      const csBox3& bbox = objects[i]->GetBBox ();

      // get upper and lower bound
      float bbox_min = bbox.Min (split_axis);
      float bbox_max = bbox.Max (split_axis);

      // keep track whether we already removed ourself as leaf for the object
      bool leaf_replaced = false;

      // check whether the object belongs in the left node
      // @@@NOTE: SMALL_EPSILON is used to ensure that when bbox_min
      //          is equal to bbox_max we don't get a situation where
      //          both of the if's are not used.
      if (bbox_min-SMALL_EPSILON <= split_location)
      {
	// remove us as leaf for the object and set the child as leaf
	objects[i]->ReplaceLeaf (this, child1);

	// indicate we're already removed as leaf
	leaf_replaced = true;

	// add object to the child
	child1->AddObject (objects[i]);
      }
      // check whether the object (also) belongs to the right node
      if (bbox_max >= split_location)
      {
	// if we already removed ourself, simply add the other leaf
	if (leaf_replaced)
	{
	  objects[i]->AddLeaf (child2);
	}
	// else remove ourself and add the leaf
	else
	{
	  objects[i]->ReplaceLeaf (this, child2);
	  leaf_replaced = true;
	}

	// add object to the child
	child2->AddObject (objects[i]);
      }

      // ensure the object went into a child
      CS_ASSERT (leaf_replaced);
    }

    // update our object count
    num_objects = 0;

    // @@@TODO: Clean up objects array if there are too many objects?
    //          There should be some threshold at least.
  }

  /**
   * Traverse the tree from front to back. Every node of the
   * tree will be encountered. Returns false if traversal should stop.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back (const csVector3& pos, VisitFunc* func,
        void* userdata, uint32 cur_timestamp, uint32 frustum_mask)
  {
    // check whether we want to continue the traversal
    if (!func (this, userdata, cur_timestamp, frustum_mask))
    {
      // we don't, abort
      return;
    }

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // check whether left one goes first
      if (pos[split_axis] <= split_location)
      {
	// yes, continue with left, then right one
	child1->Front2Back (pos, func, userdata, cur_timestamp, frustum_mask);
	child2->Front2Back (pos, func, userdata, cur_timestamp, frustum_mask);
      }
      else
      {
	// no, continue with right, then left one
	child2->Front2Back (pos, func, userdata, cur_timestamp, frustum_mask);
	child1->Front2Back (pos, func, userdata, cur_timestamp, frustum_mask);
      }
    }
  }

  /**
   * Traverse the tree in undefined order. Every node of the
   * tree will be encountered. Returns false if traversal should stop.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void TraverseRandom (VisitFunc* func,
        void* userdata, uint32 cur_timestamp, uint32 frustum_mask)
  {
    // check whether we want to continue the traversal
    if (!func (this, userdata, cur_timestamp, frustum_mask))
    {
      // we don't, abort
      return;
    }

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // we do, continue traversal there
      child1->TraverseRandom (func, userdata, cur_timestamp, frustum_mask);
      child2->TraverseRandom (func, userdata, cur_timestamp, frustum_mask);
    }
  }

  /**
   * Reset timestamps of all objects in this treenode.
   */
  void ResetTimestamps ()
  {
    // clear timestamps for all objects
    for (int i = 0 ; i < num_objects ; i++)
    {
      objects[i]->timestamp = 0;
    }

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // also reset timestamps for children
      child1->ResetTimestamps ();
      child2->ResetTimestamps ();
    }
  }

  /**
   * Flatten the children of this node to the given node.
   */
  void FlattenTo (Self* node)
  {
    // ensure we have either no or two children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (!child1)
    {
      // nope, nothing to be done
      return;
    }

    // First flatten the children.
    // @@@TODO: Is this the most optimal solution?
    child1->FlattenTo (node);
    child2->FlattenTo (node);

    // free children
    TreeAlloc ().Free (child1);
    TreeAlloc ().Free (child2);
    child1 = nullptr;
    child2 = nullptr;

    // add our objects to the target
    for (int i = 0; i < num_objects ; i++)
    {
      // get the object to process
      Child* obj = objects[i];

      // check whether it already belongs to the target
      if (obj->FindLeaf (node) != -1)
      {
	// it does, simply remove us from the leaf list
	obj->RemoveLeaf (this);
      }
      else
      {
	// it doesn't, replace us with the target
	obj->ReplaceLeaf (this, node);

	// add object to the target
	node->AddObject (obj);
      }
    }

    // update our object count
    num_objects = 0;
    estimate_total_objects = 0;
  }

public:
  /// Create a new empty KD-tree.
  KDTree () :
    // scf initialization
    scfImplementationType (this),

    // allocator initialization
    childAlloc (nullptr), treeAlloc (nullptr),

    // child-parent initialization
    child1 (nullptr), child2 (nullptr), parent (nullptr),

    // box initialization
    node_bbox (-KDTREE_MAX, -KDTREE_MAX, -KDTREE_MAX,
	        KDTREE_MAX,  KDTREE_MAX,  KDTREE_MAX),

    // split initialization
    split_axis (CS_KDTREE_AXISINVALID),

    // object storage initialization
    objects (nullptr), num_objects (0), max_objects (0),
    estimate_total_objects (0),

    // distribution initialization
    min_split_objects (20), disallow_distribute (0),

    // global timestamp initialization
    global_timestamp (1)
  {
  }

  /// Destroy the KD-tree.
  virtual ~KDTree ()
  {
    Clear ();
  }

  /// Set the parent.
  void SetParent (Self* p)
  {
    parent = p;
  }

  /// For debugging: set the object descriptor.
  void SetObjectDescriptor (iObjectDescriptor* d)
  {
    descriptor = d;
  }

  /**
   * Set the minimum amount of objects before we consider splitting this tree.
   * By default this is set to 1.
   */
  void SetMinimumSplitAmount (int m)
  {
    min_split_objects = m;
  }

  /// Make the tree empty.
  void Clear ()
  {
    // go over all objects and remove them
    for (int i = 0 ; i < num_objects ; i++)
    {
      objects[i]->RemoveLeaf (this);

      // destruct this object if there are no more leafs refering to it.
      if (objects[i]->GetLeafCount () == 0)
	ChildAlloc().Free (objects[i]);
    }

    // clear object storage
    delete[] objects;
    objects = 0;
    num_objects = 0;
    max_objects = 0;
    estimate_total_objects = 0;

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // destruct children
    if (child1)
    {
      TreeAlloc().Free (child1);
      child1 = nullptr;
      TreeAlloc().Free (child2);
      child2 = nullptr;
    }

    // clear split
    split_axis = CS_KDTREE_AXISINVALID;

    // reset bounding box
    node_bbox.Set(-KDTREE_MAX, -KDTREE_MAX, -KDTREE_MAX,
		   KDTREE_MAX,  KDTREE_MAX,  KDTREE_MAX);

    // clear distribution
    disallow_distribute = 0;

    // free user object
    userobject.Invalidate();
  }

  /// Get the user object attached to this node.
  inline iUserData* GetUserObject () const
  {
    return userobject;
  }

  /**
   * Set the user object for this node. Can be 0 to clear
   * it. The old user object will be DecRef'ed and the (optional)
   * new one will be IncRef'ed.
   */
  void SetUserObject (iUserData* userobj)
  {
    userobject = userobj;
  }

  /**
   * Add an object to this kd-tree node.
   * Returns a Child pointer which represents the object
   * inside the kd-tree. Object addition is delayed. This function
   * will not yet alter the structure of the kd-tree. Distribute()
   * will do that.
   */
  Child* AddObject (BoundType const& bounds, void* object)
  {
    // allocate a new child
    Child* obj = ChildAlloc ().Alloc ();

    // set the object we got on it
    obj->SetObject (object);

    // set boundaries on the object
    obj->SetBounds (bounds);

    // add the child to our tree
    AddObjectInt (obj);

    // return the created child
    return obj;
  }

  /**
   * Remove an object from the kd-tree. The 'Child' instance
   * will be deleted.
   */
  void RemoveObject (Child* object)
  {
    UnlinkObject (object);
    ChildAlloc ().Free (object);
  }

  /**
   * Move an object (give it a new bounding box).
   */
  void MoveObject (Child* object, BoundType const& bounds)
  {
    // ensure the object actually belongs somewhere
    CS_ASSERT (object->GetLeafCount () > 0);

    // get old box
    csBox3 old_bbox(object->GetBBox ());

    // update bounds for the object
    object->SetBounds (bounds);

    // get new box
    csBox3 const& new_bbox = object->GetBBox ();

    // First check if the bounding box actually changed.
    csVector3 dmin = old_bbox.Min () - new_bbox.Min ();
    csVector3 dmax = old_bbox.Max () - new_bbox.Max ();
    if ((dmin < .00001f) && (dmax < .00001f))
    {
      return;
    }

    // get the first leaf for the object
    Self* leaf = static_cast<Self*> (object->GetLeaf (0));

    // if the object only belongs to one leaf check whether the leaf still
    // contains the whole bounding box of the object - if yes we don't have
    // to do anything
    if (object->GetLeafCount () == 1)
    {
      if (leaf->GetNodeBBox ().Contains (new_bbox))
      {
	// Even after moving we are still completely inside the bounding box
	// of the current leaf.
	if (leaf->disallow_distribute > 0)
	  leaf->disallow_distribute--;
	return;
      }
    }

    // remove object from all current leaves
    UnlinkObject (object);

    // find the first parent of an old leaf that contains the new bounding box
    while (leaf->parent && !leaf->GetNodeBBox ().Contains (new_bbox))
    {
      leaf = leaf->parent;
    }

    // add the object to it
    leaf->AddObjectInt (object);
  }

  /**
   * Distribute all objects in this node to its children.
   * This may also create new children if needed. Note that this
   * will only distribute one level (this node) and will not
   * recurse into the children.
   */
  void Distribute ()
  {
    // check whether there is anything to distribute and
    // whether distribution is blocked
    if (num_objects == 0 || disallow_distribute > 0)
    {
      // nothing to be done
      return;
    }

    // ensure we have either no childs or two childs
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // if we already have childs simply distribute the objects among
    // our children
    if (child1)
    {
      // distribute the objects
      DistributeLeafObjects ();

      // ensure nothing is left
      CS_ASSERT (num_objects == 0);

      // update the estimated object count
      estimate_total_objects = child1->GetEstimatedObjectCount ()
	  + child2->GetEstimatedObjectCount ();
    }
    // we don't have children yet, so we have to try and find a split
    // if we actually have enough objects to justify a distribution
    else if (num_objects > min_split_objects)
    {
      // to find a split location we evaluate multiple options for each
      // axis and use the one with the best quality
      float best_split_loc;
      float split_loc_tmp;

      // start with the x axis
      int best_axis = CS_KDTREE_AXISX;
      long best_qual = FindBestSplitLocation (CS_KDTREE_AXISX, best_split_loc);

      // check y axis
      long qual = FindBestSplitLocation (CS_KDTREE_AXISY, split_loc_tmp);

      // check whether it's better than x
      if (qual > best_qual)
      {
	// it is, set it as best
	best_axis = CS_KDTREE_AXISY;
	best_qual = qual;
	best_split_loc = split_loc_tmp;
      }

      // check z axis
      qual = FindBestSplitLocation (CS_KDTREE_AXISZ, split_loc_tmp);

      // check whether it's better than x and y
      if (qual > best_qual)
      {
	// it is, set it as best
	best_axis = CS_KDTREE_AXISZ;
	best_qual = qual;
	best_split_loc = split_loc_tmp;
      }

      // check whether the best split is good enough
      if (best_qual > 0)
      {
	// it is, set it as split
	split_axis = best_axis;
	split_location = best_split_loc;

	// allocate children
	child1 = TreeAlloc ().Alloc ();
	child2 = TreeAlloc ().Alloc ();

	// validate allocations
	CS_ASSERT (child1);
	CS_ASSERT (child2);

	// set us as parent
	child1->SetParent (this);
	child2->SetParent (this);

	// set object descriptor
	child1->SetObjectDescriptor (descriptor);
	child2->SetObjectDescriptor (descriptor);

	// set bounding boxes
	child1->node_bbox = GetNodeBBox ();
	child1->node_bbox.SetMax (split_axis, split_location);
	child2->node_bbox = GetNodeBBox ();
	child2->node_bbox.SetMin (split_axis, split_location);

	// distribute objects according to the split
	DistributeLeafObjects ();

	// ensure all objects are distributed
	CS_ASSERT (num_objects == 0);

	// update estimated object count
	estimate_total_objects = child1->GetEstimatedObjectCount ()
	  + child2->GetEstimatedObjectCount ();
      }
      else
      {
	// bad split, block distribution
	disallow_distribute = DISALLOW_DISTRIBUTE_TIME;
      }
    }
  }

  /**
   * Do a full distribution of this node and all children.
   */
  void FullDistribute ()
  {
    // distribute our objects
    Distribute ();

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // distribute the objects for the children as well
      child1->FullDistribute ();
      child2->FullDistribute ();
    }
  }

  /**
   * Do a full flatten of this node. This means that all
   * objects are put back in the object list of this node and
   * the KD-tree children are removed.
   */
  void Flatten ()
  {
    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // we do, flatten them to ourself
      FlattenTo (this);

      // remove distribution block if there is one.
      disallow_distribute = 0;
    }
  }

  /**
   * Traverse the tree in random order.
   * The mask parameter is optionally used for frustum checking.
   * TraverseRandom will pass it to the tree nodes.
   */
  void TraverseRandom (VisitFunc* func,
        void* userdata, uint32 frustum_mask)
  {
    NewTraversal ();
    TraverseRandom (func, userdata, global_timestamp, frustum_mask);
  }

  /**
   * Traverse the tree from front to back. Every node of the
   * tree will be encountered.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back (const csVector3& pos, VisitFunc* func,
        void* userdata, uint32 frustum_mask)
  {
    NewTraversal ();
    Front2Back (pos, func, userdata, global_timestamp, frustum_mask);
  }

  /**
   * Start a new traversal. This will basically make a new
   * timestamp and return it. You can then use that timestamp
   * to check if objects have been visited already. This function
   * is automatically called by Front2Back() but it can be useful
   * to call this if you plan to do a manual traversal of the tree.
   */
  uint32 NewTraversal ()
  {
    // use the parent timestamp if we have a parent
    if (parent)
    {
      return parent->NewTraversal ();
    }

    // For safety reasons we will reset all timestamps to 0
    // for all objects in the tree and also set the global
    // timestamp to 1 again every 4.000.000.000 calls of Front2Back
    if (global_timestamp > 4000000000u)
    {
      ResetTimestamps ();
      global_timestamp = 1;
    }
    else
    {
      global_timestamp++;
    }
    return global_timestamp;
  }

  /**
   * Get left child.
   */
  inline Self* GetChild1 () const
  {
    return child1;
  }

  /**
   * Get right child.
   */
  inline Self* GetChild2 () const
  {
    return child2;
  }

  /**
   * Return the number of objects in this node.
   */
  inline int GetObjectCount () const
  {
    return num_objects;
  }

  /**
   * Get the estimated total number of objects in this node and
   * all children. This is only an estimate as it isn't kept up-to-date
   * constantly but it should give a rough idea about the complexity
   * of this node.
   */
  inline int GetEstimatedObjectCount ()
  {
    return estimate_total_objects;
  }

  /**
   * Return the array of objects in this node.
   */
  inline Child** GetObjects () const
  {
    return objects;
  }

  /**
   * Return the bounding box of the node itself (does not always contain
   * all children since children are not split by the tree).
   */
  inline const csBox3& GetNodeBBox () const
  {
    return node_bbox;
  }

private:
  // debugging functions
  // perform various sanity checks on the tree for validation
  bool Debug_CheckTree (csString& str)
  {
#   define KDT_ASSERT_BOOL(test,msg) \
    if (!(test)) \
    { \
      csString ss; \
      ss.Format ("csKDTree failure (%d,%s): %s\n", int(__LINE__), \
	  #msg, #test); \
      str.Append (ss); \
      return false; \
    }

    // ensure we have either none or two children
    KDT_ASSERT_BOOL ((child1 == nullptr) == (child2 == nullptr), "child consistency");

    // check whether we have children
    if (child1)
    {
      //-------
      // Test-cases in case this is a node.
      //-------

      // ensure we have a valid split axis
      KDT_ASSERT_BOOL (split_axis >= CS_KDTREE_AXISX && split_axis <= CS_KDTREE_AXISZ, "axis");

      // ensure the node bounding box contains the bounding boxes of the children
      KDT_ASSERT_BOOL (GetNodeBBox ().Contains (child1->GetNodeBBox ()), "node_bbox mismatch");
      KDT_ASSERT_BOOL (GetNodeBBox ().Contains (child2->GetNodeBBox ()), "node_bbox mismatch");

      // ensure the split location is contained in the node bounding box
      KDT_ASSERT_BOOL (split_location >= GetNodeBBox ().Min (split_axis), "split/node");
      KDT_ASSERT_BOOL (split_location <= GetNodeBBox ().Max (split_axis), "split/node");

      // compute union of child bounding boxes
      csBox3 new_node_bbox = child1->GetNodeBBox () + child2->GetNodeBBox ();

      // enure the node bounding box is the union of the child bounding boxes
      KDT_ASSERT_BOOL (new_node_bbox == GetNodeBBox (), "node_bbox mismatch");

      // ensure we're set as parent for our children
      KDT_ASSERT_BOOL (child1->parent == this, "parent check");
      KDT_ASSERT_BOOL (child2->parent == this, "parent check");

      // perform checks for our children
      if (!child1->Debug_CheckTree (str))
	return false;
      if (!child2->Debug_CheckTree (str))
	return false;
    }

    //-------
    // Test-cases in case this is a leaf (or not a leaf but has
    // objects waiting for distribution).
    //-------

    // ensure we don't have more objects than our storage can hold
    KDT_ASSERT_BOOL (num_objects <= max_objects, "object list");

    // check all objects for validity
    for (int i = 0 ; i < num_objects ; i++)
    {
      // get the current object
      Child* o = objects[i];

      // ensure we only occur as parent once in the list
      int parcnt = 0;
      // check all leaves this object is associated with
      for (int j = 0 ; j < o->GetLeafCount () ; j++)
      {
	// check whether we are this leaf
	if (static_cast<Self*> (o->GetLeaf (j)) == this)
	{
	  // we are in the leaf list
	  parcnt++;

	  // ensure we didn't already occur earlier in the list
	  KDT_ASSERT_BOOL (parcnt <= 1, "parent occurs multiple times");
	}
      }

      // ensure we occured at least once in the list
      KDT_ASSERT_BOOL (parcnt == 1, "leaf list doesn't contain parent");
    }

    // all checks passed
    return true;

#   undef KDT_ASSERT_BOOL
  }

  void Debug_Dump (csString& str, int indent)
  {
    // get a string for indentation
    csString ind("");
    ind.PadLeft(indent);

    // get debug statistics for this node
    csRef<iString> stats = Debug_Statistics ();

    // append our data to the dump
    str.AppendFmt ("%s KDT disallow_dist=%d\n%s     node_bbox=%s\n%s %s",
	  ind.GetData (), disallow_distribute,
	  ind.GetData (), GetNodeBBox ().Description ().GetData (),
	  ind.GetData (), stats->GetData ());

    // append object count
    str.AppendFmt ("%s   %d objects\n", ind.GetData (), num_objects);

    // ensure we have either two or no children
    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if (child1)
    {
      // ensure axis is valid
      CS_ASSERT (split_axis >= CS_KDTREE_AXISX && split_axis <= CS_KDTREE_AXISY);

      // append our split split data
      char axis[3] = {'x','y','z'};
      str.AppendFmt ("%s   axis=%c loc=%g\n",
	  ind.GetData (), axis[split_axis], split_location);

      // dump our children
      child1->Debug_Dump (str, indent+2);
      child2->Debug_Dump (str, indent+2);
    }
  }

  void Debug_Statistics (int& tot_objects,
        int& tot_nodes, int& tot_leaves, int depth, int& max_depth,
        float& balance_quality)
  {
    // keep track of the total amount of objects
    tot_objects += num_objects;

    CS_ASSERT ((child1 == nullptr) == (child2 == nullptr));

    // check what kind of node we have
    if (child1)
    {
      // we got a branch (it has children)
      tot_nodes++;
    }
    else
    {
      // no children, we have a leaf
      tot_leaves++;
    }

    // we are one level deeper than our parent
    depth++;

    // update max_depth if this is deeper than the deepest
    // branch found so far
    if (depth > max_depth)
    {
      max_depth = depth;
    }

    // check whether we have children
    if (child1)
    {
      // we do, check how many objects are in each of them
      // so we can evaluate balancing

      // grab statistics for left child
      int left = 0;
      child1->Debug_Statistics (left, tot_nodes,
	  tot_leaves, depth, max_depth, balance_quality);

      // grab statistics for right child
      int right = 0;
      child2->Debug_Statistics (right, tot_nodes,
	  tot_leaves, depth, max_depth, balance_quality);

      // add the objects of the child to the total amount of objects
      tot_objects += left;
      tot_objects += right;

      // calculate balance
      float qual_balance = 2.0 * float (left) / float (left+right);
      balance_quality += qual_balance;
    }
  }

  csPtr<iString> Debug_Statistics ()
  {
    // get a scf string to output our results to
    scfString* rc = new scfString ();

    // get it's associated cs string so we can work with it more easily
    csString& str = rc->GetCsString ();

    // place holders for results
    // total amount of objects in the tree
    int tot_objects = 0;
    // total amount of nodes in the tree
    int tot_nodes = 0;
    // total amount of leaves in the tree
    int tot_leaves = 0;
    // highest depth level reached
    int max_depth = 0;

    // overall quality of the tree
    float balance_quality = 0;

    // collect the statictics by traversing the tree
    Debug_Statistics (tot_objects, tot_nodes, tot_leaves, 0, max_depth,
	  balance_quality);

    // format our output
    str.Format ("#o=%d #n=%d #l=%d maxd=%d balqual=%g\n",
	  tot_objects, tot_nodes, tot_leaves, max_depth,
	  balance_quality / float (tot_nodes));

    // return output
    return csPtr<iString> ((iString*)rc);
  }

  csTicks Debug_Benchmark (int num_iterations)
  {
    // start of first benchmark
    csTicks pass0 = csGetTicks ();

    // tree building benchmark:
    // build num_iterations random trees
    for (int i = 0 ; i < num_iterations ; i++)
    {
      // clear the tree
      Clear ();

      // build a random one
      for (int j = 0 ; j < 500 ; j++)
      {
	// add a random object
	AddObject (Child::RandomBound (), (void*)0);

	// distribute after 20 insertions
	if (i % 20 == 0)
	{
	  FullDistribute ();
	}
      }
    }

    // end of first/start of second benchmark
    csTicks pass1 = csGetTicks ();

    // unoptimized tree traversal benchmark:
    // perform num_iterations traversals in approximate
    // front to back order on an incremently built tree
    for (int i = 0 ; i < num_iterations ; i++)
    {
      Front2Back (csVector3 (0, 0, 0), Debug_TraverseFuncBenchmark, 0, 0);
    }

    // end of second/start of third benchmark
    csTicks pass2 = csGetTicks ();

    // tree distribution benchmark:
    // flatten the tree completely and completely
    // distribute it num_iterations times
    for (int i = 0 ; i < num_iterations ; i++)
    {
      Flatten ();
      FullDistribute ();
    }

    // end of third/start of last benchmark
    csTicks pass3 = csGetTicks ();

    // optimized tree traversal benchmark:
    // perform num_iterations traversals in approximate
    // front to back order on a tree that was distributed
    // with all information available
    for (int i = 0 ; i < num_iterations ; i++)
    {
      Front2Back (csVector3 (0, 0, 0), Debug_TraverseFuncBenchmark, 0, 0);
    }

    // end of last benchmark
    csTicks pass4 = csGetTicks ();

    // output results
    csPrintf ("Creating the tree:        %u ms\n", pass1-pass0);
    csPrintf ("Unoptimized Front2Back:   %u ms\n", pass2-pass1);
    csPrintf ("Flatten + FullDistribute: %u ms\n", pass3-pass2);
    csPrintf ("Optimized Front2Back:     %u ms\n", pass4-pass3);

    return pass4-pass0;
  }

  static bool Debug_TraverseFuncBenchmark (Self* treenode, void*,
	  uint32 cur_timestamp, uint32&)
  {
    treenode->Distribute ();

    int num_objects = treenode->GetObjectCount ();
    Child** objects = treenode->GetObjects ();
    for (int i = 0 ; i < num_objects ; i++)
    {
      if (objects[i]->timestamp != cur_timestamp)
	objects[i]->timestamp = cur_timestamp;
    }

    return true;
  }


public:
  // iDebugHelper

  // indicate that we support statetest, text dump and benchmark
  virtual int GetSupportedTests () const
  {
    return CS_DBGHELP_STATETEST |
      CS_DBGHELP_TXTDUMP | CS_DBGHELP_BENCHMARK;
  }

  // performs a state test
  virtual csPtr<iString> StateTest ()
  {
    // allocate output
    scfString* rc = new scfString ();

    // perform check
    if (!Debug_CheckTree (rc->GetCsString ()))
    {
      // return error if it failed
      return csPtr<iString> (rc);
    }

    // free output as there is none
    delete rc;

    // return empty result to indicate no error occured
    return nullptr;
  }

  // performs a benchmark and returns the time it took
  virtual csTicks Benchmark (int num_iterations)
  {
    return Debug_Benchmark (num_iterations);
  }

  // performs a text dump of the tree
  virtual csPtr<iString> Dump ()
  {
    scfString* rc = new scfString ();
    Debug_Dump (rc->GetCsString (), 0);
    return csPtr<iString> (rc);
  }

  // we don't support graphical dumping
  virtual void Dump (iGraphics3D* /*g3d*/)
  {
  }

  // handles debug commands - as we don't have any
  // simply indicate we didn't handle that command
  virtual bool DebugCommand (const char*)
  {
    return false;
  }
};

} // namespace Geometry
} // namespace CS

typedef CS::Geometry::KDTree<csKDTreeChild> csKDTree;
typedef csKDTree::VisitFunc csKDTreeVisitFunc;
typedef csKDTree::iObjectDescriptor iKDTreeObjectDescriptor;
typedef csKDTree::iUserData iKDTreeUserData;

/** @} */

#endif // __CS_KDTREE_H__

