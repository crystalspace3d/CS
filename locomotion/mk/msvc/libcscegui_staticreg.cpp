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
static char const metainfo_cscegui[] =
"<?xml version=\"1.0\"?>"
"<!-- cscegui.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.cegui.wrapper</name>"
"        <implementation>Renderer</implementation>"
"        <description>Crystal Space CEGUI Wrapper</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"          <class>crystalspace.graphic.image.io.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef Renderer_FACTORY_REGISTER_DEFINED 
  #define Renderer_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(Renderer) 
  #endif

class cscegui
{
SCF_REGISTER_STATIC_LIBRARY(cscegui,metainfo_cscegui)
  #ifndef Renderer_FACTORY_REGISTERED 
  #define Renderer_FACTORY_REGISTERED 
    Renderer_StaticInit Renderer_static_init__; 
  #endif
public:
 cscegui();
};
cscegui::cscegui() {}

}
