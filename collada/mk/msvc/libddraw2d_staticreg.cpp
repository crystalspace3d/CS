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
static char const metainfo_ddraw2d[] =
"<?xml version=\"1.0\"?>"
"<!-- ddraw2d.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphics2d.directdraw</name>"
"        <implementation>csGraphics2DDDraw3</implementation>"
"        <description>Crystal Space 2D DirectDraw driver</description>"
"        <requires>"
"          <class>crystalspace.font.server.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGraphics2DDDraw3_FACTORY_REGISTER_DEFINED 
  #define csGraphics2DDDraw3_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGraphics2DDDraw3) 
  #endif

class ddraw2d
{
SCF_REGISTER_STATIC_LIBRARY(ddraw2d,metainfo_ddraw2d)
  #ifndef csGraphics2DDDraw3_FACTORY_REGISTERED 
  #define csGraphics2DDDraw3_FACTORY_REGISTERED 
    csGraphics2DDDraw3_StaticInit csGraphics2DDDraw3_static_init__; 
  #endif
public:
 ddraw2d();
};
ddraw2d::ddraw2d() {}

}
