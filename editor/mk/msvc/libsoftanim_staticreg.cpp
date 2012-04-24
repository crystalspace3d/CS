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
static char const metainfo_softanim[] =
"<?xml version=\"1.0\"?>"
"<!-- bullet.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.dynamics.softanim</name>"
"        <implementation>SoftBodyControlType</implementation>"
"        <description>Generic animation of a genmesh from the simulation of soft bodies</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef SoftBodyControlType_FACTORY_REGISTER_DEFINED 
  #define SoftBodyControlType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SoftBodyControlType) 
  #endif

class softanim
{
SCF_REGISTER_STATIC_LIBRARY(softanim,metainfo_softanim)
  #ifndef SoftBodyControlType_FACTORY_REGISTERED 
  #define SoftBodyControlType_FACTORY_REGISTERED 
    SoftBodyControlType_StaticInit SoftBodyControlType_static_init__; 
  #endif
public:
 softanim();
};
softanim::softanim() {}

}
