/*
    Copyright (C) 2000 by Norman Kraemer
  
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

#include "sysdef.h"
#include "alphamap.h"

IMPLEMENT_IBASE (csGlideAlphaMap)
  IMPLEMENTS_INTERFACE (iTextureHandle)
IMPLEMENT_IBASE_END

void csGlideAlphaMap::savemap()
{
  FILE *h = fopen( "alpha.map", "w" );
  fwrite( alpha, 1, width*height, h );
  fclose( h );
}

