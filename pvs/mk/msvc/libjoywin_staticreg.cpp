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
static char const metainfo_joywin[] =
"<?xml version=\"1.0\"?>"
"<!-- joywin.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.device.joystick.windows</name>"
"        <implementation>csWindowsJoystick</implementation>"
"        <description>Crystal Space Joystick plugin for Windows</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csWindowsJoystick_FACTORY_REGISTER_DEFINED 
  #define csWindowsJoystick_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csWindowsJoystick) 
  #endif

class joywin
{
SCF_REGISTER_STATIC_LIBRARY(joywin,metainfo_joywin)
  #ifndef csWindowsJoystick_FACTORY_REGISTERED 
  #define csWindowsJoystick_FACTORY_REGISTERED 
    csWindowsJoystick_StaticInit csWindowsJoystick_static_init__; 
  #endif
public:
 joywin();
};
joywin::joywin() {}

}
