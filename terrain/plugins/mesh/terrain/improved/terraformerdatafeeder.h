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

#ifndef __CS_TERRAIN_TERRAFORMERDATAFEEDER_H__
#define __CS_TERRAIN_TERRAFORMERDATAFEEDER_H__

#include "csutil/scf_implementation.h"

#include "iterrain/terraindatafeeder.h"

#include "csutil/csstring.h"
#include "iutil/comp.h"

#include "ivaria/terraform.h"

#include "csgeom/box.h"

CS_PLUGIN_NAMESPACE_BEGIN(ImprovedTerrain)
{

class csTerrainTerraFormerDataFeeder :
  public scfImplementation2<csTerrainTerraFormerDataFeeder,
                            iTerrainDataFeeder,
                            iComponent>
{
protected:
  iObjectRegistry* object_reg;

  csRef<iTerraFormer> former;
  csBox2 sample_region;

public:
  csTerrainTerraFormerDataFeeder (iBase* parent);

  virtual ~csTerrainTerraFormerDataFeeder ();

  // ------------ iTerrainDataFeeder implementation ------------

  virtual bool PreLoad (iTerrainCell* cell);
  virtual bool Load (iTerrainCell* cell);

  virtual void SetParam(const char* param, const char* value);
  
  // ------------ iComponent implementation ------------
  virtual bool Initialize (iObjectRegistry* object_reg);
};

}
CS_PLUGIN_NAMESPACE_END(ImprovedTerrain)

#endif // __CS_TERRAIN_TERRAFORMERDATAFEEDER_H__
