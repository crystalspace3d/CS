/*
    Dynamic arrays of engine objects
    Copyright (C) 1999 by Andrew Zabolotny

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
#include "parrays.h"
#include "curve.h"
#include "polygon.h"

//------------------------------------------------------+ csPolygonArray +----//
csPolygonArray::~csPolygonArray ()
{
  DeleteAll ();
}

bool csPolygonArray::FreeItem (csSome Item)
{
  delete (csPolygon3D *)Item;
  return true;
}

int csPolygonArray::CompareKey (csSome Item, csConstSome Key, int Mode) const
{
  (void)Mode;

  const char *name = ((csPolygon3D *) Item)->GetName ();
  return name ? strcmp (name, (char *)Key) : -1;
}

csPolygon3D *csPolygonArray::Get (int iIndex) const
{
  return (csPolygon3D *)csVector::Get (iIndex);
}

