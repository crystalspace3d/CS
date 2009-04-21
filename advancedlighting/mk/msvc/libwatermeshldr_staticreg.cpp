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
static char const metainfo_watermeshldr[] =
"<?xml version=\"1.0\"?>"
"<!-- watermeshldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.loader.factory.watermesh</name>"
"        <implementation>csWaterFactoryLoader</implementation>"
"        <description>Crystal Space Water Mesh Factory Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.loader.watermesh</name>"
"        <implementation>csWaterMeshLoader</implementation>"
"        <description>Crystal Space Water Mesh Mesh Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.factory.watermesh</name>"
"        <implementation>csWaterFactorySaver</implementation>"
"        <description>Crystal Space Water Mesh Factory Saver</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.watermesh</name>"
"        <implementation>csWaterMeshSaver</implementation>"
"        <description>Crystal Space Water Mesh Mesh Saver</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csWaterFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csWaterFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWaterFactoryLoader) 
  #endif
  #ifndef csWaterMeshLoader_FACTORY_REGISTER_DEFINED 
  #define csWaterMeshLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWaterMeshLoader) 
  #endif
  #ifndef csWaterFactorySaver_FACTORY_REGISTER_DEFINED 
  #define csWaterFactorySaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWaterFactorySaver) 
  #endif
  #ifndef csWaterMeshSaver_FACTORY_REGISTER_DEFINED 
  #define csWaterMeshSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWaterMeshSaver) 
  #endif

class watermeshldr
{
SCF_REGISTER_STATIC_LIBRARY(watermeshldr,metainfo_watermeshldr)
  #ifndef csWaterFactoryLoader_FACTORY_REGISTERED 
  #define csWaterFactoryLoader_FACTORY_REGISTERED 
    csWaterFactoryLoader_StaticInit csWaterFactoryLoader_static_init__; 
  #endif
  #ifndef csWaterMeshLoader_FACTORY_REGISTERED 
  #define csWaterMeshLoader_FACTORY_REGISTERED 
    csWaterMeshLoader_StaticInit csWaterMeshLoader_static_init__; 
  #endif
  #ifndef csWaterFactorySaver_FACTORY_REGISTERED 
  #define csWaterFactorySaver_FACTORY_REGISTERED 
    csWaterFactorySaver_StaticInit csWaterFactorySaver_static_init__; 
  #endif
  #ifndef csWaterMeshSaver_FACTORY_REGISTERED 
  #define csWaterMeshSaver_FACTORY_REGISTERED 
    csWaterMeshSaver_StaticInit csWaterMeshSaver_static_init__; 
  #endif
public:
 watermeshldr();
};
watermeshldr::watermeshldr() {}

}
