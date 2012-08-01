/**
 * A very basic Agent implementation.
 */

#ifndef __PHYSTUT2_AGENT_H
#define __PHYSTUT2_AGENT_H

// TODO: Need some sort of new WorldPosition class that just gives a world transform for an item, or similar
//   get rid of item owner?
// TODO: Figure out ownership relation between items, objects, agents and actors

#include "items.h"

/**
 * An agent is either a player or an AI.
 * An agent owns her own inventory.
 * An agent is represented by some iCollisionObject.
 */
class Agent
{
protected:
  Inventory inventory;
  csRef<CS::Collisions::iCollisionObject> object;
  
public:
  Agent()
  {
    inventory.owner = this;
  }

  Inventory& GetInventory() { return inventory; }

  CS::Collisions::iCollisionObject* GetObject() const { return object; }
  void SetObject(CS::Collisions::iCollisionObject* o) { object = o; } 

  CS::Collisions::iActor* GetActor() const { return object ? object->QueryActor() : nullptr; }
};

#endif