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
static char const metainfo_gmeshanim[] =
"<?xml version=\"1.0\"?>"
"<!-- gmeshanim.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.anim.genmesh</name>"
"        <implementation>csGenmeshAnimationControlType</implementation>"
"        <description>Crystal Space General Mesh Animation Control</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGenmeshAnimationControlType_FACTORY_REGISTER_DEFINED 
  #define csGenmeshAnimationControlType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGenmeshAnimationControlType) 
  #endif

class gmeshanim
{
SCF_REGISTER_STATIC_LIBRARY(gmeshanim,metainfo_gmeshanim)
  #ifndef csGenmeshAnimationControlType_FACTORY_REGISTERED 
  #define csGenmeshAnimationControlType_FACTORY_REGISTERED 
    csGenmeshAnimationControlType_StaticInit csGenmeshAnimationControlType_static_init__; 
  #endif
public:
 gmeshanim();
};
gmeshanim::gmeshanim() {}

}
