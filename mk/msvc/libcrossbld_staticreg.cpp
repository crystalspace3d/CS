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
static char const metainfo_crossbld[] =
"<?xml version=\"1.0\"?>"
"<!-- crossbld.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.crossbuilder</name>"
"        <implementation>csCrossBuilder</implementation>"
"        <description>Modeldata-to-Meshobject cross builder</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csCrossBuilder_FACTORY_REGISTER_DEFINED 
  #define csCrossBuilder_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csCrossBuilder) 
  #endif

class crossbld
{
SCF_REGISTER_STATIC_LIBRARY(crossbld,metainfo_crossbld)
  #ifndef csCrossBuilder_FACTORY_REGISTERED 
  #define csCrossBuilder_FACTORY_REGISTERED 
    csCrossBuilder_StaticInit csCrossBuilder_static_init__; 
  #endif
public:
 crossbld();
};
crossbld::crossbld() {}

}
