/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein
    Written by Andrew Zabolotny

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

#ifndef __CS_SCAN_H__
#define __CS_SCAN_H__

#include "csutil/scf.h"
#include "csgfxldr/rgbpixel.h"

//---//---//---//---//---//---//---//---//---//---/ Forward declarations //---//

class csTextureSoftware;
class csTextureMMSoftware;
class csGraphics3DSoftwareCommon;
struct iPolygonTexture;

//---//---//---//---//---//---//---//---//---//---//---//---// Constants //---//

#define INTER_MODE_SMART	0
#define INTER_MODE_STEP8	1
#define INTER_MODE_STEP16	2
#define INTER_MODE_STEP32	3

// Specifies boundary where texel filtering does not occur
#define BAILOUT_CONSTANT	32768

// A coefficient for planar fog density: bigger is denser
#define PLANAR_FOG_DENSITY_COEF	6

/// At this point QRound (255 * exp (-float (i) / 256.)) reaches zero
#define EXP_256_SIZE			1420
/// Same for QRound (32 * exp (-float (i) / 256.))
#define EXP_32_SIZE			1065

/// Blending table indices
#define BLENDTABLE_ADD                  0x00
#define BLENDTABLE_MULTIPLY             0x01
#define BLENDTABLE_MULTIPLY2            0x02
#define BLENDTABLE_ALPHA25              0x03
#define BLENDTABLE_ALPHA50              0x04
#define BLENDTABLE_ALPHA75              0x05
#define NUMBLENDINGTABLES               6

//---//---//---//---//---//---//---//---//---//---//---/ Precomputed data /---//

/*
 * Structure with precalculated data<p>
 * There is generally one global structure of this type.
 * However, to make assembler life easier we do it in a structure
 * rather than defining many static variables.<p>
 * <b>WARNING:</p> Do not forget to synchronise all changes
 * made to this structure with i386/scan.ash!
 */
struct csScanSetup
{
  /// Interpolation step for semi-perspective correct texture mapping
  int InterpolStep;
  /// Interpolation step (shift-value) for semi-perspective correct texture mapping
  int InterpolShift;
  /// Interpolation mode
  int InterpolMode;

  /// Fog color
  unsigned long FogR;
  unsigned long FogG;
  unsigned long FogB;
  /// The fog pixel (R|G|B for true/hicolor) or index into palette of fog color
  unsigned long FogPix;
  /// Fog density
  unsigned int FogDensity;
  /// The fog table for paletted (currently only 8-bit) modes
  unsigned char *Fog8;

  /// A pointer to the texture.
  csTextureMMSoftware *Texture;

  /// The lighted texture bitmap from the texture cache.
  unsigned char *bitmap2;
  /// Width of the texture from the texture cache.
  int tw2;
  /// Height of the texture from the texture cache.
  int th2;
  /// Texture width in 16:16 fixed-point format - 1
  int tw2fp;
  /// Texture height in 16:16 fixed-point format - 1
  int th2fp;

  /// The unlighted texture bitmap.
  unsigned char *bitmap;
  /// Width of unlighted texture.
  int tw;
  /// Height of unlighted texture.
  int th;

  /**
   * The following fields are used by the polygon drawer
   * and contain information fot calculating the 1/z, u/z, and v/z linear
   * equations.
   */

  /// Difference value for every horizontal dx.
  float M;
  /// Difference value for every horizontal dx.
  float J1;
  /// Difference value for every horizontal dx.
  float K1;
  /// Difference for every 16 pixels.
  float dM;
  /// Difference for every 16 pixels.
  float dJ1;
  /// Difference for every 16 pixels.
  float dK1;

  /// Mean color value.
  int FlatColor;
  /// R/G/B components of flatcolor
  RGBPixel FlatRGB;

  /// Alpha mask for 16-bit renderer.
  unsigned int AlphaMask;
  /// General alpha factor for 16-bit renderer (0 to 255).
  int AlphaFact;

  /// log2 (texture_u)
  int shf_u;

  /**
   * The following fields are only used when drawing
   * unlighted tiled textures (the scan_..._map_...
   * routines don't need them).
   */

