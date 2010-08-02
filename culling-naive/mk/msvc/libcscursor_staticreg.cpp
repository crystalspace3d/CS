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
static char const metainfo_cscursor[] =
"<?xml version=\"1.0\"?>"
"<!-- cscursor.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphic.cursor</name>"
"        <implementation>csCursor</implementation>"
"        <description>Custom cursor selector</description>"
"        <requires>"
"          <class>crystalspace.graphics2d.</class>"
"          <class>crystalspace.graphic.image.io.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csCursor_FACTORY_REGISTER_DEFINED 
  #define csCursor_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csCursor) 
  #endif

class cscursor
{
SCF_REGISTER_STATIC_LIBRARY(cscursor,metainfo_cscursor)
  #ifndef csCursor_FACTORY_REGISTERED 
  #define csCursor_FACTORY_REGISTERED 
    csCursor_StaticInit csCursor_static_init__; 
  #endif
public:
 cscursor();
};
cscursor::cscursor() {}

}
