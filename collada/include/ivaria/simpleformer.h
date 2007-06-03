/*
    Copyright (C) 2004 by Anders Stenberg, Daniel Duhprey

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

#ifndef __CS_IVARIA_SIMPLEFORMER_H__
#define __CS_IVARIA_SIMPLEFORMER_H__

/**\file
 * Simple terraformer interfaces
 */

#include "csutil/scf.h"
#include "csutil/strset.h"

struct iImage;

class csVector3;

/**
 * iSimpleFormerState exposes implementation specific methods
 * for the SimpleTerraformer plugin
 */
struct iSimpleFormerState : public virtual iBase
{
  SCF_INTERFACE (iSimpleFormerState, 1, 0, 0);
 
  /**
   * Set a heightmap to be used. The heightmap will by default be
   * covering a region from -1..1 along X and Z, and areas outside
   * this will return a height of 0
   */
  virtual void SetHeightmap (iImage *heightmap) = 0;
  
  /**
   * Set a heightmap to be used. The heightmap will by default be
   * covering a region from -1..1 along X and Z, and areas outside
   * this will return a height of 0
   * \remarks The plugin will make a copy of the data. So you must
   * delete it after calling this function.
   */
  virtual void SetHeightmap (float* data, unsigned int width,
  	unsigned int height) = 0;

  /**
   * Set a scaling factor to be applied to the heightmap region (X, Z)
   * and height (Y)
   */
  virtual void SetScale (csVector3 scale) = 0;

  /**
   * Set a offset to be applied to the heightmap region (X, Z)
   * and height (Y)
   */
  virtual void SetOffset (csVector3 scale) = 0;

  /**
   * Set a generic additional integer map to be used.
   * \param type is the ID for this map. To get values of this you need to
   *        fetch the stringset with tag 'crystalspace.shared.stringset' and
   *        'Request()' an ID from that.
   * \param map is the image from which this map will be made. This must be an
   *        indexed (palette) image.
   * \param scale The scale to apply to the map.
   * \param offset The offset to apply to the map.
   * \return false on error (bad dimension or image).
   */
  virtual bool SetIntegerMap (csStringID type, iImage* map, int scale = 1,
  	int offset = 0) = 0;

  /**
   * Set a generic additional float map to be used.
   * \param type The ID for this map. To get values of this you need to
   *        fetch the stringset with tag 'crystalspace.shared.stringset' and
   *        'Request()' an ID from that.
   * \param map The image from which this map will be made. If this is an
   *        indexed image then the integer index will be casted to float,
   *        diviced by 256 and then scaled+offset. If this is a 24-bit image
   *        then the three color components are averaged resulting in a value
   *        between 0 and 1 too.
   * \param scale The scale to apply to the map.
   * \param offset The offset to apply to the map.
   * \return false on error (bad dimension or image).
   */
  virtual bool SetFloatMap (csStringID type, iImage* map, float scale = 1.0,
  	float offset = 0.0) = 0;

    /// Gets the processed heightmap data.
  virtual float *GetFloatMap () = 0;

  /**
   * Sets the materials scale
   * \param scale is the amount to scale the material.
   */
  virtual void SetMaterialScale(csVector2 scale)=0;

};

#endif // __CS_IVARIA_SIMPLEFORMER_H__


