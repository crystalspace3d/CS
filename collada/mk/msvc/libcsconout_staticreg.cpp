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
static char const metainfo_csconout[] =
"<?xml version=\"1.0\"?>"
"<!-- csconout.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.console.output.standard</name>"
"        <implementation>csConsoleOutput</implementation>"
"        <description>Crystal Space standard output console</description>"
"        <requires>"
"          <class>crystalspace.kernel.</class>"
"          <class>crystalspace.graphics3d.</class>"
"          <class>crystalspace.graphics2d.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csConsoleOutput_FACTORY_REGISTER_DEFINED 
  #define csConsoleOutput_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csConsoleOutput) 
  #endif

class csconout
{
SCF_REGISTER_STATIC_LIBRARY(csconout,metainfo_csconout)
  #ifndef csConsoleOutput_FACTORY_REGISTERED 
  #define csConsoleOutput_FACTORY_REGISTERED 
    csConsoleOutput_StaticInit csConsoleOutput_static_init__; 
  #endif
public:
 csconout();
};
csconout::csconout() {}

}
