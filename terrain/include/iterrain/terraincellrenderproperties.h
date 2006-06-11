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

#ifndef __CS_ITERRAIN_TERRAINCELLRENDERPROPERTIES_H__
#define __CS_ITERRAIN_TERRAINCELLRENDERPROPERTIES_H__

#include "csutil/scf.h"

struct iTerrainCellRenderProperties : public virtual iBase
{
  SCF_INTERFACE (iTerrainCellRenderProperties, 1, 0, 0);

  virtual bool GetVisible() const = 0;
  virtual void SetVisible(bool value) = 0;
};

#endif // __CS_ITERRAIN_TERRAINCELLRENDERPROPERTIES_H__
