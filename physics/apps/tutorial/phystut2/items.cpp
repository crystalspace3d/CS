/**
 * This code is a (very very) basic implementation of an item and weapon system.
 */

#include "cssysdef.h"
#include "items.h"
#include "agent.h"

using namespace CS::Collisions;
using namespace CS::Physics;

ItemMgr* ItemMgr::Instance = new ItemMgr;

Item* Inventory::AddItem(ItemTemplate& templ)
{
  csRef<Item> item = ItemMgr::Instance->CreateItem(templ);
  item->inventory = this;
  items.Push(item);
  return item;
}

Item::Item() : 
  scfImplementationType(this),
  amount(1)
{
}

csReversibleTransform Item::GetFullTransform() const
{
  if (inventory)
  {
    return relativeTransform * GetOwner()->GetObject()->GetTransform();
  }
  return relativeTransform;
}

Agent* Item::GetOwner() const
{
  return !inventory ? nullptr : inventory->GetOwner();
}

/// Returns the sector that this item belongs to
CS::Physics::iPhysicalSector* Item::GetSector() const 
{
  return !inventory ? nullptr : dynamic_cast<CS::Physics::iPhysicalSector*>(inventory->GetOwner()->GetObject()->GetSector());
}

/// Returns the system that this item belongs to
CS::Physics::iPhysicalSystem* Item::GetSystem() const 
{ 
  return !inventory ? nullptr : dynamic_cast<CS::Physics::iPhysicalSystem*>(inventory->GetOwner()->GetObject()->GetSector()->GetSystem()); 
}


csPtr<Item> ItemMgr::CreateItem(ItemTemplate& templ)
{
  Item* item = new Item();
  item->InitItem(&templ);
  return csPtr<Item>(item);
}

ItemTemplate& ItemMgr::CreateTemplate(const csString& name)
{
  size_t i = Templates.GetSize();
  Templates.SetSize(i + 1);
  ItemTemplate& templ = Templates[i];
  templ.index = i;
  templ.name = name;
  return templ;
}


ProjectileTemplate::ProjectileTemplate(csScalar speed) :
speed(speed)
{

}

bool ProjectileLauncher::Use(Item* item)
{
  // TODO: Find ammunition in inventory
  
  csReversibleTransform trans = item->GetFullTransform();

  csVector3 forward = trans.GetT2O() * csVector3(0, 0, 1);
  forward.Normalize();
  
  // Get starting point and velocity of projectile
  //const csVector3& from(trans.GetOrigin());
  //csVector3 velocity = GetProjectileTemplate().GetSpeed() * forward;
  
  // TODO: Create projectile object & RigidBody

  // TODO: Fire! 
  
  // TODO: Keep track of trajectory using continuous collision detection
  
  return false;
}