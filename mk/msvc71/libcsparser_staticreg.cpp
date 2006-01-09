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
static char const metainfo_csparser[] =
"<?xml version=\"1.0\"?>"
"<!-- csparser.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.level.loader</name>"
"        <implementation>csLoader</implementation>"
"        <description>Level and library file loader</description>"
"        <requires>"
"          <class>crystalspace.kernel.</class>"
"          <class>crystalspace.sound.loader.</class>"
"          <class>crystalspace.sndsys.element.loader</class>"
"          <class>crystalspace.image.loader</class>"
"          <class>crystalspace.mesh.loader.</class>"
"          <class>crystalspace.engine.3d</class>"
"          <class>crystalspace.graphics3d.</class>"
"          <class>crystalspace.sound.render.</class>"
"          <class>crystalspace.sndsys.renderer.</class>"
"          <class>crystalspace.mesh.crossbuilder</class>"
"          <class>crystalspace.modelconverter.</class>"
"          <class>crystalspace.documentsystem.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.checkerboard</name>"
"        <implementation>csCheckerTextureLoader</implementation>"
"        <description>Checkerboard texture loader</description>"
"        <requires>"
"          <class>crystalspace.level.loader</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.image</name>"
"        <implementation>csImageTextureLoader</implementation>"
"        <description>Image texture loader</description>"
"        <requires>"
"          <class>crystalspace.level.loader</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.cubemap</name>"
"        <implementation>csCubemapTextureLoader</implementation>"
"        <description>Cube map texture loader</description>"
"        <requires>"
"          <class>crystalspace.level.loader</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.tex3d</name>"
"        <implementation>csTexture3DLoader</implementation>"
"        <description>3D texture loader</description>"
"        <requires>"
"          <class>crystalspace.level.loader</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csLoader_FACTORY_REGISTER_DEFINED 
  #define csLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csLoader) 
  #endif
  #ifndef csCheckerTextureLoader_FACTORY_REGISTER_DEFINED 
  #define csCheckerTextureLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csCheckerTextureLoader) 
  #endif
  #ifndef csImageTextureLoader_FACTORY_REGISTER_DEFINED 
  #define csImageTextureLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csImageTextureLoader) 
  #endif
  #ifndef csCubemapTextureLoader_FACTORY_REGISTER_DEFINED 
  #define csCubemapTextureLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csCubemapTextureLoader) 
  #endif
  #ifndef csTexture3DLoader_FACTORY_REGISTER_DEFINED 
  #define csTexture3DLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTexture3DLoader) 
  #endif

class csparser
{
SCF_REGISTER_STATIC_LIBRARY(csparser,metainfo_csparser)
  #ifndef csLoader_FACTORY_REGISTERED 
  #define csLoader_FACTORY_REGISTERED 
    csLoader_StaticInit csLoader_static_init__; 
  #endif
  #ifndef csCheckerTextureLoader_FACTORY_REGISTERED 
  #define csCheckerTextureLoader_FACTORY_REGISTERED 
    csCheckerTextureLoader_StaticInit csCheckerTextureLoader_static_init__; 
  #endif
  #ifndef csImageTextureLoader_FACTORY_REGISTERED 
  #define csImageTextureLoader_FACTORY_REGISTERED 
    csImageTextureLoader_StaticInit csImageTextureLoader_static_init__; 
  #endif
  #ifndef csCubemapTextureLoader_FACTORY_REGISTERED 
  #define csCubemapTextureLoader_FACTORY_REGISTERED 
    csCubemapTextureLoader_StaticInit csCubemapTextureLoader_static_init__; 
  #endif
  #ifndef csTexture3DLoader_FACTORY_REGISTERED 
  #define csTexture3DLoader_FACTORY_REGISTERED 
    csTexture3DLoader_StaticInit csTexture3DLoader_static_init__; 
  #endif
public:
 csparser();
};
csparser::csparser() {}

}
