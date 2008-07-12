/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein
    Copyright (C) 1998-2000 by Ivan Avramovic <ivan@avramovic.com>

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

#include "cssysdef.h"

#include "iengine/engine.h"
#include "iengine/material.h"
#include "imap/loader.h"
#include "iutil/object.h"
#include "ivaria/reporter.h"

#include "csloadercontext.h"

CS_PLUGIN_NAMESPACE_BEGIN(csparser)
{
  csLoaderContext::csLoaderContext (iObjectRegistry* object_reg, iEngine* Engine,
    iCollection* collection, bool searchCollectionOnly, bool checkDupes,
    iMissingLoaderData* missingdata, uint keepFlags, bool do_verbose)
    : scfImplementationType (this), object_reg(object_reg), Engine(Engine),
    collection(collection), searchCollectionOnly(searchCollectionOnly),
    checkDupes(checkDupes), missingdata(missingdata), keepFlags(keepFlags),
    do_verbose(do_verbose)
  {
  }

  csLoaderContext::~csLoaderContext ()
  {
  }

  iSector* csLoaderContext::FindSector(const char* name)
  {
    iSector* s = Engine->FindSector(name, searchCollectionOnly ? collection : 0);

    if(!s && missingdata)
    {
      s = missingdata->MissingSector(name);
    }

    return s;
  }

  iMaterialWrapper* csLoaderContext::FindMaterial(const char* filename)
  {
    iMaterialWrapper* mat = Engine->FindMaterial(filename, searchCollectionOnly ? collection : 0);

    if(!mat && missingdata)
    {
      mat = missingdata->MissingMaterial(0, filename);
    }

    if(!mat && do_verbose)
    {
      ReportNotify("Could not find material '%s'.", filename);
    }

    return mat;
  }

  iMaterialWrapper* csLoaderContext::FindNamedMaterial(const char* name, const char *filename)
  {
    iMaterialWrapper* mat = Engine->FindMaterial(name, searchCollectionOnly ? collection : 0);

    if(missingdata)
    {
      mat = missingdata->MissingMaterial(name, filename);
    }

    if(!mat && do_verbose)
    {
      ReportNotify("Could not find material '%s' with filename '%s'.", name, filename);
    }

    return mat;
  }


  iMeshFactoryWrapper* csLoaderContext::FindMeshFactory(const char* name)
  {
    iMeshFactoryWrapper* fact = Engine->FindMeshFactory(name, searchCollectionOnly ? collection : 0);

    if(!fact && missingdata)
    {
      fact = missingdata->MissingFactory(name);
    }

    if(!fact && do_verbose)
    {
      ReportNotify("Could not find mesh factory '%s'.", name);
    }

    return fact;
  }

  iMeshWrapper* csLoaderContext::FindMeshObject(const char* name)
  {
    iMeshWrapper* mesh = Engine->FindMeshObject(name, searchCollectionOnly ? collection : 0);

    if (!mesh && missingdata)
    {
      mesh = missingdata->MissingMesh(name);
    }

    if(!mesh && do_verbose)
    {
      ReportNotify("Could not find mesh object '%s'.", name);
    }

    return mesh;
  }

  iLight* csLoaderContext::FindLight(const char *name)
  {
    csRef<iLightIterator> li = Engine->GetLightIterator(searchCollectionOnly ? collection : 0);

    iLight *light;

    while(li->HasNext())
    {
      light = li->Next();
      if(!strcmp(light->QueryObject()->GetName(), name))
      {
        break;
      }
    }

    if(missingdata)
    {
      light = missingdata->MissingLight(name);
    }

    if(!light && do_verbose)
    {
      ReportNotify("Could not find light '%s'.", name);
    }

    return light;
  }

  iShader* csLoaderContext::FindShader(const char *name)
  {
    csRef<iShaderManager> shaderMgr = csQueryRegistry<iShaderManager>(object_reg);

    if(!shaderMgr)
    {
      return 0;
    }

    iShader* shader;

    // Always look up builtin shaders globally
    if((!searchCollectionOnly || !collection) || (name && *name == '*'))
    {
      shader = shaderMgr->GetShader(name);
      if(!shader && missingdata)
      {
        shader = missingdata->MissingShader(name);
      }
    }
    else
    {
      csRefArray<iShader> shaders = shaderMgr->GetShaders();
      for(size_t i=0; i<shaders.GetSize(); i++)
      {
        shader = shaders[i];
        if(collection)
        {
          if((collection->IsParentOf(shader->QueryObject()) ||
            collection->FindShader(shader->QueryObject()->GetName())) &&
            !strcmp(name, shader->QueryObject()->GetName()))
          {
            break;
          }
        }
      }

      if(missingdata)
      {
        shader = missingdata->MissingShader(name);
      }
    }

    if(!shader && do_verbose)
    {
      ReportNotify("Could not find shader '%s'.", name);
    }

    return 0;
  }

  iTextureWrapper* csLoaderContext::FindTexture(const char* name)
  {
    iTextureWrapper* result;
    if(collection && searchCollectionOnly)
    {
      result = collection->FindTexture(name);
    }
    else
    {
      result = Engine->GetTextureList()->FindByName (name);
    }

    if(!result && missingdata)
    {
      result = missingdata->MissingTexture (name, 0);
    }

    if(!result && do_verbose)
    {
      ReportNotify ("Could not find texture '%s'. Attempting to load.", name);
    }

    return result;
  }

  iTextureWrapper* csLoaderContext::FindNamedTexture (const char* name,
    const char *filename)
  {
    iTextureWrapper* result;
    if(collection && searchCollectionOnly)
    {
      result = collection->FindTexture(name);
    }
    else
    {
      result = Engine->GetTextureList()->FindByName(name);
    }

    if (!result && missingdata)
    {
      result = missingdata->MissingTexture(name, filename);
    }

    if(!result && do_verbose)
    {
      ReportNotify ("Could not find texture '%s'. Attempting to load.", name);
    }

    return result;
  }

  void csLoaderContext::AddToCollection(iObject* obj)
  {
    if(collection)
    {
      collection->Add(obj);
    }
  }

  void csLoaderContext::ReportNotify (const char* description, ...)
  {
    va_list arg;
    va_start (arg, description);
    csReportV (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "crystalspace.maploader", description, arg);
    va_end (arg);
  }
}
CS_PLUGIN_NAMESPACE_END(csparser)
