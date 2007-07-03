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
static char const metainfo_instmeshldr[] =
"<?xml version=\"1.0\"?>"
"<!-- instmeshldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.loader.factory.instmesh</name>"
"        <implementation>csInstFactoryLoader</implementation>"
"        <description>Crystal Space Instancing Mesh Factory Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.loader.instmesh</name>"
"        <implementation>csInstMeshLoader</implementation>"
"        <description>Crystal Space Instancing Mesh Mesh Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.factory.instmesh</name>"
"        <implementation>csInstFactorySaver</implementation>"
"        <description>Crystal Space Instancing Mesh Factory Saver</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.instmesh</name>"
"        <implementation>csInstMeshSaver</implementation>"
"        <description>Crystal Space Instancing Mesh Mesh Saver</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csInstFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csInstFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csInstFactoryLoader) 
  #endif
  #ifndef csInstMeshLoader_FACTORY_REGISTER_DEFINED 
  #define csInstMeshLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csInstMeshLoader) 
  #endif
  #ifndef csInstFactorySaver_FACTORY_REGISTER_DEFINED 
  #define csInstFactorySaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csInstFactorySaver) 
  #endif
  #ifndef csInstMeshSaver_FACTORY_REGISTER_DEFINED 
  #define csInstMeshSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csInstMeshSaver) 
  #endif

class instmeshldr
{
SCF_REGISTER_STATIC_LIBRARY(instmeshldr,metainfo_instmeshldr)
  #ifndef csInstFactoryLoader_FACTORY_REGISTERED 
  #define csInstFactoryLoader_FACTORY_REGISTERED 
    csInstFactoryLoader_StaticInit csInstFactoryLoader_static_init__; 
  #endif
  #ifndef csInstMeshLoader_FACTORY_REGISTERED 
  #define csInstMeshLoader_FACTORY_REGISTERED 
    csInstMeshLoader_StaticInit csInstMeshLoader_static_init__; 
  #endif
  #ifndef csInstFactorySaver_FACTORY_REGISTERED 
  #define csInstFactorySaver_FACTORY_REGISTERED 
    csInstFactorySaver_StaticInit csInstFactorySaver_static_init__; 
  #endif
  #ifndef csInstMeshSaver_FACTORY_REGISTERED 
  #define csInstMeshSaver_FACTORY_REGISTERED 
    csInstMeshSaver_StaticInit csInstMeshSaver_static_init__; 
  #endif
public:
 instmeshldr();
};
instmeshldr::instmeshldr() {}

}
