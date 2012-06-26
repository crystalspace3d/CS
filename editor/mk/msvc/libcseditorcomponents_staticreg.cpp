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
static char const metainfo_cseditorcomponents[] =
"<?xml version=\"1.0\"?>"
"<!-- cseditorcomponents.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.editor.component.engine</name>"
"        <implementation>CS3DEngine</implementation>"
"        <description>Base engine and menu entries</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.editor.component.maploader</name>"
"        <implementation>MapLoader</implementation>"
"        <description>Default map and library loader</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.editor.component.exit</name>"
"        <implementation>Exit</implementation>"
"        <description>Management of application exit</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef CS3DEngine_FACTORY_REGISTER_DEFINED 
  #define CS3DEngine_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(CS3DEngine) 
  #endif
  #ifndef MapLoader_FACTORY_REGISTER_DEFINED 
  #define MapLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(MapLoader) 
  #endif
  #ifndef Exit_FACTORY_REGISTER_DEFINED 
  #define Exit_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(Exit) 
  #endif

class cseditorcomponents
{
SCF_REGISTER_STATIC_LIBRARY(cseditorcomponents,metainfo_cseditorcomponents)
  #ifndef CS3DEngine_FACTORY_REGISTERED 
  #define CS3DEngine_FACTORY_REGISTERED 
    CS3DEngine_StaticInit CS3DEngine_static_init__; 
  #endif
  #ifndef MapLoader_FACTORY_REGISTERED 
  #define MapLoader_FACTORY_REGISTERED 
    MapLoader_StaticInit MapLoader_static_init__; 
  #endif
  #ifndef Exit_FACTORY_REGISTERED 
  #define Exit_FACTORY_REGISTERED 
    Exit_StaticInit Exit_static_init__; 
  #endif
public:
 cseditorcomponents();
};
cseditorcomponents::cseditorcomponents() {}

}
