/*
    Copyright (C) 2001 by W.C.A. Wijngaards

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

#ifndef __PROCSKYTEX_H__
#define __PROCSKYTEX_H__

#include "csgeom/math3d.h"
#include "csgfxldr/rgbpixel.h"
#include "csutil/cscolor.h"
#include "csfx/proctex.h"

class csProcSky;

/** 
 * A polygon of a sky.
*/
class csProcSkyTexture : public csProcTexture {
  /// the sky this is a part of
  csProcSky *sky;

  /// texture orig,udir,vdir (in world coordinates as used in the sky)
  csVector3 txtorig, txtu, txtv;
  /// the cached intersection points
  csVector3 *isect;

public:
  /// create, given a sky it belongs to.
  csProcSkyTexture(csProcSky *par);
  ///
  virtual ~csProcSkyTexture();

  virtual bool PrepareAnim ();

  /// Draw the next frame.
  virtual void Animate (cs_time current_time);

  /// methods for the Sky parent - get the g2d
  iGraphics2D* GetG2D() const {return ptG2D;}
  /// get the g3d
  iGraphics3D* GetG3D() const {return ptG3D;}
  /// get the texture manager (used for encoding colors)
  iTextureManager* GetTextureManager() const {return ptTxtMgr;}
  /// get the width of this texture
  int GetWidth() const {return mat_w;}
  /// get the height of this texture
  int GetHeight() const {return mat_h;}
  /// get texturespace values
  void GetTextureSpace(csVector3& orig, csVector3& u, csVector3& v) const
  { orig = txtorig; u = txtu; v = txtv; }
  /// set cached isects cache array
  void SetIntersect(csVector3 *icache) {isect = icache;}
  /// get cached isects cache array
  csVector3 *GetIntersect() const {return isect;}

  /** 
   * Set the texturemapping of the sky onto this texture
   * txtorig is a corner point of the polygon (say the topleft point)
   *   (in world coordinates)
   * txtu is the vector towards the right for the length of the polygon
   *   (thus txtorig+txtu is the topright point)
   * txtv is the vector towards the bottom for the length of the polygon
   *   (thus txtorig+txtv is the bottomleft point)
  */
  void SetTextureSpace(const csVector3& tex_orig, const csVector3& total_u,
    const csVector3& total_v) {txtorig=tex_orig; txtu=total_u; txtv=total_v;}

  CSOBJTYPE;
};


/**
 * a sky, this represents a whole sphere of sky - so multiple polygons
 * can be rendered to.
*/
class csProcSky {
  /// sphere radius
  float radius;
  /// sphere center
  csVector3 center;
  /// camera point
  csVector3 cam;
  /// sun position
  csVector3 sunpos;
  /// sun's color
  csColor suncolor;

  /// nr of octaves of the clouds.
  int nr_octaves;
  /// size of an octave
  int octsize;
  /// the octaves, for the current state
  uint8 *octaves;
  /// the enlarged octaves
  uint8** enlarged;


  /// is it animation (if not - no recalculation is performed)
  bool animated;
  /// force (re)rendering of the next frame
  bool rerender;
  /// periods for each octave - total new random after this many msec
  int *periods;
  /// current time position (in msec) per octaves
  int *curposition;
  /** 
   * start and end images for each octave; (like octaves but start and
   * end positions of this period of animation
   */
  uint8 *startoctaves, *endoctaves;
  /// the previous time of animated frame
  cs_time old_time;

  /// init the texture
  void Initialize();
  /// init an octave with new random/smoothed content
  void InitOctave(uint8 *octs, int nr);
  /// enlarge an octave, size is scaled by 2**factor, values >> rshift;
  void Enlarge(uint8 *dest, uint8 *src, int factor, int rshift);
  /// take weighted average of start&end into dest, pos(0=start) of max(=end).
  void Combine(uint8 *dest, uint8 *start, uint8 *end, int pos, int max, int nr);
  /// animate octave nr, given elapsed time (msec);
  void AnimOctave(int nr, int elapsed);
  /// octave value get/set
  uint8& GetOctave(uint8 *octaves, int oct, int x, int y) 
  { return octaves [ oct*octsize*octsize + y*octsize + x ]; }
  void SetOctave(uint8 *octaves, int oct, int x, int y, uint8 val) 
  { octaves[ oct*octsize*octsize + y*octsize + x ] = val; }
  /// copy one octave to another
  void CopyOctave(uint8 *srcocts, int srcnr, uint8 *destocts, int destnr);

  /// get the intersection with sphere (false = no intersection)
  bool SphereIntersect(const csVector3& point, csVector3& isect);
  /// get sky bluishness at a point on the sphere. below==the ground
  csRGBcolor GetSkyBlue(const csVector3& spot, float& haze, float sundist,
    bool& below);
  /// get combined octave value, cloudval.
  uint8 GetCloudVal(int x, int y);
  /// get sundistance value
  float GetSundist(const csVector3& spot);

public:
  csProcSky();
  ~csProcSky();

  /// do a nextframe like drawing update
  void DrawToTexture(csProcSkyTexture *skytex, cs_time current_time);

  /// Make intersection point cache in a texture
  void MakeIntersectCache(csProcSkyTexture *skytex);

  /// Enable or disable sky animation. Sky animation is very slow.
  void SetAnimated(bool anim=true) {animated=anim;}
  /// See if the prsky is animated
  bool GetAnimated() const {return animated;}
  /// Force a re-render (only once) of the sky, in the next frame.
  void ForceRerender() {rerender = true;}
  /// no longer force a rerender (undo a ForceRerender call)
  void DonotRerender() {rerender = false;}
};

#endif // __PROCSKYTEX_H__
