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
static char const metainfo_skeleton[] =
"<?xml version=\"1.0\"?>"
"<!-- skeleton.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graveyard</name>"
"        <implementation>csSkeletonGraveyard</implementation>"
"        <description>Crystal Space Graveyard class</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSkeletonGraveyard_FACTORY_REGISTER_DEFINED 
  #define csSkeletonGraveyard_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSkeletonGraveyard) 
  #endif

class skeleton
{
SCF_REGISTER_STATIC_LIBRARY(skeleton,metainfo_skeleton)
  #ifndef csSkeletonGraveyard_FACTORY_REGISTERED 
  #define csSkeletonGraveyard_FACTORY_REGISTERED 
    csSkeletonGraveyard_StaticInit csSkeletonGraveyard_static_init__; 
  #endif
public:
 skeleton();
};
skeleton::skeleton() {}

}
