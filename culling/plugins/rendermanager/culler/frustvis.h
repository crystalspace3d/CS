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

#ifndef __CS_OCCCULL_H__
#define __CS_OCCCULL_H__

#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "iutil/dbghelp.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/render.h"
#include "csutil/array.h"
#include "csutil/list.h"
#include "csutil/parray.h"
#include "csutil/scf_implementation.h"
#include "csutil/hash.h"
#include "csutil/set.h"
#include "csutil/weakref.h"
#include "csgeom/plane3.h"
#include "csgeom/kdtree.h"
#include "imesh/objmodel.h"
#include "iengine/viscull.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "chcpp.h"

//typedef CS::RenderManager::RenderTree<
//CS::RenderManager::RenderTreeStandardTraits> RenderTreeType;

enum NodeVisibility
{
  NODE_INVISIBLE,
  NODE_VISIBLE,
  NODE_INSIDE
};

class csKDTree;
class csKDTreeChild;
class csFrustumVis;
struct iMovable;
struct iMeshWrapper;

struct FrustTest_Front2BackData
{
  csVector3 pos;
  iRenderView* rview;
  csPlane3* frustum;
  // this is the callback to call when we discover a visible node
  iVisibilityCullerListener* viscallback;
};

//----------------------------------------------------------------------

struct NodeTraverseData
{
  NodeTraverseData() : kdtParent(0), kdtNode(0), u32Frustum_Mask(0), u32Timestamp(0)
  {
  }
  /*NodeTraverseData(NodeTraverseData &ntd)
  {
    kdtParent=ntd.kdtParent;
    kdtNode=ntd.kdtNode;
    // add a new user object only if there's none
    if(kdtNode && !kdtNode->GetUserObject())
      kdtNode->SetUserObject(new csVisibilityObjectHistory());
    u32Frustum_Mast=ntd.u32Frustum_Mast;
  }*/
  NodeTraverseData(csKDTree* kdtN, csKDTree* kdtP, const uint32 frustum_mask,const uint32 timestamp=0)
  {
    kdtParent=kdtP;
    kdtNode=kdtN;
    // add a new user object only if there's none
    if(kdtNode && !kdtNode->GetUserObject())
      kdtNode->SetUserObject(new csVisibilityObjectHistory());
    u32Frustum_Mask=frustum_mask;
    u32Timestamp=timestamp;
  }

  csVisibilityObjectHistory* GetVisibilityObjectHistory() const
  {
    if(!kdtNode) return 0;
    return static_cast<csVisibilityObjectHistory*>(kdtNode->GetUserObject());
  }

  bool IsLeaf() const
  {
    if(!kdtNode) return false;
    return kdtNode->IsLeaf();
  }

  bool GetVisibility() const
  {
    if(!kdtNode) return false;
    return GetVisibilityObjectHistory()->GetVisibility();
  }

  /*unsigned int GetID() const
  {
    if(!kdtNode) return false;
    return GetVisibilityObjectHistory()->GetID();
  }*/

  int GetSplitAxis() const
  {
    if(!kdtNode) return 0;
    return kdtNode->GetSplitAxis();
  }

  int GetSplitLocation() const
  {
    if(!kdtNode) return 0;
    return kdtNode->GetSplitLocation();
  }

  uint32 GetFrustumMask() const
  {
    return u32Frustum_Mask;
  }

  uint32 GetTimestamp() const
  {
    return u32Timestamp;
  }

  void SetVisibility(const bool bV) const
  {
    if(!kdtNode) return ;
    GetVisibilityObjectHistory()->SetVisibility(bV);
  }

  /*void SetID(const unsigned int ID) const
  {
    if(!kdtNode) return ;
    GetVisibilityObjectHistory()->SetID(ID);
  }*/

  void SetFrustumMask(uint32 frust_mask)
  {
    u32Frustum_Mask=frust_mask;
  }

  void SetTimestamp(uint32 timestamp)
  {
    u32Timestamp=timestamp;
  }

  bool operator == (const NodeTraverseData & ntd) const
  {
    return (kdtParent==ntd.kdtParent
            && kdtNode==ntd.kdtNode
            && u32Frustum_Mask==ntd.u32Frustum_Mask
            && u32Timestamp==ntd.u32Timestamp);
  }

  csKDTree* kdtParent;
  csKDTree* kdtNode;
  uint32 u32Frustum_Mask;
  uint32 u32Timestamp;

  ~NodeTraverseData()
  {
  }
};

/**
 * Occlusion query record.
 */
struct OccQuery
{
  OccQuery() : qID(0), numQueries(0), ntdNode()
  {
  }

  unsigned int *qID;
  unsigned int numQueries;
  NodeTraverseData ntdNode;

  bool IsMultiQuery() const
  {
    return (numQueries>1);
  }

