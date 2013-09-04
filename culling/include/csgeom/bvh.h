/*
    Copyright (C) 2002 by Jorrit Tyberghein
    Copyright (C) 2013 by Matthieu Kraus

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

#ifndef __CS_BVH_H__
#define __CS_BVH_H__

#include "csextern.h"

#include "csgeom/box.h"

#include "csutil/allocator.h"
#include "csutil/blockallocator.h"
#include "csutil/ref.h"
#include "csutil/scfstr.h"
#include "csutil/scf_implementation.h"

#include "iutil/dbghelp.h"

#include <limits>

/**\file
 * Bounding Intervall Hierachy implementation.
 */
/**\addtogroup geom_utils
 * @{ */

/**
 * The data type for user data to be attached to the BVH.
 * It provides no functions but makes it possible to do a direct cast
 * for performance instead of doing an scfQueryInterface.
 */
struct iBVHUserData : public virtual iBase
{
  SCF_INTERFACE(iBVHUserData, 0, 0, 1);
};

enum
{
  CS_BVH_AXISINVALID = -1,
  CS_BVH_AXISX = 0,
  CS_BVH_AXISY = 1,
  CS_BVH_AXISZ = 2
};

/**
 * Default BVH Child implementation using AABB.
 */
class CS_CRYSTALSPACE_EXPORT csBVHChild : public csBox3
{
private:
  void* object; // Pointer to object supplied by user.
  void* leaf; // pointer to the tree holding this child

public:
  // used by BVH implementation
  typedef csBox3 BoundType;

  // set user-supplied boundaries
  inline void SetBounds(BoundType const& bounds)
  {
    Set(bounds.Min(), bounds.Max());
  }

  inline csBox3 const& GetBBox() const
  {
    return *this;
  }

  // object setter/getter
  inline void* GetObject() const
  {
    return object;
  }
  inline void SetObject(void* o)
  {
    object = o;
  }

  // leaf setter/getter
  inline void* GetLeaf() const
  {
    return leaf;
  }

  inline void SetLeaf(void* l)
  {
    leaf = l;
  }
};

