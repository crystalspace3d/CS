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
static char const metainfo_trans_std[] =
"<?xml version=\"1.0\"?>"
"<!-- trans_std.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.translator.standard</name>"
"        <implementation>csTranslator</implementation>"
"        <description>Standard Translator</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.translator.loader.xml</name>"
"        <implementation>csTranslatorLoaderXml</implementation>"
"        <description>Xml Translator Loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csTranslator_FACTORY_REGISTER_DEFINED 
  #define csTranslator_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTranslator) 
  #endif
  #ifndef csTranslatorLoaderXml_FACTORY_REGISTER_DEFINED 
  #define csTranslatorLoaderXml_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTranslatorLoaderXml) 
  #endif

class trans_std
{
SCF_REGISTER_STATIC_LIBRARY(trans_std,metainfo_trans_std)
  #ifndef csTranslator_FACTORY_REGISTERED 
  #define csTranslator_FACTORY_REGISTERED 
    csTranslator_StaticInit csTranslator_static_init__; 
  #endif
  #ifndef csTranslatorLoaderXml_FACTORY_REGISTERED 
  #define csTranslatorLoaderXml_FACTORY_REGISTERED 
    csTranslatorLoaderXml_StaticInit csTranslatorLoaderXml_static_init__; 
  #endif
public:
 trans_std();
};
trans_std::trans_std() {}

}
