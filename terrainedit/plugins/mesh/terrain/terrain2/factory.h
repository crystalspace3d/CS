/*
  Copyright (C) 2006 by Kapoulkine Arseny
                2007 by Marten Svanfeldt

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

#ifndef __CS_TERRAIN_FACTORY_H__
#define __CS_TERRAIN_FACTORY_H__

#include "csutil/csstring.h"
#include "csutil/flags.h"
#include "csutil/refarr.h"
#include "csutil/scf_implementation.h"

#include "imesh/object.h"
#include "imesh/terrain2.h"

#include "cstool/modifiableimpl.h" //Added modifiableimple.h for iModifiable implementation

CS_PLUGIN_NAMESPACE_BEGIN(Terrain2)
{
class csTerrainSystem;
class csTerrainCell;

class csTerrainFactoryCell :
  public scfImplementation2<csTerrainFactoryCell,
                            iTerrainFactoryCell,CS::Utility::iModifiable>
{
public:
  csTerrainFactoryCell (const char* name, 
    int gridWidth, int gridHeight, int materialMapWidth,
    int materialMapHeight, bool materialMapPersistent,
    const csVector2& position, const csVector3& size,
    iTerrainCellRenderProperties* renderProp,
    iTerrainCellCollisionProperties* collisionProp,
    iTerrainCellFeederProperties* feederProp);
  csTerrainFactoryCell (const csTerrainFactoryCell& copyFrom);

  csPtr<csTerrainCell> CreateCell (csTerrainSystem* terrain);

  void SetRenderProperties (iTerrainCellRenderProperties* p)
  { rendererProperties = p; }
  void SetCollisionProperties (iTerrainCellCollisionProperties* p)
  { colliderProperties = p; }
  void SetFeederProperties (iTerrainCellFeederProperties* p)
  { feederProperties = p; }

  // ------------ iTerrainFactoryCell implementation ------------
  virtual iTerrainCellRenderProperties* GetRenderProperties () const;
  virtual iTerrainCellCollisionProperties* GetCollisionProperties () const;
  virtual iTerrainCellFeederProperties* GetFeederProperties () const;

  virtual void SetBaseMaterial (iMaterialWrapper* material);
  virtual void SetAlphaSplatMaterial (iMaterialWrapper* material);
  virtual void SetSplatBaseMaterial (iMaterialWrapper* material);

  virtual const char* GetName() { return name; }
  virtual void SetName (const char* name) { this->name = name; }

  virtual int GetGridWidth () const { return gridWidth; }
  virtual int GetGridHeight () const { return gridHeight; }

  virtual const csVector2& GetPosition () const { return position; }
  virtual const csVector3& GetSize () const { return size; }

  virtual int GetMaterialMapWidth () const { return materialMapWidth; }
  virtual int GetMaterialMapHeight () const { return materialMapHeight; }

  virtual iMaterialWrapper* GetBaseMaterial () const { return baseMaterial; }
  virtual iMaterialWrapper* GetAlphaSplatMaterial () const { return alphaSplatMaterial; }
  virtual iMaterialWrapper* GetSplatBaseMaterial () const { return splatBaseMaterial; }
  virtual bool GetMaterialPersistent() const { return materialMapPersistent; }

  virtual void SetGridWidth (int w) { gridWidth = w; }
  virtual void SetGridHeight (int h) { gridHeight = h; }

  virtual void SetPosition (const csVector2& pos) { position = pos; }
  virtual void SetSize (const csVector3& size) { this->size = size; }

  virtual void SetMaterialMapWidth (int w) { materialMapWidth = w; }
  virtual void SetMaterialMapHeight (int h) { materialMapHeight = h; }

  virtual void SetMaterialPersistent (bool flag) { materialMapPersistent = flag; }

  //---------------------------iModifiable Implementaion-------------------------------------------------
MODIF_DECLARE (8);

    MODIF_GETDESCRIPTION_BEGIN ("CS_TERRAIN_FACTORY_CELL", "Terrain Factory Cell selected");
    MODIF_GETDESCRIPTION (STRING, "NAME", "CellName", "Name of the cell");
    MODIF_GETDESCRIPTION (VECTOR2, "POSITION", "Position", "The position of the cell");
    MODIF_GETDESCRIPTION (VECTOR3, "SIZE", "Size", "The size of the cell");
    MODIF_GETDESCRIPTION (FLOAT, "GRID_WIDTH", "GridWidth", "The grid width");
    MODIF_GETDESCRIPTION (FLOAT, "GRID_HEIGHT", "GridHeight", "The grid height");
    MODIF_GETDESCRIPTION (FLOAT, "MATERIAL_MAP_WIDTH", "MaterialMapWidth", "The material map width for the cell");
    MODIF_GETDESCRIPTION (FLOAT, "MATERIAL_MAP_HEIGHT", "MaterialMapHeight", "The material map height for the cell");
    MODIF_GETDESCRIPTION (BOOL, "MATERIAL_MAP_PERSISTENT", "MateriaMapPersistent", "The material map persistant boolean setting");
    MODIF_GETDESCRIPTION_END ();

    MODIF_GETPARAMETERVALUE_BEGIN ();
    MODIF_GETPARAMETERVALUE (0, String, name);
    MODIF_GETPARAMETERVALUE (1, Vector2, position);
    MODIF_GETPARAMETERVALUE (2, Vector3, size);
    MODIF_GETPARAMETERVALUE (3, Float, gridWidth);
    MODIF_GETPARAMETERVALUE (4, Float, gridHeight);
    MODIF_GETPARAMETERVALUE (5, Float, materialMapWidth);
    MODIF_GETPARAMETERVALUE (6, Float, materialMapHeight);
    MODIF_GETPARAMETERVALUE (7, Bool, materialMapPersistent);
    MODIF_GETPARAMETERVALUE_END ();

    MODIF_SETPARAMETERVALUE_BEGIN ();
    MODIF_SETPARAMETERVALUE (0, String, name);
    MODIF_SETPARAMETERVALUE (1, Vector2, position);
    MODIF_SETPARAMETERVALUE (2, Vector3, size);
    MODIF_SETPARAMETERVALUE (3, Float, gridWidth);
    MODIF_SETPARAMETERVALUE (4, Float, gridHeight);
    MODIF_SETPARAMETERVALUE (5, Float, materialMapWidth);
    MODIF_SETPARAMETERVALUE (6, Float, materialMapHeight);
    MODIF_SETPARAMETERVALUE (7, Bool, materialMapPersistent);
    MODIF_SETPARAMETERVALUE_END ();


private:
  csString name;
  csVector2 position;
  csVector3 size;
  int gridWidth, gridHeight, materialMapWidth, materialMapHeight;
  bool materialMapPersistent;
  csRef<iMaterialWrapper> baseMaterial;
  csRef<iMaterialWrapper> splatBaseMaterial;
  csRef<iMaterialWrapper> alphaSplatMaterial;

  csRef<iTerrainCellRenderProperties> rendererProperties;
  csRef<iTerrainCellCollisionProperties> colliderProperties;
  csRef<iTerrainCellFeederProperties> feederProperties;
};

class csTerrainFactory :
  public scfImplementation3<csTerrainFactory,
                            iMeshObjectFactory,
                            iTerrainFactory, CS::Utility::iModifiable>
{
public:
  csTerrainFactory (iMeshObjectType *pParent);

  virtual ~csTerrainFactory ();

  // ------------ iMeshObjectFactory implementation ------------

  virtual csFlags& GetFlags ();
  virtual csPtr<iMeshObject> NewInstance ();
  virtual csPtr<iMeshObjectFactory> Clone ();
  virtual void HardTransform (const csReversibleTransform& t);
  virtual bool SupportsHardTransform () const;
  virtual void SetMeshFactoryWrapper (iMeshFactoryWrapper* lp);
  virtual iMeshFactoryWrapper* GetMeshFactoryWrapper () const;
  virtual iMeshObjectType* GetMeshObjectType () const;
  virtual iObjectModel* GetObjectModel ();
  virtual bool SetMaterialWrapper (iMaterialWrapper* material);
  virtual iMaterialWrapper* GetMaterialWrapper () const;

  virtual void SetMixMode (uint mode);
  virtual uint GetMixMode () const;

  // ------------ iTerrainFactory implementation ------------
  virtual void SetRenderer (iTerrainRenderer* renderer);
  virtual void SetCollider (iTerrainCollider* collider);
  virtual void SetFeeder (iTerrainDataFeeder* feeder);

  virtual iTerrainRenderer* GetRenderer () { return renderer; }
  virtual iTerrainCollider* GetCollider () { return collider; }
  virtual iTerrainDataFeeder* GetFeeder () { return dataFeeder; }

  virtual size_t GetMaxLoadedCells () { return maxLoadedCells; }

  virtual size_t GetCellCount () { return cells.GetSize(); }
  virtual iTerrainFactoryCell* GetCell (size_t index) { return cells[index]; }
  virtual iTerrainFactoryCell* GetCell (const char* name);

  /// Get pseudo-cell with default settings for all cells
  virtual iTerrainFactoryCell* GetDefaultCell () { return &defaultCell; }

  virtual iTerrainFactoryCell* AddCell (const char* name, 
    int gridWidth, int gridHeight, int materialMapWidth,
    int materialMapHeight, bool materialMapPersistent,
    const csVector2& position, const csVector3& size);
  virtual iTerrainFactoryCell* AddCell ();

  virtual void RemoveCell (iTerrainFactoryCell*);

  virtual void SetMaxLoadedCells (size_t value);
  virtual void SetVirtualViewDistance (float distance);
  virtual void SetAutoPreLoad (bool mode);


  //---------------------------iModifiable Implementaion-------------------------------------------------
    MODIF_DECLARE (3);

    MODIF_GETDESCRIPTION_BEGIN ("CS_TERRAIN_FACTORY", "Terrain Factory selected");
    MODIF_GETDESCRIPTION (FLOAT, "MAX_LOADED_CELLS", "MaxLoadedCells", "Maximum loaded cells in terrain");
    MODIF_GETDESCRIPTION (FLOAT, "VIRTUAL_VIEW_DISTANCE", "VirtualViewDistance", "The virtual view distance for the terrain");
    MODIF_GETDESCRIPTION (BOOL, "AUTO_PRELOAD", "AutoPreload", "Set autopreload on/off");
    MODIF_GETDESCRIPTION_END ();

    MODIF_GETPARAMETERVALUE_BEGIN ();
    MODIF_GETPARAMETERVALUE (0, Float, maxLoadedCells);
    MODIF_GETPARAMETERVALUE (1, Float, virtualViewDistance);
    MODIF_GETPARAMETERVALUE (2, Bool, autoPreLoad);
    MODIF_GETPARAMETERVALUE_END ();

    MODIF_SETPARAMETERVALUE_BEGIN ();
    MODIF_SETPARAMETERVALUE (0, Float, maxLoadedCells);
    MODIF_SETPARAMETERVALUE (1, Float, virtualViewDistance);
    MODIF_SETPARAMETERVALUE (2, Bool, autoPreLoad);
    MODIF_SETPARAMETERVALUE_END ();
 

private:
  csRef<iTerrainRenderer> renderer;
  csRef<iTerrainCollider> collider;
  csRef<iTerrainDataFeeder> dataFeeder;
  csRef<iMeshObjectType> type;

  csTerrainFactoryCell defaultCell;
  csRefArray<csTerrainFactoryCell> cells;

  iMeshFactoryWrapper* logParent;
  csFlags flags;
  size_t maxLoadedCells;
  float virtualViewDistance;
  bool autoPreLoad;
};

}

CS_PLUGIN_NAMESPACE_END(Terrain2)

#endif // __CS_TERRAIN_FACTORY_H__
