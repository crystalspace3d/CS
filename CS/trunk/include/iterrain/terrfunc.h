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

#ifndef __ITERRAIN_TERRFUNC_H_
#define __ITERRAIN_TERRFUNC_H_

#include "csutil/scf.h"

struct iEngine;
struct iImage;
class csVector3;
class csColor;

/**
 * Function for the terrain engine. Expects values for dx and dy between
 * 0 and 1 and returns a height.
 */
typedef float (csTerrainHeightFunction)(void* data, float dx, float dy);
/**
 * Function for the terrain engine. Expects values for dx and dy between
 * 0 and 1 and returns a normal.
 */
typedef csVector3 (csTerrainNormalFunction)(void* data, float dx, float dy);

SCF_VERSION (iTerrFuncState, 0, 0, 4);

/**
 * This interface describes the API for the terrain object.
 */
struct iTerrFuncState : public iBase
{
  /// Load a group of materials.
  virtual void LoadMaterialGroup (iEngine* engine, const char *pName,
  	int iStart, int iEnd) = 0;
  /// Set the top-left corner of the terrain.
  virtual void SetTopLeftCorner (const csVector3& topleft) = 0;
  // Get the top-left corner.
  virtual csVector3 GetTopLeftCorner () = 0;
  /// Set the scale of the terrain.
  virtual void SetScale (const csVector3& scale) = 0;
  /// Get the scale of the terrain.
  virtual csVector3 GetScale () = 0;
  /// Setup the number of blocks in the terrain.
  virtual void SetResolution (int x, int y) = 0;
  /// Get the x resolution.
  virtual int GetXResolution () = 0;
  /// Get the y resolution.
  virtual int GetYResolution () = 0;
  /**
   * Setup the number of grid points in every block for the base mesh.
   */
  virtual void SetGridResolution (int x, int y) = 0;
  /// Get the x resolution for a block.
  virtual int GetXGridResolution () = 0;
  /// Get the y resolution for a block.
  virtual int GetYGridResolution () = 0;
  /// Set the base color.
  virtual void SetColor (const csColor& col) = 0;
  /// Get the base color.
  virtual csColor GetColor () const = 0;
  /// Set the function to use for the terrain.
  virtual void SetHeightFunction (csTerrainHeightFunction* func,
  	void* data) = 0;
  /// Set the normal function to use for the terrain.
  virtual void SetNormalFunction (csTerrainNormalFunction* func,
  	void* data) = 0;
  /// Use the given iImage to get a height function from.
  virtual void SetHeightMap (iImage* im, float hscale, float hshift) = 0;

  /**
   * Set the distance at which to switch to the given lod level
   * (lod from 1 to 3).
   */
  virtual void SetLODDistance (int lod, float dist) = 0;
  /**
   * Get the distance at which lod will switch to that level.
   */
  virtual float GetLODDistance (int lod) = 0;
  /**
   * Set the maximum cost for LOD level (1..3).
   */
  virtual void SetMaximumLODCost (int lod, float maxcost) = 0;
  /**
   * Get the maximum cost for LOD level (1..3).
   */
  virtual float GetMaximumLODCost (int lod) = 0;
};

#endif // __ITERRAIN_TERRFUNC_H_

