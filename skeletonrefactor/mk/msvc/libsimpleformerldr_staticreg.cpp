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
static char const metainfo_simpleformerldr[] =
"<?xml version=\"1.0\"?>"
"<!-- simpleformerldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.terraformer.simple.loader</name>"
"        <implementation>csSimpleFormerLoader</implementation>"
"        <description>Simple terrain formation loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSimpleFormerLoader_FACTORY_REGISTER_DEFINED 
  #define csSimpleFormerLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSimpleFormerLoader) 
  #endif

class simpleformerldr
{
SCF_REGISTER_STATIC_LIBRARY(simpleformerldr,metainfo_simpleformerldr)
  #ifndef csSimpleFormerLoader_FACTORY_REGISTERED 
  #define csSimpleFormerLoader_FACTORY_REGISTERED 
    csSimpleFormerLoader_StaticInit csSimpleFormerLoader_static_init__; 
  #endif
public:
 simpleformerldr();
};
simpleformerldr::simpleformerldr() {}

}
