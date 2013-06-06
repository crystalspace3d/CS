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
static char const metainfo_shaderweaver[] =
"<?xml version=\"1.0\"?>"
"<!-- shaderweaver.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphics3d.shadercompiler.weaver</name>"
"        <implementation>WeaverCompiler</implementation>"
"        <description>Shader weaver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.shadercompiler.xmlshader</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef WeaverCompiler_FACTORY_REGISTER_DEFINED 
  #define WeaverCompiler_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(WeaverCompiler) 
  #endif

class shaderweaver
{
SCF_REGISTER_STATIC_LIBRARY(shaderweaver,metainfo_shaderweaver)
  #ifndef WeaverCompiler_FACTORY_REGISTERED 
  #define WeaverCompiler_FACTORY_REGISTERED 
    WeaverCompiler_StaticInit WeaverCompiler_static_init__; 
  #endif
public:
 shaderweaver();
};
shaderweaver::shaderweaver() {}

}
