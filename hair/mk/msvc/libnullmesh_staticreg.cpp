// This file is automatically generated.
#include "cssysdef.h"
#include "csutil/scf.h"

// Put static linking stuff into own section.
// The idea is that this allows the section to be swapped out but not
// swapped in again b/c something else in it was needed.
#if !defined(CS_DEBUG) && defined(CS_COMPILER_MSVC)
#pragma const_seg(".CSmetai")
#pragma comment(linker, "/section:.CSmetai,r")
#pragma code_seg(".CSmeta")
#pragma comment(linker, "/section:.CSmeta,er")
#pragma comment(linker, "/merge:.CSmetai=.CSmeta")
#endif

namespace csStaticPluginInit
{
static char const metainfo_nullmesh[] =
"<?xml version=\"1.0\"?>"
"<!-- nullmesh.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.null</name>"
"        <implementation>csNullmeshMeshObjectType</implementation>"
"        <description>Crystal Space Null Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csNullmeshMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csNullmeshMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csNullmeshMeshObjectType) 
  #endif

class nullmesh
{
SCF_REGISTER_STATIC_LIBRARY(nullmesh,metainfo_nullmesh)
  #ifndef csNullmeshMeshObjectType_FACTORY_REGISTERED 
  #define csNullmeshMeshObjectType_FACTORY_REGISTERED 
    csNullmeshMeshObjectType_StaticInit csNullmeshMeshObjectType_static_init__; 
  #endif
public:
 nullmesh();
};
nullmesh::nullmesh() {}

}