  /// log2(texture_width)
  int shf_w;
  /// 1 << shf_w - 1
  int and_w;
  /// log2(texture_height)
  int shf_h;
  /// 1 << shf_h - 1
  int and_h;
  /// U at the origin of the texture
  int min_u;
  /// V at the origin of the texture
  int min_v;

  /// Actual texture palette
  RGBPixel *TexturePalette;
  /// 8-bit to native pixel format conversion table
  void *PaletteTable;
  /// Set up by poly renderer to alpha blending table
  uint8 *AlphaMap;

  /// Current blending table
  unsigned char *BlendTable;

  /// The inverse colormap (extracted from texture manager)
  UByte *inv_cmap;

  /// This is the global palette -> uint16 conversion table (for 8-bit modes)
  uint16 *GlobalCMap;
  /// And this one is the texture private palette -> uint16 conversion table
  uint16 *PrivateCMap;

  /**
   * A table of 4096 1/z values where z is in fixed-point 0.12 format
   * Used in fog routines to get the real value of Z. The result is
   * an 8.24 fixed-point number.
   */
  unsigned int *one_div_z;

  /**
   * A table of exp(x) in the range 0..255; x == 0..EXP_256_SIZE
   */
  unsigned char *exp_256;
  /**
   * Same in the range 0..31 for 8-bit fog
   */
  unsigned char *exp_16;

  /**
   * This table contains eight subtables - one for each available blending
   * mode (see BLENDTABLE_XXX constants). Since SRC can be twice brighter
   * than maximum, BlendingTable has a twice larger SRC dimension (i.e.
   * it is a (BLENDTABLE_MAX*2 x BLENDTABLE_MAX) matrix);
   */
  unsigned char *BlendingTable [NUMBLENDINGTABLES];
};

