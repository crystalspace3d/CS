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

#ifndef __CS_BITS64_H__
#define __CS_BITS64_H__

//#define SHIFT_TILECOL 5
//#define SHIFT_TILEROW 6

#define SHIFT_TILECOL 6
#define SHIFT_TILEROW 5

#define NUM_TILECOL (1<<SHIFT_TILECOL)
#define NUM_TILEROW (1<<SHIFT_TILEROW)
#define NUM_DEPTH ((NUM_TILEROW/8) * (NUM_TILECOL/8))

#if NUM_TILEROW==32
/**
 * 32-bit integer class with a few primitive operations defined
 * specifically made for coverage buffer.
 */
class csTileCol
{
public:
  uint32 b1;

public:
  inline void Set (uint32 b1)
  {
    csTileCol::b1 = b1;
  }
  inline csTileCol& operator |= (const csTileCol& b)
  {
    b1 |= b.b1;
    return *this;
  }
  inline csTileCol& operator &= (const csTileCol& b)
  {
    b1 &= b.b1;
    return *this;
  }
  inline csTileCol& operator ^= (const csTileCol& b)
  {
    b1 ^= b.b1;
    return *this;
  }
  inline void AndInverted (const csTileCol& b)
  {
    b1 &= ~b.b1;
  }
  inline void Invert ()
  {
    b1 = ~b1;
  }
  inline bool TestInvertedMask (const csTileCol& b) const
  {
    return b1 & ~b.b1;
  }
  inline bool TestMask (const csTileCol& b) const
  {
    return b1 & b.b1;
  }
  inline void XorBit (int b)
  {
    b1 ^= 1<<b;
  }
  inline bool TestBit (int b) const
  {
    return !!(b1 & (1<<b));
  }
  inline void Empty ()
  {
    b1 = 0;
  }
  inline bool IsEmpty () const
  {
    return b1 == 0;
  }
  inline void Full ()
  {
    b1 = ~0;
  }
  inline bool IsFull () const
  {
    return b1 == (uint32)~0;
  }
  // Check if byte 0 contains a 1.
  inline bool CheckByte0 () const { return b1 & 0xff; }
  inline bool CheckByte1 () const { return b1 & 0xff00; }
  inline bool CheckByte2 () const { return b1 & 0xff0000; }
  inline bool CheckByte3 () const { return b1 & 0xff000000; }
};
#else
/**
 * 64-bit integer class with a few primitive operations defined
 * specifically made for coverage buffer. It might be useful to
 * make MMX or other native 64-bit implementations of this class.
 */
class csTileCol
{
public:
  uint32 b1, b2;

public:
  inline void Set (uint32 b1, uint32 b2)
  {
    csTileCol::b1 = b1;
    csTileCol::b2 = b2;
  }
  inline csTileCol& operator |= (const csTileCol& b)
  {
    b1 |= b.b1;
    b2 |= b.b2;
    return *this;
  }
  inline csTileCol& operator &= (const csTileCol& b)
  {
    b1 &= b.b1;
    b2 &= b.b2;
    return *this;
  }
  inline csTileCol& operator ^= (const csTileCol& b)
  {
    b1 ^= b.b1;
    b2 ^= b.b2;
    return *this;
  }
  inline void AndInverted (const csTileCol& b)
  {
    b1 &= ~b.b1;
    b2 &= ~b.b2;
  }
  inline void Invert ()
  {
    b1 = ~b1;
    b2 = ~b2;
  }
  inline bool TestInvertedMask (const csTileCol& b) const
  {
    if (b1 & ~b.b1) return true;
    return b2 & ~b.b2;
  }
  inline bool TestMask (const csTileCol& b) const
  {
    if (b1 & b.b1) return true;
    return b2 & b.b2;
  }
  inline void XorBit (int b)
  {
    if (b >= 32)
      b2 ^= 1<<(b-32);
    else
      b1 ^= 1<<b;
  }
  inline bool TestBit (int b) const
  {
    if (b >= 32)
      return !!(b2 & (1<<(b-32)));
    else
      return !!(b1 & (1<<b));
  }
  inline void Empty ()
  {
    b1 = 0;
    b2 = 0;
  }
  inline bool IsEmpty () const
  {
    return b1 == 0 && b2 == 0;
  }
  inline void Full ()
  {
    b1 = ~0;
    b2 = ~0;
  }
  inline bool IsFull () const
  {
    return b1 == (uint32)~0 && b2 == (uint32)~0;
  }
  // Check if byte 0 contains a 1.
  inline bool CheckByte0 () const { return b1 & 0xff; }
  inline bool CheckByte1 () const { return b1 & 0xff00; }
  inline bool CheckByte2 () const { return b1 & 0xff0000; }
  inline bool CheckByte3 () const { return b1 & 0xff000000; }
  inline bool CheckByte4 () const { return b2 & 0xff; }
  inline bool CheckByte5 () const { return b2 & 0xff00; }
  inline bool CheckByte6 () const { return b2 & 0xff0000; }
  inline bool CheckByte7 () const { return b2 & 0xff000000; }
};
#endif

#endif // __CS_BITS64_H__

