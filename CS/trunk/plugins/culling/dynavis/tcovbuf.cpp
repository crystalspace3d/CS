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
#include "ivaria/bugplug.h"
#include "qint.h"
#include "qsqrt.h"
#include "csgeom/box.h"
#include "csgeom/math3d.h"
#include "csgeom/csrect.h"
#include "csgeom/transfrm.h"
#include "tcovbuf.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/txtmgr.h"
#include "cssys/csprocessorcap.h"

//---------------------------------------------------------------------------

csTileCol csCoverageTile::coverage_cache[NUM_TILECOL];
csTileCol csCoverageTile::precalc_end_lines[NUM_TILEROW];
csTileCol csCoverageTile::precalc_start_lines[NUM_TILEROW];
bool csCoverageTile::precalc_init = false;

void csCoverageTile::MakePrecalcTables ()
{
  if (precalc_init) return;
  precalc_init = true;
  int i, j;
  for (i = 0 ; i < NUM_TILEROW ; i++)
  {
    precalc_start_lines[i].Empty ();
    for (j = 0 ; j <= i ; j++)
      precalc_start_lines[i].XorBit (j);
    precalc_end_lines[i].Empty ();
    for (j = i ; j < NUM_TILEROW ; j++)
      precalc_end_lines[i].XorBit (j);
  }
}

csLineOperation& csCoverageTile::AddOperation ()
{
  CS_ASSERT (num_operations <= max_operations);
  if (num_operations >= max_operations)
  {
    if (max_operations < 100)
      max_operations += max_operations;
    else
      max_operations += 100;
    csLineOperation* new_op = new csLineOperation [max_operations];
    if (num_operations > 0)
      memcpy (new_op, operations, sizeof (csLineOperation)*num_operations);
    delete[] operations;
    operations = new_op;
  }
  num_operations++;
  return operations[num_operations-1];
}

void csCoverageTile::PushLine (int x1, int y1, int x2, int y2, int dx)
{
  CS_ASSERT (x1 >= 0);
  CS_ASSERT (x1 < (NUM_TILECOL<<16));
  CS_ASSERT (x2 >= 0);
  CS_ASSERT (x2 < (NUM_TILECOL<<16));
  CS_ASSERT (y1 >= 0);
  CS_ASSERT (y1 < NUM_TILEROW);
  CS_ASSERT (y2 >= 0);
  CS_ASSERT (y2 < NUM_TILEROW);
  CS_ASSERT (x1+ABS (y2-y1)*dx >= 0);
  CS_ASSERT (x1+ABS (y2-y1)*dx < (NUM_TILECOL<<16));
  //@@@ VERY SUSPICIOUS! These asserts are triggerred in some cases!
  CS_ASSERT (x2-ABS (y1-y2)*dx >= 0);
  CS_ASSERT (x2-ABS (y1-y2)*dx < (NUM_TILECOL<<16));
  csLineOperation& op = AddOperation ();
  op.op = OP_LINE;
  op.x1 = x1;
  op.y1 = y1;
  op.x2 = x2;
  op.y2 = y2;
  op.dx = dx;
//printf ("        LINE %d,%d - %d,%d (dx=%d)\n", x1>>16, y1, x2>>16, y2, dx);
}

void csCoverageTile::PushVLine (int x, int y1, int y2)
{
  CS_ASSERT (x >= 0);
  CS_ASSERT (x < (NUM_TILECOL<<16));
  CS_ASSERT (y1 >= 0);
  CS_ASSERT (y1 < NUM_TILEROW);
  CS_ASSERT (y2 >= 0);
  CS_ASSERT (y2 < NUM_TILEROW);
  csLineOperation& op = AddOperation ();
  op.op = OP_VLINE;
  op.x1 = x;
  op.y1 = y1;
  op.y2 = y2;
//printf ("        VLINE %d    %d - %d\n", x>>16, y1, y2);
}

void csCoverageTile::PushFullVLine (int x)
{
  CS_ASSERT (x >= 0);
  CS_ASSERT (x < (NUM_TILECOL<<16));
  csLineOperation& op = AddOperation ();
  op.op = OP_FULLVLINE;
  op.x1 = x;
//printf ("        FULLVLINE %d\n", x>>16);
}

void csCoverageTile::PerformOperations ()
{
  int i;

  memset (coverage_cache, 0, sizeof (csTileCol)*NUM_TILECOL);

  // First draw all lines.
  for (i = 0 ; i < num_operations ; i++)
  {
    csLineOperation& op = operations[i];
    if (op.op == OP_FULLVLINE)
    {
      CS_ASSERT (op.x1 >= 0 && op.x1 <= (NUM_TILECOL<<16));
      coverage_cache[op.x1 >> 16].Invert ();
    }
    else if (op.op == OP_VLINE)
    {
      CS_ASSERT (op.x1 >= 0 && op.x1 <= (NUM_TILECOL<<16));
      CS_ASSERT (op.y1 >= 0);
      CS_ASSERT (op.y1 < NUM_TILEROW);
      CS_ASSERT (op.y2 >= 0);
      CS_ASSERT (op.y2 < NUM_TILEROW);
      int y1, y2;
      if (op.y1 < op.y2) { y1 = op.y1; y2 = op.y2; }
      else { y1 = op.y2; y2 = op.y1; }
      const csTileCol& start = precalc_start_lines[y2];
      const csTileCol& end = precalc_end_lines[y1];
      // Xor the line with the coverage cache. This happens in three stages:
      csTileCol& cc = coverage_cache[op.x1 >> 16];
      cc ^= start;
      cc ^= end;
      cc.Invert ();
    }
    else // OP_LINE
    {
      CS_ASSERT (op.x1 >= 0 && op.x1 <= (NUM_TILECOL<<16));
      CS_ASSERT (op.x2 >= 0 && op.x2 <= (NUM_TILECOL<<16));
      CS_ASSERT (op.y1 >= 0);
      CS_ASSERT (op.y1 < NUM_TILEROW);
      CS_ASSERT (op.y2 >= 0);
      CS_ASSERT (op.y2 < NUM_TILEROW);
      int x1, y1, x2, y2;
      if (op.y1 < op.y2) { x1 = op.x1; y1 = op.y1; x2 = op.x2; y2 = op.y2; }
      else { x1 = op.x2; y1 = op.y2; x2 = op.x1; y2 = op.y1; }
      int dy = y2-y1;
      int x = x1;
      int y = y1;
      int dx = op.dx;
      while (dy >= 0)
      {
	CS_ASSERT ((x>>16) >= 0);
	CS_ASSERT ((x>>16) < NUM_TILECOL);
	csTileCol& cc = coverage_cache[x >> 16];
	cc.XorBit (y);
        x += dx;
        y++;
        dy--;
      }
    }
  }
}

void csCoverageTile::FlushOperations ()
{
  PerformOperations ();
  // Clear all operations.
  num_operations = 0;
}

void csCoverageTile::PerformOperationsOnlyFValue (csTileCol& fvalue)
{
  int i;

  // Here we update the fvalue while ignoring the current contents
  // of the coverage buffer (we assume it is full).
  for (i = 0 ; i < num_operations ; i++)
  {
    csLineOperation& op = operations[i];
    if (op.op == OP_FULLVLINE)
    {
      // We have a full line (from top to bottom). In this case
      // we simply invert the fvalue.
      fvalue.Invert ();
    }
    else
    {
      // We can ignore the x value of the line here. So VLINE and
      // LINE are equivalent in this case.
      CS_ASSERT (op.y1 >= 0);
      CS_ASSERT (op.y1 < NUM_TILEROW);
      CS_ASSERT (op.y2 >= 0);
      CS_ASSERT (op.y2 < NUM_TILEROW);
      int y1, y2;
      if (op.y1 < op.y2) { y1 = op.y1; y2 = op.y2; }
      else { y1 = op.y2; y2 = op.y1; }
      const csTileCol& start = precalc_start_lines[y2];
      const csTileCol& end = precalc_end_lines[y1];
      // Xor the line with the fvalue. This happens in three stages:
      fvalue ^= start;
      fvalue ^= end;
      fvalue.Invert ();
    }
  }
}

void csCoverageTile::FlushOperationsOnlyFValue (csTileCol& fvalue)
{
  PerformOperationsOnlyFValue (fvalue);
  num_operations = 0;
}

void csCoverageTile::FlushForEmptyConstFValue (csTileCol& fvalue,
	float maxdepth)
{
  int i;
  MakeEmptyQuick ();

  // Special case. Tile is empty and there are no operations.
  // In this special case we don't have to compare with the current contents
  // of the tile since that is empty. We don't have to test for depth but just
  // update it.

  csTileCol* c = coverage;      
  // Since there are no operations in this tile we can simply set the fvalue
  // to all columns.
  for (i = 0 ; i < NUM_TILECOL ; i++)
  {
    *c++ = fvalue;
  }
  // Since the tile is empty we can set tile_full to true if fvalue is
  // full.
  tile_full = fvalue.IsFull ();

  // Now do the depth update. Here we will use fvalue to detect where
  // to update depth because we know every column is set to that.
  if (fvalue.CheckByte0 ())
    depth[0] = depth[1] = depth[2] = depth[3] = maxdepth;
  if (fvalue.CheckByte1 ())
    depth[4] = depth[5] = depth[6] = depth[7] = maxdepth;
  if (fvalue.CheckByte2 ())
    depth[8] = depth[9] = depth[10] = depth[11] = maxdepth;
  if (fvalue.CheckByte3 ())
    depth[12] = depth[13] = depth[14] = depth[15] = maxdepth;
#if NUM_TILEROW==64
  if (fvalue.CheckByte4 ())
    depth[16] = depth[17] = depth[18] = depth[19] = maxdepth;
  if (fvalue.CheckByte5 ())
    depth[20] = depth[21] = depth[22] = depth[23] = maxdepth;
  if (fvalue.CheckByte6 ())
    depth[24] = depth[25] = depth[26] = depth[27] = maxdepth;
  if (fvalue.CheckByte7 ())
    depth[28] = depth[29] = depth[30] = depth[31] = maxdepth;
#endif

  // Tile was empty before so we can simply set min and max depth
  // to the input depth.
  tile_max_depth = maxdepth;
  tile_min_depth = maxdepth;
  return;
}

void csCoverageTile::FlushForFullConstFValue (csTileCol&, float)
{
  // Special case. Tile is full and fvalue is constant. There is
  // nothing to do here.
}