// The following should be accessible from assembly, so avoid C++ mangling
extern "C"
{

/// The only instance of this structure
extern csScanSetup Scan;

#ifdef TOP8BITS_R8G8B8_USED
#define PIXEL_ADJUST , int pixel_adjust
#define PIXEL_INIT = 0
#else
#define PIXEL_ADJUST
#define PIXEL_INIT
#endif

/// The interface definition for all scan_XXX routines
typedef void (csDrawScanline)
  (int xx, unsigned char* d, unsigned long* z_buf, float inv_z,
   float u_div_z, float v_div_z PIXEL_ADJUST PIXEL_INIT);
/// The interface definition for all scan_pi_XXX routines
typedef void (csDrawPIScanline)
  (void *dest, int len, unsigned long *zbuff, long u, long du, long v, long dv,
   unsigned long z, long dz, unsigned char *bitmap, int bitmap_log2w);
/// The interface definition for all scan_pi_XXX_gou_XXX routines
typedef void (csDrawPIScanlineGouraud)
  (void *dest, int len, unsigned long *zbuff, long u, long du, long v, long dv,
   unsigned long z, long dz, unsigned char *bitmap, int bitmap_log2w,
   ULong r, ULong g, ULong b, long dr, long dg, long db, bool clamp);

#undef PIXEL_INIT

//---//---//---//---//---//---//---//---//---//---//---//---//- Routines //---//

/// Initialize all required tables
void csScan_Initialize ();
/// Free all tables
void csScan_Finalize ();
/// Calculate blending tables (should be called each time pixel format changes)
void csScan_CalcBlendTables (int rbits, int gbits, int bbits);
/// Initialize the scanline variables
void csScan_InitDraw (int MipMap, csGraphics3DSoftwareCommon* g3d,
  iPolygonTexture* tex, csTextureMMSoftware* texture, csTextureSoftware *untxt);
/// Dump debugging information about last polygon
void csScan_dump (csGraphics3DSoftwareCommon* pG3D);
/// Pixel-depth independent routine
csDrawScanline csScan_scan_zfil;
/// Pixel-depth independent routine
csDrawPIScanline csScan_scan_pi_zfil;

//---//---//---//---//---//---//---//---//---//-- 8-bit drawing routines //---//

/// Draw one horizontal scanline with no texture mapping and fill Z buffer
csDrawScanline csScan_8_scan_flat_zfil;
/// Draw one horizontal scanline with no texture mapping (but use Z buffer).
csDrawScanline csScan_8_scan_flat_zuse;
/// Draw one horizontal scanline (no lighting).
csDrawScanline csScan_8_scan_tex_zfil;
/// Draw one horizontal scanline (Z buffer and no lighting).
csDrawScanline csScan_8_scan_tex_zuse;
/// Draw one horizontal scanline (lighting).
csDrawScanline csScan_8_scan_map_zfil;
/// Draw one horizontal scanline (Z buffer and lighting).
csDrawScanline csScan_8_scan_map_zuse;
/// Draw one horizontal scanline (lighting and filtering).
csDrawScanline csScan_8_scan_map_filt_zfil;
/// Draw one horizontal scanline (transparent and no lighting, with Z-fill).
csDrawScanline csScan_8_scan_tex_key_zfil;
/// Draw one horizontal scanline (transparent and no lighting, with Z-check).
csDrawScanline csScan_8_scan_tex_key_zuse;
/// Draw one horizontal scanline (transparent with lighting, with Z-fill).
csDrawScanline csScan_8_scan_map_key_zfil;
/// Draw one horizontal scanline (transparent with lighting, with Z-check).
csDrawScanline csScan_8_scan_map_key_zuse;

/// Draw one horizontal scanline for fog.
csDrawScanline csScan_8_scan_fog;
/// Draw one horizontal scanline for fog when camera is inside fog
csDrawScanline csScan_8_scan_fog_view;

/// Draw one horizontal scanline (lighting and fixed alpha transparency).
csDrawScanline csScan_8_scan_map_fixalpha1;
/// Draw one horizontal scanline (lighting and fixed alpha transparency).
csDrawScanline csScan_8_scan_map_fixalpha2;
/// Draw one horizontal scanline (alphamap and no lighting).
csDrawScanline csScan_8_scan_tex_alpha;
/// Draw one horizontal scanline (alphamap with lighting).
csDrawScanline csScan_8_scan_map_alpha;

/*
 * The following methods are used by DrawPolygonFX() and do not require
 * perspective-correct texture mapping. They do not require InitDraw ()
 * to be called before using.
 */
/// Draw a flat-lighted perspective-incorrect with Z-fill
csDrawPIScanline csScan_8_scan_pi_flat_zfil;
/// Draw a flat-lighted perspective-incorrect polygon scanline
csDrawPIScanline csScan_8_scan_pi_flat_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline. Z fill only
csDrawPIScanline csScan_8_scan_pi_tex_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_8_scan_pi_tex_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline. Z fill only
csDrawPIScanline csScan_8_scan_pi_tex_key_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_8_scan_pi_tex_key_zuse;
/// Draw a flat-lighted perspective-incorrect with table-driven effects with Z-fill
csDrawPIScanline csScan_8_scan_pi_flat_fx_zfil;
/// Draw a flat-lighted perspective-incorrect polygon scanline with table-driven effects
csDrawPIScanline csScan_8_scan_pi_flat_fx_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_8_scan_pi_tex_fx_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_8_scan_pi_tex_fx_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_8_scan_pi_tex_fxkey_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_8_scan_pi_tex_fxkey_zuse;
/// Draw a flat-shaded alpha-mapped texture
csDrawPIScanline csScan_8_scan_pi_tex_alpha;

/// Draw a single-color Gouraud-shaded polygon with Z-fill
csDrawPIScanlineGouraud csScan_8_scan_pi_flat_gou_zfil;
/// Draw a single-color Gouraud-shaded polygon
csDrawPIScanlineGouraud csScan_8_scan_pi_flat_gou_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading. Z fill only
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_gou_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading.
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_gou_zuse;
/// Perspective-incorrect textured polygon with gouraud shading and color keying and Z-fill
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goukey_zfil;
/// Perspective-incorrect textured polygon with gouraud shading and color keying and Z-use
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goukey_zuse;
/// Draw a single-color Gouraud-shaded polygon with Z-fill
csDrawPIScanlineGouraud csScan_8_scan_pi_flat_goufx_zfil;
/// Draw a single-color Gouraud-shaded polygon
csDrawPIScanlineGouraud csScan_8_scan_pi_flat_goufx_zuse;
/// Draw a perspective-incorrect polygon scanline with various effects Z fill only
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goufx_zfil;
/// Draw a perspective-incorrect polygon scanline with various effects
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goufx_zuse;
/// Draw a perspective-incorrect polygon scanline with various effects and color keying and Z-fill
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goufxkey_zfil;
/// Draw a perspective-incorrect polygon scanline with various effects and color keying and Z-use
csDrawPIScanlineGouraud csScan_8_scan_pi_tex_goufxkey_zuse;

#ifdef DO_MMX
/// Draw one horizontal scanline (lighting) using MMX
csDrawScanline csScan_8_mmx_scan_map_zfil;
/// Draw one horizontal scanline (no lighting) using MMX
csDrawScanline csScan_8_mmx_scan_tex_zfil;
#endif

//---//---//---//---//---//---//---//---//---//- 16-bit drawing routines //---//

/// Draw one horizontal scanline (no texture mapping).
csDrawScanline csScan_16_scan_flat_zfil;
/// Draw one horizontal scanline (Z buffer and no texture mapping).
csDrawScanline csScan_16_scan_flat_zuse;
/// Draw one horizontal scanline (no lighting).
csDrawScanline csScan_16_scan_tex_zfil;
/// Draw one horizontal scanline (Z buffer and no lighting).
csDrawScanline csScan_16_scan_tex_zuse;
/// Draw one horizontal scanline (lighting).
csDrawScanline csScan_16_scan_map_zfil;
/// Draw one horizontal scanline (Z buffer and lighting).
csDrawScanline csScan_16_scan_map_zuse;
/// Draw one horizontal scanline (lighting and filtering).
csDrawScanline csScan_16_scan_map_filt_zfil;
/// Draw one horizontal scanline (lighting and more filtering).
csDrawScanline csScan_16_scan_map_filt2_zfil_555;
csDrawScanline csScan_16_scan_map_filt2_zfil_565;
/// Draw one horizontal scanline (lighting and more filtering, Z-check).
csDrawScanline csScan_16_scan_map_filt2_zuse_555;
csDrawScanline csScan_16_scan_map_filt2_zuse_565;
/// Draw one horizontal scanline (transparent and no lighting, Z-fill).
csDrawScanline csScan_16_scan_tex_key_zfil;
/// Draw one horizontal scanline (transparent and no lighting, Z-check).
csDrawScanline csScan_16_scan_tex_key_zuse;
/// Draw one horizontal scanline (transparent with lighting).
csDrawScanline csScan_16_scan_map_key_zfil;
/// Draw one horizontal scanline (transparent with lighting, with Z-check).
csDrawScanline csScan_16_scan_map_key_zuse;

/// Draw one horizontal scanline for fog.
csDrawScanline csScan_16_scan_fog_555;
csDrawScanline csScan_16_scan_fog_565;
/// Draw one horizontal scanline for fog assuming the camera is in fog.
csDrawScanline csScan_16_scan_fog_view_555;
csDrawScanline csScan_16_scan_fog_view_565;

/// Draw one horizontal scanline (lighting and fixed alpha transparency).
csDrawScanline csScan_16_scan_map_fixalpha50;
/// Draw one horizontal scanline (lighting and fixed alpha transparency). General case.
csDrawScanline csScan_16_scan_map_fixalpha_555;
csDrawScanline csScan_16_scan_map_fixalpha_565;
/// Draw one horizontal scanline (alphamap and no lighting).
csDrawScanline csScan_16_scan_tex_alpha_555;
csDrawScanline csScan_16_scan_tex_alpha_565;
/// Draw one horizontal scanline (alphamap with lighting).
csDrawScanline csScan_16_scan_map_alpha_555;
csDrawScanline csScan_16_scan_map_alpha_565;

/// Draw a flat-lighted perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_16_scan_pi_flat_zuse;
/// Draw a flat-lighted perspective-incorrect textured scanline with Z-fill
csDrawPIScanline csScan_16_scan_pi_flat_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline. Z fill only
csDrawPIScanline csScan_16_scan_pi_tex_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_16_scan_pi_tex_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline, color keying. Z fill only
csDrawPIScanline csScan_16_scan_pi_tex_key_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline, color keying
csDrawPIScanline csScan_16_scan_pi_tex_key_zuse;

/// Draw a flat-lighted perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_16_scan_pi_flat_fx_zuse_555;
csDrawPIScanline csScan_16_scan_pi_flat_fx_zuse_565;
/// Draw a flat-lighted perspective-incorrect textured scanline with Z-fill
csDrawPIScanline csScan_16_scan_pi_flat_fx_zfil_555;
csDrawPIScanline csScan_16_scan_pi_flat_fx_zfil_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_16_scan_pi_tex_fx_zfil_555;
csDrawPIScanline csScan_16_scan_pi_tex_fx_zfil_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_16_scan_pi_tex_fx_zuse_555;
csDrawPIScanline csScan_16_scan_pi_tex_fx_zuse_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_16_scan_pi_tex_fxkey_zfil_555;
csDrawPIScanline csScan_16_scan_pi_tex_fxkey_zfil_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_16_scan_pi_tex_fxkey_zuse_555;
csDrawPIScanline csScan_16_scan_pi_tex_fxkey_zuse_565;
/// Draw a flat-shaded alpha-mapped texture
csDrawPIScanline csScan_16_scan_pi_tex_alpha_555;
csDrawPIScanline csScan_16_scan_pi_tex_alpha_565;

/// Draw a single-color Gouraud-shaded polygon with Z-fill
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_gou_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_gou_zfil_565;
/// Draw a single-color Gouraud-shaded polygon
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_gou_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_gou_zuse_565;

/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading. Z fill only
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_gou_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_gou_zfil_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading.
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_gou_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_gou_zuse_565;

/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading. Z fill only
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goukey_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goukey_zfil_565;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading.
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goukey_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goukey_zuse_565;

/// Draw a single-color Gouraud-shaded polygon with Z-fill
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_goufx_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_goufx_zfil_565;
/// Draw a single-color Gouraud-shaded polygon
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_goufx_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_flat_goufx_zuse_565;

/// Draw a perspective-incorrect polygon scanline with various effects Z fill only
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufx_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufx_zfil_565;
/// Draw a perspective-incorrect polygon scanline with various effects
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufx_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufx_zuse_565;

/// Draw a perspective-incorrect polygon scanline with various effects Z fill only (colorkeying)
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufxkey_zfil_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufxkey_zfil_565;
/// Draw a perspective-incorrect polygon scanline with various effects (colorkeying)
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufxkey_zuse_555;
csDrawPIScanlineGouraud csScan_16_scan_pi_tex_goufxkey_zuse_565;

#if defined (DO_MMX) && !defined (DO_NASM)
// There are no MMX perspective-incorrect routines for GAS and VC assembler
#  define csScan_16_mmx_scan_pi_tex_zuse csScan_16_scan_pi_tex_zuse
#endif

#ifdef DO_MMX
/// Draw one horizontal scanline (lighting) using MMX
csDrawScanline csScan_16_mmx_scan_map_zfil;
/// Draw one horizontal scanline (no lighting).
csDrawScanline csScan_16_mmx_scan_tex_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_16_mmx_scan_pi_tex_zuse;
#endif

//---//---//---//---//---//---//---//---//---//- 32-bit scanline drawers //---//

/// Draw one horizontal scanline (no texture mapping).
csDrawScanline csScan_32_scan_flat_zfil;
/// Draw one horizontal scanline (Z buffer and no texture mapping).
csDrawScanline csScan_32_scan_flat_zuse;
/// Draw one horizontal scanline (no lighting).
csDrawScanline csScan_32_scan_tex_zfil;
/// Draw one horizontal scanline (Z buffer and no lighting).
csDrawScanline csScan_32_scan_tex_zuse;
/// Draw one horizontal scanline (lighting).
csDrawScanline csScan_32_scan_map_zfil;
/// Draw one horizontal scanline (Z buffer and lighting).
csDrawScanline csScan_32_scan_map_zuse;
/// Same as above but do bilinear filtering (Z-fill)
csDrawScanline csScan_32_scan_map_filt2_zfil;
/// Same as above but do bilinear filtering (Z-check)
csDrawScanline csScan_32_scan_map_filt2_zuse;
/// Draw one horizontal scanline (transparent and no lighting, Z-fill).
csDrawScanline csScan_32_scan_tex_key_zfil;
/// Draw one horizontal scanline (transparent and no lighting, Z-check).
csDrawScanline csScan_32_scan_tex_key_zuse;
/// Draw one horizontal scanline (transparent with lighting).
csDrawScanline csScan_32_scan_map_key_zfil;
/// Draw one horizontal scanline (transparent with lighting, with Z-check).
csDrawScanline csScan_32_scan_map_key_zuse;

/// Draw one horizontal scanline for fog
csDrawScanline csScan_32_scan_fog;
/// Draw one horizontal scanline for fog assuming the camera is in fog
csDrawScanline csScan_32_scan_fog_view;

/// Draw one horizontal scanline (lighting and fixed alpha transparency).
csDrawScanline csScan_32_scan_map_fixalpha50;
/// Draw one horizontal scanline (lighting and fixed alpha transparency). General case.
csDrawScanline csScan_32_scan_map_fixalpha;
/// Draw one horizontal scanline (alphamap and no lighting).
csDrawScanline csScan_32_scan_tex_alpha;
/// Draw one horizontal scanline (alphamap with lighting).
csDrawScanline csScan_32_scan_map_alpha;

/// Draw a perspective-incorrect flat shaded polygon scanline with Z-fill
csDrawPIScanline csScan_32_scan_pi_flat_zfil;
/// Draw a perspective-incorrect flat shaded polygon scanline
csDrawPIScanline csScan_32_scan_pi_flat_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline. Z fill only
csDrawPIScanline csScan_32_scan_pi_tex_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline
csDrawPIScanline csScan_32_scan_pi_tex_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with color keying. Z fill only
csDrawPIScanline csScan_32_scan_pi_tex_key_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with color keying
csDrawPIScanline csScan_32_scan_pi_tex_key_zuse;
/// Draw a perspective-incorrect flat shaded polygon scanline with table-driven effects with Z-fill
csDrawPIScanline csScan_32_scan_pi_flat_fx_zfil;
/// Draw a perspective-incorrect flat shaded polygon scanline with table-driven effects
csDrawPIScanline csScan_32_scan_pi_flat_fx_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_32_scan_pi_tex_fx_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_32_scan_pi_tex_fx_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects. Z fill only
csDrawPIScanline csScan_32_scan_pi_tex_fxkey_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with table-driven effects
csDrawPIScanline csScan_32_scan_pi_tex_fxkey_zuse;
/// Draw a flat-shaded alpha-mapped texture
csDrawPIScanline csScan_32_scan_pi_tex_alpha;

/// Draw a perspective-incorrect flat shaded polygon scanline with Gouraud and Z-fill
csDrawPIScanlineGouraud csScan_32_scan_pi_flat_gou_zfil;
/// Draw a perspective-incorrect flat shaded polygon scanline with Gouraud shading
csDrawPIScanlineGouraud csScan_32_scan_pi_flat_gou_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading.
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_gou_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading. Z fill only
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_gou_zuse;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading and color keying.
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goukey_zfil;
/// Draw a perspective-incorrect texture mapped polygon scanline with gouraud shading and color keying. Z fill only
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goukey_zuse;
/// Draw a perspective-incorrect flat shaded polygon scanline with Gouraud and Z-fill
csDrawPIScanlineGouraud csScan_32_scan_pi_flat_goufx_zfil;
/// Draw a perspective-incorrect flat shaded polygon scanline with Gouraud shading
csDrawPIScanlineGouraud csScan_32_scan_pi_flat_goufx_zuse;
/// Draw a perspective-incorrect polygon scanline with various effects Z fill only
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goufx_zfil;
/// Draw a perspective-incorrect polygon scanline with various effects
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goufx_zuse;
/// Draw a perspective-incorrect polygon scanline with various effects Z fill (colorkeying)
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goufxkey_zfil;
/// Draw a perspective-incorrect polygon scanline with various effects (colorkeying)
csDrawPIScanlineGouraud csScan_32_scan_pi_tex_goufxkey_zuse;

#if defined (DO_MMX) && defined (DO_NASM)
/// Draw a perspective-incorrect texture mapped polygon scanline using MMX
csDrawScanline csScan_32_mmx_scan_map_zfil;
/// Draw one horizontal scanline (no lighting) using MMX
csDrawScanline csScan_32_mmx_scan_tex_zfil;
#endif

} // extern "C"

#endif // __CS_SCAN_H__
