/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein
    Copyright (C) 2003 by Anders Stenberg

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

#ifndef __CS_SOFT_RENDER3D_H__
#define __CS_SOFT_RENDER3D_H__

#include "iutil/config.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "sft3dcom.h"

/// Software 3D renderer
class csSoftwareRender3D : public csSoftwareRender3DCommon
{
public:
  SCF_DECLARE_IBASE_EXT(csSoftwareRender3DCommon);
  /// Constructor
  csSoftwareRender3D (iBase*);
  /// Destructor
  virtual ~csSoftwareRender3D ();
  /// Initialize iComponent.
  virtual bool Initialize (iObjectRegistry*);
  /// Open a canvas.
  virtual bool Open ();

  struct eiSoftConfig : public iConfig
  {
    SCF_DECLARE_EMBEDDED_IBASE (csSoftwareRender3D);
    virtual bool GetOptionDescription (int idx, csOptionDescription*);
    virtual bool SetOption (int id, csVariant* value);
    virtual bool GetOption (int id, csVariant* value);
  } scfiConfig;
  friend struct eiSoftConfig;
};

#endif // __CS_SOFT_RENDER3D_H__
