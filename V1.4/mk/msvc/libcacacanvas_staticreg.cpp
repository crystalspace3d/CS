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
static char const metainfo_cacacanvas[] =
"<?xml version=\"1.0\"?>"
"<!-- cacacanvas.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphics2d.cacacanvas</name>"
"        <implementation>csGraphics2DCaca</implementation>"
"        <description>Colour AsCii Art 2D graphics driver for Crystal Space</description>"
"        <requires>"
"          <class>crystalspace.font.server.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGraphics2DCaca_FACTORY_REGISTER_DEFINED 
  #define csGraphics2DCaca_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGraphics2DCaca) 
  #endif

class cacacanvas
{
SCF_REGISTER_STATIC_LIBRARY(cacacanvas,metainfo_cacacanvas)
  #ifndef csGraphics2DCaca_FACTORY_REGISTERED 
  #define csGraphics2DCaca_FACTORY_REGISTERED 
    csGraphics2DCaca_StaticInit csGraphics2DCaca_static_init__; 
  #endif
public:
 cacacanvas();
};
cacacanvas::cacacanvas() {}

}