void csCoverageTile::FlushNoDepthConstFValue (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  int i;

  // If our new depth is smaller than the minimum depth
  // of this tile then we can do a more optimal routine since
  // we don't have to check the depth buffer.
  // This version has a constant fvalue (no operations).

  // @@@ TODO: Check if we can improve depth for fully covered blocks!
  // Should be easy in this version.

  csTileCol* c = coverage;
  // 'fulltest' will be used to test if the resulting tile is full.
  // We will initialize it to full and then and it with every column. If
  // 'fulltest' is still full after the loop then we know the tile is full.
  csTileCol fulltest;
  fulltest.Full ();

  if (modified)
  {
    // If 'modified' is already true we avoid the test for modification.
    for (i = 0 ; i < NUM_TILECOL ; i++)
    {
      *c |= fvalue;
      fulltest &= *c;
      c++;
    }
  }
  else
  {
    // 'test' is a work variable used through the loop.
    csTileCol test;

    // 'mods' is used to test if our 'fvalue' mask modifies the
    // coverage buffer anywhere.
    csTileCol mods;
    mods.Empty ();

    for (i = 0 ; i < NUM_TILECOL ; i++)
    {
      test = fvalue;
      test.AndInverted (*c);
      mods |= test;
      *c |= fvalue;
      fulltest &= *c;
      c++;
    }
    if (!mods.IsEmpty ())
      modified = true;
  }

  // Check for full coverage.
  csTileCol test;
  test = fvalue;
  test.Invert ();
  bool recheck_depth = false;
  if (!test.CheckByte0 ())
  {
    if (maxdepth < depth[0]) { depth[0] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[1]) { depth[1] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[2]) { depth[2] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[3]) { depth[3] = maxdepth; recheck_depth = true; }
  }
  if (!test.CheckByte1 ())
  {
    if (maxdepth < depth[4]) { depth[4] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[5]) { depth[5] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[6]) { depth[6] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[7]) { depth[7] = maxdepth; recheck_depth = true; }
  }
  if (!test.CheckByte2 ())
  {
    if (maxdepth < depth[8]) { depth[8] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[9]) { depth[9] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[10]) { depth[10] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[11]) { depth[11] = maxdepth; recheck_depth = true; }
  }
  if (!test.CheckByte3 ())
  {
    if (maxdepth < depth[12]) { depth[12] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[13]) { depth[13] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[14]) { depth[14] = maxdepth; recheck_depth = true; }
    if (maxdepth < depth[15]) { depth[15] = maxdepth; recheck_depth = true; }
  }
  if (recheck_depth)
  {
    modified = true;
    tile_min_depth = depth[0];
    tile_max_depth = depth[0];
    for (i = 1 ; i < NUM_DEPTH ; i++)
      if (depth[i] < tile_min_depth) tile_min_depth = depth[i];
      else if (depth[i] > tile_max_depth) tile_max_depth = depth[i];
  }
  
  tile_full = fulltest.IsFull ();
}

