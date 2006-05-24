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
static char const metainfo_snowldr[] =
"<?xml version=\"1.0\"?>"
"<!-- snowldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.loader.factory.snow</name>"
"        <implementation>csSnowFactoryLoader</implementation>"
"        <description>Crystal Space Snow Factory Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.loader.snow</name>"
"        <implementation>csSnowLoader</implementation>"
"        <description>Crystal Space Snow Mesh Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.factory.snow</name>"
"        <implementation>csSnowFactorySaver</implementation>"
"        <description>Crystal Space Snow Factory Saver</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.snow</name>"
"        <implementation>csSnowSaver</implementation>"
"        <description>Crystal Space Snow Mesh Saver</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSnowFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csSnowFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSnowFactoryLoader) 
  #endif
  #ifndef csSnowLoader_FACTORY_REGISTER_DEFINED 
  #define csSnowLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSnowLoader) 
  #endif
  #ifndef csSnowFactorySaver_FACTORY_REGISTER_DEFINED 
  #define csSnowFactorySaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSnowFactorySaver) 
  #endif
  #ifndef csSnowSaver_FACTORY_REGISTER_DEFINED 
  #define csSnowSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSnowSaver) 
  #endif

class snowldr
{
SCF_REGISTER_STATIC_LIBRARY(snowldr,metainfo_snowldr)
  #ifndef csSnowFactoryLoader_FACTORY_REGISTERED 
  #define csSnowFactoryLoader_FACTORY_REGISTERED 
    csSnowFactoryLoader_StaticInit csSnowFactoryLoader_static_init__; 
  #endif
  #ifndef csSnowLoader_FACTORY_REGISTERED 
  #define csSnowLoader_FACTORY_REGISTERED 
    csSnowLoader_StaticInit csSnowLoader_static_init__; 
  #endif
  #ifndef csSnowFactorySaver_FACTORY_REGISTERED 
  #define csSnowFactorySaver_FACTORY_REGISTERED 
    csSnowFactorySaver_StaticInit csSnowFactorySaver_static_init__; 
  #endif
  #ifndef csSnowSaver_FACTORY_REGISTERED 
  #define csSnowSaver_FACTORY_REGISTERED 
    csSnowSaver_StaticInit csSnowSaver_static_init__; 
  #endif
public:
 snowldr();
};
snowldr::snowldr() {}

}
