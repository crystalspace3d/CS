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
static char const metainfo_haze[] =
"<?xml version=\"1.0\"?>"
"<!-- haze.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.haze</name>"
"        <implementation>csHazeMeshObjectType</implementation>"
"        <description>Crystal Space Haze Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csHazeMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csHazeMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csHazeMeshObjectType) 
  #endif

class haze
{
SCF_REGISTER_STATIC_LIBRARY(haze,metainfo_haze)
  #ifndef csHazeMeshObjectType_FACTORY_REGISTERED 
  #define csHazeMeshObjectType_FACTORY_REGISTERED 
    csHazeMeshObjectType_StaticInit csHazeMeshObjectType_static_init__; 
  #endif
public:
 haze();
};
haze::haze() {}

}
