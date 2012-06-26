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
static char const metainfo_cseditoroperators[] =
"<?xml version=\"1.0\"?>"
"<!-- cseditoroperators.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.editor.operator.select</name>"
"        <implementation>SelectOperator</implementation>"
"        <label>Select</label>"
"        <description>Select something</description>"
"      </class>"
"      <!--class>"
"        <name>crystalspace.editor.operator.move</name>"
"        <implementation>MoveOperator</implementation>"
"        <label>Move an object</label>"
"        <description>Move an object operator</description>"
"      </class-->"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef SelectOperator_FACTORY_REGISTER_DEFINED 
  #define SelectOperator_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SelectOperator) 
  #endif
  #ifndef MoveOperator_FACTORY_REGISTER_DEFINED 
  #define MoveOperator_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(MoveOperator) 
  #endif

class cseditoroperators
{
SCF_REGISTER_STATIC_LIBRARY(cseditoroperators,metainfo_cseditoroperators)
  #ifndef SelectOperator_FACTORY_REGISTERED 
  #define SelectOperator_FACTORY_REGISTERED 
    SelectOperator_StaticInit SelectOperator_static_init__; 
  #endif
  #ifndef MoveOperator_FACTORY_REGISTERED 
  #define MoveOperator_FACTORY_REGISTERED 
    MoveOperator_StaticInit MoveOperator_static_init__; 
  #endif
public:
 cseditoroperators();
};
cseditoroperators::cseditoroperators() {}

}
