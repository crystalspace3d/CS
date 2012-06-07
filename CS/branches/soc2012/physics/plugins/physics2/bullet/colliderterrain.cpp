/*
  Copyright (C) 2011 by Liu Lu

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "cssysdef.h"

#include "csutil/stringquote.h"
#include "igeom/trimesh.h"
#include "imesh/objmodel.h"

// Bullet includes
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "colliderterrain.h"
#include "collisionobject2.h"
#include "bulletsystem.h"

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
  

HeightMapCollider::HeightMapCollider ( float* gridData, 
                                   iTerrainCell *cell, 
                                   float minHeight, float maxHeight,
                                   float internalScale)
                                   : btHeightfieldTerrainShape (cell->GetGridWidth(), cell->GetGridHeight(),
                                   gridData, 1.0f, minHeight, maxHeight,
                                   1, PHY_FLOAT, false),
                                   heightData (gridData)
{
  // Apply the local scaling on the shape
  const csVector3& size = cell->GetSize();
  localScale.setValue (size[0] * internalScale / (cell->GetGridWidth() - 1),
    internalScale,
    size[2] * internalScale/ (cell->GetGridHeight() - 1));
  this->setLocalScaling (localScale);
}

HeightMapCollider::~HeightMapCollider ()
{
  delete heightData;
}

void HeightMapCollider::SetLocalScale (const csVector3& scale)
{
  this->setLocalScaling (btVector3(localScale.getX () * scale.x,
    localScale.getY () * scale.y,
    localScale.getZ () * scale.z));
}

void HeightMapCollider::UpdataMinHeight (float minHeight)
{
  //this->initialize (m_heightStickWidth, m_heightStickLength,
  //  heightData, 1.0f, minHeight, m_maxHeight, 1, PHY_FLOAT, false);

}

void HeightMapCollider::UpdateMaxHeight (float maxHeight)
{
  //this->initialize (m_heightStickWidth, m_heightStickLength,
  //  heightData, 1.0f, m_minHeight, maxHeight, 1, PHY_FLOAT, false);
}

void HeightMapCollider::UpdateHeight(const csRect& area)
{
  int w = cell->GetGridWidth();
  int h = cell->GetGridHeight();
  csLockedHeightData newData = cell->LockHeightData (area);

  for (size_t y = 0; y < size_t (area.Height()); y++)
  {
    for (size_t x = 0; x < size_t (area.Width()); x++)
    {
      heightData[CSToBulletIndex2D(x + area.xmin, y + area.ymin, w, h)] = newData.data[y * w + x];
    }
  }
}

csBulletColliderTerrain::csBulletColliderTerrain (iTerrainSystem* terrain, float minimumHeight,
                                                  float maximumHeight, csBulletSystem* sys)
  : scfImplementationType (this), terrainSystem (terrain), minimumHeight (minimumHeight), 
  maximumHeight (maximumHeight)
{
  collSystem = sys;
  unload = true;

  terrain->AddCellLoadListener (this);
  terrain->AddCellHeightUpdateListener(this);

  // Find the transform of the terrain
  csRef<iMeshObject> mesh = scfQueryInterface<iMeshObject> (terrain);
  terrainTransform = mesh->GetMeshWrapper ()->GetMovable ()->GetFullTransform ();

  if(unload)
  {
    for (size_t i =0; i<terrainSystem->GetCellCount (); i++)
    {
      iTerrainCell* cell = terrainSystem->GetCell (i);

      if (cell->GetLoadState () != iTerrainCell::Loaded)
        continue;
      LoadCellToCollider (cell);       
    }
    unload = true;
  }
  margin = 0.04 * collSystem->getInverseInternalScale ();

  volume = FLT_MAX;
}

csBulletColliderTerrain::~csBulletColliderTerrain ()
{
  for (size_t i = 0; i < colliders.GetSize (); i++)
    delete colliders[i];
  for (size_t i = 0; i < bodies.GetSize (); i++)
    delete bodies[i];
}

void csBulletColliderTerrain::SetLocalScale (const csVector3& scale)
{
  this->scale = scale;
  for (size_t i = 0; i < colliders.GetSize (); i++)
    colliders[i]->SetLocalScale (scale);
}

void csBulletColliderTerrain::SetMargin (float margin)
{
  this->margin = margin;
  for (size_t i = 0; i < colliders.GetSize (); i++)
    colliders[i]->setMargin (margin * collSystem->getInternalScale ());
}

void csBulletColliderTerrain::OnCellLoad (iTerrainCell *cell)
{
  LoadCellToCollider (cell);
}

void csBulletColliderTerrain::OnCellPreLoad (iTerrainCell *cell)
{
}

void csBulletColliderTerrain::OnCellUnload (iTerrainCell *cell)
{
  for (size_t i = 0;i<colliders.GetSize ();i++)
    if (colliders[i]->cell == cell)
    {
      delete colliders[i];
      colliders.DeleteIndexFast (i);
      collSector->bulletWorld->removeRigidBody (bodies[i]);
      delete bodies[i];
      bodies.DeleteIndexFast (i);
      break;
    }
}

void csBulletColliderTerrain::LoadCellToCollider (iTerrainCell *cell)
{
  float minHeight,maxHeight;
  csLockedHeightData cellData = cell->GetHeightData ();
  // Check if the min/max have to be computed
  bool needExtremum =  (minimumHeight == 0.0f && maximumHeight == 0.0f);
  if  (needExtremum)
    minHeight = maxHeight = cellData.data[0];
  int gridWidth = cell->GetGridWidth ();
  int gridHeight = cell->GetGridHeight ();

  // Initialize the bullet terrain height data
  float* btHeightData = new float[gridHeight*gridWidth];
  for (int j=0;j<gridHeight;j++)
  {
    for (int i=0;i<gridWidth;i++)
    {
      float height = btHeightData[CSToBulletIndex2D(i, j, gridWidth, gridHeight)] = cellData.data[j * gridWidth + i];

      if (needExtremum)
      {
        minHeight = csMin (minHeight, height);
        maxHeight = csMax (maxHeight, height);
      }
    }
  }

  csOrthoTransform cellTransform (terrainTransform);
  csVector3 cellPosition  (cell->GetPosition ()[0], 0.0f, cell->GetPosition ()[1]);
  cellTransform.SetOrigin (terrainTransform.GetOrigin ()
    + terrainTransform.This2OtherRelative (cellPosition));

  // Create the terrain shape
  HeightMapCollider* colliderData = new HeightMapCollider (
    btHeightData, cell, minHeight, maxHeight,
    collSystem->getInternalScale ());

  colliders.Push (colliderData);

  csVector3 offset (cell->GetSize ()[0] * 0.5f,
    (maxHeight - minHeight) * 0.5f + minHeight,
    cell->GetSize ()[2] * 0.5f);

  // Set the origin to the middle of the heightfield
  cellTransform.SetOrigin (cellTransform.GetOrigin () + cellTransform.This2OtherRelative (offset));
  btTransform tr = CSToBullet (cellTransform, collSystem->getInternalScale ());

  // Create the rigid body and add it to the world
  btRigidBody* body = new btRigidBody (0, 0, colliderData, btVector3 (0, 0, 0));
  body->setWorldTransform (tr);

  if (collSector)
    collSector->bulletWorld->addRigidBody (body);
  if (collBody)
    body->setUserPointer (static_cast<CS::Collisions::iCollisionObject*> (collBody));
  bodies.Push (body);
}

// height in the given rectangle of the given cell has changed
void csBulletColliderTerrain::OnHeightUpdate (iTerrainCell* cell, const csRect& rectangle) 
{
  // find cell collider:
  HeightMapCollider* collider = GetCellCollider(cell);
  
  // this method is actually fired prior to the load event
  if (collider)
  {
    collider->UpdateHeight(rectangle);
  }
}

HeightMapCollider* csBulletColliderTerrain::GetCellCollider(iTerrainCell* cell)
{
  for (int i = 0; i < (int)colliders.GetSize(); ++i)
  {
    HeightMapCollider* collider = colliders[i];

    if (collider->cell == cell) return collider;
  }
  return nullptr;
}

void csBulletColliderTerrain::RemoveRigidBodies ()
{
  for (size_t i = 0; i < colliders.GetSize (); i++)
  {
    collSector->bulletWorld->removeRigidBody (bodies[i]);
    /*delete bodies[i];
    bodies.Empty ();*/
  }
}

void csBulletColliderTerrain::AddRigidBodies (csBulletSector* sector, csBulletCollisionObject* body)
{
  collSector = sector;
  collBody = body;
  for (size_t i = 0; i < bodies.GetSize (); i++)
  {
    iTerrainCell* cell = terrainSystem->GetCell (i);
    if (cell->GetLoadState () != iTerrainCell::Loaded)
      continue;
    bodies[i]->setUserPointer (static_cast<CS::Collisions::iCollisionObject*> (body));
    sector->bulletWorld->addRigidBody (bodies[i], body->collGroup.value, body->collGroup.mask);
  }
}
}
CS_PLUGIN_NAMESPACE_END(Bullet2)