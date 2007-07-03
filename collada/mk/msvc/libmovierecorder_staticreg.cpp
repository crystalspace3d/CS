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
static char const metainfo_movierecorder[] =
"<?xml version=\"1.0\"?>"
"<!-- movierecorder.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.utilities.movierecorder</name>"
"        <implementation>csMovieRecorder</implementation>"
"        <description>Realtime movie recorder</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csMovieRecorder_FACTORY_REGISTER_DEFINED 
  #define csMovieRecorder_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csMovieRecorder) 
  #endif

class movierecorder
{
SCF_REGISTER_STATIC_LIBRARY(movierecorder,metainfo_movierecorder)
  #ifndef csMovieRecorder_FACTORY_REGISTERED 
  #define csMovieRecorder_FACTORY_REGISTERED 
    csMovieRecorder_StaticInit csMovieRecorder_static_init__; 
  #endif
public:
 movierecorder();
};
movierecorder::movierecorder() {}

}