namespace CS
{
namespace Geometry
{

/**
 * A Bounding Intervall Hierachy (BVH).
 * A BVH is a binary tree that organizes 3D space.
 * This implementation is dynamic. It allows moving, adding, and
 * removing of objects which will alter the tree dynamically.
 * The main purpose of this tree is to provide fast approximate
 * front to back ordering.
 * <p>
 * The BVH supports delayed insertion. When objects are inserted
 * in the tree they are not immediatelly distributed over the
 * nodes but instead they remain in the main node until it is really
 * needed to distribute them. The advantage of this is that you can
 * insert/remove a lot of objects in the tree and then do the distribution
 * calculation only once. This is more efficient and it also generates
 * a better tree as more information is available then.
 * <p>
 * As this implementation is supposed to be as fast as possible sanity
 * checks are only performed in debug mode, so those should be performed
 * additionally if needed.
 */
template<class Child = csBVHChild> class BVH : public scfImplementation<BVH<Child> >
{
public:
  // convenience typedefs
  typedef BVH<Child> Self;
  typedef Child Child;
  typedef typename Child::BoundType BoundType;

  /**
   * A callback function for visiting a BVH node. If this function
   * returns true the traversal will continue. Otherwise Front2Back()
   * will return to the next traversal point or stop.
   * <p>
   * This function is itself responsible for calling Distribute() on
   * the given treenode to ensure that the objects in this node
   * are properly distributed to the children. If the function doesn't
   * want or need this functionality it doesn't have to do Distribute().
   * <p>
   * 'frustum_mask' can be modified by this function to reduce the number
   * of plane tests (for frustum culling) that have to occur for children
   * of this node.
   */
  typedef bool (VisitFunc)(Self* treenode, void* userdata, uint32& frustum_mask);

private:
  typedef float (Split)[4];

  // allocators
  csBlockAllocator<Child>* childAlloc;
  csBlockAllocator<Self>* treeAlloc;

  // bbox of our tree
  csBox3 box;

  // child and parent relations
  Self* child1; // null if this is a leaf
  Self* child2; // null if this is a leaf
  Self* parent; // null if this is the root

  // An optional user object for this node.
  csRef<iBVHUserData> userObject;

  // blocking value - if a split is of bad quality we may block for
  // a number of addiditions/removals to disallow further split attempts
  int block;
  float blockThreshold; // minimum quality that'll be accepted
  static int const blockTime = 20; // maybe set this based on minSplitObjects?

  // minimum amount of objects before we consider splitting
  int minSplitObjects;

  // Objects in this node. If this node also has children (child1
  // and child2) then the objects here have to be moved to these
  // children. The 'Distribute()' function will do that.
  Child** objects;     // storage
  int numObjects;      // current storage used
  int maxObjects;      // current storage capacity
  int estimateObjects; // estimated number of objects in whole tree
  static int const objectGrowth = 80; // storage growth minimum

  // helper structure used to find split locations
  struct SortElement
  {
    float center;
    csBox3 box;
    int idx;

    static int compare(SortElement const& l, SortElement const& r)
    {
      if(l.center < r.center) return -1;
      else if(l.center > r.center) return 1;
      else return 0;
    }
  };

  // get tree allocator
  inline csBlockAllocator<Self>& TreeAlloc()
  {
    if(treeAlloc == nullptr)
    {
      if(parent == nullptr)
      {
	treeAlloc = new csBlockAllocator<Self>();
      }
      else
      {
	treeAlloc = &parent->TreeAlloc();
      }
    }
    return *treeAlloc;
  }

  // get child allocator
  inline csBlockAllocator<Child>& ChildAlloc()
  {
    if(childAlloc == nullptr)
    {
      if(parent == nullptr)
      {
	childAlloc = new csBlockAllocator<Child>();
      }
      else
      {
	childAlloc = &parent->ChildAlloc();
      }
    }
    return *childAlloc;
  }

  /// Set parent of this tree node.
  inline void SetParent(Self* p)
  {
    parent = p;
  }

  /// Add an object to this tree node.
  void AddObjectInternal(Child* obj)
  {
    CS_ASSERT(obj);

    if(block > 0)
    {
      --block;
    }

    AddObject(obj);
  }

  /// Physically add a child to this tree node.
  void AddObject(Child* obj)
  {
    // validate obejct
    CS_ASSERT(obj);

    // validate storage
    CS_ASSERT((maxObjects == 0) == (objects == nullptr));

    // check whether we have to grow the storage
    if(numObjects >= maxObjects)
    {
      // get new storage size
      maxObjects += csMin(maxObjects+2, objectGrowth);

      // relocate storage
      Child** relocation = static_cast<Child**>(cs_realloc(objects, sizeof(Child*)*maxObjects));

      // validate relocation
      CS_ASSERT(relocation);

      // update storage location
      objects = relocation;
    }

    // add object
    objects[numObjects] = obj;

    // let it know we're it's leaf now
    obj->SetLeaf(static_cast<void*>(this));

    // update object counts
    ++numObjects;
    ++estimateObjects;
  }

  /// Physically remove a child from this tree node.
  void RemoveObject(int idx)
  {
    CS_ASSERT(objects);
    CS_ASSERT(idx >= 0 && idx < numObjects);

    // check whether we have to move objects
    bool move = idx != numObjects;

    // update object counts
    --numObjects;
    --estimateObjects;

    if(move)
    {
      memmove(&objects[idx], &objects[idx+1], sizeof(Child*)*(numObjects-idx));
    }
  }

  /// Find an object. Returns -1 if not found.
  int FindObject(Child* obj)
  {
    CS_ASSERT(obj);
    CS_ASSERT((objects == nullptr) == (maxObjects == 0));

    // iterate over all objects
    for(int i = 0; i < numObjects; ++i)
    {
      // check whether we found the object
      if(objects[i] == obj)
      {
	// we did - return it's index
	return i;
      }
    }

    // couldn't locate object
    return -1;
  }

  /**
   * If this node is a leaf then we will split the objects currently
   * in this leaf according to the pre-filled values of split_axis
   * and split.
   */
  void DistributeLeafObjects()
  {
    // we shouldn't be in blocking mode
    CS_ASSERT(block == 0);

    // we have to have children
    CS_ASSERT(child1 != nullptr && child2 != nullptr);

    // keep track how many objects couldn't be distributed
    int failed = 0;

    // get center and split axis as we sort objects
    // into children based on it
    int const split = box.GetSize().DominantAxis();
    float const center = box.GetCenter()[split];

    // distribute objects
    for(int i = 0; i < numObjects; ++i)
    {
      // get object to distribute
      Child* obj = objects[i];

      // get object box
      csBox3 const& objBox = obj->GetBBox();

      // check whether it belongs to child1
      if(objBox.GetCenter()[split] < center)
      {
	child1->AddObject(obj);
	child1->box += objBox;
      }
      // else it belongs to child2
      else
      {
	// add object to child2
	child2->AddObject(obj);
	child2->box += objBox;
      }
    }

    // update our bounding box
    box = child1->box + child2->box;

    // update object count for our node (this is equivalent
    // to a truncation of our array)
    numObjects = 0;
  }

  /**
   * Flatten the children of this node to the given node.
   */
  void FlattenTo(Self* node)
  {
    // check we aren't trying to flatten ourselves
    CS_ASSERT(this != node);

    // ensure we have either two or no children
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // check whether we're a leaf
    if(child1)
    {
      // flatten children
      child1->FlattenTo(node);
      child2->FlattenTo(node);
    }

    // check the storage is valid
    CS_ASSERT((maxObjects == 0) == (objects == nullptr));
    CS_ASSERT(numObjects <= maxObjects);

    // add our own (undistributed) objects to the target
    for(int i = 0; i < numObjects; ++i)
    {
      node->AddObject(objects[i]);
    }

    // free our storage
    cs_free(objects);
    objects = nullptr;
    maxObjects = 0;
    numObjects = 0;
  }

public:
  /// Create a new empty BVH.
  BVH() :
    // scf initialization
    scfImplementation(this),

    // allocator initialization
    childAlloc(nullptr), treeAlloc(nullptr),

    // box initialization
    box(-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),
	std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()),

