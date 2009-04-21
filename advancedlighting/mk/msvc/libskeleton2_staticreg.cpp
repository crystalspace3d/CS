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
static char const metainfo_skeleton2[] =
"<?xml version=\"1.0\"?>"
"<!-- skeleton2.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.skeletalanimation</name>"
"        <implementation>SkeletonSystem</implementation>"
"        <description>Crystal Space Skeletal animation plugin</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef SkeletonSystem_FACTORY_REGISTER_DEFINED 
  #define SkeletonSystem_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SkeletonSystem) 
  #endif

class skeleton2
{
SCF_REGISTER_STATIC_LIBRARY(skeleton2,metainfo_skeleton2)
  #ifndef SkeletonSystem_FACTORY_REGISTERED 
  #define SkeletonSystem_FACTORY_REGISTERED 
    SkeletonSystem_StaticInit SkeletonSystem_static_init__; 
  #endif
public:
 skeleton2();
};
skeleton2::skeleton2() {}

}
