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
static char const metainfo_noise[] =
"<?xml version=\"1.0\"?>"
"<!-- noise.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.animesh.animnode.noise</name>"
"        <implementation>NoiseNodeManager</implementation>"
"        <description>Crystal Space noise animation nodes of an animated mesh</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef NoiseNodeManager_FACTORY_REGISTER_DEFINED 
  #define NoiseNodeManager_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(NoiseNodeManager) 
  #endif

class noise
{
SCF_REGISTER_STATIC_LIBRARY(noise,metainfo_noise)
  #ifndef NoiseNodeManager_FACTORY_REGISTERED 
  #define NoiseNodeManager_FACTORY_REGISTERED 
    NoiseNodeManager_StaticInit NoiseNodeManager_static_init__; 
  #endif
public:
 noise();
};
noise::noise() {}

}
