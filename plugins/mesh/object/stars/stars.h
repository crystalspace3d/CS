/*
    Copyright (C) 2001 by Jorrit Tyberghein

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

#ifndef _STARS_H_
#define _STARS_H_

#include "csgeom/vector3.h"
#include "csgeom/transfrm.h"
#include "csutil/cscolor.h"
#include "imesh/object.h"
#include "imesh/stars.h"
#include "ivideo/graph3d.h"

struct iMaterialWrapper;
class csStarsMeshObjectFactory;

/**
 * Stars version of mesh object.
 */
class csStarsMeshObject : public iMeshObject
{
private:
  iMeshObjectFactory* factory;
  csBox3 box;
  csMeshCallback* vis_cb;
  void* vis_cbData;
  csColor color;
  csColor max_color;
  bool use_max_color;
  float density;
  float max_dist;

  int seed;

  bool initialized;
  csVector3 max_radius;
  long shapenr;

  /**
   * Setup this object. This function will check if setup is needed.
   */
  void SetupObject ();

public:
  /// Constructor.
  csStarsMeshObject (iMeshObjectFactory* factory);

  /// Destructor.
  virtual ~csStarsMeshObject ();

  /// Set total box.
  void SetBox (const csBox3& b)
  {
    initialized = false;
    box = b;
    shapenr++;
  }
  void GetBox (csBox3& b) const { b = box; }

  /// Set density.
  void SetDensity (float d) { initialized = false; density = d; }
  /// Get density.
  float GetDensity () const { return density; }

  /// Set max distance at which stars are visible.
  void SetMaxDistance (float maxdist) { max_dist = maxdist; }
  /// Get max distance at which stars are visible.
  float GetMaxDistance () const { return max_dist; }
  
  /// Set the color to use. Will be added to the lighting values.
  void SetColor (const csColor& col) { color = col; }
  /// Get the color.
  csColor GetColor () { return color; }

  /**
   * Set the color used in the distance.
   * If this is used then stars at max distance will have
   * this color (fading is used).
   */
  void SetMaxColor (const csColor& col)
  {
    max_color = col;
    use_max_color = true;
  }
  /// Get the max color.
  csColor GetMaxColor () const { return max_color; }
  /// Return true if max color is used.
  bool IsMaxColorUsed () const { return use_max_color; }

  ///---------------------- iMeshObject implementation ------------------------
  DECLARE_IBASE;

  virtual iMeshObjectFactory* GetFactory () { return factory; }
  virtual bool DrawTest (iRenderView* rview, iMovable* movable);
  virtual void UpdateLighting (iLight** lights, int num_lights,
      	iMovable* movable);
  virtual bool Draw (iRenderView* rview, iMovable* movable, csZBufMode mode);
  virtual void SetVisibleCallback (csMeshCallback* cb, void* cbData)
  {
    vis_cb = cb;
    vis_cbData = cbData;
  }
  virtual csMeshCallback* GetVisibleCallback ()
  {
    return vis_cb;
  }
  virtual void GetObjectBoundingBox (csBox3& bbox, int type = CS_BBOX_NORMAL);
  virtual csVector3 GetRadius () { return max_radius; }
  virtual void NextFrame (cs_time /*current_time*/) { }
  virtual bool WantToDie () { return false; }
  virtual void HardTransform (const csReversibleTransform&) { }
  virtual bool SupportsHardTransform () { return false; }
  virtual bool HitBeamObject (const csVector3&, const csVector3&,
  	csVector3&, float*) { return false; }
  virtual long GetShapeNumber () { return shapenr; }

  //------------------------- iStarsState implementation ----------------
  class StarsState : public iStarsState
  {
    DECLARE_EMBEDDED_IBASE (csStarsMeshObject);
    virtual void SetBox (const csBox3& b)
    {
      scfParent->SetBox (b);
    }
    virtual void GetBox (csBox3& b) const
    {
      scfParent->GetBox (b);
    }

    virtual void SetColor (const csColor& col)
    {
      scfParent->SetColor (col);
    }
    virtual csColor GetColor () const
    {
      return scfParent->GetColor ();
    }
    virtual void SetMaxColor (const csColor& col)
    {
      scfParent->SetMaxColor (col);
    }
    virtual csColor GetMaxColor () const
    {
      return scfParent->GetMaxColor ();
    }
    virtual bool IsMaxColorUsed () const
    {
      return scfParent->IsMaxColorUsed ();
    }

    virtual void SetDensity (float d)
    {
      scfParent->SetDensity (d);
    }
    virtual float GetDensity ()
    {
      return scfParent->GetDensity ();
    }
    virtual void SetMaxDistance (float maxdist)
    {
      scfParent->SetMaxDistance (maxdist);
    }
    virtual float GetMaxDistance () const
    {
      return scfParent->GetMaxDistance ();
    }
  } scfiStarsState;
  friend class StarsState;
};

/**
 * Factory for balls.
 */
class csStarsMeshObjectFactory : public iMeshObjectFactory
{
public:
  /// Constructor.
  csStarsMeshObjectFactory (iBase *pParent);

  /// Destructor.
  virtual ~csStarsMeshObjectFactory ();

  //------------------------ iMeshObjectFactory implementation --------------
  DECLARE_IBASE;

  virtual iMeshObject* NewInstance ();
  virtual void HardTransform (const csReversibleTransform&) { }
  virtual bool SupportsHardTransform () { return false; }
};

/**
 * Stars type. This is the plugin you have to use to create instances
 * of csStarsMeshObjectFactory.
 */
class csStarsMeshObjectType : public iMeshObjectType
{
public:
  /// Constructor.
  csStarsMeshObjectType (iBase*);

  /// Destructor.
  virtual ~csStarsMeshObjectType ();

  /// Register plugin with the system driver
  virtual bool Initialize (iSystem *pSystem);

  //------------------------ iMeshObjectType implementation --------------
  DECLARE_IBASE;

  /// New factory.
  virtual iMeshObjectFactory* NewFactory ();
};

#endif // _STARS_H_

