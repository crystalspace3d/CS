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
static char const metainfo_cstgaimg[] =
"<?xml version=\"1.0\"?>"
"<!-- cstgaimg.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphic.image.io.tga</name>"
"        <implementation>csTGAImageIO</implementation>"
"        <description>CrystalSpace TGA image format I/O plugin</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csTGAImageIO_FACTORY_REGISTER_DEFINED 
  #define csTGAImageIO_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTGAImageIO) 
  #endif

class cstgaimg
{
SCF_REGISTER_STATIC_LIBRARY(cstgaimg,metainfo_cstgaimg)
  #ifndef csTGAImageIO_FACTORY_REGISTERED 
  #define csTGAImageIO_FACTORY_REGISTERED 
    csTGAImageIO_StaticInit csTGAImageIO_static_init__; 
  #endif
public:
 cstgaimg();
};
cstgaimg::cstgaimg() {}

}