    // child-parent initialization
    child1(nullptr), child2(nullptr), parent(nullptr),

    // split initialization
    block(0), blockThreshold(0.0f), minSplitObjects(10),

    // objects initialization
    objects(nullptr), numObjects(0), maxObjects(0), estimateObjects(0)
  {
  }

  /// Destroy the tree.
  virtual ~BVH()
  {
    Clear();
  }

  /// Get parent of this tree node.
  inline Self* GetParent() const
  {
    return parent;
  }

  /**
   * Set the minimum amount of objects before we consider splitting this tree.
   * By default this is set to 10.
   */
  inline void SetMinimumSplitAmount(int m)
  {
    minSplitObjects = m;
  }

  /// Make the tree empty.
  void Clear()
  {
    // free first child
    TreeAlloc().Free(child1);
    child1 = nullptr;

    // free second child
    TreeAlloc().Free(child2);
    child2 = nullptr;

    // free objects
    for(int i = 0; i < numObjects; ++i)
    {
      ChildAlloc().Free(objects[i]);
    }
    numObjects = 0;
    estimateObjects = 0;

    // free object storage
    cs_free(objects);
    objects = nullptr;
    maxObjects = 0;

    // reset blocking state
    block = 0;

    // clear bounding box
    box.StartBoundingBox();

    // invalidate user data
    userObject.Invalidate();

    // check if we're the root node
    if(parent == nullptr)
    {
      // delete tree allocator
      delete treeAlloc;
      treeAlloc = nullptr;

      // delete child allocator
      delete childAlloc;
      childAlloc = nullptr;
    }
  }

