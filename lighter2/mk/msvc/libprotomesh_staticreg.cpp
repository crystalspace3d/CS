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
static char const metainfo_protomesh[] =
"<?xml version=\"1.0\"?>"
"<!-- protomesh.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.protomesh</name>"
"        <implementation>csProtoMeshObjectType</implementation>"
"        <description>Crystal Space Proto Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csProtoMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csProtoMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csProtoMeshObjectType) 
  #endif

class protomesh
{
SCF_REGISTER_STATIC_LIBRARY(protomesh,metainfo_protomesh)
  #ifndef csProtoMeshObjectType_FACTORY_REGISTERED 
  #define csProtoMeshObjectType_FACTORY_REGISTERED 
    csProtoMeshObjectType_StaticInit csProtoMeshObjectType_static_init__; 
  #endif
public:
 protomesh();
};
protomesh::protomesh() {}

}
