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
static char const metainfo_skelldr[] =
"<?xml version=\"1.0\"?>"
"<!-- skelldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graveyard.loader</name>"
"        <implementation>csSkeletonFactoryLoader</implementation>"
"        <description>Crystal Space General Skeleton Factory Loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSkeletonFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csSkeletonFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSkeletonFactoryLoader) 
  #endif

class skelldr
{
SCF_REGISTER_STATIC_LIBRARY(skelldr,metainfo_skelldr)
  #ifndef csSkeletonFactoryLoader_FACTORY_REGISTERED 
  #define csSkeletonFactoryLoader_FACTORY_REGISTERED 
    csSkeletonFactoryLoader_StaticInit csSkeletonFactoryLoader_static_init__; 
  #endif
public:
 skelldr();
};
skelldr::skelldr() {}

}
