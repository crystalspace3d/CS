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
static char const metainfo_stdpt[] =
"<?xml version=\"1.0\"?>"
"<!-- stdpt.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.texture.loader.dots</name>"
"        <implementation>csPtDotsLoader</implementation>"
"        <description>'Dots' procedural texture loader</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.fire</name>"
"        <implementation>csPtFireLoader</implementation>"
"        <description>'Fire' procedural texture loader</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.plasma</name>"
"        <implementation>csPtPlasmaLoader</implementation>"
"        <description>'Plasma' procedural texture loader</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.sky</name>"
"        <implementation>csPtSkyLoader</implementation>"
"        <description>'Sky' procedural texture loader</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.loader.water</name>"
"        <implementation>csPtWaterLoader</implementation>"
"        <description>'Water' procedural texture loader</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.type.dots</name>"
"        <implementation>csPtDotsType</implementation>"
"        <description>'Dots' procedural texture factory</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.type.fire</name>"
"        <implementation>csPtFireType</implementation>"
"        <description>'Fire' procedural texture factory</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.type.plasma</name>"
"        <implementation>csPtPlasmaType</implementation>"
"        <description>'Plasma' procedural texture factory</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.type.sky</name>"
"        <implementation>csPtSkyType</implementation>"
"        <description>'Sky' procedural texture factory</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.type.water</name>"
"        <implementation>csPtWaterType</implementation>"
"        <description>'Water' procedural texture factory</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.saver.dots</name>"
"        <implementation>csPtDotsSaver</implementation>"
"        <description>'Dots' procedural texture saver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.saver.fire</name>"
"        <implementation>csPtFireSaver</implementation>"
"        <description>'Fire' procedural texture saver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.saver.plasma</name>"
"        <implementation>csPtPlasmaSaver</implementation>"
"        <description>'Plasma' procedural texture saver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.saver.sky</name>"
"        <implementation>csPtSkySaver</implementation>"
"        <description>'Sky' procedural texture saver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"      <class>"
"        <name>crystalspace.texture.saver.water</name>"
"        <implementation>csPtWaterSaver</implementation>"
"        <description>'Water' procedural texture saver</description>"
"        <requires>"
"          <class>crystalspace.graphics3d.</class>"
"        </requires>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csPtDotsLoader_FACTORY_REGISTER_DEFINED 
  #define csPtDotsLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtDotsLoader) 
  #endif
  #ifndef csPtFireLoader_FACTORY_REGISTER_DEFINED 
  #define csPtFireLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtFireLoader) 
  #endif
  #ifndef csPtPlasmaLoader_FACTORY_REGISTER_DEFINED 
  #define csPtPlasmaLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtPlasmaLoader) 
  #endif
  #ifndef csPtSkyLoader_FACTORY_REGISTER_DEFINED 
  #define csPtSkyLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtSkyLoader) 
  #endif
  #ifndef csPtWaterLoader_FACTORY_REGISTER_DEFINED 
  #define csPtWaterLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtWaterLoader) 
  #endif
  #ifndef csPtDotsType_FACTORY_REGISTER_DEFINED 
  #define csPtDotsType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtDotsType) 
  #endif
  #ifndef csPtFireType_FACTORY_REGISTER_DEFINED 
  #define csPtFireType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtFireType) 
  #endif
  #ifndef csPtPlasmaType_FACTORY_REGISTER_DEFINED 
  #define csPtPlasmaType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtPlasmaType) 
  #endif
  #ifndef csPtSkyType_FACTORY_REGISTER_DEFINED 
  #define csPtSkyType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtSkyType) 
  #endif
  #ifndef csPtWaterType_FACTORY_REGISTER_DEFINED 
  #define csPtWaterType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtWaterType) 
  #endif
  #ifndef csPtDotsSaver_FACTORY_REGISTER_DEFINED 
  #define csPtDotsSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtDotsSaver) 
  #endif
  #ifndef csPtFireSaver_FACTORY_REGISTER_DEFINED 
  #define csPtFireSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtFireSaver) 
  #endif
  #ifndef csPtPlasmaSaver_FACTORY_REGISTER_DEFINED 
  #define csPtPlasmaSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtPlasmaSaver) 
  #endif
  #ifndef csPtSkySaver_FACTORY_REGISTER_DEFINED 
  #define csPtSkySaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtSkySaver) 
  #endif
  #ifndef csPtWaterSaver_FACTORY_REGISTER_DEFINED 
  #define csPtWaterSaver_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPtWaterSaver) 
  #endif

