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
static char const metainfo_rm_osm[] =
"<?xml version=\"1.0\"?>"
"<!-- rm_osm.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.rendermanager.osm</name>"
"        <implementation>RMOSM</implementation>"
"        <description>Rendermanager (osm)</description>"
"        <requires>"
"          <class>crystalspace.engine.</class>"
"          <class>crystalspace.graphics3d.</class>         "
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef RMOSM_FACTORY_REGISTER_DEFINED 
  #define RMOSM_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(RMOSM) 
  #endif

class rm_osm
{
SCF_REGISTER_STATIC_LIBRARY(rm_osm,metainfo_rm_osm)
  #ifndef RMOSM_FACTORY_REGISTERED 
  #define RMOSM_FACTORY_REGISTERED 
    RMOSM_StaticInit RMOSM_static_init__; 
  #endif
public:
 rm_osm();
};
rm_osm::rm_osm() {}

}
