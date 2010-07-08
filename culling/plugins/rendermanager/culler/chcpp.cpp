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

#include "cssysdef.h"
#include <string.h>
#include "csutil/sysfunc.h"
#include "csutil/scf.h"
#include "csutil/util.h"
#include "csutil/scfstr.h"
#include "csutil/event.h"
#include "csutil/eventnames.h"
#include "iutil/event.h"
#include "iutil/eventq.h"
#include "csgeom/frustum.h"
#include "csgeom/matrix3.h"
#include "csgeom/math3d.h"
#include "csgeom/obb.h"
#include "csgeom/segment.h"
#include "csgeom/sphere.h"
#include "csgeom/kdtree.h"
#include "imesh/objmodel.h"
#include "csutil/flags.h"
#include "iutil/objreg.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/txtmgr.h"
#include "ivideo/fontserv.h"
#include "iengine/movable.h"
#include "iengine/rview.h"
#include "iengine/camera.h"
#include "iengine/mesh.h"
#include "imesh/object.h"
#include "iutil/object.h"
#include "ivaria/reporter.h"
#include "frustvis.h"
#include "chcpp.h"

void csFrustumVis::QueryPreviouslyInvisibleNode(NodeTraverseData &ntdNode)
{
  I_Queue.PushBack(ntdNode);
  if(I_Queue.Size()>=PREV_INV_BATCH_SIZE)
  {
    // here we'll issue multi queries
  }
}

/* Pulls up the visibility */
void csFrustumVis::PullUpVisibility(NodeTraverseData &ntdNode)
{
  NodeTraverseData ntdAux=ntdNode;
  while(ntdAux.kdtParent && !ntdAux.GetVisibility())
  {
    ntdAux.SetVisibility(true);
    ntdAux.kdtNode=ntdAux.kdtParent;
    ntdAux.kdtParent=ntdAux.kdtParent->GetParent();
  }
}

void csFrustumVis::TraverseNode(NodeTraverseData &ntdNode,const int cur_timestamp)
{
  if (ntdNode.IsLeaf()) // if node is leaf we render it
  {
    const int num_objects = ntdNode.kdtNode->GetObjectCount ();
    csKDTreeChild** objects = ntdNode.kdtNode->GetObjects ();
    for (int i = 0 ; i < num_objects ; i++)
    {
      if (objects[i]->timestamp != cur_timestamp)
      {
        objects[i]->timestamp = cur_timestamp;
        csFrustVisObjectWrapper* visobj_wrap = (csFrustVisObjectWrapper*)
      	  objects[i]->GetObject ();
        TestObjectVisibility (visobj_wrap, &f2bData, ntdNode.GetFrustumMask());
      }
    }
  }
  else // else we push it's children on to the traverse queue
  {
    csKDTree* child1 = ntdNode.kdtNode->GetChild1 ();
    T_Queue.PushBack(NodeTraverseData(ntdNode.kdtNode,child1,ntdNode.GetFrustumMask()));
    csKDTree* child2 = ntdNode.kdtNode->GetChild2 ();
    T_Queue.PushBack(NodeTraverseData(ntdNode.kdtNode,child2,ntdNode.GetFrustumMask()));
  }
}
