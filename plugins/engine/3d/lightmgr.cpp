/*
    Copyright (C) 2004 by Jorrit Tyberghein

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
#include "plugins/engine/3d/light.h"
#include "plugins/engine/3d/lightmgr.h"
#include "plugins/engine/3d/meshobj.h"
#include "plugins/engine/3d/sector.h"

#include "csgeom/sphere.h"

using namespace CS_PLUGIN_NAMESPACE_NAME(Engine);

static csLightInfluence MakeInfluence (csLight* light)
{
  csLightInfluence l;
  l.light = light;
  if (light != 0)
  {
    l.type = light->csLight::GetType();
    l.flags = light->csLight::GetFlags();
    l.dynamicType = light->csLight::GetDynamicType();
  }
  return l;
}

// ---------------------------------------------------------------------------

csLightManager::csLightManager ()
  : scfImplementationType (this), tempInfluencesUsed (false)
{
}

csLightManager::~csLightManager ()
{
}

void csLightManager::GetRelevantLights (iMeshWrapper* meshObject, 
    iLightInfluenceArray* lightArray, int maxLights, uint flags)
{
  const csBox3& meshBox = meshObject->GetMeshObject()
    ->GetObjectModel()->GetObjectBoundingBox();
  csReversibleTransform objectToWorld =
    meshObject->GetMovable()->GetFullTransform();
  iSectorList* sectors = meshObject->GetMovable ()->GetSectors ();
  if (sectors && sectors->GetCount() > 0)
  {
    GetRelevantLights (sectors->Get (0), meshBox, lightArray, maxLights,
      &objectToWorld, flags);
  }
}

void csLightManager::GetRelevantLights (iMeshWrapper* meshObject, 
  iLightInfluenceCallback* lightCallback, int maxLights, 
  uint flags)
{
  const csBox3& meshBox = meshObject->GetMeshObject()
    ->GetObjectModel()->GetObjectBoundingBox();
  csReversibleTransform objectToWorld =
    meshObject->GetMovable()->GetFullTransform();
  iSectorList* sectors = meshObject->GetMovable ()->GetSectors ();
  if (sectors && sectors->GetCount() > 0)
  {
    GetRelevantLights (sectors->Get (0), meshBox, lightCallback, maxLights,
      &objectToWorld, flags);
  }
}


void csLightManager::GetRelevantLights (iSector* sector, 
  iLightInfluenceArray* lightArray, int maxLights, 
  uint flags)
{
  const csBox3 bigBox (csVector3 (-CS_BOUNDINGBOX_MAXVALUE),
    csVector3 (CS_BOUNDINGBOX_MAXVALUE));
  GetRelevantLights (sector, bigBox, lightArray, maxLights, 0, flags);
}

void csLightManager::GetRelevantLights (iSector* sector, 
  iLightInfluenceCallback* lightCallback, int maxLights, 
  uint flags)
{
  const csBox3 bigBox (csVector3 (-CS_BOUNDINGBOX_MAXVALUE),
    csVector3 (CS_BOUNDINGBOX_MAXVALUE));
  GetRelevantLights (sector, bigBox, lightCallback, maxLights, 0, flags);
}

struct BoxSpaceIdentity
{
  csSphere FromWorld (const csSphere& sphere) const { return sphere; }
  csBox3 ToWorld (const csBox3& box) const { return box; }
};

struct BoxSpaceTransform
{
  BoxSpaceTransform (const csReversibleTransform& tf) : tf (tf) {}

  csSphere FromWorld (const csSphere& sphere) const
  { return tf.Other2This (sphere); }
  csBox3 ToWorld (const csBox3& box) const
  { return tf.This2Other (box); }
protected:
  const csReversibleTransform& tf;
};

struct IntersectInnerBBox
{
  IntersectInnerBBox (const csBox3& box)
    : testBox (box)
  {}

  bool operator() (const csSectorLightList::LightAABBTree::Node* node)
  {
    return testBox.TestIntersect (node->GetBBox ());
  }

  const csBox3& testBox;
};

template<typename BoxSpace>
struct LightCollectArray
{
  LightCollectArray (const BoxSpace& boxSpace,
    const csBox3& box, const csBox3& boxWorld,
    iLightInfluenceArray* lightArray)
    : boxSpace (boxSpace), testBox (box), testBoxWorld (boxWorld),
      lightArray (lightArray)
  {}

  bool operator() (const csSectorLightList::LightAABBTree::Node* node)
  {
    if (!testBoxWorld.TestIntersect (node->GetBBox ()))
      return true;

    for (size_t i = 0; i < node->GetObjectCount (); ++i)
    {
      csLight* light = node->GetLeafData (i);
      
      csSphere lightSphere (light->GetMovable()->GetFullPosition(),
        light->GetCutoffDistance());
      lightSphere = boxSpace.FromWorld (lightSphere);
      if (!csIntersect3::BoxSphere (testBox, lightSphere.GetCenter(),
          lightSphere.GetRadius()*lightSphere.GetRadius()))
        continue;
      
      csLightInfluence newInfluence = MakeInfluence (light);
      lightArray->Push (newInfluence);
    }
      
    return true;
  }

  const BoxSpace& boxSpace;
  const csBox3& testBox;
  const csBox3& testBoxWorld;
  iLightInfluenceArray* lightArray;
};

template<typename BoxSpace>
struct LightCollectCallback
{
  LightCollectCallback (const BoxSpace& boxSpace,
    const csBox3& box, const csBox3& boxWorld,
    iLightInfluenceCallback* lightCallback)
    : boxSpace (boxSpace), testBox (box), testBoxWorld (boxWorld),
      lightCallback (lightCallback)
  {}

  bool operator() (const csSectorLightList::LightAABBTree::Node* node)
  {
    if (!testBoxWorld.TestIntersect (node->GetBBox ()))
      return true;

    for (size_t i = 0; i < node->GetObjectCount (); ++i)
    {
      csLight* light = node->GetLeafData (i);
      
      csSphere lightSphere (light->GetMovable()->GetFullPosition(),
        light->GetCutoffDistance());
      lightSphere = boxSpace.FromWorld (lightSphere);
      if (!csIntersect3::BoxSphere (testBox, lightSphere.GetCenter(),
          lightSphere.GetRadius()*lightSphere.GetRadius()))
        continue;
      
      csLightInfluence newInfluence = MakeInfluence (light);
      lightCallback->LightInfluence (newInfluence);
    }
    return true;
  }

  const BoxSpace& boxSpace;
  const csBox3& testBox;
  const csBox3& testBoxWorld;
  iLightInfluenceCallback* lightCallback;
};


void csLightManager::GetRelevantLights (iSector* sector, const csBox3& boundingBox,
  iLightInfluenceArray* lightArray, int maxLights, 
  const csReversibleTransform* bboxToWorld, uint flags)
{
  iLightList* llist = sector->GetLights ();
  csSectorLightList* sectorLightList = static_cast<csSectorLightList*> (llist);

  // Get the primary lights from same sector
  const csSectorLightList::LightAABBTree& aabbTree = sectorLightList->GetLightAABBTree ();
  if (!bboxToWorld || bboxToWorld->IsIdentity())
  {
    BoxSpaceIdentity boxSpace;
    IntersectInnerBBox inner (boundingBox);
    LightCollectArray<BoxSpaceIdentity> leaf (boxSpace, boundingBox, 
      boundingBox, lightArray);
    aabbTree.Traverse (inner, leaf);
  }
  else
  {
    BoxSpaceTransform boxSpace (*bboxToWorld);
    csBox3 boxWorld (boxSpace.ToWorld (boundingBox));
    IntersectInnerBBox inner (boxWorld);
    LightCollectArray<BoxSpaceTransform> leaf (boxSpace, boundingBox, 
      boxWorld, lightArray);
    aabbTree.Traverse (inner, leaf);
  } 

  //@@TODO: Implement cross-sector lookups
}

void csLightManager::GetRelevantLights (iSector* sector, const csBox3& boundingBox,
  iLightInfluenceCallback* lightCallback, int maxLights, 
  const csReversibleTransform* bboxToWorld, uint flags)
{
  iLightList* llist = sector->GetLights ();
  csSectorLightList* sectorLightList = static_cast<csSectorLightList*> (llist);

  // Get the primary lights from same sector
  const csSectorLightList::LightAABBTree& aabbTree = sectorLightList->GetLightAABBTree ();
  if (!bboxToWorld || bboxToWorld->IsIdentity())
  {
    BoxSpaceIdentity boxSpace;
    IntersectInnerBBox inner (boundingBox);
    LightCollectCallback<BoxSpaceIdentity> leaf (boxSpace, boundingBox, 
      boundingBox, lightCallback);
    aabbTree.Traverse (inner, leaf);
  }
  else
  {
    BoxSpaceTransform boxSpace (*bboxToWorld);
    csBox3 boxWorld (boxSpace.ToWorld (boundingBox));
    IntersectInnerBBox inner (boxWorld);
    LightCollectCallback<BoxSpaceTransform> leaf (boxSpace, boundingBox, 
      boxWorld, lightCallback);
    aabbTree.Traverse (inner, leaf);
  } 

}

// ---------------------------------------------------------------------------

void csLightManager::FreeInfluenceArray (csLightInfluence* Array)
{
  if (Array == 0) return;

  if (tempInfluencesUsed && (Array == tempInfluences.GetArray()))
  {
    tempInfluences.Empty();
    tempInfluencesUsed = false;
  }
  else
    cs_free (Array);
}

template<typename T>
class csDirtyAccessArrayDetach : 
  public csDirtyAccessArray<T, csArrayElementHandler<T>,
                            CS::Memory::AllocatorMalloc>
{
public:
  T* Detach()
  {
    T* p = this->GetArray();
    this->SetData (0);
    return p;
  }
};

typedef csDirtyAccessArrayDetach<csLightInfluence> LightInfluenceArray;

template<typename ArrayType, typename BoxSpace>
struct LightCollectArrayPtr
{
  LightCollectArrayPtr (const BoxSpace& boxSpace,
    const csBox3& box, const csBox3& boxWorld,
    ArrayType& arr, size_t max)
    : boxSpace (boxSpace), testBox (box), testBoxWorld (boxWorld),
      arr (arr), max (max)
  {}

  bool operator() (const csSectorLightList::LightAABBTree::Node* node)
  {
    if (!testBoxWorld.TestIntersect (node->GetBBox ()))
      return true;

    for (size_t i = 0; i < node->GetObjectCount (); ++i)
    {
      csLight* light = node->GetLeafData (i);
      
      csSphere lightSphere (light->GetMovable()->GetFullPosition(),
        light->GetCutoffDistance());
      lightSphere = boxSpace.FromWorld (lightSphere);
      if (!csIntersect3::BoxSphere (testBox, lightSphere.GetCenter(),
          lightSphere.GetRadius()*lightSphere.GetRadius()))
        continue;
      
      if (arr.GetSize() < max)
      {
        csLightInfluence newInfluence = MakeInfluence (light);
        arr.Push (newInfluence);
      }
      else
        return false;
    }
    return true;
  }

  const BoxSpace& boxSpace;
  const csBox3& testBox;
  const csBox3& testBoxWorld;
  ArrayType& arr;
  size_t max;
};


void csLightManager::GetRelevantLights (iMeshWrapper* meshObject,
                                        csLightInfluence*& lightArray, 
                                        size_t& numLights, 
                                        size_t maxLights, uint flags)
{
  const csBox3& meshBox = meshObject->GetMeshObject()
    ->GetObjectModel()->GetObjectBoundingBox();
  csReversibleTransform objectToWorld =
    meshObject->GetMovable()->GetFullTransform();
  iSectorList* sectors = meshObject->GetMovable ()->GetSectors ();
  if (sectors && sectors->GetCount() > 0)
  {
    csLightManager::GetRelevantLights (sectors->Get (0), meshBox, lightArray,
      numLights, maxLights, &objectToWorld, flags);
  }
}

template<typename BoxSpace>
void csLightManager::GetRelevantLightsWorker (const BoxSpace& boxSpace,
                                              iSector* sector, 
                                              const csBox3& boundingBox,
                                              csLightInfluence*& lightArray, 
                                              size_t& numLights,
                                              size_t maxLights,
                                              uint flags)
{
  iLightList* llist = sector->GetLights ();
  csSectorLightList* sectorLightList = static_cast<csSectorLightList*> (llist);

  // Get the primary lights from same sector
  //@@TODO: Implement cross-sector lookups
  const csSectorLightList::LightAABBTree& aabbTree = sectorLightList->GetLightAABBTree ();
  csBox3 boxWorld (boxSpace.ToWorld (boundingBox));
  IntersectInnerBBox inner (boxWorld);
  if (!tempInfluencesUsed)
  {
    tempInfluencesUsed = true;
    LightCollectArrayPtr<TempInfluences, BoxSpace> leaf (boxSpace,
      boundingBox, boxWorld, tempInfluences, maxLights);
    aabbTree.Traverse (inner, leaf);
    
    numLights = tempInfluences.GetSize();
    if (numLights > 0)
      lightArray = tempInfluences.GetArray();
    else
    {
      lightArray = 0;
      tempInfluencesUsed = false;
    }
  }
  else
  {
    LightInfluenceArray tmpLightArray;
    LightCollectArrayPtr<LightInfluenceArray, BoxSpace> leaf (boxSpace,
      boundingBox, boxWorld, tmpLightArray, maxLights);
    aabbTree.Traverse (inner, leaf);
    
    numLights = tmpLightArray.GetSize();
    if (numLights > 0)
      lightArray = tmpLightArray.Detach();
    else
      lightArray = 0;
  }
}

void csLightManager::GetRelevantLights (iSector* sector, 
                                        const csBox3& boundingBox, 
                                        csLightInfluence*& lightArray, 
                                        size_t& numLights, 
                                        size_t maxLights,
                                        const csReversibleTransform* bboxToWorld,
                                        uint flags)
{
  if (!bboxToWorld || bboxToWorld->IsIdentity())
  {
    BoxSpaceIdentity boxspace;
    GetRelevantLightsWorker (boxspace, sector, boundingBox, lightArray, numLights,
      maxLights, flags);
  }
  else
  {
    BoxSpaceTransform boxspace (*bboxToWorld);
    GetRelevantLightsWorker (boxspace, sector, boundingBox, lightArray, numLights,
      maxLights, flags);
  } 
}
  
void csLightManager::GetRelevantLights (iSector* sector, 
                                        csLightInfluence*& lightArray, 
                                        size_t& numLights, 
                                        size_t maxLights, uint flags)
{
  const csBox3 bigBox;  
  csLightManager::GetRelevantLights (sector, bigBox, lightArray, numLights,
    maxLights, 0, flags);
}
