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
static char const metainfo_pagingformerldr[] =
"<?xml version=\"1.0\"?>"
"<!-- pagingformerldr.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.terraformer.paging.loader</name>"
"        <implementation>csPagingFormerLoader</implementation>"
"        <description>Paging terrain formation loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csPagingFormerLoader_FACTORY_REGISTER_DEFINED 
  #define csPagingFormerLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csPagingFormerLoader) 
  #endif

class pagingformerldr
{
SCF_REGISTER_STATIC_LIBRARY(pagingformerldr,metainfo_pagingformerldr)
  #ifndef csPagingFormerLoader_FACTORY_REGISTERED 
  #define csPagingFormerLoader_FACTORY_REGISTERED 
    csPagingFormerLoader_StaticInit csPagingFormerLoader_static_init__; 
  #endif
public:
 pagingformerldr();
};
pagingformerldr::pagingformerldr() {}

}