  bool operator == (const OccQuery &oq) const
  {
    return (qID==oq.qID && numQueries==oq.numQueries && ntdNode==oq.ntdNode);
  }

  ~OccQuery()
  {
  }
};

/**
 * This object is a wrapper for an iVisibilityObject from the engine.
 */
class csFrustVisObjectWrapper :
  public scfImplementation2<csFrustVisObjectWrapper,
    iObjectModelListener, iMovableListener>
{
public:
  csFrustumVis* frustvis;
  csRef<iVisibilityObject> visobj;
  csKDTreeChild* child;
  long update_number;	// Last used update_number from movable.
  long shape_number;	// Last used shape_number from model.

  bool bVisible;

  // Optional data for shadows. Both fields can be 0.
  csRef<iMeshWrapper> mesh;

  csFrustVisObjectWrapper (csFrustumVis* frustvis) :
    scfImplementationType(this), frustvis(frustvis) { }
  virtual ~csFrustVisObjectWrapper () { }

  /// The object model has changed.
  virtual void ObjectModelChanged (iObjectModel* model);
  /// The movable has changed.
  virtual void MovableChanged (iMovable* movable);
  /// The movable is about to be destroyed.
  virtual void MovableDestroyed (iMovable*) { }
};

/**
 * A simple frustum based visibility culling system.
 */
class csFrustumVis :
  public scfImplementation1<csFrustumVis, iVisibilityCuller>
{
public:
  // List of objects to iterate over (after VisTest()).
  typedef csArray<iVisibilityObject*, csArrayElementHandler<iVisibilityObject*>,
    CS::Container::ArrayAllocDefault, csArrayCapacityFixedGrow<256> >
    VistestObjectsArray;
  VistestObjectsArray vistest_objects;
  bool vistest_objects_inuse;	// If true the vector is in use.

private:
  iObjectRegistry *object_reg;
  csEventID CanvasResize;
  csRef<iEventHandler> weakEventHandler;
  csKDTree* kdtree;
  // Ever growing box of all objects that were ever in the tree.
  // This puts an upper limit of all boxes in the kdtree itself because
  // those go off to infinity.
  csBox3 kdtree_box;
  csRefArray<csFrustVisObjectWrapper, CS::Container::ArrayAllocDefault, 
    csArrayCapacityFixedGrow<256> > visobj_vector;
  int scr_width, scr_height;	// Screen dimensions.
  uint32 current_vistest_nr;

  // This hash set holds references to csFrustVisObjectWrapper instances
  // that require updating in the culler.
  csSet<csPtrKey<csFrustVisObjectWrapper> > update_queue;
  // The 'updating' flag is true if the objects are being updated. This flag
  // is to prevent us from updating it again (if the callback is fired
  // again).
  bool updating;

  // Update all objects in the update queue.
  void UpdateObjects ();

  // Fill the bounding box with the current object status.
  void CalculateVisObjBBox (iVisibilityObject* visobj, csBox3& bbox);

  iGraphics3D* g3d;
  iShaderManager* smShaderManager;

  CHCList<NodeTraverseData> T_Queue; // Traversal Queue (aka DistanceQueue)
  CHCList<NodeTraverseData> I_Queue; // I queue (invisible queue)
  CHCList<NodeTraverseData> V_Queue; // V queue (nodes that are scheduled for testing in the current frame)
  CHCList<OccQuery> Q_Queue; // Q queue (query queue)

  // Frustum data (front to back)
  FrustTest_Front2BackData f2bData;

  bool WasVisible(NodeTraverseData &ntdNode,const int cur_timestamp) const;
  void QueryPreviouslyInvisibleNode(NodeTraverseData &ntdNode);
  void PullUpVisibility(const NodeTraverseData &ntdNode);
  void TraverseNode(iRenderView* rview,NodeTraverseData &ntdNode,
                    const csVector3& pos,const int cur_timestamp);

  void IssueQueries(iRenderView* rview, csArray<csKDTreeChild*> objArray);

  /**
   *  Gets the first finished query from the query list.
   * Return 1 if visible -1 if not or 0 if no finished 
   * queries are located.
   */
  int GetFinishedQuery(OccQuery &oq);

public:
  csFrustumVis ();
  virtual ~csFrustumVis ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  void CallVisibilityCallbacksForSubtree (NodeTraverseData &ntdNode,
	FrustTest_Front2BackData* data, uint32 cur_timestamp);

  // Test visibility for the given node. Returns 2 if camera is inside node,
  // 1 if visible normally, or 0 if not visible.
  // This function will also modify the frustum_mask in 'data'. So
  // take care to restore this later if you recurse down.
  int TestNodeVisibility (csKDTree* treenode,
  	FrustTest_Front2BackData* data, uint32& frustum_mask);

  // Test visibility for the given object. Returns true if visible.
  bool TestObjectVisibility (csFrustVisObjectWrapper* obj,
  	FrustTest_Front2BackData* data, uint32 frustum_mask);

  // Add an object to the update queue. That way it will be updated
  // in the kdtree later when needed.
  void AddObjectToUpdateQueue (csFrustVisObjectWrapper* visobj_wrap);

  // Update one object in FrustVis. This is called whenever the movable
  //   or object model changes.
  void UpdateObject (csFrustVisObjectWrapper* visobj_wrap);

  virtual void Setup (const char* name);
  virtual void RegisterVisObject (iVisibilityObject* visobj);
  virtual void UnregisterVisObject (iVisibilityObject* visobj);
  virtual bool VisTest (iRenderView* rview, 
    iVisibilityCullerListener* viscallback, int w = 0, int h = 0);
  virtual bool csFrustumVis::VisTest (iRenderView* rview, 
    iVisibilityCullerListener* viscallback,iShaderManager* smShaderMan, int w=0, int h=0);
  virtual void PrecacheCulling () { VisTest ((iRenderView*)0, 0); }
  virtual csPtr<iVisibilityObjectIterator> VisTest (const csBox3& box);
  virtual csPtr<iVisibilityObjectIterator> VisTest (const csSphere& sphere);
  virtual void VisTest (const csSphere& sphere, 
    iVisibilityCullerListener* viscallback);
  virtual csPtr<iVisibilityObjectIterator> VisTest (csPlane3* planes,
  	int num_planes);
  virtual void VisTest (csPlane3* planes,
  	int num_planes, iVisibilityCullerListener* viscallback);
  virtual csPtr<iVisibilityObjectIterator> IntersectSegmentSloppy (
    const csVector3& start, const csVector3& end);
  virtual csPtr<iVisibilityObjectIterator> IntersectSegment (
    const csVector3& start, const csVector3& end, bool accurate = false);
  virtual bool IntersectSegment (const csVector3& start,
    const csVector3& end, csVector3& isect, float* pr = 0,
    iMeshWrapper** p_mesh = 0, int* poly_idx = 0,
    bool accurate = true);
  virtual const char* ParseCullerParameters (iDocumentNode*) { return 0; }
};

