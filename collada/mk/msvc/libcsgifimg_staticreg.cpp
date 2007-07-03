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
static char const metainfo_csgifimg[] =
"<?xml version=\"1.0\"?>"
"<!-- csgifimg.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphic.image.io.gif</name>"
"        <implementation>csGIFImageIO</implementation>"
"        <description>CrystalSpace GIF image format I/O plugin</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGIFImageIO_FACTORY_REGISTER_DEFINED 
  #define csGIFImageIO_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGIFImageIO) 
  #endif

class csgifimg
{
SCF_REGISTER_STATIC_LIBRARY(csgifimg,metainfo_csgifimg)
  #ifndef csGIFImageIO_FACTORY_REGISTERED 
  #define csGIFImageIO_FACTORY_REGISTERED 
    csGIFImageIO_StaticInit csGIFImageIO_static_init__; 
  #endif
public:
 csgifimg();
};
csgifimg::csgifimg() {}

}
