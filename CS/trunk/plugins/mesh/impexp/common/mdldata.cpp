/*
    Copyright (C) 2001 by Martin Geisse <mgeisse@gmx.net>

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
#include "mdldata.h"

#define IMPLEMENT_ARRAY_INTERFACE_NONUM(clname,type,sing_name,mult_name) \
  type clname::Get##sing_name (int n) const				\
  { return mult_name[n]; }						\
  void clname::Set##sing_name (int n, type val)				\
  { mult_name[n] = val; }

#define IMPLEMENT_ARRAY_INTERFACE(clname,type,sing_name,mult_name)	\
  IMPLEMENT_ARRAY_INTERFACE_NONUM (clname, type, sing_name, mult_name)	\
  int clname::GetNum##mult_name () const				\
  { return mult_name.Length (); }					\
  void clname::Add##sing_name (type v)					\
  { mult_name.Push (v); }

#define IMPLEMENT_ACCESSOR_METHOD(clname,type,name)			\
  type clname::Get##name () const					\
  { return name; }							\
  void clname::Set##name (type val)					\
  { name = val; }

#define IMPLEMENT_ACCESSOR_METHOD_REF(clname,type,name)			\
  type clname::Get##name () const					\
  { return name; }							\
  void clname::Set##name (type val)					\
  { if (name) name->DecRef (); name = val; if (name) name->IncRef (); }

#define IMPLEMENT_OBJECT_INTERFACE(clname)				\
  IMPLEMENT_EMBEDDED_OBJECT (clname::Embedded_csObject);		\
  iObject* clname::QueryObject ()					\
  { return &scfiObject; }

/*** csModelDataPolygon ***/

SCF_IMPLEMENT_IBASE (csModelDataPolygon)
  SCF_IMPLEMENTS_INTERFACE (iModelDataPolygon)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelDataPolygon);

csModelDataPolygon::csModelDataPolygon ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
  Material = NULL;
}

csModelDataPolygon::~csModelDataPolygon ()
{
  if (Material) Material->DecRef ();
}

int csModelDataPolygon::GetVertexCount () const
{
  return Vertices.Length ();
}

void csModelDataPolygon::AddVertex (int PositionIndex,
  const csVector3 &Normal, const csColor &Color, const csVector2 &TexCoords)
{
  Vertices.Push (PositionIndex);
  Normals.Push (Normal);
  Colors.Push (Color);
  TextureCoords.Push (TexCoords);
}

void csModelDataPolygon::DeleteVertex (int n)
{
  Vertices.Delete (n);
  Normals.Delete (n);
  Colors.Delete (n);
  TextureCoords.Delete (n);
}

IMPLEMENT_ARRAY_INTERFACE_NONUM (csModelDataPolygon,
	int, Vertex, Vertices);
IMPLEMENT_ARRAY_INTERFACE_NONUM (csModelDataPolygon,
	const csVector3 &, Normal, Normals);
IMPLEMENT_ARRAY_INTERFACE_NONUM (csModelDataPolygon,
	const csVector2 &, TextureCoords, TextureCoords);
IMPLEMENT_ARRAY_INTERFACE_NONUM (csModelDataPolygon,
	const csColor &, Color, Colors);
IMPLEMENT_ACCESSOR_METHOD (csModelDataPolygon, iModelDataMaterial *, Material);

/*** csModelDataObject ***/

SCF_IMPLEMENT_IBASE (csModelDataObject)
  SCF_IMPLEMENTS_INTERFACE (iModelDataObject)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelDataObject);

IMPLEMENT_ARRAY_INTERFACE (csModelDataObject,
	const csVector3 &, Vertex, Vertices);

csModelDataObject::csModelDataObject ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
}

/*** csModelDataCamera ***/

SCF_IMPLEMENT_IBASE (csModelDataCamera)
  SCF_IMPLEMENTS_INTERFACE (iModelDataCamera)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelDataCamera);

IMPLEMENT_ACCESSOR_METHOD (csModelDataCamera, const csVector3 &, Position);
IMPLEMENT_ACCESSOR_METHOD (csModelDataCamera, const csVector3 &, UpVector);
IMPLEMENT_ACCESSOR_METHOD (csModelDataCamera, const csVector3 &, FrontVector);
IMPLEMENT_ACCESSOR_METHOD (csModelDataCamera, const csVector3 &, RightVector);

csModelDataCamera::csModelDataCamera ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
}

void csModelDataCamera::ComputeUpVector ()
{
  UpVector = FrontVector % RightVector;
}

void csModelDataCamera::ComputeFrontVector ()
{
  FrontVector = RightVector % UpVector;
}

void csModelDataCamera::ComputeRightVector ()
{
  RightVector = UpVector % FrontVector;
}

void csModelDataCamera::Normalize ()
{
  UpVector.Normalize ();
  FrontVector.Normalize ();
  RightVector.Normalize ();
}

bool csModelDataCamera::CheckOrthogonality () const
{
  float x = UpVector * FrontVector;
  float y = RightVector * FrontVector;
  float z = UpVector * RightVector;
  return (ABS(x) < SMALL_EPSILON) && (ABS(y) < SMALL_EPSILON) &&
    (ABS(z) < SMALL_EPSILON);
}

/*** csModelDataLight ***/

SCF_IMPLEMENT_IBASE (csModelDataLight)
  SCF_IMPLEMENTS_INTERFACE (iModelDataLight)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelDataLight);

IMPLEMENT_ACCESSOR_METHOD (csModelDataLight, float, Radius);
IMPLEMENT_ACCESSOR_METHOD (csModelDataLight, const csColor &, Color);

csModelDataLight::csModelDataLight ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
}

/*** csModelDataMaterial ***/

SCF_IMPLEMENT_IBASE (csModelDataMaterial)
  SCF_IMPLEMENTS_INTERFACE (iModelDataMaterial)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelDataMaterial);

csModelDataMaterial::csModelDataMaterial ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
}

/*** csModelData ***/

SCF_IMPLEMENT_IBASE (csModelData)
  SCF_IMPLEMENTS_INTERFACE (iModelData)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iObject)
SCF_IMPLEMENT_IBASE_END

IMPLEMENT_OBJECT_INTERFACE (csModelData);

csModelData::csModelData ()
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
}
