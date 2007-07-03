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
static char const metainfo_spr2d[] =
"<?xml version=\"1.0\"?>"
"<!-- spr2d.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.sprite.2d</name>"
"        <implementation>csSprite2DMeshObjectType</implementation>"
"        <description>Crystal Space Sprite2D Mesh Type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csSprite2DMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csSprite2DMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csSprite2DMeshObjectType) 
  #endif

class spr2d
{
SCF_REGISTER_STATIC_LIBRARY(spr2d,metainfo_spr2d)
  #ifndef csSprite2DMeshObjectType_FACTORY_REGISTERED 
  #define csSprite2DMeshObjectType_FACTORY_REGISTERED 
    csSprite2DMeshObjectType_StaticInit csSprite2DMeshObjectType_static_init__; 
  #endif
public:
 spr2d();
};
spr2d::spr2d() {}

}