  /// Get the user object attached to this node.
  inline iBVHUserData* GetUserObject() const
  {
    return userObject;
  }

  /**
   * Set the user object for this node. Can be 0 to clear
   * it. The old user object will be DecRef'ed and the (optional)
   * new one will be IncRef'ed.
   */
  inline void SetUserObject (iBVHUserData* obj)
  {
    userObject = obj;
  }

  /**
   * Add an object to this BVH node.
   * Returns a Child pointer which represents the object
   * inside the BVH. Object addition is delayed. This function
   * will not yet alter the structure of the kd-tree. Distribute()
   * will do that.
   */
  Child* AddObject(BoundType const& bounds, void* object)
  {
    // validate the object is valid
    CS_ASSERT(object);

    // allocate tree child to hold this object
    Child* obj = ChildAlloc().Alloc();

    // set boundaries
    obj->SetBounds(bounds);

    // validate the object has valid boundaries
#   ifdef CS_DEBUG
    for(int i = 0; i < 3; ++i)
    {
      // validate minimum is normal
      float min = obj->GetMin(i);
      CS_ASSERT(CS::IsFinite(min) && !CS::IsNaN(min));

      // validate maximum is normal
      float max = obj->GetMax(i);
      CS_ASSERT(CS::IsFinite(max) && !CS::IsNaN(max));

      // validate minimum is smaller than maximum
      CS_ASSERT(min <= max);
    }
#   endif

    // set object
    obj->SetObject(object);

    // add child to the tree
    AddObjectInternal(obj);

    return obj;
  }

  /**
   * Remove an object from the tree. The 'Child' instance
   * will be deleted.
   */
  void RemoveObject(Child* obj)
  {
    // get tree the object belongs to
    Self* tree = static_cast<Self*>(obj->GetLeaf());

    // validate tree
    CS_ASSERT(tree);

    // find object in the tree
    int idx = tree->FindObject(obj);

    // ensure it was found
    CS_ASSERT(idx >= 0);

    // unlink object from tree
    tree->RemoveObject(idx);

    // free object
    ChildAlloc().Free(obj);

    // get parent of the tree
    Self* p = tree->GetParent();

    // check whether that leaf is empty and not root
    if(tree->numObjects == 0 && p != nullptr)
    {
      // it's empty and has a parent, flatten it
      p->Flatten();
    }
  }

  /**
   * Move an object (give it a new bound).
   */
  void MoveObject(Child* obj, BoundType const& newBound)
  {
    // get old bbox
    csBox3 oldBox = obj->GetBBox();

    // set new bounds
    obj->SetBounds(newBound);

    // get new bbox
    csBox3 newBox = obj->GetBBox();

    // validate it actually moved
    if((oldBox.Min() - newBox.Min()).IsZero() && (oldBox.Max() - newBox.Max()).IsZero())
    {
      // didn't move, nothing to be done
      return;
    }

    // get the leaf this object belongs to
    Self* tree = static_cast<Self*>(obj->GetLeaf());

    // validate the leaf is valid
    CS_ASSERT(tree);

    // check whether the object still fits in that node
    if(tree->GetNodeBBox().Contains(newBox))
    {
      // does fit, no need to actually move the object
      // decrement block count if there is a block
      if(tree->block > 0)
      {
	--tree->block;
      }
    }
    else
    {
      // find the object in it's tree
      int idx = tree->FindObject(obj);

      // validate it was found
      CS_ASSERT(idx >= 0);

      // remove of it from tree
      tree->RemoveObject(idx);

      // find the first ancestor it fits into
      for(tree = tree->GetParent(); tree; tree = tree->GetParent())
      {
	if(tree->GetNodeBBox().Contains(newBox))
	{
	  break;
	}
      }

      // validate it did fit somewhere (it should at least fit into root)
      CS_ASSERT(tree);

      // add the object to the ancestor we found
      tree->AddObjectInternal(obj);
    }
  }

