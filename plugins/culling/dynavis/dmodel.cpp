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
#include "cssys/sysfunc.h"
#include "csutil/scfstr.h"
#include "iutil/string.h"
#include "qint.h"
#include "qsqrt.h"
#include "csgeom/math3d.h"
#include "csgeom/pmtools.h"
#include "igeom/objmodel.h"
#include "igeom/polymesh.h"
#include "iengine/mesh.h"
#include "iutil/object.h"
#include "dmodel.h"

//---------------------------------------------------------------------------

csObjectModel::csObjectModel ()
{
  planes = NULL;
  num_planes = -1;
  edges = NULL;
  num_edges = -1;
  dirty_obb = true;
  has_obb = false;
}

csObjectModel::~csObjectModel ()
{
  delete[] planes;
  delete[] edges;
}

void csObjectModel::UpdateOutline (const csVector3& pos)
{
  if (!imodel->GetPolygonMeshViscull ()) return;

  int num_vertices = imodel->GetPolygonMeshViscull ()->GetVertexCount ();

  bool recalc_outline = false;
  if (!outline_info.outline_edges)
  {
    // @@@ Only allocate active edges.
    outline_info.outline_edges = new int [num_edges*2];
    outline_info.outline_verts = new bool [num_vertices];
    recalc_outline = true;
  }
  else
  {
    float sqdist = csSquaredDist::PointPoint (pos, outline_info.outline_pos);
    if (sqdist > outline_info.valid_radius * outline_info.valid_radius)
      recalc_outline = true;
  }

  if (recalc_outline)
  {
    csPolygonMeshTools::CalculateOutline (edges, num_edges,
    	planes, num_vertices, pos,
	outline_info.outline_edges, outline_info.num_outline_edges,
	outline_info.outline_verts,
	outline_info.valid_radius);
    outline_info.outline_pos = pos;
  }
}

bool csObjectModel::HasOBB ()
{
  GetOBB ();
  return has_obb;
}

const csOBB& csObjectModel::GetOBB ()
{
  if (dirty_obb)
  {
    dirty_obb = false;
    if (imodel->GetPolygonMeshViscull ())
    {
      int num_vertices = imodel->GetPolygonMeshViscull ()->GetVertexCount ();
      csVector3* verts = imodel->GetPolygonMeshViscull ()->GetVertices ();
      obb.FindOBB (verts, num_vertices);
      has_obb = true;
    }
    else
    {
      has_obb = false;
    }
  }
  return obb;
}

//---------------------------------------------------------------------------

csObjectModelManager::csObjectModelManager () : models (12263)
{
}

csObjectModelManager::~csObjectModelManager ()
{
  csHashIterator it (&models);
  while (it.HasNext ())
  {
    csObjectModel* model = (csObjectModel*)it.Next ();
    delete model;
  }
}

csObjectModel* csObjectModelManager::CreateObjectModel (iObjectModel* imodel)
{
  csObjectModel* model = (csObjectModel*)models.Get ((csHashKey)imodel);
  if (model)
  {
    model->ref_cnt++;
  }
  else
  {
    model = new csObjectModel ();
    model->ref_cnt = 1;
    model->imodel = imodel;
    // To make sure we will recalc we set shape_number to one less.
    model->shape_number = imodel->GetShapeNumber ()-1;
  }
  return model;
}

void csObjectModelManager::ReleaseObjectModel (csObjectModel* model)
{
  CS_ASSERT (model->ref_cnt > 0);
  if (model->ref_cnt == 1)
  {
    // We are about to delete the model.
    models.DeleteAll ((csHashKey)(model->imodel));
    delete model;
    return;
  }
  model->ref_cnt--;
}

static int show_notclosed = 6;

bool csObjectModelManager::CheckObjectModel (csObjectModel* model,
	iMeshWrapper* mw)
{
  CS_ASSERT (model->ref_cnt > 0);
  model->use_outline_filler = true;
  if (model->imodel->GetShapeNumber () != model->shape_number)
  {
    model->shape_number = model->imodel->GetShapeNumber ();
    model->outline_info.Clear ();
    model->dirty_obb = true;
    iPolygonMesh* mesh = model->imodel->GetPolygonMeshViscull ();
    if (mesh)
    {
      if (model->num_planes != mesh->GetPolygonCount ())
      {
        delete[] model->planes;
        model->num_planes = mesh->GetPolygonCount ();
        model->planes = new csPlane3 [model->num_planes];
      }
      csPolygonMeshTools::CalculatePlanes (mesh, model->planes);
      delete[] model->edges;
      model->edges = csPolygonMeshTools::CalculateEdges (
      	mesh, model->num_edges);
      csPolygonMeshTools::CheckActiveEdges (model->edges, model->num_edges,
      	model->planes);

      // Here we scan all edges and see if there are edges that have only
      // one adjacent polygon. If we find such an edge then we will not use
      // outline based culling for this object. This is not good as it will
      // slow down culling so you should try to avoid this situation in levels.

      int i;
      for (i = 0 ; i < model->num_edges ; i++)
        if (model->edges[i].poly2 == -1)
	{
	  model->use_outline_filler = false;
	  if (show_notclosed > 0)
	  {
	    printf ("WARNING! Object '%s' is not closed!\n", mw != NULL ?
	  	mw->QueryObject ()->GetName () : "<no mesh>");
	    fflush (stdout);
	    show_notclosed--;
	  }
	  else if (show_notclosed == 0)
	  {
	    printf ("...\n");
	    fflush (stdout);
	    show_notclosed--;
	  }
	  break;
	}
    }
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

