/*
    Copyright (C) 1998 by Jorrit Tyberghein
    csObject library (C) 1999 by Ivan Avramovic <ivan@avramovic.com>
  
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
#include "csobject/csobject.h"
#include "csobject/objiter.h"
#include "csobject/dataobj.h"
#include "csobject/pobject.h"

#include <stdlib.h>
#include <string.h>

#define CONTAINER_LIMIT_DELTA	16

class csObjContainer
{
public:
  // Amount of child objects
  unsigned short count, limit;
  // Some dumb compilers (i.e. Watcom) do not like [0] arrays
  csObject *obj [1];
  // Enlarge the array of children
  static void SetLimit (csObjContainer *&iContainer, int iLimit);
};

void csObjContainer::SetLimit (csObjContainer *&iContainer, int iLimit)
{
  if (iLimit == 0)
  {
    free (iContainer);
    iContainer = NULL;
    return;
  }
  else if (iContainer == NULL)
  {
    iContainer = (csObjContainer *)malloc (
      sizeof (csObjContainer) + sizeof (csObject *) * (iLimit - 1));
    iContainer->count = 0;
  }
  else
    iContainer = (csObjContainer *)realloc (iContainer,
      sizeof (csObjContainer) + sizeof (csObject *) * (iLimit - 1));
  iContainer->limit = iLimit;
}

const csIdType csObject::Type ("csObject");

const csIdType& csObject::GetType () const
{
  return Type;
}

IMPLEMENT_IBASE (csObject)
  IMPLEMENTS_INTERFACE (iObject)
IMPLEMENT_IBASE_END

csObject::csObject () : csBase (), iObject (), children (NULL), Name (NULL)
{
  CONSTRUCT_IBASE (NULL);
  static CS_ID id = 0;
  csid = id++;
}

csObject::csObject (csObject& iObj) : csBase (), iObject (iObj), csid (iObj.csid),
  children (NULL), Name (NULL)
{
  CONSTRUCT_IBASE (NULL);
  if (iObj.children)
  {
    int size = sizeof (csObjContainer) + sizeof (csObject *) * (iObj.children->limit - 1);
    if (size > 0)
    {
      children = (csObjContainer *)malloc (size);
      memcpy (children, iObj.children, size);
    }
  }
}

csObject::~csObject ()
{
  if (children)
  {
    for (int i = 0; i < children->count; i++)
      delete (children->obj [i]);
    free (children);
  }
  delete [] Name;
}

csObjectNoDel::~csObjectNoDel ()
{
  if (children)
  {
    free (children);
    children = NULL;
  }
}

csObject *csObject::GetChild (const csIdType& iType, bool derived) const
{
  if (!children)
    return NULL;
  int i;
  if (derived)
  {
    for (i = 0; i < children->count; i++)
      if (children->obj [i]->GetType () >= iType)
        return children->obj [i];
  }
  else
  {
    for (i = 0; i < children->count; i++)
      if (&children->obj [i]->GetType () == &iType)
        return children->obj [i];
  }
  return NULL;
}

void csObject::ObjAdd (csObject *obj)
{
  if (!obj)
    return;

  if (!children)
    csObjContainer::SetLimit (children, CONTAINER_LIMIT_DELTA);
  else if (children->count >= children->limit)
    csObjContainer::SetLimit (children, children->limit + CONTAINER_LIMIT_DELTA);

  children->obj [children->count++] = obj;
  obj->SetObjectParent (this);
}

void csObject::ObjRelease (csObject *obj)
{ 
  if (!children || !obj)
    return;
  for (int i = 0; i < children->count; i++)
    if (children->obj [i] == obj)
    {
      memmove (&children->obj [i], &children->obj [i + 1],
        (children->limit - (i + 1)) * sizeof (csObject *));
      if (--children->count <= children->limit - CONTAINER_LIMIT_DELTA)
        csObjContainer::SetLimit (children, children->limit - CONTAINER_LIMIT_DELTA);
      break;
    }
}

void csObject::ObjRemove (csObject *obj)
{ 
  ObjRelease (obj);
  delete obj; 
}

//------------------------------------------------------ Object iterator -----//

csObjIterator::csObjIterator (const csIdType &iType, const csObject &iObj,
	bool derived)
{
  csObjIterator::derived = derived;
  Reset (iType, iObj);
}

void csObjIterator::Reset (const csIdType &iType, const csObject &iObj)
{
  Type = &iType;
  Container = iObj.children;
  Index = -1;
  Next ();
}

csObject* csObjIterator::GetObj () const
{
  return Container ? Container->obj [Index] : NULL;
}

void csObjIterator::Next ()
{
  if (Container)
  {
    for (;;)
    {
      Index++;
      if (Index >= Container->count)
      {
        Container = NULL;
        break;
      }
      if (derived)
      {
        if (Container->obj [Index]->GetType () >= *Type) break;
      }
      else
      {
        if (&Container->obj [Index]->GetType () == Type) break;
      }
    }
  }
}

bool csObjIterator::FindName(const char* name)
{
  while (!IsFinished ())
  {
    if (strcmp (GetObj ()->GetName (), name) == 0)
      return true;
    Next ();
  }
  return false;
}

//-------------------- miscelaneous simple classes derived from csObject -----//

IMPLEMENT_CSOBJTYPE (csDataObject, csObject);
IMPLEMENT_CSOBJTYPE (csPObject, csObject);
