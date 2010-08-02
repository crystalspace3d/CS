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
static char const metainfo_rm_culler[] =
"<?xml version=\"1.0\"?>"
"<!-- rm_culler.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.rendermanager.culler</name>"
"        <implementation>RMCuller</implementation>"
"        <description>Rendermanager (culler)</description>"
"        <requires>"
"          <class>crystalspace.engine.</class>"
"          <class>crystalspace.graphics3d.</class>         "
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef RMCuller_FACTORY_REGISTER_DEFINED 
  #define RMCuller_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(RMCuller) 
  #endif

class rm_culler
{
SCF_REGISTER_STATIC_LIBRARY(rm_culler,metainfo_rm_culler)
  #ifndef RMCuller_FACTORY_REGISTERED 
  #define RMCuller_FACTORY_REGISTERED 
    RMCuller_StaticInit RMCuller_static_init__; 
  #endif
public:
 rm_culler();
};
rm_culler::rm_culler() {}

}
