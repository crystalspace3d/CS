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
static char const metainfo_sprcal3dldr[] =
"<?xml version=\"1.0\"?>"
"<!-- sprcal3dldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.loader.factory.sprite.cal3d</name>"
"        <implementation>csSpriteCal3DFactoryLoader</implementation>"
"        <description>Crystal Space SpriteCal3D Mesh Factory Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.loader.sprite.cal3d</name>"
"        <implementation>csSpriteCal3DLoader</implementation>"
"        <description>Crystal Space SpriteCal3D Mesh Loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.factory.sprite.cal3d</name>"
"        <implementation>csSpriteCal3DFactorySaver</implementation>"
"        <description>Crystal Space SpriteCal3D Mesh Factory Saver</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.saver.sprite.cal3d</name>"
"        <implementation>csSpriteCal3DSaver</implementation>"
"        <description>Crystal Space SpriteCal3D Mesh Saver</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSpriteCal3DFactoryLoader_FACTORY_REGISTER_DEFINED 
  #define csSpriteCal3DFactoryLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSpriteCal3DFactoryLoader) 
  #endif
  #ifndef csSpriteCal3DLoader_FACTORY_REGISTER_DEFINED 
  #define csSpriteCal3DLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSpriteCal3DLoader) 
  #endif
  #ifndef csSpriteCal3DFactorySaver_FACTORY_REGISTER_DEFINED 
  #define csSpriteCal3DFactorySaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSpriteCal3DFactorySaver) 
  #endif
  #ifndef csSpriteCal3DSaver_FACTORY_REGISTER_DEFINED 
  #define csSpriteCal3DSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSpriteCal3DSaver) 
  #endif

class sprcal3dldr
{
SCF_REGISTER_STATIC_LIBRARY(sprcal3dldr,metainfo_sprcal3dldr)
  #ifndef csSpriteCal3DFactoryLoader_FACTORY_REGISTERED 
  #define csSpriteCal3DFactoryLoader_FACTORY_REGISTERED 
    csSpriteCal3DFactoryLoader_StaticInit csSpriteCal3DFactoryLoader_static_init__; 
  #endif
  #ifndef csSpriteCal3DLoader_FACTORY_REGISTERED 
  #define csSpriteCal3DLoader_FACTORY_REGISTERED 
    csSpriteCal3DLoader_StaticInit csSpriteCal3DLoader_static_init__; 
  #endif
  #ifndef csSpriteCal3DFactorySaver_FACTORY_REGISTERED 
  #define csSpriteCal3DFactorySaver_FACTORY_REGISTERED 
    csSpriteCal3DFactorySaver_StaticInit csSpriteCal3DFactorySaver_static_init__; 
  #endif
  #ifndef csSpriteCal3DSaver_FACTORY_REGISTERED 
  #define csSpriteCal3DSaver_FACTORY_REGISTERED 
    csSpriteCal3DSaver_StaticInit csSpriteCal3DSaver_static_init__; 
  #endif
public:
 sprcal3dldr();
};
sprcal3dldr::sprcal3dldr() {}

}
