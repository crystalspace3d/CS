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
static char const metainfo_aws[] =
"<?xml version=\"1.0\"?>"
"<!-- aws.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.window.alternatemanager</name>"
"        <implementation>awsManager</implementation>"
"        <description>Crystal Space alternate window manager</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.window.preferencemanager</name>"
"        <implementation>awsPrefManager</implementation>"
"        <description>Crystal Space window preference manager</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.window.sinkmanager</name>"
"        <implementation>awsSinkManager</implementation>"
"        <description>Crystal Space window sink manager</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef awsManager_FACTORY_REGISTER_DEFINED 
  #define awsManager_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(awsManager) 
  #endif
  #ifndef awsPrefManager_FACTORY_REGISTER_DEFINED 
  #define awsPrefManager_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(awsPrefManager) 
  #endif
  #ifndef awsSinkManager_FACTORY_REGISTER_DEFINED 
  #define awsSinkManager_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(awsSinkManager) 
  #endif

class aws
{
SCF_REGISTER_STATIC_LIBRARY(aws,metainfo_aws)
  #ifndef awsManager_FACTORY_REGISTERED 
  #define awsManager_FACTORY_REGISTERED 
    awsManager_StaticInit awsManager_static_init__; 
  #endif
  #ifndef awsPrefManager_FACTORY_REGISTERED 
  #define awsPrefManager_FACTORY_REGISTERED 
    awsPrefManager_StaticInit awsPrefManager_static_init__; 
  #endif
  #ifndef awsSinkManager_FACTORY_REGISTERED 
  #define awsSinkManager_FACTORY_REGISTERED 
    awsSinkManager_StaticInit awsSinkManager_static_init__; 
  #endif
public:
 aws();
};
aws::aws() {}

}
