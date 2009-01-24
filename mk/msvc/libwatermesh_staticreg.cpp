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
static char const metainfo_watermesh[] =
"<?xml version=\"1.0\"?>"
"<!-- watermesh.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.watermesh</name>"
"        <implementation>csWaterMeshObjectType</implementation>"
"        <description>Crystal Space Water Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csWaterMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csWaterMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWaterMeshObjectType) 
  #endif

class watermesh
{
SCF_REGISTER_STATIC_LIBRARY(watermesh,metainfo_watermesh)
  #ifndef csWaterMeshObjectType_FACTORY_REGISTERED 
  #define csWaterMeshObjectType_FACTORY_REGISTERED 
    csWaterMeshObjectType_StaticInit csWaterMeshObjectType_static_init__; 
  #endif
public:
 watermesh();
};
watermesh::watermesh() {}

}
