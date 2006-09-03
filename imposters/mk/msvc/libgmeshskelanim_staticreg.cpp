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
static char const metainfo_gmeshskelanim[] =
"<?xml version=\"1.0\"?>"
"<!-- gmeshanimen.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.anim.skeleton</name>"
"        <implementation>csGenmeshSkelAnimationControlTypeOld</implementation>"
"        <description>Crystal Space General Mesh Animation Control</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGenmeshSkelAnimationControlTypeOld_FACTORY_REGISTER_DEFINED 
  #define csGenmeshSkelAnimationControlTypeOld_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGenmeshSkelAnimationControlTypeOld) 
  #endif

class gmeshskelanim
{
SCF_REGISTER_STATIC_LIBRARY(gmeshskelanim,metainfo_gmeshskelanim)
  #ifndef csGenmeshSkelAnimationControlTypeOld_FACTORY_REGISTERED 
  #define csGenmeshSkelAnimationControlTypeOld_FACTORY_REGISTERED 
    csGenmeshSkelAnimationControlTypeOld_StaticInit csGenmeshSkelAnimationControlTypeOld_static_init__; 
  #endif
public:
 gmeshskelanim();
};
gmeshskelanim::gmeshskelanim() {}

}
