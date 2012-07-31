/**
 * Create different kinds of geometry
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"
#include "physdemo.h"

#include "ivaria/colliders.h"

using namespace CS::Collisions;
using namespace CS::Geometry;


csPtr<iMeshWrapper> PhysDemo::CreateCylinderYMesh(csScalar length, csScalar radius, const char* materialName, const char* meshName)
{
  csRef<iMeshFactoryWrapper> cylinderFact = engine->CreateMeshFactory("crystalspace.mesh.object.genmesh", "cylinderFact");
  if (!cylinderFact)
  {
    ReportError ("Error creating mesh object factory!");
    return csPtr<iMeshWrapper> (nullptr);
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<iGeneralFactoryState> (cylinderFact->GetMeshObjectFactory());
  gmstate->GenerateCylinder (length, radius, 10);

  csVector3 artificialOffset (0, 0, 0);
  //csReversibleTransform hardTransform (csZRotMatrix3 (PI/2.0), artificialOffset);   // cylinder meshes are X-oriented
  csReversibleTransform hardTransform;
  cylinderFact->HardTransform (hardTransform);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (cylinderFact, meshName));

  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName (materialName);
  if (!mat)
  {
    ReportError ("Error - Invalid material: %s", materialName);
    return csPtr<iMeshWrapper> (nullptr);
  }
  mesh->GetMeshObject()->SetMaterialWrapper (mat);
  
  return csPtr<iMeshWrapper>(mesh);
}

csPtr<iMeshWrapper> PhysDemo::CreateBoxMesh(const csVector3& extents, const char* materialName, const char* meshName)
{ 
  DensityTextureMapper mapper (0.3f);
  TesselatedBox tbox (-extents/2, extents/2);
  tbox.SetLevel (3);
  tbox.SetMapper (&mapper);
  
  csRef<iMeshFactoryWrapper> fact = GeneralMeshBuilder::CreateFactory(engine, "box", &tbox);
  iMaterialWrapper* mat = engine->GetMaterialList()->FindByName (materialName);
  csRef<iMeshWrapper> mesh = fact->CreateMeshWrapper();
  mesh->QueryObject()->SetName(meshName);
  mesh->GetMeshObject()->SetMaterialWrapper (mat);
  return csPtr<iMeshWrapper>(mesh);
}
