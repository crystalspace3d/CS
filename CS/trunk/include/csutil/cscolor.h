/*
    Copyright (C) 1998 by Jorrit Tyberghein
  
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

#ifndef __CSCOLOR_H__
#define __CSCOLOR_H__

/**
 * A class used to represent a color in RGB space.
 * This class is similar to RGBPixel and RGBcolor except that
 * it uses floating-point values to store R,G,B values.
 */
class csColor
{
public:
  /// Red (0..1)
  float red;
  /// Green (0..1)
  float green;
  /// Blue (0..1)
  float blue;

public:
  /// Initialize a color object (contents undefined)
  csColor () { }
  /// Initialize a color object with given R,G,B components
  csColor (float r, float g, float b)
  { red = r; green = g; blue = b; }
  /// Initialize a color object with an existing color
  csColor (const csColor& c)
  { red = c.red; green = c.green; blue = c.blue; }
  /// Set color to given R,G,B components
  void Set (float r, float g, float b)
  { red = r; green = g; blue = b; }
  /// Clamp color to given R,G,B values
  void Clamp (float r, float g, float b)
  { 
    if (red > r) red = r;
    if (green > g) green = g;
    if (blue > b) blue = b;
  }
  /// Assign one color object to another
  csColor& operator= (const csColor& c)
  { red = c.red; green = c.green; blue = c.blue; return *this; }
  /// Multiply this color by a scalar value
  csColor& operator*= (float f)
  { red *= f; green *= f; blue *= f; return *this; }
};

/// Multiply a color by a scalar value
inline csColor operator* (const csColor& s, float f)
{ csColor c (s); c *= f; return c; }

/// Multiply a scalar value by a color
inline csColor operator* (float f, const csColor& s)
{ csColor c (s); c *= f; return c; }

#endif // __CSCOLOR_H__
