/*
Copyright (C) 2008 by Jorrit Tyberghein and Michael Gist

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

#ifndef __COLLECTION_H__
#define __COLLECTION_H__

#include "csutil/csobject.h"
#include "csutil/scf_implementation.h"
#include "iengine/collection.h"

class csEngine;

/**
 * A collection is used to store related objects in a simple structure
 * to guarentee that they won't be freed by the engine. The engine has
 * a default collection where all iObjects are placed unless explicitly
 * placed in another collectino.
 */

class csCollection : public scfImplementationExt1<csCollection,
                                                  csObject,
                                                  iCollection>
{
public:
  /**
   * Initialize an empty collection.
   */
  csCollection() : scfImplementationType (this) {}
  /**
   * Delete this collection, releasing all references held.
   */
  ~csCollection() { ReleaseAllObjects(); }

  /**
  * Get the iObject for this collection.
  */
  iObject *QueryObject() { return this; }

  /**
   * Add an object to this collection.
   */
  inline void Add(iObject *obj) { ObjAdd(obj); }

  /**
   * Remove an object from this collection.
   */
  inline void Remove(iObject *obj) { ObjRemove(obj); }

  /**
   * Release all references to objects held by this collection.
   */
  inline void ReleaseAllObjects() { ObjRemoveAll(); }

  /**
   * Returns true if this collection is the parent of the object passed.
   */
  inline bool IsParentOf(iObject* obj) { return this == obj->GetObjectParent(); }

  /**
   * Looks to see if this collection contains the sector. If so,
   * it returns the sector.
   */
  inline iSector* FindSector(const char *name)
  {
    csRef<iSector> s (CS::GetNamedChildObject<iSector>(this, name));
    return s;
  }

  /**
   * Looks to see if this collection contains the sector. If so,
   * it returns the sector.
   */
  inline iMeshWrapper* FindMeshObject(const char *name)
  {
    csRef<iMeshWrapper> mw (CS::GetNamedChildObject<iMeshWrapper>(this, name));
    return mw;
  }

  /**
   * Looks to see if this collection contains the mesh factory. If so,
   * it returns the mesh factory.
   */
  inline iMeshFactoryWrapper* FindMeshFactory (const char *name)
  {
    csRef<iMeshFactoryWrapper> mfw (CS::GetNamedChildObject<iMeshFactoryWrapper>(this, name));
    return mfw;
  }

  /**
   * Looks to see if this collection contains the texture. If so,
   * it returns the texture.
   */
  inline iTextureWrapper* FindTexture(const char *name)
  {
    csRef<iTextureWrapper> tw (CS::GetNamedChildObject<iTextureWrapper>(this, name));
    return tw;
  }

  /**
   * Looks to see if this collection contains the material. If so,
   * it returns the material.
   */
  inline iMaterialWrapper* FindMaterial(const char *name)
  {
    csRef<iMaterialWrapper> mw (CS::GetNamedChildObject<iMaterialWrapper>(this, name));
    return mw;
  }

  /**
   * Looks to see if this collection contains the shader. If so,
   * it returns the shader.
   */
  inline iShader* FindShader (const char *name)
  {
    csRef<iShader> s (CS::GetNamedChildObject<iShader>(this, name));
    return s;
  }
};

#endif // __COLLECTION_H__
