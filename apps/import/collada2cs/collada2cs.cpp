/*
    Copyright (C) 2008 by Mike Gist

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This application is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this application; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <cssysdef.h>

#include "cstool/initapp.h"

#include "ivaria/collada.h"

#define CS_REQUEST_COLLADA CS_REQUEST_PLUGIN("crystalspace.utilities.colladaconvertor", \
                                             iColladaConvertor)
#define COLLADA_VERSION "1.4"

CS_IMPLEMENT_APPLICATION

int main(int argc, char** argv)
{
  iObjectRegistry* object_reg = csInitializer::CreateEnvironment (argc, argv);
  if(!object_reg)
  {
    printf("Object Reg failed to Init!\n");
    return 1;
  }

  csInitializer::RequestPlugins(object_reg, CS_REQUEST_COLLADA, CS_REQUEST_END);

  csRef<iColladaConvertor> collada = csQueryRegistry<iColladaConvertor>(object_reg);
  if(!collada)
  {
    printf("Collada plugin failed to load!\n");
    return 1;
  }

#ifdef CS_DEBUG
  collada->SetWarnings(true);
#endif

  // Default to library.
  collada->SetOutputFiletype(CS_LIBRARY_FILE);

  csArray<csString> args;
  for(int i=1; i<argc; i++)
  {
    args.Push(argv[i]);
  }

  printf("Collada %s to Crystal Space Convertor\n\n", COLLADA_VERSION);

  for(size_t i=0; i<args.GetSize(); i++)
  {
    if(args[i].Compare("-library"))
    {
      collada->SetOutputFiletype(CS_LIBRARY_FILE);
    }
    else if(args[i].Compare("-map"))
    {
      collada->SetOutputFiletype(CS_MAP_FILE);
    }
    else
    {
      printf("File %u of %u:\n", i+1, args.GetSize());
      csString fileIn = "/this/";
      fileIn.Append(args[i]);
      csString fileOut = fileIn;

      fileOut.Truncate(fileOut.FindLast('.'));
      fileOut.Append(".xml");

      printf("- Loading file: %s\n", args[i].GetData());
      collada->Load(fileIn);

      printf("- Converting\n");
      collada->Convert();

      printf("- Writing file: %s\n\n", fileOut.GetData());
      collada->Write(fileOut);
    }
  }

  return 0;
}