class stdpt
{
SCF_REGISTER_STATIC_LIBRARY(stdpt,metainfo_stdpt)
  #ifndef csPtDotsLoader_FACTORY_REGISTERED 
  #define csPtDotsLoader_FACTORY_REGISTERED 
    csPtDotsLoader_StaticInit csPtDotsLoader_static_init__; 
  #endif
  #ifndef csPtFireLoader_FACTORY_REGISTERED 
  #define csPtFireLoader_FACTORY_REGISTERED 
    csPtFireLoader_StaticInit csPtFireLoader_static_init__; 
  #endif
  #ifndef csPtPlasmaLoader_FACTORY_REGISTERED 
  #define csPtPlasmaLoader_FACTORY_REGISTERED 
    csPtPlasmaLoader_StaticInit csPtPlasmaLoader_static_init__; 
  #endif
  #ifndef csPtSkyLoader_FACTORY_REGISTERED 
  #define csPtSkyLoader_FACTORY_REGISTERED 
    csPtSkyLoader_StaticInit csPtSkyLoader_static_init__; 
  #endif
  #ifndef csPtWaterLoader_FACTORY_REGISTERED 
  #define csPtWaterLoader_FACTORY_REGISTERED 
    csPtWaterLoader_StaticInit csPtWaterLoader_static_init__; 
  #endif
  #ifndef csPtDotsType_FACTORY_REGISTERED 
  #define csPtDotsType_FACTORY_REGISTERED 
    csPtDotsType_StaticInit csPtDotsType_static_init__; 
  #endif
  #ifndef csPtFireType_FACTORY_REGISTERED 
  #define csPtFireType_FACTORY_REGISTERED 
    csPtFireType_StaticInit csPtFireType_static_init__; 
  #endif
  #ifndef csPtPlasmaType_FACTORY_REGISTERED 
  #define csPtPlasmaType_FACTORY_REGISTERED 
    csPtPlasmaType_StaticInit csPtPlasmaType_static_init__; 
  #endif
  #ifndef csPtSkyType_FACTORY_REGISTERED 
  #define csPtSkyType_FACTORY_REGISTERED 
    csPtSkyType_StaticInit csPtSkyType_static_init__; 
  #endif
  #ifndef csPtWaterType_FACTORY_REGISTERED 
  #define csPtWaterType_FACTORY_REGISTERED 
    csPtWaterType_StaticInit csPtWaterType_static_init__; 
  #endif
  #ifndef csPtDotsSaver_FACTORY_REGISTERED 
  #define csPtDotsSaver_FACTORY_REGISTERED 
    csPtDotsSaver_StaticInit csPtDotsSaver_static_init__; 
  #endif
  #ifndef csPtFireSaver_FACTORY_REGISTERED 
  #define csPtFireSaver_FACTORY_REGISTERED 
    csPtFireSaver_StaticInit csPtFireSaver_static_init__; 
  #endif
  #ifndef csPtPlasmaSaver_FACTORY_REGISTERED 
  #define csPtPlasmaSaver_FACTORY_REGISTERED 
    csPtPlasmaSaver_StaticInit csPtPlasmaSaver_static_init__; 
  #endif
  #ifndef csPtSkySaver_FACTORY_REGISTERED 
  #define csPtSkySaver_FACTORY_REGISTERED 
    csPtSkySaver_StaticInit csPtSkySaver_static_init__; 
  #endif
  #ifndef csPtWaterSaver_FACTORY_REGISTERED 
  #define csPtWaterSaver_FACTORY_REGISTERED 
    csPtWaterSaver_StaticInit csPtWaterSaver_static_init__; 
  #endif
public:
 stdpt();
};
stdpt::stdpt() {}

}
