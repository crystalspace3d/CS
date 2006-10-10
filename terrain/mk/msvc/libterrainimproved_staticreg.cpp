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
static char const metainfo_terrainimproved[] =
"<?xml version=\"1.0\"?>"
"<!-- terrainimproved.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved.bruteblockrenderer</name>"
"        <implementation>csTerrainBruteBlockRenderer</implementation>"
"        <description>Bruteblock terrain renderer</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved.simplerenderer</name>"
"        <implementation>csTerrainSimpleRenderer</implementation>"
"        <description>Simple terrain renderer</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved.terraformerdatafeeder</name>"
"        <implementation>csTerrainTerraFormerDataFeeder</implementation>"
"        <description>Terraformer-based data feeder</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved.threadeddatafeeder</name>"
"        <implementation>csTerrainThreadedDataFeeder</implementation>"
"        <description>Terrain multithreaded data feeder</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved.simpledatafeeder</name>"
"        <implementation>csTerrainSimpleDataFeeder</implementation>"
"        <description>Terrain simple data feeder</description>"
"      </class>"
"      <class>"
"        <name>crystalspace.mesh.object.terrainimproved</name>"
"        <implementation>csTerrainMeshObjectType</implementation>"
"        <description>Terrain mesh type</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csTerrainBruteBlockRenderer_FACTORY_REGISTER_DEFINED 
  #define csTerrainBruteBlockRenderer_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainBruteBlockRenderer) 
  #endif
  #ifndef csTerrainSimpleRenderer_FACTORY_REGISTER_DEFINED 
  #define csTerrainSimpleRenderer_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainSimpleRenderer) 
  #endif
  #ifndef csTerrainTerraFormerDataFeeder_FACTORY_REGISTER_DEFINED 
  #define csTerrainTerraFormerDataFeeder_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainTerraFormerDataFeeder) 
  #endif
  #ifndef csTerrainThreadedDataFeeder_FACTORY_REGISTER_DEFINED 
  #define csTerrainThreadedDataFeeder_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainThreadedDataFeeder) 
  #endif
  #ifndef csTerrainSimpleDataFeeder_FACTORY_REGISTER_DEFINED 
  #define csTerrainSimpleDataFeeder_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainSimpleDataFeeder) 
  #endif
  #ifndef csTerrainMeshObjectType_FACTORY_REGISTER_DEFINED 
  #define csTerrainMeshObjectType_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csTerrainMeshObjectType) 
  #endif

class terrainimproved
{
SCF_REGISTER_STATIC_LIBRARY(terrainimproved,metainfo_terrainimproved)
  #ifndef csTerrainBruteBlockRenderer_FACTORY_REGISTERED 
  #define csTerrainBruteBlockRenderer_FACTORY_REGISTERED 
    csTerrainBruteBlockRenderer_StaticInit csTerrainBruteBlockRenderer_static_init__; 
  #endif
  #ifndef csTerrainSimpleRenderer_FACTORY_REGISTERED 
  #define csTerrainSimpleRenderer_FACTORY_REGISTERED 
    csTerrainSimpleRenderer_StaticInit csTerrainSimpleRenderer_static_init__; 
  #endif
  #ifndef csTerrainTerraFormerDataFeeder_FACTORY_REGISTERED 
  #define csTerrainTerraFormerDataFeeder_FACTORY_REGISTERED 
    csTerrainTerraFormerDataFeeder_StaticInit csTerrainTerraFormerDataFeeder_static_init__; 
  #endif
  #ifndef csTerrainThreadedDataFeeder_FACTORY_REGISTERED 
  #define csTerrainThreadedDataFeeder_FACTORY_REGISTERED 
    csTerrainThreadedDataFeeder_StaticInit csTerrainThreadedDataFeeder_static_init__; 
  #endif
  #ifndef csTerrainSimpleDataFeeder_FACTORY_REGISTERED 
  #define csTerrainSimpleDataFeeder_FACTORY_REGISTERED 
    csTerrainSimpleDataFeeder_StaticInit csTerrainSimpleDataFeeder_static_init__; 
  #endif
  #ifndef csTerrainMeshObjectType_FACTORY_REGISTERED 
  #define csTerrainMeshObjectType_FACTORY_REGISTERED 
    csTerrainMeshObjectType_StaticInit csTerrainMeshObjectType_static_init__; 
  #endif
public:
 terrainimproved();
};
terrainimproved::terrainimproved() {}

}
