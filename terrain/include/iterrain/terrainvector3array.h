/*
    Copyright (C) 2006 by Kapoulkine Arseny

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

#ifndef __CS_ITERRAIN_TERRAINVECTOR3ARRAY_H__
#define __CS_ITERRAIN_TERRAINVECTOR3ARRAY_H__

#include "csutil/scf.h"
#include "iutil/array.h"

class csVector3;

struct iTerrainVector3Array : public iArrayChangeAll<csVector3>
{
  SCF_IARRAYCHANGEALL_INTERFACE(iTerrainVector3Array);
};

#endif // __CS_ITERRAIN_TERRAINVECTOR3ARRAY_H__
