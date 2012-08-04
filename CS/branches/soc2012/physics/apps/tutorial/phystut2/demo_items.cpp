/**
 * Create and handle some items that actors can use
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/campos.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"
#include "physdemo.h"

#include "ivaria/colliders.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;


ItemTemplate* camMgr;


#define ItemFunctionClassName(name) name##function

#define ItemFunctionClassNameI(name, i) name##function##i

// Creates a new ItemFunction class with the given name, description and code
// and adds it as a "kind" (Primary or Secondary) function to the ItemTemplate called "templ"
#define AddItemFunction(kind, name, desc, code) \
  AddItemFunctionToTempl(templ, kind, name, desc, code)

#define AddItemFunctionToTempl(templ, kind, name, desc, code) \
  { \
  class ItemFunctionClassName(name) : public ItemFunction { \
  public: ItemFunctionClassName(name)() : ItemFunction(desc) {} \
  bool Use(Item* item) { code; return true; } \
  }; \
  templ.Add##kind##Function(new ItemFunctionClassName(name)); \
  } 
  

// TODO: Cache-friendly storage of polymorphic objects

void PhysDemo::CreateItemTemplates()
{
  {
    // Primitive Object Spawner
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Primitive Object Spawner");
    
    AddItemFunction(Secondary, SpawnSphere, "Spawn Sphere", physDemo.SpawnSphere());
    AddItemFunction(Secondary, SpawnBox, "Spawn Box", physDemo.SpawnBox());
    AddItemFunction(Secondary, SpawnCapsule, "Spawn Capsule", physDemo.SpawnCapsule());
    AddItemFunction(Secondary, SpawnCylinder, "Spawn Cylinder", physDemo.SpawnCylinder());
    AddItemFunction(Secondary, SpawnCone, "Spawn Cone", physDemo.SpawnCone());
    AddItemFunction(Secondary, SpawnBoxStacks, "Spawn Box Stacks", physDemo.SpawnBoxStacks(10, 5, .5, 80));
  }

  {
    // Complex Object Spawner
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Complex Object Spawner");
    
    AddItemFunction(Secondary, SpawnCompound, "Spawn Compound", physDemo.SpawnCompound());
    AddItemFunction(Secondary, SpawnConcaveMesh, "Spawn Concave Mesh", physDemo.SpawnConcaveMesh());
    AddItemFunction(Secondary, SpawnJointed, "Spawn Jointed", physDemo.SpawnJointed());
    AddItemFunction(Secondary, SpawnChain, "Spawn Chain", physDemo.SpawnChain());
    AddItemFunction(Secondary, SpawnKrystalRagdoll, "Spawn Krystal", physDemo.SpawnKrystalRagdoll());
    AddItemFunction(Secondary, SpawnFrankieRagdoll, "Spawn Frankie", physDemo.SpawnFrankieRagdoll());
  }

  if (isSoftBodyWorld)
  {
    // Softbody Object Spawner spawns softbodies, such as cloth or ropes
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Softbody Object Spawner");

    AddItemFunction(Secondary, SpawnRope, "Spawn Rope", physDemo.SpawnRope());
    AddItemFunction(Secondary, SpawnCloth, "Spawn Cloth", physDemo.SpawnCloth());
    AddItemFunction(Secondary, SpawnSoftBody, "Spawn Soft Ball", physDemo.SpawnSoftBody());
  }
  
  {
    // Object & Force Manipulator: Manipulates objects and introduces fun forces
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Object & Force Manipulator");

    AddItemFunction(Secondary, PullObject, "Pull Object", physDemo.PullObject());
    AddItemFunction(Secondary, DeleteObject, "Delete Object", physDemo.DeleteObject());
    AddItemFunction(Secondary, ToggleObjectDynamic, "Toggle Dynamic", physDemo.ToggleObjectDynamic());
  }

  {
    // Vehicle Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Vehicle Manager");
    
    AddItemFunction(Primary, EnterTargetVehicle, "Enter Target Vehicle", physDemo.EnterTargetVehicle());
    AddItemFunction(Primary, DeleteTargetVehicle, "Delete Target Vehicle", physDemo.DeleteTargetVehicle());
    
    AddItemFunction(Secondary, AccelerateVehicle, "Leave Current Vehicle", physDemo.LeaveCurrentVehicle());
    AddItemFunction(Secondary, SpawnVehicle, "Spawn New Vehicle", physDemo.SpawnVehicle());
    AddItemFunction(Secondary, AccelerateVehicle, "Accelerate Target Vehicle", physDemo.AccelerateTargetVehicle());
  }

  {
    // Level Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Level Manager");
    
    AddItemFunction(Secondary, SwitchToBox, "Go to Box level", physDemo.SetLevel(PhysDemoLevelBox));
    AddItemFunction(Secondary, SwitchToPortals, "Go to Portals level", physDemo.SetLevel(PhysDemoLevelPortals));
    AddItemFunction(Secondary, SwitchToTerrain, "Go to Terrain level", physDemo.SetLevel(PhysDemoLevelTerrain));
    AddItemFunction(Secondary, SwitchToCastle, "Go to Castle level (static)", physDemo.SetLevel(PhysDemoLevelCastle));
    AddItemFunction(Secondary, SwitchToCastleWithDecomp, "Go to Castle level (dynamic, loads very slowly)", physDemo.SetLevel(PhysDemoLevelCastle, true));
  }

  {
    // Camera Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate("Camera Manager");
    
    // Functions are added dynamically
    camMgr = &templ;
  }
}



class TeleToPosFunction : public ItemFunction 
{
  iCameraPosition* pos;

public:
  TeleToPosFunction(iCameraPosition* pos) : ItemFunction(pos->QueryObject()->GetName()), pos(pos)
  {
  }
  
  bool Use(Item* item)
  { 
    physDemo.TeleportObject(item->GetInventory()->GetOwner()->GetObject(), pos); 
    return true;
  }
};

void PhysDemo::UpdateCameraManager()
{
  camMgr->Clear();

  for (size_t i = 0; i < size_t(engine->GetCameraPositions()->GetCount()); ++i)
  {
    iCameraPosition* pos = engine->GetCameraPositions()->Get(i);

    camMgr->AddSecondaryFunction(new TeleToPosFunction(pos));
  }
}

/**
 * 
 */