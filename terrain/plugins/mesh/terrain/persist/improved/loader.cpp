/*
    Copyright (C) 2006 by Kapoulkine Arseny

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

#include "csutil/cscolor.h"
#include "csutil/sysfunc.h"
#include "csutil/stringarray.h"
#include "csutil/refarr.h"

#include "iengine/material.h"
#include "iengine/mesh.h"
#include "imap/services.h"
#include "imap/ldrctxt.h"
#include "imap/loader.h"
#include "imesh/object.h"
#include "iutil/comp.h"
#include "iutil/document.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"

#include "iterrain/terrainsystem.h"
#include "iterrain/terrainrenderer.h"
#include "iterrain/terraincollider.h"
#include "iterrain/terraincell.h"
#include "iterrain/terraindatafeeder.h"
#include "iterrain/terrainfactory.h"
#include "iterrain/terraincellrenderproperties.h"
#include "iterrain/terraincellcollisionproperties.h"

#include "loader.h"

CS_IMPLEMENT_PLUGIN

enum
{
  XMLTOKEN_RENDERER,
  XMLTOKEN_COLLIDER,
  XMLTOKEN_CELLS,
  XMLTOKEN_CELL,
  XMLTOKEN_GRIDSIZE,
  XMLTOKEN_MATERIALSIZE,
  XMLTOKEN_POSITION,
  XMLTOKEN_SIZE,
  XMLTOKEN_FEEDER,
  XMLTOKEN_PLUGIN,
  XMLTOKEN_PARAM,
  XMLTOKEN_RENDERPROPERTIES,
  XMLTOKEN_COLLISIONPROPERTIES,
  XMLTOKEN_MATERIALPERSISTENT,
  
  XMLTOKEN_FACTORY,
  XMLTOKEN_MATERIALPALETTE,
  XMLTOKEN_MATERIAL,
};

namespace
{
  struct ParamValuePair
  {
    const char* name;
	const char* value;
  };
};

SCF_IMPLEMENT_FACTORY (csTerrainFactoryLoader)

SCF_IMPLEMENT_FACTORY (csTerrainObjectLoader)

csTerrainFactoryLoader::csTerrainFactoryLoader (iBase* parent)
 : scfImplementationType (this, parent)
{
}

csTerrainFactoryLoader::~csTerrainFactoryLoader ()
{
}

bool csTerrainFactoryLoader::Initialize (iObjectRegistry* objreg)
{
  object_reg = objreg;
  synldr = csQueryRegistry<iSyntaxService> (object_reg);

  xmltokens.Register ("renderer", XMLTOKEN_RENDERER);
  xmltokens.Register ("collider", XMLTOKEN_COLLIDER);
  xmltokens.Register ("cells", XMLTOKEN_CELLS);
  xmltokens.Register ("cell", XMLTOKEN_CELL);
  xmltokens.Register ("grid_size", XMLTOKEN_GRIDSIZE);
  xmltokens.Register ("material_size", XMLTOKEN_MATERIALSIZE);
  xmltokens.Register ("position", XMLTOKEN_POSITION);
  xmltokens.Register ("size", XMLTOKEN_SIZE);
  xmltokens.Register ("feeder", XMLTOKEN_FEEDER);
  xmltokens.Register ("plugin", XMLTOKEN_PLUGIN);
  xmltokens.Register ("param", XMLTOKEN_PARAM);
  xmltokens.Register ("render_properties", XMLTOKEN_RENDERPROPERTIES);
  xmltokens.Register ("collision_properties", XMLTOKEN_COLLISIONPROPERTIES);
  xmltokens.Register ("material_persistent", XMLTOKEN_MATERIALPERSISTENT);
  
  return true;
}

csPtr<iBase> csTerrainFactoryLoader::Parse (iDocumentNode* node,
  iStreamSource*, iLoaderContext* /*ldr_context*/, iBase* /*context*/)
{
  csRef<iPluginManager> plugin_mgr = csQueryRegistry<iPluginManager> (
    object_reg);
    
  csRef<iMeshObjectType> mesh_type = csLoadPlugin<iMeshObjectType> (
    object_reg, "crystalspace.mesh.object.terrainimproved");
  
  csRef<iMeshObjectFactory> mesh_factory = mesh_type->NewFactory ();
  
  csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory> (
    mesh_factory);
  
  csRef<iTerrainRenderer> renderer;
  csRef<iTerrainCollider> collider;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  
  csArray<ParamValuePair> render_properties, collision_properties;

  while (it->HasNext())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_RENDERER:
      {
        const char* pluginname = child->GetContentsValue ();
        renderer = csLoadPluginCheck<iTerrainRenderer> (
          plugin_mgr, pluginname);
              
        if (!renderer)
        {
          synldr->ReportError ("crystalspace.terrain.loader.factory",
            node, "Could not load %s!", pluginname);
          return 0;
        }
        
        factory->SetRenderer(renderer);
        
        break;
      }
      case XMLTOKEN_COLLIDER:
      {
        const char* pluginname = child->GetContentsValue ();
        collider = csLoadPluginCheck<iTerrainCollider> (
          plugin_mgr, pluginname);
              
        if (!collider)
        {
          synldr->ReportError ("crystalspace.terrain.loader.factory",
            node, "Could not load %s!", pluginname);
          return 0;
        }
        
        factory->SetCollider(collider);
        
        break;
      }
      case XMLTOKEN_CELLS:
      {
        csRef<iDocumentNodeIterator> it = child->GetNodes ();
        
        while (it->HasNext())
        {
          csRef<iDocumentNode> child = it->Next ();
          if (child->GetType () != CS_NODE_ELEMENT) continue;
          const char* value = child->GetValue ();
          csStringID id = xmltokens.Request (value);
          switch (id)
          {
            case XMLTOKEN_CELL:
            {
              const char* cell_name = child->GetAttributeValue ("name");
              int grid_width = 0, grid_height = 0, material_width = 0;
              int material_height = 0;
              bool material_persistent = false;
              csVector2 position;
              csVector3 size;
              csRef<iTerrainDataFeeder> feeder;

              render_properties.SetSize (0);
              collision_properties.SetSize (0);
              
              if (!cell_name) cell_name = "";
              
              csRef<iDocumentNodeIterator> it = child->GetNodes ();
        
              while (it->HasNext())
              {
                csRef<iDocumentNode> child = it->Next ();
                if (child->GetType () != CS_NODE_ELEMENT) continue;
                const char* value = child->GetValue ();
                csStringID id = xmltokens.Request (value);
                switch (id)
                {
                  case XMLTOKEN_GRIDSIZE:
                  {
                    grid_width = child->GetAttributeValueAsInt ("x");
                    grid_height = child->GetAttributeValueAsInt ("y");
                    break;
                  }
                  case XMLTOKEN_MATERIALSIZE:
                  {
                    material_width = child->GetAttributeValueAsInt ("x");
                    material_height = child->GetAttributeValueAsInt ("y");
                    break;
                  }
                  case XMLTOKEN_MATERIALPERSISTENT:
                  {
                    material_persistent = true;
                    break;
                  }
                  case XMLTOKEN_POSITION:
                  {
                    position.x = child->GetAttributeValueAsFloat ("x");
                    position.y = child->GetAttributeValueAsFloat ("y");
                    break;
                  }
                  case XMLTOKEN_SIZE:
                  {
                    size.x = child->GetAttributeValueAsFloat ("x");
                    size.y = child->GetAttributeValueAsFloat ("y");
                    size.z = child->GetAttributeValueAsFloat ("z");
                    break;
                  }
                  case XMLTOKEN_FEEDER:
                  {
                    csRef<iDocumentNodeIterator> it = child->GetNodes ();
        
                    while (it->HasNext())
                    {
                      csRef<iDocumentNode> child = it->Next ();
                      if (child->GetType () != CS_NODE_ELEMENT) continue;
                      const char* value = child->GetValue ();
                      csStringID id = xmltokens.Request (value);
                      switch (id)
                      {
                        case XMLTOKEN_PLUGIN:
                        {
                          const char* pluginname = child->GetContentsValue ();
                          feeder = csLoadPlugin<iTerrainDataFeeder> (
                            plugin_mgr, pluginname);
              
                          if (!feeder)
                          {
                            synldr->ReportError (
                              "crystalspace.terrain.loader.factory",
                              node, "Could not load %s!", pluginname);
                            return 0;
                          }
                          break;
                        }
                        case XMLTOKEN_PARAM:
                        {
                          feeder->SetParam (child->GetAttributeValue (
                            "name"), child->GetAttributeValue
                            ("value"));
                          break;
                        }
                        default:
                          synldr->ReportError (
                            "crystalspace.terrain.factory.loader",
                            child, "Unknown token!");
                      }
                    }
                    
                    break;
                  }
                  case XMLTOKEN_RENDERPROPERTIES:
                  {
                    csRef<iDocumentNodeIterator> it = child->GetNodes ();
        
                    while (it->HasNext())
                    {
                      csRef<iDocumentNode> child = it->Next ();
                      if (child->GetType () != CS_NODE_ELEMENT) continue;
                      const char* value = child->GetValue ();
                      csStringID id = xmltokens.Request (value);
                      switch (id)
                      {
                        case XMLTOKEN_PARAM:
                        {
						  ParamValuePair p = {
						    child->GetAttributeValue ("name"),
							child->GetAttributeValue ("value") };

                          render_properties.Push (p);

                          break;
                        }
                        default:
                          synldr->ReportError (
                            "crystalspace.terrain.factory.loader",
                            child, "Unknown token!");
                      }
                    }
                    
                    break;
                  }
                  case XMLTOKEN_COLLISIONPROPERTIES:
                  {
                    csRef<iDocumentNodeIterator> it = child->GetNodes ();
        
                    while (it->HasNext())
                    {
                      csRef<iDocumentNode> child = it->Next ();
                      if (child->GetType () != CS_NODE_ELEMENT) continue;
                      const char* value = child->GetValue ();
                      csStringID id = xmltokens.Request (value);
                      switch (id)
                      {
                        case XMLTOKEN_PARAM:
                        {
						  ParamValuePair p = {
                            child->GetAttributeValue ("name"),
							child->GetAttributeValue ("value") };

                          collision_properties.Push (p);

                          break;
                        }
                        default:
                          synldr->ReportError (
                            "crystalspace.terrain.factory.loader",
                            child, "Unknown token!");
                      }
                    }
                    
                    break;
                  }
                  default:
                    synldr->ReportError ("crystalspace.terrain.factory.loader",
                      child, "Unknown token!");
                }
              }

              iTerrainCell* cell = factory->AddCell(cell_name, grid_width,
                grid_height, material_width, material_height,
                material_persistent, position, size, feeder);

              iTerrainCellRenderProperties* render_p =
                cell->GetRenderProperties ();

              if (render_p)
                for (size_t i = 0; i < render_properties.GetSize (); ++i)
                  render_p->SetParam (render_properties[i].name,
                                      render_properties[i].value);

              iTerrainCellCollisionProperties* collision_p =
                cell->GetCollisionProperties ();
              
              if (collision_p)
                for (size_t i = 0; i < collision_properties.GetSize (); ++i)
                  collision_p->SetParam (collision_properties[i].name,
                                         collision_properties[i].value);

              break;
            }
            default:
              synldr->ReportError ("crystalspace.terrain.factory.loader",
                child, "Unknown token!");
          }
        }
        
        break;
      }
      default:
        synldr->ReportError ("crystalspace.terrain.factory.loader",
          child, "Unknown token!");
    }
  }
  
  return csPtr<iBase> (mesh_factory);
}

