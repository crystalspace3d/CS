/**
 * This code is a (very very) basic implementation of an item and weapon system.
 */

#ifndef __PHYSTUT2_ITEMS_H__
#define __PHYSTUT2_ITEMS_H__

#include "ivaria/physics.h"
#include "ivaria/collisions.h"

#include "imesh/animesh.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/modifiableterrain.h"

#include "iengine/movable.h"

#include "csutil/scf.h"
#include "csutil/list.h"
#include "csutil/csobject.h"

static const int MaxFunctionsPerItem = 14;

class Agent;

class Item;
class ItemTemplate;
class ItemMgr;

// #########################################################################################################################
// Inventory

/**
 * An inventory is a collection of all items that belong to some object
 */
class Inventory
{
  Agent* owner;
  csRefArray<Item> items;

public:
  Inventory()
  {
  }
  
  Agent* GetOwner() const { return owner; }
  void SetOwner(Agent* o) { owner = o; }
  
  csRefArray<Item> GetItems() { return items; }
  
  Item* GetItem(int i) { return items[i]; }

  Item* AddItem(ItemTemplate& templ);
};

// #########################################################################################################################
// Items

/**
 * One function of an item
 */
class ItemFunction
{
protected:
  csString name;

public:
  ItemFunction(const csString& name) : name(name) {}
  const csString& GetName() const { return name; }

  /// Tries to use the item. Returns false, if function could not be used.
  virtual bool Use(Item* item) = 0;
};


/**
 * An ItemTemplate is shared by all instances of the type of item it represents.
 * That means for every type of item, you only need one global instance of ItemTemplate.
 */
class ItemTemplate
{
  friend class Item;
  friend class ItemMgr;
private:
  int index;
  csString name;
  
protected:
  csArray<ItemFunction*> primaryFunctions;
  csArray<ItemFunction*> secondaryFunctions;

public:
  ItemTemplate()
  {
  }

  void AddPrimaryFunction(ItemFunction* function)
  {
    primaryFunctions.Push(function);
  }

  void AddSecondaryFunction(ItemFunction* function)
  {
    secondaryFunctions.Push(function);
  }

  /// The index within the ItemMgr's array of all ItemTemplates
  int GetIndex() const { return index; }
  
  const csArray<ItemFunction*>& GetPrimaryFunctions() const { return primaryFunctions; }
  const csArray<ItemFunction*>& GetSecondaryFunctions() const { return secondaryFunctions; }

  ItemFunction* GetPrimaryFunction(int i) { return primaryFunctions[i]; }
  ItemFunction* GetSecondaryFunction(int i) { return secondaryFunctions[i]; }

  int GetFunctionCount() const { return primaryFunctions.GetSize() + secondaryFunctions.GetSize(); }
  
  const csString& GetName() const { return name; }
};

/**
 * Represents a stack of items (with only one by default), attached to an iCollisionObject.
 * Items are just movables that have a specific set of functions which can be used by actors.
 */
class Item : public scfVirtImplementationExt1<Item, csObject, iBase>
{
  friend class ItemMgr;

protected:
  Inventory* inventory;
  int amount;
  ItemTemplate* templ;
  csOrthoTransform relativeTransform;

  void InitItem(ItemTemplate* t)
  {
    templ = t;
  }

public:
  Item();

  /// Get the inventory to which this item currently belongs
  inline Inventory* GetInventory() const { return inventory; }
  /// Set the inventory to which this item currently belongs
  inline void SetInventory(Inventory* inv) { inventory = inv; }

  inline int GetAmount() const { return amount; }
  inline void SetAmount(int value) { amount = value; }
  
  inline ItemTemplate& GetTemplate() { return *templ; }

  /// The name of this item
  virtual const char* GetName() const { return templ->GetName().GetData(); }

  inline Agent* GetOwner() const;

  /// Returns the sector that this item belongs to
  inline CS::Physics::iPhysicalSector* GetSector() const;

  /// Returns the system that this item belongs to
  inline CS::Physics::iPhysicalSystem* GetSystem() const;

  /// Transform of this item in world space
  inline csReversibleTransform GetFullTransform() const;

};

/**
 * Manages templates
 */
class ItemMgr
{
public:
  static ItemMgr* Instance;
  
public:
  csArray<ItemTemplate> Templates;

public:
  ItemMgr() 
  {
    Instance = this;
  }

public:
  csPtr<Item> CreateItem(ItemTemplate& templ);

  ItemTemplate& CreateTemplate(const csString& name);

  size_t GetTemplateCount() { return Templates.GetSize(); }

  ItemTemplate& GetTemplate(size_t i) { return Templates[i]; }
};

// #########################################################################################################################
// Weapons & Projectiles

/**
 * Defines one sort of projectile
 */
class ProjectileTemplate
{
protected:
  // TODO: Need some sort of representation? 
  // Maybe a mesh, maybe a billboard, for big projectiles a combination of collider and mesh
  csScalar speed;

public:
  ProjectileTemplate(csScalar speed);

  csScalar GetSpeed() const { return speed; }
};

/**
 * Projectile launchers are a function for items that launches projectiles
 */
class ProjectileLauncher : public ItemFunction
{
protected:
  ProjectileTemplate& projectileTemplate;

public:
  ProjectileLauncher(const csString& name, ProjectileTemplate& projectileTemplate) : 
      ItemFunction(name),
        projectileTemplate(projectileTemplate)
  {
  }
  
  ProjectileTemplate& GetProjectileTemplate() { return projectileTemplate; }
  const ProjectileTemplate& GetProjectileTemplate() const { return projectileTemplate; }

  virtual bool Use(Item* item);
};

/**
 * Information that is attached to an iRigidBody which represents the actual projectile 
 * TODO
 */
class ProjectileInfo // : CS::Collisions::iCollisionObject
{
protected:
public:
};

/**
 * A weapon is an item that can fire projectiles.
 */
class Weapon : public Item
{
protected:
public:
  Weapon() :
      Item()
  {
  }
};

#endif