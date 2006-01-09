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
static char const metainfo_rain[] =
"<?xml version=\"1.0\"?>"
"<!-- rain.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.rain</name>"
"        <implementation>csRainPlugin</implementation>"
"        <description>Crystal Space Rain Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csRainPlugin_FACTORY_REGISTER_DEFINED 
  #define csRainPlugin_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csRainPlugin) 
  #endif

class rain
{
SCF_REGISTER_STATIC_LIBRARY(rain,metainfo_rain)
  #ifndef csRainPlugin_FACTORY_REGISTERED 
  #define csRainPlugin_FACTORY_REGISTERED 
    csRainPlugin_StaticInit csRainPlugin_static_init__; 
  #endif
public:
 rain();
};
rain::rain() {}

}
