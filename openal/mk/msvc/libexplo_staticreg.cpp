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
static char const metainfo_explo[] =
"<?xml version=\"1.0\"?>"
"<!-- explo.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.explosion</name>"
"        <implementation>csExploMeshObjectType</implementation>"
"        <description>Crystal Space Explosion Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csExploMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csExploMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csExploMeshObjectType) 
  #endif

class explo
{
SCF_REGISTER_STATIC_LIBRARY(explo,metainfo_explo)
  #ifndef csExploMeshObjectType_FACTORY_REGISTERED 
  #define csExploMeshObjectType_FACTORY_REGISTERED 
    csExploMeshObjectType_StaticInit csExploMeshObjectType_static_init__; 
  #endif
public:
 explo();
};
explo::explo() {}

}