void csCoverageTile::FlushGeneralConstFValue (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  int i;

  // The general case for no operations. It is possible we have to update the
  // depth buffer.

  // 'fulltest' will be used to test if the resulting tile is full.
  // We will initialize it to full and then and it with every column. If
  // 'fulltest' is still full after the loop then we know the tile is full.
  csTileCol fulltest;
  fulltest.Full ();

  csTileCol* c = coverage;      
  // For every 8 columns...
  for (i = 0 ; i < (NUM_TILECOL/8) ; i++)
  {
    // 'test' is a work variable used through the loop.
    csTileCol test;
    // 'mods' is used to test if our 'fvalue' mask modifies the
    // coverage buffer anywhere. Only where 'mods' is true do we have
    // to update depth later.
    csTileCol mods;
    mods.Empty ();
    // 'fullcover' is used to detect if we fully cover some 8x8 block.
    // In that case we can reduce max depth instead of increasing it
    // (potentially improving culling efficiency).
    csTileCol fullcover;
    fullcover.Full ();

    int j = 8;
    while (j > 0)
    {
      test = fvalue;
      test.AndInverted (*c);
      mods |= test;
      fullcover &= fvalue;
      *c |= fvalue;
      fulltest &= *c;
      c++;
      j--;
    }
    // @@@ check on fullcover even if mods is empty!!!

    // If 'mods' is not empty we test individual bytes of 'mods' to
    // see which depth values we have to update.
    if (!mods.IsEmpty ())
    {
      modified = true;
      fullcover.Invert ();
      float* ldepth = &depth[i];
      {
        float& d = ldepth[0];
        if (!fullcover.CheckByte0 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte0 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[4];
        if (!fullcover.CheckByte1 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte1 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[8];
        if (!fullcover.CheckByte2 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte2 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[12];
        if (!fullcover.CheckByte3 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte3 ())
	  if (maxdepth > d) d = maxdepth;
      }
#if NUM_TILEROW==64
      {
        float& d = ldepth[16];
        if (!fullcover.CheckByte4 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte4 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[20];
        if (!fullcover.CheckByte5 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte5 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[24];
        if (!fullcover.CheckByte6 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte6 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[28];
        if (!fullcover.CheckByte7 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte7 ())
	  if (maxdepth > d) d = maxdepth;
      }
#endif
    }
  }

  tile_full = fulltest.IsFull ();

  if (maxdepth < tile_min_depth || maxdepth > tile_max_depth)
  {
    tile_min_depth = depth[0];
    tile_max_depth = depth[0];
    for (i = 1 ; i < NUM_DEPTH ; i++)
      if (depth[i] < tile_min_depth) tile_min_depth = depth[i];
      else if (depth[i] > tile_max_depth) tile_max_depth = depth[i];
  }
}

void csCoverageTile::FlushForEmpty (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  int i;
  MakeEmptyQuick ();

  // Special case. Tile is empty.
  // In this special case we don't have to compare with the current contents
  // of the tile since that is empty. We don't have to test for depth but just
  // update it.
  FlushOperations ();

  // Now perform the XOR sweep and OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer. In the mean time the coverage_cache
  // buffer contents will be modified to be true wherever the
  // coverage_cache actually modified the coverage buffer.
  csTileCol* c = coverage;      
  csTileCol* cc = coverage_cache;

  // 'fulltest' will be used to test if the resulting tile is full.
  // We will initialize it to full and then and it with every column. If
  // 'fulltest' is still full after the loop then we know the tile is full.
  csTileCol fulltest;
  fulltest.Full ();

  // Now do the depth update. Here we will use the coverage (instead of
  // coverage_cache which is used in the general case) to see where we
  // need to update the depth buffer.
  for (i = 0 ; i < (NUM_TILECOL/8) ; i++)
  {
    float* ldepth = &depth[i];

    // 'mods' is used to test if our 'fvalue' mask modifies the
    // coverage buffer anywhere. Only where 'mods' is true do we have
    // to update depth later.
    csTileCol mods;
    mods.Empty ();

    int j = 8;
    while (j > 0)
    {
      fvalue ^= *cc;
      *c = fvalue;
      mods |= fvalue;
      fulltest &= fvalue;
      cc++;
      c++;
      j--;
    }

    if (!mods.IsEmpty ())
    {
      modified = true;
      if (mods.CheckByte0 ()) ldepth[0] = maxdepth;
      if (mods.CheckByte1 ()) ldepth[4] = maxdepth;
      if (mods.CheckByte2 ()) ldepth[8] = maxdepth;
      if (mods.CheckByte3 ()) ldepth[12] = maxdepth;
#if NUM_TILEROW==64
      if (mods.CheckByte4 ()) ldepth[16] = maxdepth;
      if (mods.CheckByte5 ()) ldepth[20] = maxdepth;
      if (mods.CheckByte6 ()) ldepth[24] = maxdepth;
      if (mods.CheckByte7 ()) ldepth[28] = maxdepth;
#endif
    }
  }

  tile_full = fulltest.IsFull ();

  tile_min_depth = maxdepth;
  tile_max_depth = maxdepth;
  return;
}

void csCoverageTile::FlushForFull (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  if (maxdepth >= tile_max_depth)
  {
    // There is no chance we can improve on depth buffer here.
    FlushOperationsOnlyFValue (fvalue);
    return;
  }

  // FlushForFull is called a lot (second most often after
  // FlushGeneral). So it makes sense to attempt to optimize more
  // by doing the coverage buffer run and updating min depth for all
  // fully covered blocks. However using FlushGeneral() is not good since
  // that is highly expensive so we need a careful balance between more
  // work and (potentially) better culling quality.

  FlushOperations ();
  int i;

  // The general case. It is possible we have to update the
  // depth buffer.

  // Now perform the XOR sweep and OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer. In the mean time the coverage_cache
  // buffer contents will be modified to be true wherever the
  // coverage_cache actually modified the coverage buffer.

  csTileCol* cc = coverage_cache;

  // Now do the depth update. Here we will use the coverage_cache
  // to see where we need to update the depth buffer. The coverage_cache
  // will now contain true wherever the coverage buffer was modified.

  // For every 8 columns...
  for (i = 0 ; i < (NUM_TILECOL/8) ; i++)
  {
    // 'fullcover' is used to detect if we fully cover some 8x8 block.
    // In that case we can reduce max depth instead of increasing it
    // (potentially improving culling efficiency).
    csTileCol fullcover;
    fullcover.Full ();

    int j = 8;
    while (j > 0)
    {
      fvalue ^= *cc;
      fullcover &= fvalue;
      cc++;
      j--;
    }
    // If 'fullcover' is not empty we test individual bytes to
    // see which depth values we have to update.
    if (!fullcover.IsEmpty ())
    {
      fullcover.Invert ();
      if (!fullcover.CheckByte0 ())
	if (maxdepth < depth[0]) { depth[0] = maxdepth; modified = true; }
      if (!fullcover.CheckByte1 ())
	if (maxdepth < depth[4]) { depth[4] = maxdepth; modified = true; }
      if (!fullcover.CheckByte2 ())
	if (maxdepth < depth[8]) { depth[8] = maxdepth; modified = true; }
      if (!fullcover.CheckByte3 ())
	if (maxdepth < depth[12]) { depth[12] = maxdepth; modified = true; }
#if NUM_TILEROW==64
      if (!fullcover.CheckByte4 ())
	if (maxdepth < depth[16]) { depth[16] = maxdepth; modified = true; }
      if (!fullcover.CheckByte5 ())
	if (maxdepth < depth[20]) { depth[20] = maxdepth; modified = true; }
      if (!fullcover.CheckByte6 ())
	if (maxdepth < depth[24]) { depth[24] = maxdepth; modified = true; }
      if (!fullcover.CheckByte7 ())
	if (maxdepth < depth[28]) { depth[28] = maxdepth; modified = true; }
#endif
    }
  }

  if (maxdepth < tile_min_depth || maxdepth > tile_max_depth)
  {
    tile_min_depth = depth[0];
    tile_max_depth = depth[0];
    for (i = 1 ; i < NUM_DEPTH ; i++)
      if (depth[i] < tile_min_depth) tile_min_depth = depth[i];
      else if (depth[i] > tile_max_depth) tile_max_depth = depth[i];
  }
}

void csCoverageTile::FlushNoDepth (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  FlushOperations ();
  int i;

  // @@@ TODO: Check if we can improve depth for fully covered blocks!

  // If our new depth is smaller than the minimum depth
  // of this tile then we can do a more optimal routine since
  // we don't have to check the depth buffer.

  // Now perform the XOR sweep and OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer.

  // 'fulltest' will be used to test if the resulting tile is full.
  // We will initialize it to full and then and it with every column. If
  // 'fulltest' is still full after the loop then we know the tile is full.
  csTileCol fulltest;
  fulltest.Full ();

  csTileCol* cc = coverage_cache;
  csTileCol* c = coverage;
  if (modified)
  {
    for (i = 0 ; i < NUM_TILECOL ; i++)
    {
      fvalue ^= *cc;
      *c |= fvalue;
      fulltest &= *c;
      cc++;
      c++;
    }
  }
  else
  {
    // 'test' is a work variable used through the loop.
    csTileCol test;
    // 'mods' is used to test if our 'fvalue' mask modifies the
    // coverage buffer anywhere. Only where 'mods' is true do we have
    // to update depth later.
    csTileCol mods;
    mods.Empty ();
    for (i = 0 ; i < NUM_TILECOL ; i++)
    {
      fvalue ^= *cc;
      test = fvalue;
      test.AndInverted (*c);
      mods |= test;
      *c |= fvalue;
      fulltest &= *c;
      cc++;
      c++;
    }
    if (!mods.IsEmpty ())
      modified = true;
  }

  tile_full = fulltest.IsFull ();
}

void csCoverageTile::FlushGeneral (csTileCol& fvalue, float maxdepth,
	bool& modified)
{
  FlushOperations ();
  int i;

  // The general case. It is possible we have to update the
  // depth buffer.

  // Now perform the XOR sweep and OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer. In the mean time the coverage_cache
  // buffer contents will be modified to be true wherever the
  // coverage_cache actually modified the coverage buffer.

  csTileCol* c = coverage;      
  csTileCol* cc = coverage_cache;

  // Now do the depth update. Here we will use the coverage_cache
  // to see where we need to update the depth buffer. The coverage_cache
  // will now contain true wherever the coverage buffer was modified.

  // 'fulltest' will be used to test if the resulting tile is full.
  // We will initialize it to full and then and it with every column. If
  // 'fulltest' is still full after the loop then we know the tile is full.
  csTileCol fulltest;
  fulltest.Full ();

  // For every 8 columns...
  for (i = 0 ; i < (NUM_TILECOL/8) ; i++)
  {
    // 'test' is a work variable used through the loop.
    csTileCol test;
    // 'mods' is used to test if our coverage cache modifies the
    // coverage buffer anywhere. Only where 'mods' is true do we have
    // to update depth later.
    csTileCol mods;
    mods.Empty ();
    // 'fullcover' is used to detect if we fully cover some 8x8 block.
    // In that case we can reduce max depth instead of increasing it
    // (potentially improving culling efficiency).
    csTileCol fullcover;
    fullcover.Full ();

    int j = 8;
    while (j > 0)
    {
      fvalue ^= *cc;
      test = fvalue;
      test.AndInverted (*c);
      mods |= test;
      fullcover &= fvalue;
      *c |= fvalue;
      fulltest &= *c;
      cc++;
      c++;
      j--;
    }
    // If 'mods' is not empty we test individual bytes of 'mods' to
    // see which depth values we have to update.
    // @@@ check on fullcover even if mods is empty!!!
    if (!mods.IsEmpty ())
    {
      modified = true;
      fullcover.Invert ();
      float* ldepth = &depth[i];
      {
        float& d = ldepth[0];
        if (!fullcover.CheckByte0 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte0 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[4];
        if (!fullcover.CheckByte1 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte1 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[8];
        if (!fullcover.CheckByte2 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte2 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[12];
        if (!fullcover.CheckByte3 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte3 ())
	  if (maxdepth > d) d = maxdepth;
      }
#if NUM_TILEROW==64
      {
        float& d = ldepth[16];
        if (!fullcover.CheckByte4 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte4 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[20];
        if (!fullcover.CheckByte5 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte5 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[24];
        if (!fullcover.CheckByte6 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte6 ())
	  if (maxdepth > d) d = maxdepth;
      }
      {
        float& d = ldepth[28];
        if (!fullcover.CheckByte7 ())
	{
	  if (maxdepth < d) d = maxdepth;
        }
        else if (mods.CheckByte7 ())
	  if (maxdepth > d) d = maxdepth;
      }
#endif
    }
  }

  tile_full = fulltest.IsFull ();

  tile_min_depth = depth[0];
  tile_max_depth = depth[0];
  for (i = 1 ; i < NUM_DEPTH ; i++)
    if (depth[i] < tile_min_depth) tile_min_depth = depth[i];
    else if (depth[i] > tile_max_depth) tile_max_depth = depth[i];
}

void csCoverageTile::Flush (csTileCol& fvalue, float maxdepth, bool& modified)
{
  if (num_operations == 0)
  {
    // If there are no operations in this tile then there are three
    // special cases we can handle here.
    if (fvalue.IsFull ())
    {
      // fvalue is full which means that the tile will be completely filled.
      int i;
      if (queue_tile_empty)
      {
        queue_tile_empty = false;
        tile_min_depth = INIT_MIN_DEPTH;
        tile_max_depth = 0;
        for (i = 0 ; i < NUM_DEPTH ; i++)
          depth[i] = maxdepth;
        tile_min_depth = maxdepth;
        tile_max_depth = maxdepth;
        tile_full = true;
        modified = true;
      }
      else if (tile_full)
      {
        if (maxdepth >= tile_max_depth)
	{
	  // Do nothing in this case.
	}
	else if (maxdepth <= tile_min_depth)
	{
          for (i = 0 ; i < NUM_DEPTH ; i++)
            depth[i] = maxdepth;
          tile_min_depth = maxdepth;
          tile_max_depth = maxdepth;
	  modified = true;
	}
	else
	{
          for (i = 0 ; i < NUM_DEPTH ; i++)
            if (maxdepth < depth[i])
              depth[i] = maxdepth;
          tile_max_depth = maxdepth;
	  modified = true;
	}
      }
      else
      {
        for (i = 0 ; i < NUM_DEPTH ; i++)
          if (maxdepth < depth[i])
            depth[i] = maxdepth;
        if (maxdepth < tile_min_depth)
          tile_min_depth = maxdepth;
        tile_max_depth = maxdepth;
        tile_full = true;
	modified = true;
      }
      return;
    }
    else if (fvalue.IsEmpty ())
    {
      // fvalue is empty which means that there is nothing to do here.
      return;
    }
    else
    {
      // We have a constant fvalue for the entire tile.
      if (queue_tile_empty)
      {
        FlushForEmptyConstFValue (fvalue, maxdepth);
	modified = true;
        return;
      }

      if (tile_full)
      {
        FlushForFullConstFValue (fvalue, maxdepth);
        return;
      }

      if (tile_min_depth < INIT_MIN_DEPTH_CMP && maxdepth <= tile_min_depth)
      {
        FlushNoDepthConstFValue (fvalue, maxdepth, modified);
        return;
      }

      FlushGeneralConstFValue (fvalue, maxdepth, modified);
      return;
    }
  }

  if (queue_tile_empty)
  {
    FlushForEmpty (fvalue, maxdepth, modified);
    return;
  }

  if (tile_full)
  {
    FlushForFull (fvalue, maxdepth, modified);
    return;
  }

  if (tile_min_depth < INIT_MIN_DEPTH_CMP && maxdepth <= tile_min_depth)
  {
    FlushNoDepth (fvalue, maxdepth, modified);
    return;
  }

  FlushGeneral (fvalue, maxdepth, modified);
}

bool csCoverageTile::TestDepthFlushGeneral (csTileCol& fvalue, float mindepth)
{
  // This test is not needed because already done by coverage
  // tester: if (mindepth <= tile_min_depth) return true;

  if (mindepth > tile_max_depth)
  {
    FlushOperationsOnlyFValue (fvalue);
    return false;
  }

  FlushOperations ();
  int i;

  // The general case.
  // Now perform the XOR sweep and test-OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer.
  csTileCol* cc = coverage_cache;

  // For every 8 columns...
  for (i = 0 ; i < (NUM_TILECOL/8) ; i++)
  {
    // 'mods' is used to test if our 'fvalue' mask modifies the
    // coverage buffer anywhere. Only where 'mods' is true do we have
    // to test depth later.
    csTileCol mods;
    mods.Empty ();

    int j = 8;
    while (j > 0)
    {
      fvalue ^= *cc;
      mods |= fvalue;
      cc++;
      j--;
    }

    float* ldepth = &depth[i];
    if (!mods.IsEmpty ())
    {
      if (mods.CheckByte0 () && mindepth <= ldepth[0]) return true;
      if (mods.CheckByte1 () && mindepth <= ldepth[4]) return true;
      if (mods.CheckByte2 () && mindepth <= ldepth[8]) return true;
      if (mods.CheckByte3 () && mindepth <= ldepth[12]) return true;
#if NUM_TILEROW==64
      if (mods.CheckByte4 () && mindepth <= ldepth[16]) return true;
      if (mods.CheckByte5 () && mindepth <= ldepth[20]) return true;
      if (mods.CheckByte6 () && mindepth <= ldepth[24]) return true;
      if (mods.CheckByte7 () && mindepth <= ldepth[28]) return true;
#endif
    }
  }
  return false;
}

bool csCoverageTile::TestDepthFlush (csTileCol& fvalue, float mindepth)
{
  if (num_operations == 0 && fvalue.IsEmpty ())
  {
    // If there are no operations and fvalue is empty then it is not
    // possible for the polygon to become visible in this tile.
    return false;
  }

  if (queue_tile_empty)
  {
    // There are operations or the fvalue is not empty so we know the
    // polygon is visible in this tile.
    return true;
  }

  return TestDepthFlushGeneral (fvalue, mindepth);
}

bool csCoverageTile::TestCoverageFlushForFull (csTileCol& fvalue,
	float mindepth, bool& do_depth_test)
{
  // First a quick depth test.
  if (mindepth <= tile_min_depth)
    return true;
  if (mindepth <= tile_max_depth)
    do_depth_test = true;

  PerformOperationsOnlyFValue (fvalue);
  return false;
}

bool csCoverageTile::TestCoverageFlushGeneral (csTileCol& fvalue,
	float mindepth, bool& do_depth_test)
{
  // First a quick depth test.
  if (mindepth <= tile_min_depth)
    return true;
  if (mindepth <= tile_max_depth)
    do_depth_test = true;

  PerformOperations ();
  int i;

  // The general case.
  // Now perform the XOR sweep and test-OR with main coverage buffer.
  // fvalue will be the modified from left to right and will be
  // OR-ed with the main buffer.
  csTileCol* cc = coverage_cache;
  csTileCol* c = coverage;      

  // 'test' is a work variable used through the loop.
  csTileCol test;
  for (i = 0 ; i < NUM_TILECOL ; i++)
  {
    fvalue ^= *cc;
    test = fvalue;
    test.AndInverted (*c);
    if (!test.IsEmpty ())
      return true;		// We modify coverage buffer so we are visible!
    cc++;
    c++;
  }
  return false;
}

bool csCoverageTile::TestCoverageFlush (csTileCol& fvalue, float mindepth,
	bool& do_depth_test)
{
  if (num_operations == 0)
  {
    if (fvalue.IsEmpty ())
    {
      // If there are no operations and fvalue is empty then it is not
      // possible for the polygon to become visible in this tile.
      return false;
    }
    else if (fvalue.IsFull ())
    {
      // If there are no operations and fvalue is full then visibility
      // depends fully on the 'full' state of the tile. In that case
      // we have to do a depth test later too.
      do_depth_test = true;
      return !tile_full;
    }
  }

  if (queue_tile_empty)
  {
    // There are operations or the fvalue is not empty so we know the
    // polygon is visible in this tile.
    return true;
  }

  if (tile_full)
  {
    return TestCoverageFlushForFull (fvalue, mindepth, do_depth_test);
  }

  return TestCoverageFlushGeneral (fvalue, mindepth, do_depth_test);
}

bool csCoverageTile::TestFullRect (float testdepth)
{
  if (tile_full)
  {
    // Tile is full so check for depth.
    return testdepth <= tile_max_depth;
  }
  else
  {
    // Tile is not full which means the rectangle is automatically
    // visible.
    return true;
  }
}

bool csCoverageTile::TestCoverageRect (int start, int end, float testdepth,
	bool& do_depth_test)
{
  // Tile is still empty.
  if (queue_tile_empty) return true;

  // If the depth of this rectangle is smaller than the minimum depth
  // of this tile then this rectangle is automatically visible.
  if (testdepth <= tile_min_depth) return true;

  int i;

  if (!tile_full)
  {
    // Tile is not full which means we test coverage first.
    csTileCol* c = coverage+start;
    for (i = start ; i <= end ; i++)
    {
      if (!c->IsFull ())
        return true;
      c++;
    }
  }

  // If the depth of this rectangle is greater than the maximum depth
  // of this tile then we don't have to do a depth test later.
  if (testdepth <= tile_max_depth)
    do_depth_test = true;
  return false;
}

bool csCoverageTile::TestCoverageRect (const csTileCol& vermask,
	int start, int end, float testdepth, bool& do_depth_test)
{
  // Tile is still empty.
  if (queue_tile_empty) return true;

  // If the depth of this rectangle is smaller than the minimum depth
  // of this tile then this rectangle is automatically visible.
  if (testdepth <= tile_min_depth) return true;

  int i;

  if (!tile_full)
  {
    // Tile is not full which means we first test coverage.
    csTileCol* c = coverage+start;
    for (i = start ; i <= end ; i++)
    {
      if (vermask.TestInvertedMask (*c))
        return true;
      c++;
    }
  }

  // If the depth of this rectangle is greater than the maximum depth
  // of this tile then we don't have to do a depth test later.
  if (testdepth <= tile_max_depth)
    do_depth_test = true;
  return false;
}

bool csCoverageTile::TestDepthRect (int start, int end, float testdepth)
{
  int i;

  // If the depth of this rectangle is greater than the maximum depth
  // of this tile then this rectangle cannot be visible.
  if (testdepth > tile_max_depth) return false;

  // Test depth where appropriate.
  for (i = (start>>3) ; i <= (end>>3) ; i++)
  {
    float* ld = &depth[i];
    // Here we create a mask which will be set to eight 1 bits wherever
    // the corresponding depth value is relevant (i.e. testdepth < the
    // depth in the depth value). Only where the mask is true do we have
    // to check the coverage_cache.
    if (testdepth < ld[0]) return true;
    if (testdepth < ld[4]) return true;
    if (testdepth < ld[8]) return true;
    if (testdepth < ld[12]) return true;
#if NUM_TILEROW==64
    if (testdepth < ld[16]) return true;
    if (testdepth < ld[20]) return true;
    if (testdepth < ld[24]) return true;
    if (testdepth < ld[28]) return true;
#endif
  }
  return false;
}

bool csCoverageTile::TestDepthRect (const csTileCol& vermask,
	int start, int end, float testdepth)
{
  int i;

  // If the depth of this rectangle is greater than the maximum depth
  // of this tile then this rectangle cannot be visible.
  if (testdepth > tile_max_depth) return false;

  int b0 = vermask.CheckByte0 ();
  int b1 = vermask.CheckByte1 ();
  int b2 = vermask.CheckByte2 ();
  int b3 = vermask.CheckByte3 ();
#if NUM_TILEROW==64
  int b4 = vermask.CheckByte4 ();
  int b5 = vermask.CheckByte5 ();
  int b6 = vermask.CheckByte6 ();
  int b7 = vermask.CheckByte7 ();
#endif

  // Test depth where appropriate.
  for (i = (start>>3) ; i <= (end>>3) ; i++)
  {
    float* ld = &depth[i];
    // Here we create a mask which will be set to eight 1 bits wherever
    // the corresponding depth value is relevant (i.e. testdepth < the
    // depth in the depth value). Only where the mask is true do we have
    // to check the coverage_cache.
    if (b0 && testdepth < ld[0]) return true;
    if (b1 && testdepth < ld[4]) return true;
    if (b2 && testdepth < ld[8]) return true;
    if (b3 && testdepth < ld[12]) return true;
#if NUM_TILEROW==64
    if (b4 && testdepth < ld[16]) return true;
    if (b5 && testdepth < ld[20]) return true;
    if (b6 && testdepth < ld[24]) return true;
    if (b7 && testdepth < ld[28]) return true;
#endif
  }
  return false;
}

bool csCoverageTile::TestPoint (int x, int y, float testdepth)
{
  CS_ASSERT (x >= 0 && x < NUM_TILECOL);
  CS_ASSERT (y >= 0 && y < NUM_TILEROW);

  // If tile is still empty we are visible.
  if (queue_tile_empty) return true;

  // First check for depth.
  int xd = x >> 3;	// Depth x coordinate.
  int yd = y >> 3;	// Depth y coordinate.
  float d = depth[(yd * (NUM_TILECOL/8)) + xd];
  if (testdepth <= d)
  {
    // Visible regardless of coverage.
    return true;
  }

  if (tile_full)
  {
    // If tile is full we know we are not visible because depth
    // has already been checked.
    return false;
  }

  const csTileCol& c = coverage[x];
  return !c.TestBit (y);
}

csPtr<iString> csCoverageTile::Debug_Dump ()
{
  scfString* rc = new scfString ();
  csString& str = rc->GetCsString ();

  csString ss;
  ss.Format ("full=%d queue_empty=%d\n",
  	tile_full, queue_tile_empty);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[0], depth[1], depth[2], depth[3]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[4], depth[5], depth[6], depth[7]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[8], depth[9], depth[10], depth[11]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[12], depth[13], depth[14], depth[15]);
  str.Append (ss);
#if NUM_DEPTH>=32
  ss.Format ("  d %g,%g,%g,%g\n", depth[16], depth[17], depth[18], depth[19]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[20], depth[21], depth[22], depth[23]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[24], depth[25], depth[26], depth[27]);
  str.Append (ss);
  ss.Format ("  d %g,%g,%g,%g\n", depth[28], depth[29], depth[30], depth[31]);
  str.Append (ss);
#endif
  int i;
  for (i = 0 ; i < num_operations ; i++)
  {
    ss.Format ("  op %d ", i);
    str.Append (ss);
    csLineOperation& op = operations[i];
    switch (op.op)
    {
      case OP_LINE: ss.Format ("LINE %d,%d - %d,%d   dx=%d\n",
      	op.x1>>16, op.y1, op.x2>>16, op.y2, op.dx);
	str.Append (ss);
	break;
      case OP_VLINE: ss.Format ("VLINE x=%d y1=%d y2=%d\n",
      	op.x1>>16, op.y1, op.y2);
	str.Append (ss);
        break;
      case OP_FULLVLINE: ss.Format ("FULLVLINE x=%d\n", op.x1>>16);
        str.Append (ss);
        break;
      default: str.Append ("???\n");
        break;
    }
  }
  str.Append ("          1    1    2    2    3  \n");
  str.Append ("0    5    0    5    0    5    0  \n");
  for (i = 0 ; i < NUM_TILEROW ; i++)
  {
    int j;
    for (j = 0 ; j < NUM_TILECOL ; j++)
    {
      const csTileCol& c = coverage[j];
      str.Append (c.TestBit (i) ? "#" : ".");
    }
    ss.Format (" %d\n", i);
    str.Append (ss);
  }

  return csPtr<iString> (rc);
}

csPtr<iString> csCoverageTile::Debug_Dump_Cache ()
{
  scfString* rc = new scfString ();
  csString& str = rc->GetCsString ();
  csString ss;

  int i;
  str.Append ("          1    1    2    2    3  \n");
  str.Append ("0    5    0    5    0    5    0  \n");
  for (i = 0 ; i < NUM_TILEROW ; i++)
  {
    int j;
    for (j = 0 ; j < NUM_TILECOL ; j++)
    {
      const csTileCol& c = coverage_cache[j];
      str.Append (c.TestBit (i) ? "#" : ".");
    }
    ss.Format (" %d\n", i);
    str.Append (ss);
  }

  return csPtr<iString> (rc);
}

//---------------------------------------------------------------------------

SCF_IMPLEMENT_IBASE (csTiledCoverageBuffer)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iDebugHelper)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csTiledCoverageBuffer::DebugHelper)
  SCF_IMPLEMENTS_INTERFACE(iDebugHelper)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csTiledCoverageBuffer::csTiledCoverageBuffer (int w, int h)
{
  SCF_CONSTRUCT_IBASE (0);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiDebugHelper);

  tiles = 0;
  dirty_left = 0;
  dirty_right = 0;
  bugplug = 0;

  Setup (w, h);
}

csTiledCoverageBuffer::~csTiledCoverageBuffer ()
{
  delete[] tiles;
  delete[] dirty_left;
  delete[] dirty_right;
  SCF_DESTRUCT_EMBEDDED_IBASE (scfiDebugHelper);
  SCF_DESTRUCT_IBASE ();
}

void csTiledCoverageBuffer::Setup (int w, int h)
{
  delete[] tiles;
  delete[] dirty_left;
  delete[] dirty_right;

  width = w;
  height = h;
  num_tile_rows = (h+(NUM_TILEROW-1))/NUM_TILEROW;
  height_64 = num_tile_rows * NUM_TILEROW;
  CS_ASSERT (height_64 >= height);

  width_po2 = 1;
  w_shift = 0;
  while (width_po2 < width)
  {
    width_po2 <<= 1;
    w_shift++;
  }
  w_shift -= SHIFT_TILECOL;
  CS_ASSERT (w_shift >= 0);

  num_tiles = (width_po2 / NUM_TILECOL) * num_tile_rows;

  tiles = new csCoverageTile[num_tiles];
  dirty_left = new int[num_tile_rows];
  dirty_right = new int[num_tile_rows];
}

void csTiledCoverageBuffer::Initialize ()
{
  int i;
  for (i = 0 ; i < num_tiles ; i++)
  {
    tiles[i].MarkEmpty ();
    tiles[i].ClearOperations ();
  }
}

void csTiledCoverageBuffer::DrawLine (int x1, int y1, int x2, int y2,
	int yfurther)
{
  y2 += yfurther;

  if (y2 <= 0 || y1 >= height)
  {
    //------
    // Totally outside screen vertically.
    //------
    return;
  }

  if (x1 <= 0 && x2 <= 0)
  {
    //------
    // Totally on the left side. Just clamp.
    //------

    // First we need to clip vertically. This is easy to do
    // in this particular case since x=0 all the time.
    if (y1 < 0) y1 = 0;
    if (y2 >= height) y2 = height-1;

    // First calculate tile coordinates of x1,y1 and x2,y2.
    int tile_y1 = y1 >> SHIFT_TILEROW;
    int tile_y2 = (y2-1) >> SHIFT_TILEROW;
    csCoverageTile* tile = GetTile (0, tile_y1);

    if (tile_y1 == tile_y2)
    {
      //------
      // All is contained in one tile.
      //------
      tile->PushVLine (0, y1 & (NUM_TILEROW-1), (y2-1) & (NUM_TILEROW-1));
      MarkTileDirty (0, tile_y1);
    }
    else
    {
      //------
      // Multiple tiles. First do first tile, then intermediate tiles,
      // and finally the last tile.
      //------
      tile->PushVLine (0, y1 & (NUM_TILEROW-1), (NUM_TILEROW-1));
      MarkTileDirty (0, tile_y1);
      int t;
      for (t = tile_y1+1 ; t < tile_y2 ; t++)
      {
        tile += width_po2 >> SHIFT_TILECOL;
        tile->PushFullVLine (0);
	MarkTileDirty (0, t);
      }
      tile += width_po2 >> SHIFT_TILECOL;
      tile->PushVLine (0, 0, (y2-1) & (NUM_TILEROW-1));
      MarkTileDirty (0, tile_y2);
    }
    return;
  }
  else if (x1 >= width && x2 >= width)
  {
    //------
    // Lines on the far right can just be dropped since they
    // will have no effect on the coverage buffer.
    // However we must mark the right most tiles as dirty.
    //------

    // First we need to clip vertically for marking the vertical
    // row of tiles right at the end of the screen.
    if (y1 < 0) y1 = 0;
    if (y2 >= height) y2 = height-1;

    // First calculate tile coordinates of x1,y1 and x2,y2.
    int tile_y1 = y1 >> SHIFT_TILEROW;
    int tile_y2 = (y2-1) >> SHIFT_TILEROW;
    int t;
    for (t = tile_y1 ; t <= tile_y2 ; t++)
      MarkTileDirty (width_po2 >> SHIFT_TILECOL, t);
    return;
  }
  else if (x1 == x2)
  {
    //------
    // If line is fully vertical we also have a special case that
    // is easier to resolve.
    //------
    // First we need to clip vertically. This is easy to do
    // in this particular case since x=0 all the time.
    if (y1 < 0) y1 = 0;
    if (y2 >= height) y2 = height-1;

    // First calculate tile coordinates of x1,y1 and x2,y2.
    int tile_x = x1 >> SHIFT_TILECOL;	// tile_x1 == tile_x2
    int tile_y1 = y1 >> SHIFT_TILEROW;
    int tile_y2 = (y2-1) >> SHIFT_TILEROW;
    x1 &= (NUM_TILECOL-1);
    x1 <<= 16;

    csCoverageTile* tile = GetTile (tile_x, tile_y1);
    if (tile_y1 == tile_y2)
    {
      //------
      // All is contained in one tile.
      //------
      tile->PushVLine (x1, y1 & (NUM_TILEROW-1), (y2-1) & (NUM_TILEROW-1));
      MarkTileDirty (tile_x, tile_y1);
    }
    else
    {
      //------
      // Multiple tiles. First do first tile, then intermediate tiles,
      // and finally the last tile.
      //------
      tile->PushVLine (x1, y1 & (NUM_TILEROW-1), (NUM_TILEROW-1));
      MarkTileDirty (tile_x, tile_y1);
      int t;
      for (t = tile_y1+1 ; t < tile_y2 ; t++)
      {
        tile += width_po2 >> SHIFT_TILECOL;
	tile->PushFullVLine (x1);
	MarkTileDirty (tile_x, t);
      }
      tile += width_po2 >> SHIFT_TILECOL;
      tile->PushVLine (x1, 0, (y2-1) & (NUM_TILEROW-1));
      MarkTileDirty (tile_x, tile_y2);
    }
    return;
  }

  //------
  // We don't have any of the trivial vertical cases.
  // So we must clip first.
  //------
  int old_x1 = x1;
  int old_x2 = x2;
  int old_y1 = y1;
  int old_y2 = y2;
  csRect r (0, 0, width-1, height-1-yfurther);
  y2 -= yfurther;
  bool outside = !r.ClipLineSafe (x1, y1, x2, y2);

  // @@@ Is the below good?
  if (!outside && y1 == y2)
    return;  // Return if clipping results in one pixel.
  y2 += yfurther;

  if (outside)
  {
    // We know the line is fully outside. Now we have to discover if the
    // line is on the left or the right of the screen.
    // First restore old coordinates because ClipLineSafe may have
    // partially modified them.
    bool right_side;
    x1 = old_x1;
    x2 = old_x2;
    y1 = old_y1;
    y2 = old_y2;
    if (x1 < width && x2 < width)
    {
      // We're sure the line is fully on the left side.
      right_side = false;
    }
    else if (x1 >= 0 && x2 >= 0)
    {
      // We're sure the line is fully on the right side.
      right_side = true;
    }
    else
    {
      // Most general case. We have to check a bit better.
      // We will check the x value where y=0 unless y=0 is outside
      // the line vertically in which case we will use y=y1.
      int y = 0;
      if (y1 > y) y = y1;
      int x = QInt (x1 + float (y-y1) * float (x2 - x1)
      		/ float (y2-y1));
      right_side = (x > 0);
    }

    // Make sure the y coordinates are clipped.
    if (y1 < 0) y1 = 0;
    if (y2 >= height) y2 = height-1;

    // First calculate tile coordinates.
    int tile_y1 = y1 >> SHIFT_TILEROW;
    int tile_y2 = (y2-1) >> SHIFT_TILEROW;

    if (right_side)
    {
      //------
      // Lines on the far right can just be dropped since they
      // will have no effect on the coverage buffer.
      // However we must mark the right-most tiles as dirty.
      //------
      int t;
      for (t = tile_y1 ; t <= tile_y2 ; t++)
        MarkTileDirty (width_po2 >> SHIFT_TILECOL, t);
      return;
    }
    else
    {
      //------
      // Totally on the left side. Just clamp.
      //------
      csCoverageTile* tile = GetTile (0, tile_y1);
      if (tile_y1 == tile_y2)
      {
        //------
        // All is contained in one tile.
        //------
        tile->PushVLine (0, y1 & (NUM_TILEROW-1), (y2-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
      }
      else
      {
        //------
        // Multiple tiles. First do first tile, then intermediate tiles,
        // and finally the last tile.
        //------
        tile->PushVLine (0, y1 & (NUM_TILEROW-1), (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
        int t;
        for (t = tile_y1+1 ; t < tile_y2 ; t++)
        {
          tile += width_po2 >> SHIFT_TILECOL;
          tile->PushFullVLine (0);
	  MarkTileDirty (0, t);
        }
        tile += width_po2 >> SHIFT_TILECOL;
        tile->PushVLine (0, 0, (y2-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y2);
      }
      return;
    }
  }

  //------
  // We know that at least part of the line is on screen.
  // We cannot ignore the part of the line that has been clipped away but 
  // is still between the top and bottom of the screen. The reason is that
  // on the left side this must be clamped to 0 and on the right side we
  // need to mark the right-most tiles as dirty. We will handle this
  // here before we draw the remainder of the line.
  //------

  // First clip the old y coordinates to the screen vertically.
  if (old_y1 < 0) old_y1 = 0;
  if (old_y2 >= height) old_y2 = height-1;

  if (old_y1 < y1)
  {
    if (x1 <= 0)
    {
      // We have an initial part that needs to be clamped.
      int tile_y1 = old_y1 >> SHIFT_TILEROW;
      int tile_y2 = (y1-1) >> SHIFT_TILEROW;
      csCoverageTile* tile = GetTile (0, tile_y1);
      if (tile_y1 == tile_y2)
      {
        tile->PushVLine(0, old_y1 & (NUM_TILEROW-1), (y1-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
      }
      else
      {
        tile->PushVLine (0, old_y1 & (NUM_TILEROW-1), (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
        int t;
        for (t = tile_y1+1 ; t < tile_y2 ; t++)
        {
          tile += width_po2 >> SHIFT_TILECOL;
	  tile->PushFullVLine (0);
	  MarkTileDirty (0, t);
        }
        tile += width_po2 >> SHIFT_TILECOL;
        tile->PushVLine (0, 0, (y1-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y2);
      }
    }
    else if (x1 >= width-1)
    {
      // We have an initial part for which we need to mark tiles dirty.
      int tile_y1 = old_y1 >> SHIFT_TILEROW;
      int tile_y2 = (y1-1) >> SHIFT_TILEROW;
      int t;
      for (t = tile_y1 ; t <= tile_y2 ; t++)
        MarkTileDirty (width_po2 >> SHIFT_TILECOL, t);
    }
  }
  if (old_y2 > y2)
  {
    if (x2 <= 0)
    {
      // We have a final part that needs to be clamped.
      int tile_y1 = y2 >> SHIFT_TILEROW;
      int tile_y2 = (old_y2-1) >> SHIFT_TILEROW;
      csCoverageTile* tile = GetTile (0, tile_y1);
      if (tile_y1 == tile_y2)
      {
        tile->PushVLine(0, y2 & (NUM_TILEROW-1), (old_y2-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
      }
      else
      {
        tile->PushVLine (0, y2 & (NUM_TILEROW-1), (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y1);
        int t;
        for (t = tile_y1+1 ; t < tile_y2 ; t++)
        {
          tile += width_po2 >> SHIFT_TILECOL;
	  tile->PushFullVLine (0);
	  MarkTileDirty (0, t);
        }
        tile += width_po2 >> SHIFT_TILECOL;
        tile->PushVLine (0, 0, (old_y2-1) & (NUM_TILEROW-1));
        MarkTileDirty (0, tile_y2);
      }
    }
    else if (x2 >= width-1)
    {
      // We have a final part for which we need to mark tiles dirty.
      int tile_y1 = y2 >> SHIFT_TILEROW;
      int tile_y2 = (old_y2-1) >> SHIFT_TILEROW;
      int t;
      for (t = tile_y1 ; t <= tile_y2 ; t++)
        MarkTileDirty (width_po2 >> SHIFT_TILECOL, t);
    }
  }

  //------
  // First calculate tile coordinates of x1,y1 and x2,y2.
  // Now we know that this line segment is fully on screen.
  //------
  int tile_x1 = x1 >> SHIFT_TILECOL;
  int tile_y1 = y1 >> SHIFT_TILEROW;
  int tile_x2 = x2 >> SHIFT_TILECOL;
  int tile_y2 = (y2-1) >> SHIFT_TILEROW;

# define xmask ((NUM_TILECOL<<16)-1)
# define ymask ((NUM_TILEROW<<16)-1)

  if (tile_x1 == tile_x2 && tile_y1 == tile_y2)
  {
    //------
    // Easy case. The line segment is fully inside one tile.
    //------
    int dy = y2-y1;
    int dx = ((x2-x1)<<16) / (dy-yfurther);
    csCoverageTile* tile = GetTile (tile_x1, tile_y1);
// @@@@@@@ DOUBLE CHECK THE THING BELOW TO SEE IF IT IS REALLY OK!!!
    tile->PushLine ((x1 & (NUM_TILECOL-1)) << 16, y1 & (NUM_TILEROW-1),
    	((x2 & (NUM_TILECOL-1)) << 16) -(yfurther ? 0 : dx),
    	(y2-1) & (NUM_TILEROW-1), dx);
    MarkTileDirty (tile_x1, tile_y1);
    return;
  }
  else if (tile_x1 == tile_x2)
  {
    //------
    // Line is nearly vertical. This means we will stay in the same
    // column of tiles.
    //------
    int dy = y2-y1;
    int dx = ((x2-x1)<<16) / (dy-yfurther);
    x1 <<= 16;
    x2 <<= 16;
    int x = x1 + dx * ((NUM_TILEROW-1) - (y1 & (NUM_TILEROW-1)));
    csCoverageTile* tile = GetTile (tile_x1, tile_y1);
    tile->PushLine (x1 & xmask, y1 & (NUM_TILEROW-1),
    	x & xmask, (NUM_TILEROW-1), dx);
    MarkTileDirty (tile_x1, tile_y1);
    x += dx;
    int t;
    for (t = tile_y1+1 ; t < tile_y2 ; t++)
    {
      tile += width_po2 >> SHIFT_TILECOL;
      int xt = x + (dx << SHIFT_TILEROW) - dx;
      tile->PushLine (x & xmask, 0, xt & xmask, NUM_TILEROW-1, dx);
      MarkTileDirty (tile_x1, t);
      x = xt+dx;
    }
    tile += width_po2 >> SHIFT_TILECOL;
    tile->PushLine (x & xmask, 0, (x2 & xmask) - dx,
    	(y2-1) & (NUM_TILEROW-1), dx);
    MarkTileDirty (tile_x1, tile_y2);
    return;
  }

  //------
  // This is the most general case.
  // @@@ The loops below can be done more efficiently.
  //------
  int dy = y2-y1;
  int dx = ((x2-x1)<<16) / (dy-yfurther);

  if (tile_y1 == tile_y2)
  {
    //------
    // Line is nearly horizontal. This means we will stay in the same
    // row of tiles.
    //------
    MarkTileDirty (tile_x1, tile_y1);
    MarkTileDirty (tile_x2, tile_y1);
    // Calculate the slope of the line.
    int x = x1<<16;
    int y = y1;

    //------
    // Here is the remainder of the line until we go out screen again.
    //------
    bool need_to_finish = dy > 0;
    int last_x = x;
    int last_y = y;
    int cur_tile_x = tile_x1;
    csCoverageTile* stile = GetTile (tile_x1, tile_y1);
    while (dy > 0)
    {
      int tile_x = x >> (16+SHIFT_TILECOL);
      if (cur_tile_x != tile_x)
      {
        csCoverageTile* tile = stile + (cur_tile_x-tile_x1);
        tile->PushLine (last_x & xmask, last_y & (NUM_TILEROW-1),
		(x-dx) & xmask, (y-1) & (NUM_TILEROW-1), dx);
        cur_tile_x = tile_x;
        last_x = x;
        last_y = y;
      }

      x += dx;
      y++;
      dy--;
    }

    if (need_to_finish)
    {
      csCoverageTile* tile = stile + (cur_tile_x-tile_x1);
      tile->PushLine (last_x & xmask, last_y & (NUM_TILEROW-1), (x-dx) & xmask,
    	  (y-1) & (NUM_TILEROW-1), dx);
    }
    return;
  }

  // Calculate the slope of the line.
  int x = x1<<16;
  int y = y1;

  //------
  // Here is the remainder of the line until we go out screen again.
  //------
  bool need_to_finish = dy > 0;
  int last_x = x;
  int last_y = y;
  int cur_tile_x = tile_x1;
  int cur_tile_y = tile_y1;
  while (dy > 0)
  {
    int tile_x = x >> (16+SHIFT_TILECOL);
    int tile_y = y >> SHIFT_TILEROW;
    if (cur_tile_x != tile_x || cur_tile_y != tile_y)
    {
      csCoverageTile* tile = GetTile (cur_tile_x, cur_tile_y);
      tile->PushLine (last_x & xmask, last_y & (NUM_TILEROW-1), (x-dx) & xmask,
      	(y-1) & (NUM_TILEROW-1), dx);
      MarkTileDirty (cur_tile_x, cur_tile_y);
      cur_tile_x = tile_x;
      cur_tile_y = tile_y;
      last_x = x;
      last_y = y;
    }

    x += dx;
    y++;
    dy--;
  }

  if (need_to_finish)
  {
    csCoverageTile* tile = GetTile (cur_tile_x, cur_tile_y);
    tile->PushLine (last_x & xmask, last_y & (NUM_TILEROW-1), (x-dx) & xmask,
    	(y-1) & (NUM_TILEROW-1), dx);
    MarkTileDirty (cur_tile_x, cur_tile_y);
  }
}

bool csTiledCoverageBuffer::DrawPolygon (csVector2* verts, int num_verts,
	csBox2Int& bbox)
{
  int i, j;

  //---------
  // First we copy the vertices to xa/ya. In the mean time
  // we convert to integer and also search for the top vertex (lowest
  // y coordinate) and bottom vertex.
  //@@@ TODO: pre-shift x with 16
  //---------
  int xa[128], ya[128];
  xa[0] = QRound (verts[0].x);
  ya[0] = QRound (verts[0].y);
  bbox.minx = bbox.maxx = xa[0];
  bbox.miny = bbox.maxy = ya[0];
  for (i = 1 ; i < num_verts ; i++)
  {
    xa[i] = QRound (verts[i].x);
    ya[i] = QRound (verts[i].y);

    if (xa[i] < bbox.minx) bbox.minx = xa[i];
    else if (xa[i] > bbox.maxx) bbox.maxx = xa[i];

    if (ya[i] < bbox.miny)
      bbox.miny = ya[i];
    else if (ya[i] > bbox.maxy)
      bbox.maxy = ya[i];
  }

  if (bbox.maxx <= 0) return false;
  if (bbox.maxy <= 0) return false;
  if (bbox.minx >= width) return false;
  if (bbox.miny >= height) return false;

  //---------
  // First initialize dirty_left and dirty_right for every row.
  //---------
  for (i = 0 ; i < num_tile_rows ; i++)
  {
    dirty_left[i] = 1000;
    dirty_right[i] = -1;
  }

  //---------
  // Draw all lines.
  //---------
  j = num_verts-1;
  for (i = 0 ; i < num_verts ; i++)
  {
    if (ya[i] != ya[j])
    {
      int xa1, xa2, ya1, ya2;
      if (ya[i] < ya[j])
      {
	xa1 = xa[i];
        xa2 = xa[j];
	ya1 = ya[i];
	ya2 = ya[j];
      }
      else
      {
	xa1 = xa[j];
        xa2 = xa[i];
	ya1 = ya[j];
	ya2 = ya[i];
      }
      DrawLine (xa1, ya1, xa2, ya2, ya2 == bbox.maxy ? 1 : 0);
    }
    j = i;
  }

  return true;
}

// Version to cope with z <= 0. This is wrong but it in the places where
// it is used below the result is acceptable because it generates a
// conservative result (i.e. a box or outline that is bigger then reality).
static void PerspectiveWrong (const csVector3& v, csVector2& p, float fov,
    	float sx, float sy)
{
  float iz = fov / .2;
  p.x = v.x * iz + sx;
  p.y = v.y * iz + sy;
}

static void Perspective (const csVector3& v, csVector2& p,
	float fov, float sx, float sy)
{
  float iz = fov / v.z;
  p.x = v.x * iz + sx;
  p.y = v.y * iz + sy;
}

bool csTiledCoverageBuffer::DrawOutline (const csReversibleTransform& trans,
	float fov, float sx, float sy,
	csVector3* verts, int num_verts,
	bool* used_verts,
	int* edges, int num_edges,
	csBox2Int& bbox, float& max_depth,
	bool splat_outline)
{
  int i;

  static int* xa = 0, * ya = 0;
  static csVector3* camv = 0;
  static int max_tr_verts = 0;
  if (num_verts > max_tr_verts)
  {
    //@@@ MEMORY LEAK!!!
    delete[] xa;
    delete[] ya;
    delete[] camv;
    max_tr_verts = num_verts+20;
    xa = new int[max_tr_verts];
    ya = new int[max_tr_verts];
    camv = new csVector3[max_tr_verts];
  }

  //---------
  // First transform all vertices.
  // Then we copy the vertices to xa/ya. In the mean time
  // we convert to integer and also search for the top vertex (lowest
  // y coordinate) and bottom vertex.
  //@@@ TODO: pre-shift x with 16
  //---------
  max_depth = -1.0;
  const csMatrix3& trans_mat = trans.GetO2T ();
  const csVector3& trans_vec = trans.GetO2TTranslation ();

  bbox.minx = 1000000;
  bbox.maxx = -1000000;
  bbox.miny = 1000000;
  bbox.maxy = -1000000;
  csVector2 tr_vert;

  bool need_splatting = false;
  for (i = 0 ; i < num_verts ; i++)
  {
    // Normally we would calculate:
    //   csVector3 camv = trans.Other2This (verts[i]);
    // But since we often only need the z of the transformed vertex
    // we only calculate z and calculate x,y later if needed.
    csVector3 v = verts[i] - trans_vec;
    camv[i].z = trans_mat.m31*v.x + trans_mat.m32*v.y + trans_mat.m33*v.z;

    if (camv[i].z > max_depth) max_depth = camv[i].z;
    if (used_verts[i])
    {
      camv[i].x = trans_mat.m11*v.x + trans_mat.m12*v.y + trans_mat.m13*v.z;
      camv[i].y = trans_mat.m21*v.x + trans_mat.m22*v.y + trans_mat.m23*v.z;

      // @@@ Note: originally 0.1 was used here. However this could cause
      // very large coordinates to be generated and our coverage line drawer
      // cannot currently cope with that. We need to improve that line
      // drawer considerably.
      if (camv[i].z <= 0.2)
      {
        if (!splat_outline) return false;
	need_splatting = true;
        PerspectiveWrong (camv[i], tr_vert, fov, sx, sy);
      }
      else
      {
        Perspective (camv[i], tr_vert, fov, sx, sy);
      }

      xa[i] = QRound (tr_vert.x);
      ya[i] = QRound (tr_vert.y);

      if (xa[i] < bbox.minx) bbox.minx = xa[i];
      if (xa[i] > bbox.maxx) bbox.maxx = xa[i];
      if (ya[i] < bbox.miny) bbox.miny = ya[i];
      if (ya[i] > bbox.maxy) bbox.maxy = ya[i];
    }
  }

  if (bbox.maxx <= 0) return false;
  if (bbox.maxy <= 0) return false;
  if (bbox.minx >= width) return false;
  if (bbox.miny >= height) return false;

  //---------
  // First initialize dirty_left and dirty_right for every row.
  //---------
  for (i = 0 ; i < num_tile_rows ; i++)
  {
    dirty_left[i] = 1000;
    dirty_right[i] = -1;
  }

#undef COV_DRAW_LINE
#define COV_DRAW_LINE(a_xa1,a_ya1,a_xa2,a_ya2) { \
      int ya1 = a_ya1; \
      int ya2 = a_ya2; \
      if (ya1 != ya2) \
      { \
        int xa1, xa2; \
        if (ya1 < ya2) \
        { \
	  xa1 = a_xa1; \
          xa2 = a_xa2; \
        } \
        else \
        { \
          int y = ya1; \
	  ya1 = ya2; \
	  ya2 = y; \
	  xa1 = a_xa2; \
          xa2 = a_xa1; \
        } \
        DrawLine (xa1, ya1, xa2, ya2, 0); \
      } }

  //---------
  // Draw all edges.
  //---------
  if (need_splatting)
    for (i = 0 ; i < num_edges ; i++)
    {
      int vt1 = *edges++;
      int vt2 = *edges++;
      float z1 = camv[vt1].z;
      float z2 = camv[vt2].z;
      if ((z1 <= .200001 && z2 > .200001) ||
          (z1 > .200001 && z2 <= .200001))
      {
        csVector3 isect;
        csIntersect3::ZPlane (0.2f, camv[vt1], camv[vt2], isect);
        PerspectiveWrong (isect, tr_vert, fov, sx, sy);
        int isect_xa = QRound (tr_vert.x);
        int isect_ya = QRound (tr_vert.y);
        COV_DRAW_LINE (xa[vt1], ya[vt1], isect_xa, isect_ya);
        COV_DRAW_LINE (isect_xa, isect_ya, xa[vt2], ya[vt2]);
      }
      else // Both < .2 and or > .2
      {
        COV_DRAW_LINE (xa[vt1], ya[vt1], xa[vt2], ya[vt2]);
      }
    }
  else
    for (i = 0 ; i < num_edges ; i++)
    {
      int vt1 = *edges++;
      int vt2 = *edges++;
      COV_DRAW_LINE (xa[vt1], ya[vt1], xa[vt2], ya[vt2]);
    }
#undef COV_DRAW_LINE

  return true;
}

bool csTiledCoverageBuffer::TestPolygon (csVector2* verts, int num_verts,
	float min_depth)
{
  csBox2Int bbox;
  if (!DrawPolygon (verts, num_verts, bbox))
    return false;

  int tx, ty;
  int startrow, endrow;
  startrow = bbox.miny >> SHIFT_TILEROW;
  if (startrow < 0) startrow = 0;
  endrow = bbox.maxy >> SHIFT_TILEROW;
  if (endrow >= num_tile_rows) endrow = num_tile_rows-1;

  bool rc = false;
  bool do_depth_test = false;
  for (ty = startrow ; ty <= endrow ; ty++)
  {
    csTileCol fvalue;
    fvalue.Empty ();
    csCoverageTile* tile = GetTile (dirty_left[ty], ty);
    int dr = dirty_right[ty];
    if (dr >= (width_po2 >> SHIFT_TILECOL))
      dr = (width_po2 >> SHIFT_TILECOL)-1;
    for (tx = dirty_left[ty] ; tx <= dr ; tx++)
    {
      rc = tile->TestCoverageFlush (fvalue, min_depth, do_depth_test);
      if (rc) break;
      tile++;
    }
    if (rc) break;
  }
  if (rc || do_depth_test == false)
  {
    // Already visible or we don't need a depth test.
    // We have to clear the operations now.
    for (ty = startrow ; ty <= endrow ; ty++)
    {
      csCoverageTile* tile = GetTile (dirty_left[ty], ty);
      int dr = dirty_right[ty];
      if (dr >= (width_po2 >> SHIFT_TILECOL))
        dr = (width_po2 >> SHIFT_TILECOL)-1;
      for (tx = dirty_left[ty] ; tx <= dr ; tx++)
      {
        tile->ClearOperations ();
        tile++;
      }
    }
    return rc;
  }

  // We don't know visibility. Try to test depth.
  rc = false;
  for (ty = startrow ; ty <= endrow ; ty++)
  {
    csTileCol fvalue;
    fvalue.Empty ();
    csCoverageTile* tile = GetTile (dirty_left[ty], ty);
    int dr = dirty_right[ty];
    if (dr >= (width_po2 >> SHIFT_TILECOL))
      dr = (width_po2 >> SHIFT_TILECOL)-1;
    for (tx = dirty_left[ty] ; tx <= dr ; tx++)
    {
      if (!rc)
      {
        rc = tile->TestDepthFlush (fvalue, min_depth);
      }
      tile->ClearOperations ();
      tile++;
    }
  }

  return rc;
}

bool csTiledCoverageBuffer::InsertPolygon (csVector2* verts, int num_verts,
	float max_depth)
{
  csBox2Int bbox;
  if (!DrawPolygon (verts, num_verts, bbox))
    return false;

  int tx, ty;
  int startrow, endrow;
  startrow = bbox.miny >> SHIFT_TILEROW;
  if (startrow < 0) startrow = 0;
  endrow = bbox.maxy >> SHIFT_TILEROW;
  if (endrow >= num_tile_rows) endrow = num_tile_rows-1;

  bool modified = false;
  for (ty = startrow ; ty <= endrow ; ty++)
  {
    csTileCol fvalue;
    fvalue.Empty ();
    csCoverageTile* tile = GetTile (dirty_left[ty], ty);
    int dr = dirty_right[ty];
    if (dr >= (width_po2 >> SHIFT_TILECOL))
      dr = (width_po2 >> SHIFT_TILECOL)-1;
    for (tx = dirty_left[ty] ; tx <= dr ; tx++)
    {
      tile->Flush (fvalue, max_depth, modified);
      tile++;
    }
  }
  return modified;
}

bool csTiledCoverageBuffer::InsertOutline (
	const csReversibleTransform& trans, float fov, float sx, float sy,
	csVector3* verts, int num_verts,
	bool* used_verts,
	int* edges, int num_edges, bool splat_outline)
{
  csBox2Int bbox;
  float max_depth;
  if (!DrawOutline (trans, fov, sx, sy, verts, num_verts, used_verts, edges,
  	num_edges, bbox, max_depth, splat_outline))
    return false;

  int tx, ty;
  int startrow, endrow;
  startrow = bbox.miny >> SHIFT_TILEROW;
  if (startrow < 0) startrow = 0;
  endrow = bbox.maxy >> SHIFT_TILEROW;
  if (endrow >= num_tile_rows) endrow = num_tile_rows-1;

  bool modified = false;
  for (ty = startrow ; ty <= endrow ; ty++)
  {
    csTileCol fvalue;
    fvalue.Empty ();
    csCoverageTile* tile = GetTile (dirty_left[ty], ty);
    int dr = dirty_right[ty];
    if (dr >= (width_po2 >> SHIFT_TILECOL))
      dr = (width_po2 >> SHIFT_TILECOL)-1;
    for (tx = dirty_left[ty] ; tx <= dr ; tx++)
    {
      tile->Flush (fvalue, max_depth, modified);
      tile++;
    }
  }
  return modified;
}

bool csTiledCoverageBuffer::PrepareTestRectangle (const csBox2& rect,
	csTestRectData& data)
{
  if (rect.MaxX () > 10000.0) data.bbox.maxx = 10000;
  else
  {
    if (rect.MaxX () <= 0) return false;
    data.bbox.maxx = QRound (rect.MaxX ());
  }
  if (rect.MaxY () > 10000.0) data.bbox.maxy = 10000;
  else
  {
    if (rect.MaxY () <= 0) return false;
    data.bbox.maxy = QRound (rect.MaxY ());
  }

  if (rect.MinX () < -10000.0) data.bbox.minx = -10000;
  else
  {
    if (rect.MinX () > 10000.0) return false;
    data.bbox.minx = QRound (rect.MinX ());
    if (data.bbox.minx >= width) return false;
  }
  if (rect.MinY () < -10000.0) data.bbox.miny = -10000;
  else
  {
    if (rect.MinY () > 10000.0) return false;
    data.bbox.miny = QRound (rect.MinY ());
    if (data.bbox.miny >= height) return false;
  }

  if (data.bbox.miny < 0) data.bbox.miny = 0;
  data.startrow = data.bbox.miny >> SHIFT_TILEROW;
  if (data.bbox.maxy >= height) data.bbox.maxy = height-1;
  data.endrow = data.bbox.maxy >> SHIFT_TILEROW;
  CS_ASSERT (data.endrow < num_tile_rows);
  if (data.bbox.minx < 0) data.bbox.minx = 0;
  data.startcol = data.bbox.minx >> SHIFT_TILECOL;
  if (data.bbox.maxx >= width) data.bbox.maxx = width-1;
  CS_ASSERT (data.bbox.maxx < width);
  data.endcol = data.bbox.maxx >> SHIFT_TILECOL;

  data.start_x = data.bbox.minx & (NUM_TILECOL-1);
  data.end_x = data.bbox.maxx & (NUM_TILECOL-1);
  return true;
}

bool csTiledCoverageBuffer::TestRectangle (const csTestRectData& data,
	float min_depth)
{
  int tx, ty;
  csTileCol vermask;
  bool do_depth_test = false;
  for (ty = data.startrow ; ty <= data.endrow ; ty++)
  {
    bool do_vermask = false;
    vermask.Full ();
    if (ty == data.startrow && (data.bbox.miny & (NUM_TILEROW-1)) != 0)
    {
      vermask = csCoverageTile::precalc_end_lines[
      	data.bbox.miny & (NUM_TILEROW-1)];
      do_vermask = true;
    }
    if (ty == data.endrow &&
    	(data.bbox.maxy & (NUM_TILEROW-1)) != (NUM_TILEROW-1))
    {
      vermask &= csCoverageTile::precalc_start_lines[
      	data.bbox.maxy & (NUM_TILEROW-1)];
      do_vermask = true;
    }

    csCoverageTile* tile = GetTile (data.startcol, ty);
    for (tx = data.startcol ; tx <= data.endcol ; tx++)
    {
      int sx = 0, ex = NUM_TILECOL-1;
      bool do_hor = false;
      if (tx == data.startcol && data.start_x != 0)
      {
        sx = data.start_x;
	do_hor = true;
      }
      if (tx == data.endcol && data.end_x != (NUM_TILECOL-1))
      {
        ex = data.end_x;
	do_hor = true;
      }

      if (do_vermask)
      {
        if (tile->TestCoverageRect (vermask, sx, ex, min_depth, do_depth_test))
	  return true;
      }
      else if (do_hor)
      {
        if (tile->TestCoverageRect (sx, ex, min_depth, do_depth_test))
	  return true;
      }
      else
      {
        if (tile->TestFullRect (min_depth))
          return true;
      }
      tile++;
    }
  }

  if (!do_depth_test) return false;

  // If we come here then we need to do depth test on
  // the borders of the rectangle.
  for (ty = data.startrow ; ty <= data.endrow ; ty++)
  {
    bool do_vermask = false;
    vermask.Full ();
    if (ty == data.startrow && (data.bbox.miny & (NUM_TILEROW-1)) != 0)
    {
      vermask = csCoverageTile::precalc_end_lines[
      	data.bbox.miny & (NUM_TILEROW-1)];
      do_vermask = true;
    }
    if (ty == data.endrow &&
    	(data.bbox.maxy & (NUM_TILEROW-1)) != (NUM_TILEROW-1))
    {
      vermask &= csCoverageTile::precalc_start_lines[
      	data.bbox.maxy & (NUM_TILEROW-1)];
      do_vermask = true;
    }

    csCoverageTile* tile;
    if (do_vermask)
    {
      int sx, ex;
      tile = GetTile (data.startcol, ty);
      for (tx = data.startcol ; tx <= data.endcol ; tx++)
      {
        bool test = false;
        if (tx == data.startcol && data.start_x != 0)
	{
          sx = data.start_x;
	  test = true;
	}
	else
	{
	  sx = 0;
	}
        if (tx == data.endcol && data.end_x != (NUM_TILECOL-1))
	{
          ex = data.end_x;
	  test = true;
	}
	else
	{
	  ex = NUM_TILECOL-1;
	}

        if (test && tile->TestDepthRect (vermask, sx, ex, min_depth))
	  return true;
        tile++;
      }
    }
    else
    {
      if (data.startcol == data.endcol)
      {
        if (data.start_x != 0 && data.end_x != (NUM_TILECOL-1))
	{
          tile = GetTile (data.startcol, ty);
          if (tile->TestDepthRect (data.start_x, data.end_x, min_depth))
	    return true;
        }
      }
      else
      {
        if (data.start_x != 0)
	{
          tile = GetTile (data.startcol, ty);
          if (tile->TestDepthRect (data.start_x, NUM_TILECOL-1, min_depth))
	    return true;
	}
	if (data.end_x != (NUM_TILECOL-1))
	{
          tile = GetTile (data.endcol, ty);
          if (tile->TestDepthRect (0, data.end_x, min_depth))
	    return true;
        }
      }
    }
  }
  return false;
}

bool csTiledCoverageBuffer::QuickTestRectangle (const csTestRectData& data,
	float min_depth)
{
  int tx, ty;
  for (ty = data.startrow ; ty <= data.endrow ; ty++)
  {
    csCoverageTile* tile = GetTile (data.startcol, ty);
    for (tx = data.startcol ; tx <= data.endcol ; tx++)
    {
      if (tile->TestFullRect (min_depth))
          return true;
      tile++;
    }
  }
  return false;
}

int csTiledCoverageBuffer::PrepareWriteQueueTest (const csTestRectData& data,
	float min_depth)
{
  int tx, ty;
  int cnt = 0;
  for (ty = data.startrow ; ty <= data.endrow ; ty++)
  {
    csCoverageTile* tile = GetTile (data.startcol, ty);
    for (tx = data.startcol ; tx <= data.endcol ; tx++)
    {
      bool cov;
      if (tile->queue_tile_empty)
      {
        cov = false;
	tile->fully_covered = false;
      }
      else
      {
        cov = min_depth >= tile->tile_min_depth;
	if (tile->IsFull () && min_depth > tile->tile_max_depth)
	  tile->fully_covered = true;
	else
	  tile->fully_covered = false;
      }
      tile->covered = cov;
      if (!cov) cnt++;
      tile++;
    }
  }
  return cnt;
}

int csTiledCoverageBuffer::AddWriteQueueTest (const csTestRectData& maindata,
	const csTestRectData& data, bool& relevant)
{
  int sr = data.startrow;
  if (sr > maindata.endrow) return 0;
  if (sr < maindata.startrow) sr = maindata.startrow;
  int er = data.endrow;
  if (er < maindata.startrow) return 0;
  if (er > maindata.endrow) er = maindata.endrow;
  int sc = data.startcol;
  if (sc > maindata.endcol) return 0;
  if (sc < maindata.startcol) sc = maindata.startcol;
  int ec = data.endcol;
  if (ec < maindata.startcol) return 0;
  if (ec > maindata.endcol) ec = maindata.endcol;

  int tx, ty;
  int cnt = 0;
  relevant = false;
  for (ty = sr ; ty <= er ; ty++)
  {
    csCoverageTile* tile = GetTile (sc, ty);
    for (tx = sc ; tx <= ec ; tx++)
    {
      if (!tile->fully_covered)
        relevant = true;
      if (!tile->covered)
      {
        tile->covered = true;
	cnt++;
      }
      tile++;
    }
  }
  return cnt;
}

bool csTiledCoverageBuffer::TestPoint (const csVector2& point, float min_depth)
{
  int xi, yi;
  xi = QRound (point.x);
  yi = QRound (point.y);

  if (xi < 0) return false;
  if (yi < 0) return false;
  if (xi >= width) return false;
  if (yi >= height) return false;

  int ty = yi >> SHIFT_TILEROW;
  int tx = xi >> SHIFT_TILECOL;

  csCoverageTile* tile = GetTile (tx, ty);
  return tile->TestPoint (xi & (NUM_TILECOL-1), yi & (NUM_TILEROW-1),
  	min_depth);
}

#if defined(THIS_IS_UNUSED)
static void DrawZoomedPixel (iGraphics2D* g2d, int x, int y, int col, int zoom)
{
  if (zoom == 1)
    g2d->DrawPixel (x, y, col);
  else if (zoom == 2)
  {
    x <<= 1;
    y <<= 1;
    g2d->DrawPixel (x+0, y+0, col);
    g2d->DrawPixel (x+1, y+0, col);
    g2d->DrawPixel (x+0, y+1, col);
    g2d->DrawPixel (x+1, y+1, col);
  }
  else if (zoom == 3)
  {
    x *= 3;
    y *= 3;
    g2d->DrawPixel (x+0, y+0, col);
    g2d->DrawPixel (x+1, y+0, col);
    g2d->DrawPixel (x+2, y+0, col);
    g2d->DrawPixel (x+0, y+1, col);
    g2d->DrawPixel (x+1, y+1, col);
    g2d->DrawPixel (x+2, y+1, col);
    g2d->DrawPixel (x+0, y+2, col);
    g2d->DrawPixel (x+1, y+2, col);
    g2d->DrawPixel (x+2, y+2, col);
  }
}
#endif // End of THIS_IS_UNUSED

csPtr<iString> csTiledCoverageBuffer::Debug_Dump ()
{
  scfString* rc = new scfString ();
  csString& str = rc->GetCsString ();

  int x, y, tx, ty, i, j;
  for (ty = 0 ; ty < num_tile_rows ; ty++)
  {
    for (y = 0 ; y < (NUM_TILEROW/8) ; y++)
    {
      for (tx = 0 ; tx < (width_po2 >> SHIFT_TILECOL) ; tx++)
      {
        csCoverageTile* tile = GetTile (tx, ty);
	for (x = 0 ; x < (NUM_TILECOL/4) ; x++)
	{
          int cnt = 0;
	  if (!tile->queue_tile_empty)
	    for (i = 0 ; i < 8 ; i++)
	      for (j = 0 ; j < 8 ; j++)
	        cnt += tile->coverage[x*8+i].TestBit (y*8+j);
	  char c;
	  if (cnt == 64) c = '#';
	  else if (cnt > 54) c = '*';
	  else if (cnt == 0) c = ' ';
	  else if (cnt < 10) c = '.';
	  else c = 'x';
	  str.Append (c);
	}
      }
      str.Append ('\n');
    }
  }

  return csPtr<iString> (rc);
}

void csTiledCoverageBuffer::Debug_Dump (iGraphics3D* g3d, int /*zoom*/)
{
  iGraphics2D* g2d = g3d->GetDriver2D ();
  int colpoint = g2d->FindRGB (255, 0, 0);

  int x, y, tx, ty, i, j;
  for (ty = 0 ; ty < num_tile_rows ; ty++)
  {
    for (y = 0 ; y < 8 ; y++)
    {
      for (tx = 0 ; tx < (width_po2 >> SHIFT_TILECOL) ; tx++)
      {
        g2d->DrawPixel (tx*NUM_TILECOL, ty*NUM_TILEROW, colpoint);

        csCoverageTile* tile = GetTile (tx, ty);
	for (x = 0 ; x < (NUM_TILECOL/8) ; x++)
	{
	  float depth = tile->depth[y*(NUM_TILECOL/8)+x];
	  for (i = 0 ; i < 8 ; i++)
	    for (j = 0 ; j < 8 ; j++)
	    {
	      bool val;
	      if (tile->queue_tile_empty)
	        val = false;
	      else
	        val = tile->coverage[x*8+i].TestBit (y*8+j);
	      if (val)
	      {
	        int c = 255-int (depth);
		if (c < 50) c = 50;
		int col = g2d->FindRGB (c, c, c);
	        g2d->DrawPixel (tx*NUM_TILECOL+x*8+i,
			ty*NUM_TILEROW+y*8+j, col);
	      }
	    }
	}
      }
    }
  }
}

#if 0
static float rnd (int totrange, int leftpad, int rightpad)
{
  return float (((rand () >> 4) % (totrange-leftpad-rightpad)) + leftpad);
}
#endif

#define COV_ASSERT(test,msg) \
  if (!(test)) \
  { \
    str.Format("csTiledCoverageBuffer failure (%d,%s): %s\n", int(__LINE__), \
    	#msg, #test); \
    return csPtr<iString> (rc); \
  }

csPtr<iString> csTiledCoverageBuffer::Debug_UnitTest ()
{
  Setup (640, 480);

  scfString* rc = new scfString ();
  csString& str = rc->GetCsString ();

  csVector2 poly[4];
  //csCoverageTile* t;
  //iString* s;

  Initialize ();
  COV_ASSERT (TestPoint (csVector2 (100, 100), 5) == true, "tp");
  poly[0].Set (50, 50);
  poly[1].Set (600, 50);
  poly[2].Set (600, 430);
  poly[3].Set (50, 430);
  InsertPolygon (poly, 4, 10.0);
  COV_ASSERT (TestPoint (csVector2 (100, 100), 5) == true, "tp");
  COV_ASSERT (TestPoint (csVector2 (100, 100), 15) == false, "tp");
  COV_ASSERT (TestPoint (csVector2 (599, 100), 5) == true, "tp");
  COV_ASSERT (TestPoint (csVector2 (599, 100), 15) == false, "tp");
  COV_ASSERT (TestPoint (csVector2 (601, 100), 5) == true, "tp");
  COV_ASSERT (TestPoint (csVector2 (601, 100), 15) == true, "tp");

  rc->DecRef ();
  return 0;
}

csTicks csTiledCoverageBuffer::Debug_Benchmark (int /*num_iterations*/)
{
  Setup (640, 480);

  csTicks start = csGetTicks ();
  csTicks end = csGetTicks ();
  return end-start;
}
