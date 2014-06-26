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
static char const metainfo_medialoader[] =
"<?xml version=\"1.0\"?>"
"<!-- medialoader.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.videodecode.loader</name>"
"        <implementation>csVplLoader</implementation>"
"        <description>Generic Media Loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csVplLoader_FACTORY_REGISTER_DEFINED 
  #define csVplLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csVplLoader) 
  #endif

class medialoader
{
SCF_REGISTER_STATIC_LIBRARY(medialoader,metainfo_medialoader)
  #ifndef csVplLoader_FACTORY_REGISTERED 
  #define csVplLoader_FACTORY_REGISTERED 
    csVplLoader_StaticInit csVplLoader_static_init__; 
  #endif
public:
 medialoader();
};
medialoader::medialoader() {}

}