/*----------------------------------------------------------------------------*/

class csFrustVisObjIt :
  public scfImplementation1<csFrustVisObjIt, iVisibilityObjectIterator>
{
private:
  csFrustumVis::VistestObjectsArray* vector;
  size_t position;
  bool* vistest_objects_inuse;

public:
  csFrustVisObjIt (csFrustumVis::VistestObjectsArray* vector,
    bool* vistest_objects_inuse) :
    scfImplementationType(this)
  {
    csFrustVisObjIt::vector = vector;
    csFrustVisObjIt::vistest_objects_inuse = vistest_objects_inuse;
    if (vistest_objects_inuse) *vistest_objects_inuse = true;
    Reset ();
  }
  virtual ~csFrustVisObjIt ()
  {
    // If the vistest_objects_inuse pointer is not 0 we set the
    // bool to false to indicate we're no longer using the base
    // vector. Otherwise we delete the vector.
    if (vistest_objects_inuse)
      *vistest_objects_inuse = false;
    else
      delete vector;
  }

  virtual iVisibilityObject* Next()
  {
    if (position == (size_t)-1) return 0;
    iVisibilityObject* vo = vector->Get (position);
    position++;
    if (position == vector->GetSize ())
      position = (size_t)-1;
    return vo;
  }

  virtual void Reset()
  {
    if (vector == 0 || vector->GetSize () < 1)
      position = (size_t)-1;
    else
      position = 0;
  }

  virtual bool HasNext () const
  {
    return ((position != (size_t)-1) && position <= vector->GetSize ());
  }
};

class csFrustVisObjectDescriptor : public scfImplementation1<
	csFrustVisObjectDescriptor, iKDTreeObjectDescriptor>
{
public:
  csFrustVisObjectDescriptor () : scfImplementationType (this) { }
  ~csFrustVisObjectDescriptor () { }
  virtual csPtr<iString> DescribeObject (csKDTreeChild* child)
  {
    scfString* str = new scfString ();
    const csBox3& b = child->GetBBox ();
    csFrustVisObjectWrapper* obj = (csFrustVisObjectWrapper*)(
    	child->GetObject ());
    str->Format ("'%s' (%g,%g,%g)-(%g,%g,%g)",
    	obj->mesh->QueryObject ()->GetName (),
	b.MinX (), b.MinY (), b.MinZ (),
	b.MaxX (), b.MaxY (), b.MaxZ ());
    return str;
  }
};

#endif // __CS_FRUSTVIS_H__

