/*
    Copyright (C) 2002 by Jorrit Tyberghein

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdarg.h>

#include "cssysdef.h"
#include "unittest.h"
#include "csengine/engine.h"
#include "csengine/xorbuf.h"
#include "iutil/string.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/dbghelp.h"
#include "iutil/vfs.h"
#include "cstool/initapp.h"
#include "iengine/viscull.h"

//------------------------------------------------- We need the 3D engine -----

CS_IMPLEMENT_APPLICATION

// need to register the engine explicit here when not building static
#if !defined(CS_STATIC_LINKED)
SCF_REGISTER_STATIC_LIBRARY (engine)
#endif

//-----------------------------------------------------------------------------

UnitTest::UnitTest ()
{
}

UnitTest::~UnitTest ()
{
}

static void Test (iBase* obj, const char* name)
{
  if (!obj)
  {
    printf ("Object '%s' is missing!\n", name);
    return;
  }
  iDebugHelper* dbghelp = SCF_QUERY_INTERFACE (obj, iDebugHelper);
  if (dbghelp && (dbghelp->GetSupportedTests () & CS_DBGHELP_UNITTEST))
  {
    iString* str = dbghelp->UnitTest ();
    if (str)
    {
      printf ("%s unit testing failed!\n", name);
      printf ("%s\n", str->GetData ());
      str->DecRef ();
    }
    else
    {
      printf ("%s unit testing succeeded!\n", name);
    }
    dbghelp->DecRef ();
  }
  else
    printf ("%s unit test not performed (object doesn't support it).\n", name);
}

static void Benchmark (iBase* obj, const char* name, int num_iterations)
{
  if (!obj)
  {
    printf ("Object '%s' is missing!\n", name);
    return;
  }
  iDebugHelper* dbghelp = SCF_QUERY_INTERFACE (obj, iDebugHelper);
  if (dbghelp && (dbghelp->GetSupportedTests () & CS_DBGHELP_BENCHMARK))
  {
    csTicks t = dbghelp->Benchmark (num_iterations);
    printf ("Benchmarking %s: %d ms\n", name, t);
    dbghelp->DecRef ();
  }
  else
    printf ("%s benchmark not performed (object doesn't support it).\n", name);
}

/*---------------------------------------------------------------------*
 * Main function
 *---------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  // Initialize the random number generator
  srand (time (NULL));

  iObjectRegistry* object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg)
    return -1;
  if (!csInitializer::RequestPlugins (object_reg,
	CS_REQUEST_VFS,
	CS_REQUEST_ENGINE,
	CS_REQUEST_END))
  {
    csInitializer::DestroyApplication (object_reg);
    return -1;
  }

  iPluginManager* plugmgr = CS_QUERY_REGISTRY (object_reg, iPluginManager);
  if (!plugmgr)
  {
    csInitializer::DestroyApplication (object_reg);
    return -1;
  }

  csXORBuffer* buf = new csXORBuffer (640, 480);
  Test (buf, "csXORBuffer");
  //Benchmark (buf, "csXORBuffer", 10000);
  delete buf;

  iEngine* engine = CS_QUERY_REGISTRY (object_reg, iEngine);
  Test (engine, "Engine");
  if (engine) engine->DecRef ();

  iVisibilityCuller* viscull = CS_LOAD_PLUGIN (plugmgr,
  	"crystalspace.culling.dynavis", iVisibilityCuller);
  Test (viscull, "DynaVis");
  if (viscull) viscull->DecRef ();

  plugmgr->DecRef ();
  csInitializer::DestroyApplication (object_reg);
  return 0;
}

