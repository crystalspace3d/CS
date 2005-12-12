/*
    Copyright (C) 2005 by Jorrit Tyberghein

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
#include "plugins/engine/3d/meshgen.h"

csMeshGeneratorGeometry::csMeshGeneratorGeometry () :
	scfImplementationType (this)
{
  radius = 0.0f;
  density = 1.0f;
}

void csMeshGeneratorGeometry::AddFactory (iMeshFactoryWrapper* factory,
    float maxdist)
{
  factories.Push (factory);
  maxdistances.Push (maxdist);
}

void csMeshGeneratorGeometry::RemoveFactory (size_t idx)
{
  factories.DeleteIndex (idx);
  maxdistances.DeleteIndex (idx);
}

void csMeshGeneratorGeometry::SetRadius (float radius)
{
  csMeshGeneratorGeometry::radius = radius;
}

void csMeshGeneratorGeometry::SetDensity (float density)
{
  csMeshGeneratorGeometry::density = density;
}

//--------------------------------------------------------------------------

csMeshGeneratorMapping::csMeshGeneratorMapping () :
	scfImplementationType (this)
{
}

void csMeshGeneratorMapping::SetMeshWrapper (iMeshWrapper* mesh)
{
  csMeshGeneratorMapping::mesh = mesh;
}

//--------------------------------------------------------------------------

csMeshGenerator::csMeshGenerator() : scfImplementationType (this)
{
}

iMeshGeneratorGeometry* csMeshGenerator::CreateGeometry ()
{
  csMeshGeneratorGeometry* geom = new csMeshGeneratorGeometry ();
  geometries.Push (geom);
  geom->DecRef ();
  return geom;
}

void csMeshGenerator::RemoveGeometry (size_t idx)
{
  geometries.DeleteIndex (idx);
}

iMeshGeneratorMapping* csMeshGenerator::CreateMapping ()
{
  csMeshGeneratorMapping* mapping = new csMeshGeneratorMapping ();
  mappings.Push (mapping);
  mapping->DecRef ();
  return mapping;
}

void csMeshGenerator::RemoveMapping (size_t idx)
{
  mappings.DeleteIndex (idx);
}

