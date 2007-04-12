/*
    Crystal Space 3D engine
    Copyright (C) 2003 by Jorrit Tyberghein

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

#ifndef __CS_CSTOOL_OBJMODEL_H__
#define __CS_CSTOOL_OBJMODEL_H__

/**\file
 * Implementation of iObjectModel.
 */

/**
 * \addtogroup geom_utils
 * @{ */
 
#include "csextern.h"

#include "csutil/refarr.h"
#include "csutil/scf_implementation.h"
#include "csutil/hash.h"
#include "iutil/strset.h"
#include "iutil/objreg.h"
#include "imesh/objmodel.h"
#include "igeom/polymesh.h"
#include "igeom/trimesh.h"

struct iTerraFormer;

// for iPolygonMesh
#include "csutil/win32/msvc_deprecated_warn_off.h"

/**
 * Helper class to make it easier to implement iObjectModel in mesh
 * objects. This class does not implement the bounding box and radius
 * functions. 
 */
class csObjectModel : public scfImplementation1<csObjectModel,iObjectModel>
{
private:
  long shapenr;
  iPolygonMesh* polymesh_base;
  csRef<iPolygonMesh> polymesh_colldet;
  csRef<iPolygonMesh> polymesh_viscull;
  csRef<iPolygonMesh> polymesh_shadows;
  csRefArray<iObjectModelListener> listeners;

  csHash<csRef<iTriangleMesh>,csStringID> trimesh;

public:
  /**
   * Construct a new csObjectModel. Don't forget to call
   * SetPolygonMesh<xxx>()!
   */
  csObjectModel (iBase* parent = 0)
    : scfImplementationType (this, parent), shapenr (-1), polymesh_base (0)
  {
  }

  virtual ~csObjectModel () {}

  /**
   * Conveniance method to fetch the standard string registry from the
   * object registry.
   */
  csRef<iStringSet> GetStandardStringSet (iObjectRegistry* object_reg)
  {
    return csQueryRegistryTagInterface<iStringSet> (object_reg,
	"crystalspace.shared.stringset");
  }

  /**
   * Conveniance method to fetch the base string ID given the object
   * registry.
   */
  csStringID GetBaseID (iObjectRegistry* object_reg)
  {
    csRef<iStringSet> strings = GetStandardStringSet (object_reg);
    return strings->Request ("base");
  }

  /**
   * Set the pointer to the base polygon mesh.
   */
  void SetPolygonMeshBase (iPolygonMesh* base)
  {
    polymesh_base = base;
  }

  /**
   * Increase the shape number and also fire all listeners.
   */
  void ShapeChanged ()
  {
    shapenr++;
    FireListeners ();
  }

  /**
   * Set the shape number manually (should not be needed in most cases).
   */
  void SetShapeNumber (long n)
  {
    shapenr = n;
  }

  /**
   * Fire all listeners.
   */
  void FireListeners ()
  {
    size_t i;
    for (i = 0 ; i < listeners.GetSize () ; i++)
      listeners[i]->ObjectModelChanged (this);
  }

  virtual long GetShapeNumber () const { return shapenr; }
  virtual iTriangleMesh* GetTriangleData (csStringID);
  virtual void SetTriangleData (csStringID, iTriangleMesh*);
  virtual bool IsTriangleDataSet (csStringID);
  virtual void ResetTriangleData (csStringID);

  virtual iPolygonMesh* GetPolygonMeshBase () { return polymesh_base; }
  virtual iPolygonMesh* GetPolygonMeshColldet () { return polymesh_colldet; }
  virtual void SetPolygonMeshColldet (iPolygonMesh* polymesh)
  {
    polymesh_colldet = polymesh;
  }
  virtual iPolygonMesh* GetPolygonMeshViscull () { return polymesh_viscull; }
  virtual void SetPolygonMeshViscull (iPolygonMesh* polymesh)
  {
    polymesh_viscull = polymesh;
  }
  virtual iPolygonMesh* GetPolygonMeshShadows () { return polymesh_shadows; }
  virtual void SetPolygonMeshShadows (iPolygonMesh* polymesh)
  {
    polymesh_shadows = polymesh;
  }
  virtual csPtr<iPolygonMesh> CreateLowerDetailPolygonMesh (float)
  {
    return 0;
  }
  virtual void AddListener (iObjectModelListener* listener)
  {
    RemoveListener (listener);
    listeners.Push (listener);
  }
  virtual void RemoveListener (iObjectModelListener* listener)
  {
    listeners.Delete (listener);
  }
  virtual iTerraFormer* GetTerraFormerColldet()
  {
    return 0;
  }
  virtual iTerrainSystem* GetTerrainColldet () { return 0; }
};

/** @} */

// for iPolygonMesh
#include "csutil/win32/msvc_deprecated_warn_on.h"

#endif // __CS_CSTOOL_OBJMODEL_H__

