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
static char const metainfo_terrainimprovedldr[] =
"<?xml version=\"1.0\"?>"
"<!-- loader.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.loader.factory.terrainimproved</name>"
"        <implementation>csTerrainFactoryLoader</implementation>"
"        <description>Terrain Factory Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.loader.terrainimproved</name>"
"        <implementation>csTerrainObjectLoader</implementation>"
"        <description>Terrain Object Loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csTerrainFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csTerrainFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainFactoryLoader) 
  #endif
  #ifndef csTerrainObjectLoader_FACTORY_REGISTER_DEFINED 
  #define csTerrainObjectLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainObjectLoader) 
  #endif

class terrainimprovedldr
{
SCF_REGISTER_STATIC_LIBRARY(terrainimprovedldr,metainfo_terrainimprovedldr)
  #ifndef csTerrainFactoryLoader_FACTORY_REGISTERED 
  #define csTerrainFactoryLoader_FACTORY_REGISTERED 
    csTerrainFactoryLoader_StaticInit csTerrainFactoryLoader_static_init__; 
  #endif
  #ifndef csTerrainObjectLoader_FACTORY_REGISTERED 
  #define csTerrainObjectLoader_FACTORY_REGISTERED 
    csTerrainObjectLoader_StaticInit csTerrainObjectLoader_static_init__; 
  #endif
public:
 terrainimprovedldr();
};
terrainimprovedldr::terrainimprovedldr() {}

}