  /**
   * Distribute all objects in this node to its children.
   * This may also create new children if needed. Note that this
   * will only distribute one level (this node) and will not
   * recurse into the children.
   */
  void Distribute()
  {
    // check our distribution state is consistent
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // ensure our storage is valid
    CS_ASSERT(objects != nullptr && maxObjects >= numObjects);

    // check for distribution block due to failed attempts
    if(block > 0)
    {
      // we don't want to try again, yet
      return;
    }

    // check whether there is anything to distribute
    if(numObjects == 0)
    {
      // nothing to distribute
      // check that we are a leaf - leaves must have objects
      CS_ASSERT(child1 != nullptr);
      return;
    }

    // check whether we already have a split for this node
    if(child1 == nullptr) // nope
    {
      // do we have enough objects for a new split?
      if(numObjects < minSplitObjects)
      {
	// nope, nothing to be done
	return;
      }
      else
      {
	// allocate buffer to hold the split
	SortElement* buffer = static_cast<SortElement*>(cs_malloc(sizeof(SortElement)*numObjects));

	// validate allocation
	CS_ASSERT(buffer);

	// get split
	// split along longest axis
	int axis = box.GetSize().DominantAxis();

	// fill sort array
	for(int i = 0; i < numObjects; ++i)
	{
	  SortElement& s = buffer[i];
	  s.box = objects[i]->GetBBox();
	  s.center = s.box.GetCenter()[axis];
	  s.idx = i;
	}

	// sort by center
	qsort(buffer, numObjects, sizeof(SortElement), (int (*)(void const*, void const*))SortElement::compare);

	// calculate child boxes
	csBox3 leftBox;
	for(int i = 0; i < (numObjects >> 1); ++i)
	{
	  leftBox += buffer[i].box;
	}
	csBox3 rightBox;
	for(int i = numObjects >> 1; i < numObjects; ++i)
	{
	  rightBox += buffer[i].box;
	}

	// evaluate quality
	float quality = box.Volume() - leftBox.Volume() - rightBox.Volume();

	// check whether the best split is actually good enough
	if(quality > blockThreshold)
	{
	  // allocate childs
	  child1 = TreeAlloc().Alloc();
	  child2 = TreeAlloc().Alloc();

	  // verify allocations
	  CS_ASSERT(child1);
	  CS_ASSERT(child2);

	  // set us as their parent
	  child1->SetParent(this);
	  child2->SetParent(this);

	  // distribute objects according to the split we found
	  // first child gets lower half
	  for(int i = 0; i < (numObjects >> 1); ++i)
	  {
	    child1->AddObject(objects[buffer[i].idx]);
	  }

	  // second child gets upper half
	  for(int i = numObjects >> 1; i < numObjects; ++i)
	  {
	    child2->AddObject(objects[buffer[i].idx]);
	  }

	  // free our buffer as we don't need it anymore
	  cs_free(buffer);

	  // truncate our object list as we distributed all childs
	  numObjects = 0;

	  // update their boxes so they know where they belong
	  child1->box = leftBox;
	  child2->box = rightBox;

	  // update our estimated object count
	  estimateObjects = child1->numObjects + child2->numObjects;
	}
	else
	{
	  // bad split - let's wait a bit before we try again
	  block = blockTime;
	}
      }
    }
    else // yep, there's a split
    {
      // try to distribute the objects with our current split
      DistributeLeafObjects();

      // check whether it worked
      if(numObjects != 0)
      {
	// nope... any ideas? no? time to start from scratch I guess
	// @@@RlyDontKnow: TODO: we should try to fit them in somehow
	Flatten();
	Distribute();
      }
    }
  }