csTerrainObjectLoader::csTerrainObjectLoader (iBase* parent)
 : scfImplementationType (this, parent)
{
}

csTerrainObjectLoader::~csTerrainObjectLoader ()
{
}

bool csTerrainObjectLoader::Initialize (iObjectRegistry* objreg)
{
  object_reg = objreg;
  synldr = csQueryRegistry<iSyntaxService> (object_reg);

  xmltokens.Register ("factory", XMLTOKEN_FACTORY);
  xmltokens.Register ("materialpalette", XMLTOKEN_MATERIALPALETTE);
  xmltokens.Register ("material", XMLTOKEN_MATERIAL);
  return true;
}

csPtr<iBase> csTerrainObjectLoader::Parse (iDocumentNode* node, 
  iStreamSource*, iLoaderContext* ldr_context, iBase* /*context*/)
{
  csRef<iMeshObject> mesh;
  csRef<iTerrainSystem> terrain;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_FACTORY:
      {
        const char* factname = child->GetContentsValue ();
        csRef<iMeshFactoryWrapper> fact = ldr_context->FindMeshFactory (
          factname);
        if (!fact)
        {
          synldr->ReportError ("crystalspace.terrain.object.loader",
            child, "Couldn't find factory '%s'!", factname);
          return 0;
        }
        mesh = fact->GetMeshObjectFactory ()->NewInstance ();
        terrain = SCF_QUERY_INTERFACE (mesh, iTerrainSystem);
            
        if (!terrain)
        {
          synldr->ReportError (
                    "crystalspace.terrain.parse.badfactory", child,
                    "Factory '%s' doesn't appear to be a terrain factory!",
                    factname);
          return 0;
        }
            
        break;
      }
      case XMLTOKEN_MATERIALPALETTE:
      {
        csRefArray<iMaterialWrapper> pal;
  
        csRef<iDocumentNodeIterator> it = child->GetNodes ();
        while (it->HasNext ())
        {
          csRef<iDocumentNode> child = it->Next ();
          if (child->GetType () != CS_NODE_ELEMENT) continue;
          const char *value = child->GetValue ();
          csStringID id = xmltokens.Request (value);
          switch (id)
          {
            case XMLTOKEN_MATERIAL:
            {
              const char* matname = child->GetContentsValue ();
              csRef<iMaterialWrapper> mat = ldr_context->FindMaterial (matname);
              if (!mat)
              {
                synldr->ReportError (
                  "crystalspace.terrain.object.loader.materialpalette",
                  child, "Couldn't find material '%s'!", matname);
                return 0;
              }
              pal.Push (mat);
              break;
            }
            default:
              synldr->ReportError (
                "crystalspace.terrain.object.loader.materialpalette",
                child, "Unknown token in materials list!");
          }
        }

        terrain->SetMaterialPalette (pal);
        break;
      }
      default:
        synldr->ReportError ("crystalspace.terrain.object.loader",
          child, "Unknown token");
    }
  }

  return csPtr<iBase>(mesh);
}
