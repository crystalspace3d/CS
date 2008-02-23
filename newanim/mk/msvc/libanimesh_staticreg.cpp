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
static char const metainfo_animesh[] =
"<?xml version=\"1.0\"?>"
"<!-- animesh.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.animesh</name>"
"        <implementation>csAnimatedMeshObjectType</implementation>"
"        <description>Crystal Space Animated Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csAnimatedMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csAnimatedMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csAnimatedMeshObjectType) 
  #endif

class animesh
{
SCF_REGISTER_STATIC_LIBRARY(animesh,metainfo_animesh)
  #ifndef csAnimatedMeshObjectType_FACTORY_REGISTERED 
  #define csAnimatedMeshObjectType_FACTORY_REGISTERED 
    csAnimatedMeshObjectType_StaticInit csAnimatedMeshObjectType_static_init__; 
  #endif
public:
 animesh();
};
animesh::animesh() {}

}