  /**
   * Do a full distribution of this node and all children.
   */
  inline void FullDistribute()
  {
    // distribute our objects
    Distribute();

    // check we got two or no children
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // check whether we're a leaf (i.e. distribution succeeded)
    if(child1)
    {
      // distribute child objects
      child1->FullDistribute();
      child2->FullDistribute();
    }
  }

  /**
   * Do a full flatten of this node. This means that all
   * objects are put back in the object list of this node and
   * the children are removed.
   * Calling this on a leaf results in undefined behaviour.
   */
  void Flatten()
  {
    // validate children
    CS_ASSERT(child1);
    CS_ASSERT(child2);

    // flatten children
    child1->FlattenTo(this);
    child2->FlattenTo(this);

    // delete children
    TreeAlloc().Free(child1);
    child1 = nullptr;
    TreeAlloc().Free(child2);
    child2 = nullptr;
  }

  /**
   * Traverse the tree in random order.
   * The mask parameter is optionally used for frustum checking.
   * TraverseRandom will pass it to the tree nodes.
   */
  void TraverseRandom(VisitFunc* func, void* data, uint32 frustumMask)
  {
    // validate we got a visiting function
    CS_ASSERT(func);

    // check whether we want to continue this traversal
    if(!func(this, data, frustumMask))
    {
      return;
    }

    // ensure child state is consistent
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children to traverse
    if(child1)
    {
      // traverse children
      child1->TraverseRandom(func, data, frustumMask);
      child2->TraverseRandom(func, data, frustumMask);
    }
  }

  /**
   * Traverse the tree in approximate front to back order. Every node of
   * the tree will be encountered at most once. Returns false if traversal
   * in this branch should stop (may continue in an alternate branch
   * that may have been in front of this one if there is any - i.e. if
   * the childs of this node are overlapping).
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back(csVector3 const& pos, VisitFunc* func, void* data, uint32 frustumMask)
  {
    // validate we got a visiting function
    CS_ASSERT(func);

    // check whether we want to continue the traversal
    if(!func(this, data, frustumMask))
    {
      return;
    }

    // ensure child state is consistent
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // continue with children if we have any
    if(child1)
    {
      // check which child should be visited first based on the angle between
      // our direction and the difference between the centers of the children
      // get center difference vector
      csVector3 const centerDiff = child2->box.GetCenter() - child1->box.GetCenter();
      if(centerDiff * pos > 0)
      {
	// left child goes first
	child1->Front2Back(pos, func, data, frustumMask);
	child2->Front2Back(pos, func, data, frustumMask);
      }
      else
      {
	// right child goes first
	child2->Front2Back(pos, func, data, frustumMask);
	child1->Front2Back(pos, func, data, frustumMask);
      }
    }
  }

  /**
   * Get first child.
   */
  inline Self* GetChild1() const
  {
    return child1;
  }

  /**
   * Get second child.
   */
  inline Self* GetChild2() const
  {
    return child2;
  }

  /**
   * Return the number of objects in this node.
   */
  inline int GetObjectCount() const
  {
    return numObjects;
  }

  /**
   * Get the estimated total number of objects in this node and
   * all children. This is only an estimate as it isn't kept up-to-date
   * constantly but it should give a rough idea about the complexity
   * of this node.
   */
  inline int GetEstimatedObjectCount() const
  {
    return estimateTotalObjects;
  }

  /**
   * Return the array of objects in this node.
   */
  inline Child** GetObjects() const
  {
    return objects;
  }

  /**
   * Return the bounding box of the node itself (includes all children).
   */
  inline csBox3 const& GetNodeBBox() const
  {
    return box;
  }
};

} // namespace Geometry
} // namespace CS

typedef CS::Geometry::BVH<csBVHChild> csBVH;

/** @} */

#endif // __CS_BVH_H__

