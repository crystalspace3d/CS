/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein

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
#include "cssys/sysfunc.h"
#include "qint.h"
#include "csutil/scf.h"
#include "ivaria/pmeter.h"
#include "csengine/engine.h"
#include "csengine/halo.h"
#include "csengine/camera.h"
#include "csengine/campos.h"
#include "csengine/lview.h"
#include "csengine/light.h"
#include "csengine/meshobj.h"
#include "csengine/cscoll.h"
#include "csengine/sector.h"
#include "csengine/texture.h"
#include "csengine/material.h"
#include "csengine/stats.h"
#include "csengine/region.h"
#include "csengine/radiosty.h"
#include "csengine/objwatch.h"
#include "iengine/portal.h"
#include "csgeom/fastsqrt.h"
#include "csgeom/sphere.h"
#include "csgeom/kdtree.h"
#include "csgfx/csimage.h"
#include "csgfx/memimage.h"
#include "csutil/util.h"
#include "csutil/cfgacc.h"
#include "csutil/databuf.h"
#include "csutil/debug.h"
#include "csutil/vfscache.h"
#include "csutil/xmltiny.h"
#include "igraphic/image.h"
#include "igraphic/imageio.h"
#include "ivideo/halo.h"
#include "ivideo/txtmgr.h"
#include "iutil/vfs.h"
#include "iutil/event.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "iutil/cfgmgr.h"
#include "iutil/databuff.h"
#include "iutil/eventq.h"
#include "iutil/objreg.h"
#include "imap/reader.h"
#include "imap/ldrctxt.h"
#include "imesh/lighting.h"
#include "imesh/thing/thing.h"
#include "ivaria/reporter.h"
#include "ivaria/engseq.h"
#include "iutil/plugin.h"
#include "iutil/virtclk.h"
#include "imesh/thing/polygon.h"
#include "ivideo/graph3d.h"
#include "igeom/clip2d.h"

#if defined(CS_USE_NEW_RENDERER)
#include "ivideo/rendersteps/irenderstep.h"
#include "ivideo/rendersteps/irsfact.h"
#include "ivideo/rendersteps/igeneric.h"
#endif

//---------------------------------------------------------------------------
void csEngine::Report (const char *description, ...)
{
  va_list arg;
  va_start (arg, description);

  if (!Reporter) Reporter = CS_QUERY_REGISTRY (object_reg, iReporter);
  
  if (Reporter)
  {
    Reporter->ReportV (
        CS_REPORTER_SEVERITY_NOTIFY,
        "crystalspace.engine.notify",
        description,
        arg);
  }
  else
  {
    csPrintfV (description, arg);
    csPrintf ("\n");
    fflush (stdout);
  }

  va_end (arg);
}

void csEngine::Warn (const char *description, ...)
{
  va_list arg;
  va_start (arg, description);

  if (!Reporter) Reporter = CS_QUERY_REGISTRY (object_reg, iReporter);

  if (Reporter)
  {
    Reporter->ReportV (
        CS_REPORTER_SEVERITY_WARNING,
        "crystalspace.engine.warning",
        description,
        arg);
  }
  else
  {
    csPrintfV (description, arg);
    csPrintf ("\n");
  }

  va_end (arg);
}

void csEngine::ReportBug (const char *description, ...)
{
  va_list arg;
  va_start (arg, description);

  if (!Reporter) Reporter = CS_QUERY_REGISTRY (object_reg, iReporter);

  if (Reporter)
  {
    Reporter->ReportV (
        CS_REPORTER_SEVERITY_BUG,
        "crystalspace.engine.bug",
        description,
        arg);
  }
  else
  {
    csPrintfV (description, arg);
    csPrintf ("\n");
  }

  va_end (arg);
}

//---------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE(csCameraPositionList)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iCameraPositionList)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csCameraPositionList::CameraPositionList)
  SCF_IMPLEMENTS_INTERFACE(iCameraPositionList)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csCameraPositionList::csCameraPositionList ()
{
  SCF_CONSTRUCT_IBASE (0);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiCameraPositionList);
}

iCameraPosition *csCameraPositionList::NewCameraPosition (const char *name)
{
  csVector3 v (0);
  csCameraPosition *newcp = new csCameraPosition (name, "", v, v, v);
  iCameraPosition *cp = &(newcp->scfiCameraPosition);
  Push (cp);
  cp->DecRef ();
  return cp;
}

iCameraPosition *csCameraPositionList::CameraPositionList::NewCameraPosition (
  const char *name)
{
  return scfParent->NewCameraPosition (name);
}

int csCameraPositionList::CameraPositionList::GetCount () const
{
  return scfParent->Length ();
}

iCameraPosition *csCameraPositionList::CameraPositionList::Get (int n) const
{
  return scfParent->Get (n);
}

int csCameraPositionList::CameraPositionList::Add (iCameraPosition *obj)
{
  return scfParent->Push (obj);
}

bool csCameraPositionList::CameraPositionList::Remove (iCameraPosition *obj)
{
  return scfParent->Delete (obj);
}

bool csCameraPositionList::CameraPositionList::Remove (int n)
{
  return scfParent->Delete (n);
}

void csCameraPositionList::CameraPositionList::RemoveAll ()
{
  scfParent->DeleteAll ();
}

int csCameraPositionList::CameraPositionList::Find (
  iCameraPosition *obj) const
{
  return scfParent->Find (obj);
}

iCameraPosition *csCameraPositionList::CameraPositionList::FindByName (
  const char *Name) const
{
  return scfParent->FindByName (Name);
}

//---------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE(csCollectionList)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iCollectionList)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csCollectionList::CollectionList)
  SCF_IMPLEMENTS_INTERFACE(iCollectionList)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csCollectionList::csCollectionList ()
{
  SCF_CONSTRUCT_IBASE (0);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiCollectionList);
}

iCollection *csCollectionList::NewCollection (const char *name)
{
  csCollection *c = new csCollection (csEngine::current_engine);
  c->SetName (name);
  Push (&(c->scfiCollection));
  c->DecRef ();
  return &(c->scfiCollection);
}

iCollection *csCollectionList::CollectionList::NewCollection (
  const char *name)
{
  return scfParent->NewCollection (name);
}

int csCollectionList::CollectionList::GetCount () const
{
  return scfParent->Length ();
}

iCollection *csCollectionList::CollectionList::Get (int n) const
{
  return scfParent->Get (n);
}

int csCollectionList::CollectionList::Add (iCollection *obj)
{
  return scfParent->Push (obj);
}

bool csCollectionList::CollectionList::Remove (iCollection *obj)
{
  return scfParent->Delete (obj);
}

bool csCollectionList::CollectionList::Remove (int n)
{
  return scfParent->Delete (n);
}

void csCollectionList::CollectionList::RemoveAll ()
{
  scfParent->DeleteAll ();
}

int csCollectionList::CollectionList::Find (iCollection *obj) const
{
  return scfParent->Find (obj);
}

iCollection *csCollectionList::CollectionList::FindByName (
  const char *Name) const
{
  return scfParent->FindByName (Name);
}

//---------------------------------------------------------------------------

void csEngineMeshList::FreeItem (iMeshWrapper* mesh)
{
  mesh->GetMovable ()->ClearSectors ();
  csMeshList::FreeItem (mesh);
}

//---------------------------------------------------------------------------

/**
 * Iterator to iterate over sectors in the engine which are within
 * a radius from a given point.
 * This iterator assumes there are no fundamental changes
 * in the engine while it is being used.
 * If changes to the engine happen the results are unpredictable.
 */
class csSectorIt : public iSectorIterator
{
private:
  // The position and radius.
  iSector *sector;
  csVector3 pos;
  float radius;

  // Polygon index (to loop over all portals).
  // If -1 then we return current sector first.
  int cur_poly;

  // If not null then this is a recursive sector iterator
  // that we are currently using.
  csSectorIt *recursive_it;

  // If true then this iterator has ended.
  bool has_ended;

  // Last position (from Fetch).
  csVector3 last_pos;

  // Current sector.
  iSector* current_sector;

  /// Get sector from iterator. Return 0 at end.
  iSector *FetchNext ();

public:
  /// Construct an iterator and initialize to start.
  csSectorIt (iSector *sector, const csVector3 &pos, float radius);

  /// Destructor.
  virtual ~csSectorIt ();

  SCF_DECLARE_IBASE;

  /// Restart iterator.
  virtual void Reset ();

  /// Return true if there are more elements.
  virtual bool HasNext ();

  /// Get sector from iterator. Return 0 at end.
  virtual iSector *Next ();

  /**
   * Get last position that was used from Fetch. This can be
   * different from 'pos' because of space warping.
   */
  virtual const csVector3 &GetLastPosition () { return last_pos; }
};

/**
 * Iterator to iterate over meshes in the given list.
 */
class csMeshListIt : public iMeshWrapperIterator
{
  friend class csEngine;
private:
  csPArray<iMeshWrapper>* list;
  int num_objects;

  // Current index.
  int cur_idx;

private:
  /// Construct an iterator and initialize to start.
  csMeshListIt (csPArray<iMeshWrapper>* list);

public:
  /// Destructor.
  virtual ~csMeshListIt ();

  SCF_DECLARE_IBASE;

  virtual void Reset ();
  virtual iMeshWrapper* Next ();
  virtual bool HasNext () const;
};

/**
 * Iterator to iterate over objects in the given list.
 */
class csObjectListIt : public iObjectIterator
{
  friend class csEngine;
private:
  csPArray<iObject>* list;
  int num_objects;

  // Current index.
  int cur_idx;

private:
  /// Construct an iterator and initialize to start.
  csObjectListIt (csPArray<iObject>* list);

public:
  /// Destructor.
  virtual ~csObjectListIt ();

  SCF_DECLARE_IBASE;

  virtual void Reset ();
  virtual iObject* Next ();
  virtual bool HasNext () const;
  virtual iObject *GetParentObj () const
  {
    return 0;
  }

  virtual iObject* FindName (const char *name)  { (void)name; return 0; }
};

//---------------------------------------------------------------------------

SCF_IMPLEMENT_IBASE(csLightIt)
  SCF_IMPLEMENTS_INTERFACE (iLightIterator)
SCF_IMPLEMENT_IBASE_END

csLightIt::csLightIt (csEngine *e, iRegion *r) :
  engine(e),
  region(r)
{
  SCF_CONSTRUCT_IBASE (0);
  Reset ();
}

bool csLightIt::NextSector ()
{
  sector_idx++;
  if (region)
    while ( sector_idx < engine->sectors.GetCount () &&
      	!region->IsInRegion (GetLastSector ()->QueryObject ()))
      sector_idx++;
  if (sector_idx >= engine->sectors.GetCount ()) return false;
  return true;
}

void csLightIt::Reset ()
{
  sector_idx = -1;
  light_idx = 0;
  current_light = 0;
}

iLight *csLightIt::FetchNext ()
{
  iSector *sector;
  if (sector_idx == -1)
  {
    if (!NextSector ()) return 0;
    light_idx = -1;
  }

  if (sector_idx >= engine->sectors.GetCount ()) return 0;
  sector = engine->sectors.Get (sector_idx);

  // Try next light.
  light_idx++;

  if (light_idx >= sector->GetLights ()->GetCount ())
  {
    // Go to next sector.
    light_idx = -1;
    if (!NextSector ()) return 0;

    // Initialize iterator to start of sector and recurse.
    return FetchNext ();
  }

  return sector->GetLights ()->Get (light_idx);
}

bool csLightIt::HasNext ()
{
  if (current_light != 0) return true;
  current_light = FetchNext ();
  return current_light != 0;
}

iLight *csLightIt::Next ()
{
  if (current_light == 0)
  {
    current_light = FetchNext ();
  }

  iLight* l = current_light;
  current_light = 0;
  return l;
}

iSector *csLightIt::GetLastSector ()
{
  return engine->sectors.Get (sector_idx);
}

//---------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE(csSectorIt)
  SCF_IMPLEMENTS_INTERFACE(iSectorIterator)
SCF_IMPLEMENT_IBASE_END

csSectorIt::csSectorIt (
  iSector *sector,
  const csVector3 &pos,
  float radius)
{
  SCF_CONSTRUCT_IBASE (0);
  csSectorIt::sector = sector;
  csSectorIt::pos = pos;
  csSectorIt::radius = radius;
  recursive_it = 0;

  Reset ();
}

csSectorIt::~csSectorIt ()
{
  delete recursive_it;
}

void csSectorIt::Reset ()
{
  cur_poly = -1;
  delete recursive_it;
  recursive_it = 0;
  has_ended = false;
  current_sector = 0;
}

iSector *csSectorIt::FetchNext ()
{
  if (has_ended) return 0;

  if (recursive_it)
  {
    iSector *rc = recursive_it->FetchNext ();
    if (rc)
    {
      last_pos = recursive_it->GetLastPosition ();
      return rc;
    }

    delete recursive_it;
    recursive_it = 0;
  }

  if (cur_poly == -1)
  {
    cur_poly = 0;
    last_pos = pos;
    return sector;
  }

  // End search.
  has_ended = true;
  return 0;
}

bool csSectorIt::HasNext ()
{
  if (current_sector != 0) return true;
  current_sector = FetchNext ();
  return current_sector != 0;
}

iSector *csSectorIt::Next ()
{
  if (current_sector == 0)
  {
    current_sector = FetchNext ();
  }

  iSector* s = current_sector;
  current_sector = 0;
  return s;
}


//---------------------------------------------------------------------------

SCF_IMPLEMENT_IBASE(csObjectListIt)
  SCF_IMPLEMENTS_INTERFACE(iObjectIterator)
SCF_IMPLEMENT_IBASE_END

csObjectListIt::csObjectListIt (csPArray<iObject>* list)
{
  SCF_CONSTRUCT_IBASE (0);
  csObjectListIt::list = list;
  num_objects = list->Length ();
  Reset ();
}

csObjectListIt::~csObjectListIt ()
{
  delete list;
}

void csObjectListIt::Reset ()
{
  cur_idx = 0;
}

iObject* csObjectListIt::Next ()
{
  if (cur_idx >= num_objects) return 0;
  cur_idx++;
  return (*list)[cur_idx-1];
}

bool csObjectListIt::HasNext () const
{
  return cur_idx < num_objects;
}

//---------------------------------------------------------------------------

SCF_IMPLEMENT_IBASE(csMeshListIt)
  SCF_IMPLEMENTS_INTERFACE(iMeshWrapperIterator)
SCF_IMPLEMENT_IBASE_END

csMeshListIt::csMeshListIt (csPArray<iMeshWrapper>* list)
{
  SCF_CONSTRUCT_IBASE (0);
  csMeshListIt::list = list;
  num_objects = list->Length ();
  Reset ();
}

csMeshListIt::~csMeshListIt ()
{
  delete list;
}

void csMeshListIt::Reset ()
{
  cur_idx = 0;
}

iMeshWrapper* csMeshListIt::Next ()
{
  if (cur_idx >= num_objects) return 0;
  cur_idx++;
  return (*list)[cur_idx-1];
}

bool csMeshListIt::HasNext () const
{
  return cur_idx < num_objects;
}

//---------------------------------------------------------------------------
int csEngine:: frame_width;
int csEngine:: frame_height;
iObjectRegistry *csEngine:: object_reg = 0;
csEngine *csEngine:: current_engine = 0;
iEngine *csEngine:: current_iengine = 0;
bool csEngine:: use_new_radiosity = false;
int csEngine:: max_process_polygons = 2000000000;
int csEngine:: cur_process_polygons = 0;
int csEngine:: lightcache_mode = CS_ENGINE_CACHE_READ | CS_ENGINE_CACHE_NOUPDATE;
int csEngine:: lightmap_quality = 3;
bool csEngine:: do_force_revis = false;
bool csEngine:: do_rad_debug = false;
int csEngine:: max_lightmap_w = 0;
int csEngine:: max_lightmap_h = 0;

SCF_IMPLEMENT_IBASE(csEngine)
  SCF_IMPLEMENTS_INTERFACE(iEngine)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iComponent)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iConfig)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iObject)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iDebugHelper)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csEngine::eiComponent)
  SCF_IMPLEMENTS_INTERFACE(iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csEngine::DebugHelper)
  SCF_IMPLEMENTS_INTERFACE(iDebugHelper)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE
  (csEngine::iObjectInterface) void *itf = csObject::QueryInterface
    (iInterfaceID, iVersion);
  if (itf) return itf;
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (csEngine::EventHandler)
  SCF_IMPLEMENTS_INTERFACE(iEventHandler)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_FACTORY (csEngine)

csEngine::csEngine (iBase *iParent) :
  sectors(true)
{
  SCF_CONSTRUCT_IBASE (iParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiComponent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiConfig);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiObject);
  DG_TYPE (&scfiObject, "csEngine");
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiDebugHelper);
  //rad_debug = 0;
  first_dyn_lights = 0;
  object_reg = 0;
  textures = 0;
  materials = 0;
  render_context = 0;
  shared_variables = 0;
  current_camera = 0;
  current_engine = this;
  current_iengine = (iEngine*)this;
  scfiEventHandler = 0;
  clear_zbuf = false;
  clear_screen = false;
  nextframe_pending = 0;
  default_max_lightmap_w = 256;
  default_max_lightmap_h = 256;
  default_clear_zbuf = false;
  default_clear_screen = false;
  cache_mgr = 0;

  default_fastmesh_thresshold = 500;
  fastmesh_thresshold = 500;

  textures = new csTextureList ();
  materials = new csMaterialList ();
  shared_variables = new csSharedVariableList();

  BuildSqrtTable ();
  resize = false;

  ClearRenderPriorities ();
}

csEngine::~csEngine ()
{
  if (scfiEventHandler)
  {
    csRef<iEventQueue> q (CS_QUERY_REGISTRY (object_reg, iEventQueue));
    if (q != 0)
      q->RemoveListener (scfiEventHandler);

    scfiEventHandler->DecRef ();
  }

  DeleteAll ();

  int i;
  for (i = 0; i < render_priorities.Length (); i++)
  {
    char *n = (char *)render_priorities[i];
    delete[] n;
  }

  render_priorities.DeleteAll ();

  // @@@TOTALLY DISABLED FOR NOW delete rad_debug;
  delete materials;
  delete textures;
  delete shared_variables;
#if defined(CS_USE_NEW_RENDERER) && defined(CS_NR_ALTERNATE_RENDERLOOP)
  delete renderLoopManager;
#endif
}

bool csEngine::Initialize (iObjectRegistry *object_reg)
{
  csEngine::object_reg = object_reg;

  virtual_clock = CS_QUERY_REGISTRY (object_reg, iVirtualClock);
  if (!virtual_clock) return false;

#ifndef CS_USE_NEW_RENDERER
  G3D = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
#else
  G3D = CS_QUERY_REGISTRY (object_reg, iRender3D);
#endif // CS_USE_NEW_RENDERER
  if (!G3D)
  {
    // If there is no G3D then we still allow initialization of the
    // engine because it might be useful to use the engine stand-alone
    // (i.e. for calculating lighting and so on).
    Warn ("No 3D driver!");
  }

  VFS = CS_QUERY_REGISTRY (object_reg, iVFS);
  if (!VFS) return false;

  if (G3D)
    G2D = G3D->GetDriver2D ();
  else
    G2D = 0;

  // don't check for failure; the engine can work without the image loader
  ImageLoader = CS_QUERY_REGISTRY (object_reg, iImageIO);
  if (!ImageLoader) Warn ("No image loader. Loading images will fail.");

  // Reporter is optional.
  Reporter = CS_QUERY_REGISTRY (object_reg, iReporter);

  // Tell event queue that we want to handle broadcast events
  if (!scfiEventHandler) scfiEventHandler = new EventHandler (this);

  csRef<iEventQueue> q (CS_QUERY_REGISTRY (object_reg, iEventQueue));
  if (q)
    q->RegisterListener (scfiEventHandler, CSMASK_Broadcast);

  csConfigAccess cfg (object_reg, "/config/engine.cfg");
  ReadConfig (cfg);

#if defined(CS_USE_NEW_RENDERER) && defined(CS_NR_ALTERNATE_RENDERLOOP)
  renderLoopManager = new csRenderLoopManager (this);
  defaultRenderLoop = CreateDefaultRenderLoop ();
  renderLoopManager->Register (CS_DEFAULT_RENDERLOOP_NAME, 
    defaultRenderLoop);
#endif

  return true;
}

// Handle some system-driver broadcasts
bool csEngine::HandleEvent (iEvent &Event)
{
  if (Event.Type == csevBroadcast)
  {
    switch (Event.Command.Code)
    {
      case cscmdSystemOpen:
        {
          if (G3D)
          {
#ifndef CS_USE_NEW_RENDERER
            csGraphics3DCaps *caps = G3D->GetCaps ();
            fogmethod = caps->fog;
            NeedPO2Maps = caps->NeedsPO2Maps;
            MaxAspectRatio = caps->MaxAspectRatio;
#else
            NeedPO2Maps = false;
            MaxAspectRatio = 4096;
#endif // CS_USE_NEW_RENDERER
            frame_width = G3D->GetWidth ();
            frame_height = G3D->GetHeight ();
          }
          else
          {
#ifndef CS_USE_NEW_RENDERER
            fogmethod = G3DFOGMETHOD_NONE;
#endif // CS_USE_NEW_RENDERER
            NeedPO2Maps = false;
            MaxAspectRatio = 4096;
            frame_width = 640;
            frame_height = 480;
          }

          if (csCamera::GetDefaultFOV () == 0)
            csCamera::SetDefaultFOV (frame_height, frame_width);

          // Allow context resizing since we handle cscmdContextResize
          if (G2D) G2D->AllowResize (true);

          StartEngine ();

          return true;
        }

      case cscmdSystemClose:
        {
          // We must free all material and texture handles since after
          // G3D->Close() they all become invalid, no matter whenever
          // we did or didn't an IncRef on them.
          DeleteAll ();
          return true;
        }

      case cscmdContextResize:
        {
          if (((iGraphics2D *)Event.Command.Info) == G2D)
            resize = true;
          return false;
        }

      case cscmdContextClose:
        {
          return false;
        }
    } /* endswitch */
  }

  return false;
}

iMeshObjectType* csEngine::GetThingType ()
{
  if (!thing_type)
  {
    csRef<iPluginManager> plugin_mgr = CS_QUERY_REGISTRY (object_reg,
  	  iPluginManager);
    thing_type = CS_QUERY_PLUGIN_CLASS (
        plugin_mgr, "crystalspace.mesh.object.thing",
        iMeshObjectType);
    if (!thing_type)
    {
      thing_type = CS_LOAD_PLUGIN (plugin_mgr,
    	  "crystalspace.mesh.object.thing", iMeshObjectType);
    }
  }

  return (iMeshObjectType*)thing_type;
}

void csEngine::DeleteAll ()
{
  nextframe_pending = 0;
#ifndef CS_USE_NEW_RENDERER
  if (G3D) G3D->ClearCache ();
#endif // CS_USE_NEW_RENDERER
  halos.DeleteAll ();
  collections.DeleteAll ();

  GetMeshes ()->RemoveAll ();
  mesh_factories.RemoveAll ();
  sectors.RemoveAll ();
  camera_positions.DeleteAll ();

  while (first_dyn_lights)
  {
    csDynLight *dyn = first_dyn_lights->GetNext ();
    delete first_dyn_lights;
    first_dyn_lights = dyn;
  }

  delete materials;
  materials = new csMaterialList ();
  delete textures;
  textures = new csTextureList ();
  delete shared_variables;
  shared_variables = new csSharedVariableList();

  GetThingType ();
  if (thing_type != 0)
  {
    csRef<iThingEnvironment> te (
  	SCF_QUERY_INTERFACE (thing_type, iThingEnvironment));
    CS_ASSERT (((iThingEnvironment*)te) != 0);
    te->Clear ();
  }

  render_context = 0;

  // Clear all regions.
  regions.DeleteAll ();

  // Clear all render priorities.
  ClearRenderPriorities ();

  // remove objects
  QueryObject ()->ObjRemoveAll ();
}

iObject *csEngine::QueryObject ()
{
  return &scfiObject;
}

void csEngine::RegisterRenderPriority (
  const char *name,
  long priority,
  int rendsort,
  bool do_camera)
{
  int i;

  // If our priority goes over the number of defined priorities
  // then we have to initialize.
  if (priority + 1 >= render_priority_sortflags.Limit ())
  {
    render_priority_sortflags.SetLimit (priority + 2);
    render_priority_cameraflags.SetLimit (priority + 2);
  }
  for (i = render_priorities.Length (); i <= priority; i++)
  {
    render_priorities[i] = 0;
    render_priority_sortflags[i] = CS_RENDPRI_NONE;
    render_priority_cameraflags[i] = false;
  }

  char *n = (char *)render_priorities[priority];
  delete[] n;
  n = csStrNew (name);
  render_priorities[priority] = n;
  render_priority_sortflags[priority] = rendsort;
  render_priority_cameraflags[priority] = do_camera;
  if (!strcmp (name, "sky"))
    render_priority_sky = priority;
  else if (!strcmp (name, "wall"))
    render_priority_wall = priority;
  else if (!strcmp (name, "object"))
    render_priority_object = priority;
  else if (!strcmp (name, "alpha"))
    render_priority_alpha = priority;
}

long csEngine::GetRenderPriority (const char *name) const
{
  int i;
  for (i = 0; i < render_priorities.Length (); i++)
  {
    char *n = (char *)render_priorities[i];
    if (n && !strcmp (name, n)) return i;
  }

  return 0;
}

void csEngine::SetRenderPriorityCamera (long priority, bool do_camera)
{
  render_priority_cameraflags[priority] = do_camera;
}

bool csEngine::GetRenderPriorityCamera (const char *name) const
{
  int i;
  for (i = 0; i < render_priorities.Length (); i++)
  {
    char *n = (char *)render_priorities[i];
    if (n && !strcmp (name, n)) return render_priority_cameraflags[i];
  }

  return false;
}

bool csEngine::GetRenderPriorityCamera (long priority) const
{
  return render_priority_cameraflags[priority];
}

int csEngine::GetRenderPrioritySorting (const char *name) const
{
  int i;
  for (i = 0; i < render_priorities.Length (); i++)
  {
    char *n = (char *)render_priorities[i];
    if (n && !strcmp (name, n)) return render_priority_sortflags[i];
  }

  return CS_RENDPRI_NONE;
}

int csEngine::GetRenderPrioritySorting (long priority) const
{
  return render_priority_sortflags[priority];
}

void csEngine::ClearRenderPriorities ()
{
  int i;
  for (i = 0; i < render_priorities.Length (); i++)
  {
    char *n = (char *)render_priorities[i];
    delete[] n;
  }

  render_priorities.DeleteAll ();
  render_priority_sortflags.SetLimit (0);
  render_priority_cameraflags.SetLimit (0);
  RegisterRenderPriority ("sky", 2, true);
  RegisterRenderPriority ("wall", 4);
  RegisterRenderPriority ("object", 6);
  RegisterRenderPriority ("alpha", 8, CS_RENDPRI_BACK2FRONT);
}

int csEngine::GetRenderPriorityCount () const
{
  return render_priorities.Length ();
}

const char* csEngine::GetRenderPriorityName (long priority) const
{
  if (priority < 0 && priority >= render_priorities.Length ()) return 0;
  return (const char*)render_priorities[priority];
}

void csEngine::ResetWorldSpecificSettings()
{
  SetClearZBuf (default_clear_zbuf);
  SetClearScreen (default_clear_screen);
  csRef<iThingEnvironment> te (SCF_QUERY_INTERFACE (
          GetThingType (),
          iThingEnvironment));
  te->SetLightmapCellSize (16);
  SetMaxLightmapSize (default_max_lightmap_w, default_max_lightmap_h);
  SetAmbientLight (csColor (
  	default_ambient_red / 255.0f,
	default_ambient_green / 255.0f, 
        default_ambient_blue / 255.0f));
  fastmesh_thresshold = default_fastmesh_thresshold;
#ifdef CS_USE_NEW_RENDERER
  iRenderLoop* defaultRL = renderLoopManager->Retrieve 
    (CS_DEFAULT_RENDERLOOP_NAME);
  SetCurrentDefaultRenderloop (defaultRL);
#endif
}

void csEngine::PrepareTextures ()
{
  int i;

  iTextureManager *txtmgr = G3D->GetTextureManager ();

  // First register all textures to the texture manager.
  for (i = 0; i < textures->Length (); i++)
  {
    iTextureWrapper *csth = textures->Get (i);
    if (!csth->GetTextureHandle ()) csth->Register (txtmgr);
    if (!csth->KeepImage ()) csth->SetImageFile (0);
  }

  // Prepare all the textures.
  txtmgr->PrepareTextures ();

  // Then register all materials to the texture manager.
  for (i = 0; i < materials->Length (); i++)
  {
    iMaterialWrapper *csmh = materials->Get (i);
    if (!csmh->GetMaterialHandle ()) csmh->Register (txtmgr);
  }

  // Prepare all the materials.
  txtmgr->PrepareMaterials ();
}

void csEngine::PrepareMeshes ()
{
  int i;
  for (i = 0; i < meshes.GetCount (); i++)
  {
    iMeshWrapper *sp = meshes.Get (i);
    sp->GetMovable ()->UpdateMove ();
  }
}

bool csEngine::Prepare (iProgressMeter *meter)
{
  PrepareTextures ();
  PrepareMeshes ();

  // The images are no longer needed by the 3D engine.
  iTextureManager *txtmgr = G3D->GetTextureManager ();
  txtmgr->FreeImages ();

#ifndef CS_USE_NEW_RENDERER
  G3D->ClearCache ();
#endif // CS_USE_NEW_RENDERER

  // Prepare lightmaps if we have any sectors
  if (sectors.GetCount ()) ShineLights (0, meter);

  CheckConsistency ();

  return true;
}

void csEngine::ForceRelight (iRegion* region, iProgressMeter *meter)
{
#ifndef CS_USE_NEW_RENDERER
  G3D->ClearCache ();
#endif // CS_USE_NEW_RENDERER
  int old_lightcache_mode = lightcache_mode;
  lightcache_mode &= ~CS_ENGINE_CACHE_READ;
  lightcache_mode &= ~CS_ENGINE_CACHE_NOUPDATE;
  ShineLights (region, meter);
  lightcache_mode = old_lightcache_mode;
}

void csEngine::ForceRelight (iStatLight* light, iRegion* region)
{
#ifndef CS_USE_NEW_RENDERER
  G3D->ClearCache ();
#endif // CS_USE_NEW_RENDERER

  int sn;
  int num_meshes = meshes.GetCount ();

  for (sn = 0; sn < num_meshes; sn++)
  {
    iMeshWrapper *s = meshes.Get (sn);
    if (!region || region->IsInRegion (s->QueryObject ()))
    {
      iLightingInfo* linfo = s->GetLightingInfo ();
      if (linfo)
      {
	// Do not clear!
        linfo->InitializeDefault (false);
      }
    }
  }

  ((csStatLight *)(light->GetPrivateObject ()))->CalculateLighting ();

  iCacheManager* cm = GetCacheManager ();
  for (sn = 0 ; sn < num_meshes ; sn++)
  {
    iMeshWrapper *s = meshes.Get (sn);
    if (!region || region->IsInRegion (s->QueryObject ()))
    {
      iLightingInfo* linfo = s->GetLightingInfo ();
      if (linfo)
      {
	if (lightcache_mode & CS_ENGINE_CACHE_WRITE)
          linfo->WriteToCache (cm);
        linfo->PrepareLighting ();
      }
    }
  }

  if (!VFS->Sync())
  {
    Warn ("Error updating lighttable cache!");
    Warn ("Perhaps disk full or no write permission?");
  }
}

void csEngine::RemoveLight (iStatLight* light)
{
#ifndef CS_USE_NEW_RENDERER
  G3D->ClearCache ();
#endif // CS_USE_NEW_RENDERER

  int sn;
  int num_meshes = meshes.GetCount ();

  for (sn = 0; sn < num_meshes; sn++)
  {
    iMeshWrapper *s = meshes.Get (sn);
    iLightingInfo* linfo = s->GetLightingInfo ();
    if (linfo)
    {
      linfo->StaticLightDisconnect (light);
    }
  }
  light->QueryLight ()->GetSector ()->GetLights ()
  	->Remove (light->QueryLight ());
}

void csEngine::SetCacheManager (iCacheManager* cache_mgr)
{
  csEngine::cache_mgr = cache_mgr;
}

iCacheManager* csEngine::GetCacheManager ()
{
  if (!cache_mgr)
  {
    char buf[512];
    strcpy (buf, VFS->GetCwd ());
    if (buf[strlen (buf)-1] == '/')
      strcat (buf, "cache");
    else
      strcat (buf, "/cache");
    cache_mgr = csPtr<iCacheManager> (
    	new csVfsCacheManager (object_reg, buf));
  }
  return cache_mgr;
}

void csEngine::ShineLights (iRegion *region, iProgressMeter *meter)
{
  // If we have to read from the cache then we check if the 'precalc_info'
  // file exists on the VFS. If not then we cannot use the cache.
  // If the file exists but is not valid (some flags are different) then
  // we cannot use the cache either.
  // If we recalculate then we also update this 'precalc_info' file with
  // the new settings.
  struct PrecalcInfo
  {
    int lm_version; // This number identifies a version of the lightmap format.

    // If different then the format is different and we need
    // to recalculate.
    int normal_light_level; // Normal light level (unlighted level).
    int ambient_red;
    int ambient_green;
    int ambient_blue;
    int reflect;
    int radiosity;
    float cosinus_factor;
    int lightmap_size;
  };

  PrecalcInfo current;
  memset (&current, 0, sizeof (current));
  current.lm_version = 2;
  current.normal_light_level = CS_NORMAL_LIGHT_LEVEL;
  current.ambient_red = csLight::ambient_red;
  current.ambient_green = csLight::ambient_green;
  current.ambient_blue = csLight::ambient_blue;
  current.reflect = csSector::cfg_reflections;
  current.radiosity = (int)csSector::do_radiosity;
  current.cosinus_factor = 0;	//@@@
  current.lightmap_size = 0;	//@@@

  char *reason = 0;

  bool do_relight = false;
  if (!(lightcache_mode & CS_ENGINE_CACHE_READ))
  {
    if (!(lightcache_mode & CS_ENGINE_CACHE_NOUPDATE))
      do_relight = true;
    else if (lightcache_mode & CS_ENGINE_CACHE_WRITE)
      do_relight = true;
  }

  iCacheManager* cm = GetCacheManager ();
  csRef<iDataBuffer> data = 0;
  if (lightcache_mode & CS_ENGINE_CACHE_READ)
    data = cm->ReadCache ("lm_precalc_info", 0, ~0);

  if (!data)
  {
    reason = "no 'lm_precalc_info' found in cache";
  }
  else
  {
    // data, 0-terminated
    csDataBuffer* ntData = new csDataBuffer (data);
    data = 0;
    char *input = **ntData;
    while (*input)
    {
      char *keyword = input + strspn (input, " \t");
      char *endkw = strchr (input, '=');
      if (!endkw) break;
      *endkw++ = 0;
      input = strchr (endkw, '\n');
      if (input) *input++ = 0;

      float xf;
      sscanf (endkw, "%f", &xf);

      int xi = QRound (xf);

      if (!strcmp (keyword, "LMVERSION"))
      {
	if (xi != current.lm_version)
	{
	  reason = "lightmap format changed";
	  break;
	}
      }
    }
    delete ntData;
  }

  if (reason)
  {
    char data[500];
    sprintf (
      data,
      "LMVERSION=%d\nNORMALLIGHTLEVEL=%d\nAMBIENT_RED=%d\nAMBIENT_GREEN=%d\nAMBIENT_BLUE=%d\nREFLECT=%d\nRADIOSITY=%d\nCOSINUS_FACTOR=%g\nLIGHTMAP_SIZE=%d\n",
      current.lm_version,
      current.normal_light_level,
      current.ambient_red,
      current.ambient_green,
      current.ambient_blue,
      current.reflect,
      current.radiosity,
      current.cosinus_factor,
      current.lightmap_size);
    if (lightcache_mode & CS_ENGINE_CACHE_WRITE)
    {
      cm->CacheData (data, strlen (data), "lm_precalc_info", 0, ~0);
    }
    if (do_relight)
    {
      Report ("Lightmaps are not up to date (%s).", reason);
    }
    else
    {
      Warn ("Lightmaps are not up to date (%s).", reason);
      Warn ("Use -relight cmd option to calc lighting.");
    }
    lightcache_mode &= ~CS_ENGINE_CACHE_READ;
  }

  // Recalculate do_relight since the cache mode might have changed.
  do_relight = false;
  if (!(lightcache_mode & CS_ENGINE_CACHE_READ))
  {
    if (!(lightcache_mode & CS_ENGINE_CACHE_NOUPDATE))
      do_relight = true;
    else if (lightcache_mode & CS_ENGINE_CACHE_WRITE)
      do_relight = true;
  }

  if (do_relight)
  {
    Report ("Recalculation of lightmaps forced.");
  }
  else
  {
    // If no recalculation is forced we set these variables to default to
    // make sure that we don't do too many unneeded calculations.
    csSector::do_radiosity = false;
    csSector::cfg_reflections = 1;
  }

  csRef<iLightIterator> lit = GetLightIterator (region);

  // Count number of lights to process.
  iLight *l;
  int light_count = 0;
  lit->Reset ();
  while (lit->HasNext ()) { lit->Next (); light_count++; }

  int sn = 0;
  int num_meshes = meshes.GetCount ();

  if (do_relight)
  {
    Report ("Initializing lighting (%d meshes).", num_meshes);
    if (meter)
    {
      meter->SetProgressDescription (
        "crystalspace.engine.lighting.init",
        "Initializing lighting (%d meshes).",
        num_meshes);
      meter->SetTotal (num_meshes);
      meter->Restart ();
    }
  }

  int failed = 0;
  int tot_failed_meshes = 0;
  iMeshWrapper* failed_meshes[4];
  for (sn = 0; sn < num_meshes; sn++)
  {
    iMeshWrapper *s = meshes.Get (sn);
    if (!region || region->IsInRegion (s->QueryObject ()))
    {
      iLightingInfo* linfo = s->GetLightingInfo ();
      if (linfo)
      {
        if (do_relight)
          linfo->InitializeDefault (true);
        else
          if (!linfo->ReadFromCache (cm))
	  {
	    if (tot_failed_meshes < 4)
	    {
	      failed_meshes[tot_failed_meshes] = s;
	      tot_failed_meshes++;
	    }
	    failed++;
          }
      }
    }

    if (do_relight && meter) meter->Step ();
  }
  if (failed > 0)
  {
    Warn ("Couldn't load cached lighting for %d object(s):", failed);
    for (sn = 0 ; sn < tot_failed_meshes ; sn++)
    {
      Warn ("    %s", failed_meshes[sn]->QueryObject ()->GetName ());
    }
    if (tot_failed_meshes < failed)
      Warn ("    ...");
    Warn ("Use -relight cmd option to refresh lighting.");
  }

  csTicks start, stop;
  start = csGetTicks();
  if (do_relight)
  {
    Report ("Shining lights (%d lights).", light_count);
    if (meter)
    {
      meter->SetProgressDescription (
          "crystalspace.engine.lighting.calc",
        "Shining lights (%d lights)",
        light_count);
      meter->SetTotal (light_count);
      meter->Restart ();
    }

    lit->Reset ();
    while (lit->HasNext ())
    {
      l = lit->Next ();
      ((csStatLight *)(l->GetPrivateObject ()))->CalculateLighting ();
      if (meter) meter->Step ();
    }

    stop = csGetTicks ();
    Report ("Time taken: %.4f seconds.", (float)(stop - start) / 1000.);
  }

#if 0
  // TOTALLY DISABLED FOR NOW
  // Render radiosity
  if (use_new_radiosity && do_relight)
  {
    start = csGetTicks ();

    csRadiosity *rad = new csRadiosity (this, meter);
    if (do_rad_debug)
    {
      rad_debug = rad;
    }
    else
    {
      rad->DoRadiosity ();
      delete rad;
    }

    stop = csGetTicks ();
    if (do_relight)
      Report ("Time taken: %.4f seconds.", (float)(stop - start) / 1000.);
  }
#endif

  if (do_relight && (lightcache_mode & CS_ENGINE_CACHE_WRITE))
  {
    Report ("Caching lighting (%d meshes).", num_meshes);
    if (meter)
    {
      meter->SetProgressDescription (
          "crystalspace.engine.lighting.cache",
          "Caching lighting (%d meshes)",
          num_meshes);
      meter->SetTotal (num_meshes);
      meter->Restart ();
    }
  }

  for (sn = 0; sn < num_meshes; sn++)
  {
    iMeshWrapper *s = meshes.Get (sn);
    if (!region || region->IsInRegion (s->QueryObject ()))
    {
      iLightingInfo* linfo = s->GetLightingInfo ();
      if (linfo)
      {
	if (do_relight && (lightcache_mode & CS_ENGINE_CACHE_WRITE))
          linfo->WriteToCache (cm);
        linfo->PrepareLighting ();
      }
    }

    if (do_relight && meter) meter->Step ();
  }

  if (do_relight && (lightcache_mode & CS_ENGINE_CACHE_WRITE))
  {
    Report ("Updating VFS....");
    if (!VFS->Sync())
    {
      Warn ("Error updating lighttable cache!");
      Warn ("Perhaps disk full or no write permission?");
    }
    Report ("DONE!");
  }
}

void csEngine::InvalidateLightmaps ()
{
#if 0
  //@@@@ TODO!!
  csPolyIt *pit = NewPolyIterator ();
  csPolygon3D *p;
  while ((p = pit->Fetch ()) != 0)
  {
    csPolyTexLightMap *lmi = p->GetLightMapInfo ();
    if (lmi && lmi->GetLightMap ())
      ((csLightMap *)lmi->GetLightMap ())->MakeDirtyDynamicLights ();
  }

  delete pit;
#endif
}

bool csEngine::CheckConsistency ()
{
  return false;
}

void csEngine::StartEngine ()
{
  DeleteAll ();
}

iEngineSequenceManager* csEngine::GetEngineSequenceManager ()
{
  if (!eseqmgr)
  {
    eseqmgr = CS_QUERY_REGISTRY (object_reg, iEngineSequenceManager);
    if (!eseqmgr)
    {
      csRef<iPluginManager> plugin_mgr (
  	  CS_QUERY_REGISTRY (object_reg, iPluginManager));
      eseqmgr = CS_LOAD_PLUGIN (plugin_mgr,
    	  "crystalspace.utilities.sequence.engine", iEngineSequenceManager);
      if (!eseqmgr)
      {
        Warn ("Could not load the engine sequence manager!");
        return 0;
      }
      if (!object_reg->Register (eseqmgr, "iEngineSequenceManager"))
      {
        Warn ("Could not register the engine sequence manager!");
        return 0;
      }
    }
  }
  return eseqmgr;
}

#if !defined(CS_USE_NEW_RENDERER) || !defined(CS_NR_ALTERNATE_RENDERLOOP)
void csEngine::StartDraw (iCamera *c, iClipper2D *view, csRenderView &rview)
{
  Stats::polygons_considered = 0;
  Stats::polygons_drawn = 0;
  Stats::portals_drawn = 0;
  Stats::polygons_rejected = 0;
  Stats::polygons_accepted = 0;

  current_camera = c;
  rview.SetEngine (this);
  rview.SetOriginalCamera (c);

  iEngineSequenceManager* eseqmgr = GetEngineSequenceManager ();
  if (eseqmgr)
  {
    eseqmgr->SetCamera (c);
  }

  // This flag is set in HandleEvent on a cscmdContextResize event
  if (resize)
  {
    resize = false;
    Resize ();
  }

  top_clipper = view;

  rview.GetClipPlane ().Set (0, 0, 1, -1);      //@@@CHECK!!!

  // Calculate frustum for screen dimensions (at z=1).
  float leftx = -c->GetShiftX () * c->GetInvFOV ();
  float rightx = (frame_width - c->GetShiftX ()) * c->GetInvFOV ();
  float topy = -c->GetShiftY () * c->GetInvFOV ();
  float boty = (frame_height - c->GetShiftY ()) * c->GetInvFOV ();
  rview.SetFrustum (leftx, rightx, topy, boty);

  cur_process_polygons = 0;
}

void csEngine::Draw (iCamera *c, iClipper2D *view)
{
  ControlMeshes ();

  csRenderView rview (c, view, G3D, G2D);
  StartDraw (c, view, rview);

  // First initialize G3D with the right clipper.
  G3D->SetClipper (view, CS_CLIPPER_TOPLEVEL);  // We are at top-level.
  G3D->ResetNearPlane ();
  G3D->SetPerspectiveAspect (c->GetFOV ());

  iSector *s = c->GetSector ();
  if (s) s->Draw (&rview);

  // draw all halos on the screen
  if (halos.Length () > 0)
  {
    csTicks elapsed = virtual_clock->GetElapsedTicks ();
    for (int halo = halos.Length () - 1; halo >= 0; halo--)
      if (!halos[halo]->Process (elapsed, *this)) halos.Delete (halo);
  }

  G3D->SetClipper (0, CS_CLIPPER_NONE);
}
#endif

#if defined(CS_NR_ALTERNATE_RENDERLOOP)
void csEngine::StartDraw (iCamera *c, iClipper2D *view, csRenderView &rview)
{
}

void csEngine::Draw (iCamera *c, iClipper2D *view)
{
  defaultRenderLoop->Draw (c, view);
}

csPtr<iRenderLoop> csEngine::CreateDefaultRenderLoop ()
{
  csRef<iRenderLoop> loop = renderLoopManager->Create ();

  csRef<iPluginManager> plugin_mgr (
  	CS_QUERY_REGISTRY (object_reg, iPluginManager));

  csRef<iRenderStepType> genType =
    CS_LOAD_PLUGIN (plugin_mgr,
      "crystalspace.renderloop.step.generic.type",
      iRenderStepType);

  csRef<iRenderStepFactory> genFact = genType->NewFactory ();

  csRef<iRenderStep> step;
  csRef<iGenericRenderStep> genStep;

  step = genFact->Create ();
  loop->AddStep (step);
  genStep = SCF_QUERY_INTERFACE (step, iGenericRenderStep);
  
  genStep->SetShaderType ("ambient");
  genStep->SetZBufMode (CS_ZBUF_USE);
  genStep->SetZOffset (true);

  csRef<iRenderStepType> liType =
    CS_LOAD_PLUGIN (plugin_mgr,
      "crystalspace.renderloop.step.lightiter.type",
      iRenderStepType);

  csRef<iRenderStepFactory> liFact = liType->NewFactory ();

  step = liFact->Create ();
  loop->AddStep (step);

  csRef<iRenderStepContainer> liContainer =
    SCF_QUERY_INTERFACE (step, iRenderStepContainer);

  csRef<iRenderStepType> stencilType =
    CS_LOAD_PLUGIN (plugin_mgr,
      "crystalspace.renderloop.step.shadow.stencil.type",
      iRenderStepType);

//  csRef<iRenderStepFactory> stencilFact = stencilType->NewFactory ();

//  step = stencilFact->Create ();
//  liContainer->AddStep (step);

  step = genFact->Create ();
  liContainer->AddStep (step);

  genStep = SCF_QUERY_INTERFACE (step, iGenericRenderStep);

  genStep->SetShaderType ("diffuse");
  genStep->SetZBufMode (CS_ZBUF_TEST);
  genStep->SetZOffset (false);

  return csPtr<iRenderLoop> (loop);
}
#endif

void csEngine::AddHalo (csLight *Light)
{
  if (!Light->GetHalo () || Light->flags.Check (CS_LIGHT_ACTIVEHALO))
    return ;

  // Transform light pos into camera space and see if it is directly visible
  csVector3 v = current_camera->GetTransform ().Other2This (
      Light->GetCenter ());

  // Check if light is behind us
  if (v.z <= SMALL_Z) return ;

  // Project X,Y into screen plane
  float iz = current_camera->GetFOV () / v.z;
  v.x = v.x * iz + current_camera->GetShiftX ();
  v.y = frame_height - 1 - (v.y * iz + current_camera->GetShiftY ());

  // If halo is not inside visible region, return
  if (!top_clipper->IsInside (csVector2 (v.x, v.y))) return ;

#ifndef CS_USE_NEW_RENDERER
  // Check if light is not obscured by anything
  float zv = G3D->GetZBuffValue (QRound (v.x), QRound (v.y));
#else
  float zv = 1;
#endif // CS_USE_NEW_RENDERER
  if (v.z > zv) return ;

  // Halo size is 1/4 of the screen height; also we make sure its odd
  int hs = (frame_height / 4) | 1;

  if (Light->GetHalo ()->Type == cshtFlare)
  {
    // put a new light flare into the queue
    // the cast is safe because of the type check above
    halos.Push (
        new csLightFlareHalo (Light, (csFlareHalo *)Light->GetHalo (), hs));
    return ;
  }

  // Okay, put the light into the queue: first we generate the alphamap
  unsigned char *Alpha = Light->GetHalo ()->Generate (hs);
#ifndef CS_USE_NEW_RENDERER
  iHalo *handle = G3D->CreateHalo (
      Light->GetColor ().red,
      Light->GetColor ().green,
      Light->GetColor ().blue,
      Alpha,
      hs,
      hs);
#else
  iHalo *handle = 0;
#endif // CS_USE_NEW_RENDERER
  // We don't need alpha map anymore
  delete[] Alpha;

  // Does 3D rasterizer support halos?
  if (!handle) return ;

  halos.Push (new csLightHalo (Light, handle));
}

void csEngine::RemoveHalo (csLight *Light)
{
  int i;
  for (i = 0 ; i < halos.Length () ; i++)
  {
    csLightHalo* lh = halos[i];
    if (lh->Light == Light)
    {
      halos.Delete (i);
      return;
    }
  }
}

iStatLight *csEngine::FindLightID (const char* light_id) const
{
  for (int i = 0; i < sectors.GetCount (); i++)
  {
    iLight *l = sectors.Get (i)->GetLights ()->FindByID (light_id);
    if (l)
    {
      csRef<iStatLight> sl (SCF_QUERY_INTERFACE (l, iStatLight));
      if (sl) return sl;	// Smart pointer DecRef() is ok in this case.
    }
  }

  return 0;
}

iStatLight *csEngine::FindLight (const char *name, bool regionOnly) const
{
  // XXX: Need to implement region?
  (void)regionOnly;

  // @@@### regionOnly
  int i;
  for (i = 0; i < sectors.GetCount (); i++)
  {
    iLight *l = sectors.Get (i)->GetLights ()->FindByName (name);
    if (l)
    {
      csRef<iStatLight> sl (SCF_QUERY_INTERFACE (l, iStatLight));
      if (sl) return sl;	// Smart pointer DecRef() is ok in this case.
    }
  }

  return 0;
}

void csEngine::AddDynLight (csDynLight *dyn)
{
  dyn->SetNext (first_dyn_lights);
  dyn->SetPrev (0);
  if (first_dyn_lights) first_dyn_lights->SetPrev (dyn);
  first_dyn_lights = dyn;
  dyn->IncRef ();
}

void csEngine::RemoveDynLight (csDynLight *dyn)
{
  if (dyn->GetNext ()) dyn->GetNext ()->SetPrev (dyn->GetPrev ());
  if (dyn->GetPrev ())
    dyn->GetPrev ()->SetNext (dyn->GetNext ());
  else if (dyn == first_dyn_lights)
    first_dyn_lights = dyn->GetNext ();
  dyn->SetNext (0);
  dyn->SetPrev (0);
  dyn->DecRef ();
}

iDynLight* csEngine::GetFirstDynLight () const
{
  return first_dyn_lights ? &(first_dyn_lights->scfiDynLight) : 0;
}

void csEngine::ControlMeshes ()
{
  nextframe_pending = virtual_clock->GetCurrentTicks ();

  // Delete particle systems that self-destructed now.
  csGlobalHashIterator it (want_to_die.GetHashMap ());
  while (it.HasNext ())
  {
    iMeshWrapper* mesh = (iMeshWrapper*)it.Next ();
    GetMeshes ()->Remove (mesh);
  }
  want_to_die.DeleteAll ();
}

char* csEngine::SplitRegionName (const char* name, iRegion*& region,
	bool& global)
{
  region = 0;
  global = false;

  char* p = strchr (name, '/');
  if (!p) return (char*)name;
  if (*name == '*' && *(name+1) == '/')
  {
    global = true;
    return p+1;
  }

  *p = 0;
  region = regions.FindByName (name);
  *p = '/';
  if (!region) return 0;
  return p+1;
}

iMaterialWrapper* csEngine::FindMaterial (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iMaterialWrapper* mat;
  if (region)
    mat = region->FindMaterial (n);
  else if (!global && reg)
    mat = reg->FindMaterial (n);
  else
    mat = GetMaterialList ()->FindByName (n);
  return mat;
}

iTextureWrapper* csEngine::FindTexture (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iTextureWrapper* txt;
  if (region)
    txt = region->FindTexture (n);
  else if (!global && reg)
    txt = reg->FindTexture (n);
  else
    txt = GetTextureList ()->FindByName (n);
  return txt;
}

iSector* csEngine::FindSector (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iSector* sect;
  if (region)
    sect = region->FindSector (n);
  else if (!global && reg)
    sect = reg->FindSector (n);
  else
    sect = GetSectors ()->FindByName (n);
  return sect;
}

iMeshWrapper* csEngine::FindMeshObject (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iMeshWrapper* mesh;
  if (region)
    mesh = region->FindMeshObject (n);
  else if (!global && reg)
    mesh = reg->FindMeshObject (n);
  else
    mesh = GetMeshes ()->FindByName (n);
  return mesh;
}

iMeshFactoryWrapper* csEngine::FindMeshFactory (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iMeshFactoryWrapper* fact;
  if (region)
    fact = region->FindMeshFactory (n);
  else if (!global && reg)
    fact = reg->FindMeshFactory (n);
  else
    fact = GetMeshFactories ()->FindByName (n);
  return fact;
}

iCameraPosition* csEngine::FindCameraPosition (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iCameraPosition* campos;
  if (region)
    campos = region->FindCameraPosition (n);
  else if (!global && reg)
    campos = reg->FindCameraPosition (n);
  else
    campos = GetCameraPositions ()->FindByName (n);
  return campos;
}

iCollection* csEngine::FindCollection (const char* name,
	iRegion* reg)
{
  iRegion* region;
  bool global;
  char* n = SplitRegionName (name, region, global);
  if (!n) return 0;

  iCollection* col;
  if (region)
    col = region->FindCollection (n);
  else if (!global && reg)
    col = reg->FindCollection (n);
  else
    col = GetCollections ()->FindByName (n);
  return col;
}

void csEngine::ReadConfig (iConfigFile *Config)
{
  default_fastmesh_thresshold = Config->GetInt (
      "Engine.FastMeshThresshold",
      500);
  fastmesh_thresshold = default_fastmesh_thresshold;

  csEngine::lightmap_quality = Config->GetInt (
      "Engine.Lighting.LightmapQuality",
      3);
  default_max_lightmap_w = 
    Config->GetInt ("Engine.Lighting.MaxLightmapWidth", default_max_lightmap_w);
  max_lightmap_w = default_max_lightmap_w;
  default_max_lightmap_h = 
    Config->GetInt ("Engine.Lighting.MaxLightmapHeight", default_max_lightmap_h);
  max_lightmap_h = default_max_lightmap_h;

  default_ambient_red = Config->GetInt (
      "Engine.Lighting.Ambient.Red",
      CS_DEFAULT_LIGHT_LEVEL);
  default_ambient_green = Config->GetInt (
      "Engine.Lighting.Ambient.Green",
      CS_DEFAULT_LIGHT_LEVEL);
  default_ambient_blue = Config->GetInt (
      "Engine.Lighting.Ambient.Blue",
      CS_DEFAULT_LIGHT_LEVEL);

  csLight::ambient_red = default_ambient_red;
  csLight::ambient_green = default_ambient_green;
  csLight::ambient_blue = default_ambient_blue;

  csSector::cfg_reflections = Config->GetInt (
      "Engine.Lighting.Reflections",
      csSector::cfg_reflections);

  //@@@ NOT THE RIGHT PLACE! csSprite3D::global_lighting_quality = Config->GetInt ("Engine.Lighting.SpriteQuality", csSprite3D::global_lighting_quality);
  csSector::do_radiosity = Config->GetBool (
      "Engine.Lighting.Radiosity",
      csSector::do_radiosity);

  // radiosity options
  csEngine::use_new_radiosity = Config->GetBool (
      "Engine.Lighting.Radiosity.Enable",
      csEngine::use_new_radiosity);
#if 0
  // @@@ TOTALLY DISABLED FOR NOW
  csRadiosity::do_static_specular = Config->GetBool (
      "Engine.Lighting.Radiosity.DoStaticSpecular",
      csRadiosity::do_static_specular);
  csRadiosity::static_specular_amount = Config->GetFloat (
      "Engine.Lighting.Radiosity.StaticSpecularAmount",
      csRadiosity::static_specular_amount);
  csRadiosity::static_specular_tightness = Config->GetInt (
      "Engine.Lighting.Radiosity.StaticSpecularTightness",
      csRadiosity::static_specular_tightness);
  csRadiosity::colour_bleed = Config->GetFloat (
      "Engine.Lighting.Radiosity.ColourBleed",
      csRadiosity::colour_bleed);
  csRadiosity::stop_priority = Config->GetFloat (
      "Engine.Lighting.Radiosity.StopPriority",
      csRadiosity::stop_priority);
  csRadiosity::stop_improvement = Config->GetFloat (
      "Engine.Lighting.Radiosity.StopImprovement",
      csRadiosity::stop_improvement);
  csRadiosity::stop_iterations = Config->GetInt (
      "Engine.Lighting.Radiosity.StopIterations",
      csRadiosity::stop_iterations);
  csRadiosity::source_patch_size = Config->GetInt (
      "Engine.Lighting.Radiosity.SourcePatchSize",
      csRadiosity::source_patch_size);
#endif

  default_clear_zbuf = 
    Config->GetBool ("Engine.ClearZBuffer", default_clear_zbuf);
  default_clear_screen = 
    Config->GetBool ("Engine.ClearScreen", default_clear_screen);
  clear_screen = default_clear_screen;
}

struct LightAndDist
{
  iLight *light;
  float sqdist;
};

// csLightArray is a subclass of csCleanable which is registered
// to csEngine.cleanup.
class csLightArray : public iBase
{
public:
  SCF_DECLARE_IBASE;

  LightAndDist *array;

  // Size is the physical size of the array. num_lights is the number of
  // lights in it.
  int size, num_lights;

  csLightArray () : array(0), size(0), num_lights(0)
  {
    SCF_CONSTRUCT_IBASE (0);
  }

  virtual ~csLightArray ()  { delete[] array; }
  void Reset ()             { num_lights = 0; }
  void AddLight (iLight *l, float sqdist)
  {
    if (num_lights >= size)
    {
      LightAndDist *new_array;
      new_array = new LightAndDist[size + 5];
      if (array)
      {
        memcpy (new_array, array, sizeof (LightAndDist) * num_lights);
        delete[] array;
      }

      array = new_array;
      size += 5;
    }

    array[num_lights].light = l;
    array[num_lights++].sqdist = sqdist;
  };
  iLight *GetLight (int i)  { return array[i].light; }
};

SCF_IMPLEMENT_IBASE(csLightArray)
  SCF_IMPLEMENTS_INTERFACE(iBase)
SCF_IMPLEMENT_IBASE_END;

static int compare_light (const void *p1, const void *p2)
{
  LightAndDist *sp1 = (LightAndDist *)p1;
  LightAndDist *sp2 = (LightAndDist *)p2;
  float z1 = sp1->sqdist;
  float z2 = sp2->sqdist;
  if (z1 < z2)
    return -1;
  else if (z1 > z2)
    return 1;
  return 0;
}

// This is a static light array which is adapted to the
// right size everytime it is used. In the beginning it means
// that this array will grow a lot but finally it will
// stabilize to a maximum size (not big). The advantage of
// this approach is that we don't have a static array which can
// overflow. And we don't have to do allocation every time we
// come here. We register this memory to the 'cleanup' array
// in csEngine so that it will be freed later.
static csLightArray *light_array = 0;

static bool FindLightPos_Front2Back (csKDTree* treenode,
	void* userdata, uint32 cur_timestamp, uint32&)
{
  csVector3 pos = *(csVector3*)userdata;

  const csBox3& node_bbox = treenode->GetNodeBBox ();

  // In the first part of this test we are going to test if the
  // position is inside the node. If not then we don't need to continue.
  if (!node_bbox.In (pos))
  {
    return false;
  }

  treenode->Distribute ();

  int num_objects;
  csKDTreeChild** objects;
  num_objects = treenode->GetObjectCount ();
  objects = treenode->GetObjects ();

  int i;
  for (i = 0 ; i < num_objects ; i++)
  {
    if (objects[i]->timestamp != cur_timestamp)
    {
      objects[i]->timestamp = cur_timestamp;
      // First test the bounding box of the object.
      const csBox3& obj_bbox = objects[i]->GetBBox ();

      if (obj_bbox.In (pos))
      {
        iLight* light = (iLight*)objects[i]->GetObject ();
	float sqdist = csSquaredDist::PointPoint (pos, light->GetCenter ());
#ifdef CS_USE_NEW_RENDERER
	if (sqdist < light->GetInfluenceRadiusSq ())
#else
	if (sqdist < light->GetSquaredRadius ())
#endif
	{
	  light_array->AddLight (light, sqdist);
	}
      }
    }
  }
  return true;
}

static bool FindLightBox_Front2Back (csKDTree* treenode,
	void* userdata, uint32 cur_timestamp, uint32&)
{
  csBox3* box = (csBox3*)userdata;

  const csBox3& node_bbox = treenode->GetNodeBBox ();

  // In the first part of this test we are going to test if the
  // box intersects with the node. If not then we don't need to continue.
  if (!node_bbox.TestIntersect (*box))
  {
    return false;
  }

  treenode->Distribute ();

  int num_objects;
  csKDTreeChild** objects;
  num_objects = treenode->GetObjectCount ();
  objects = treenode->GetObjects ();

  int i;
  for (i = 0 ; i < num_objects ; i++)
  {
    if (objects[i]->timestamp != cur_timestamp)
    {
      objects[i]->timestamp = cur_timestamp;
      // First test the bounding box of the object.
      const csBox3& obj_bbox = objects[i]->GetBBox ();

      if (obj_bbox.TestIntersect (*box))
      {
        iLight* light = (iLight*)objects[i]->GetObject ();
        csBox3 b (box->Min () - light->GetCenter (),
		  box->Max () - light->GetCenter ());
        float sqdist = b.SquaredOriginDist ();
#ifdef CS_USE_NEW_RENDERER
        if (sqdist < light->GetInfluenceRadiusSq ())
#else
        if (sqdist < light->GetSquaredRadius ())
#endif
	{
	  light_array->AddLight (light, sqdist);
	}
      }
    }
  }
  return true;
}


int csEngine::GetNearbyLights (
  iSector *sector,
  const csVector3 &pos,
  uint32 flags,
  iLight **lights,
  int max_num_lights)
{
  int i;
  float sqdist;

  if (!light_array)
  {
    light_array = new csLightArray ();
    csEngine::current_engine->cleanup.Push (light_array);
    light_array->DecRef ();
  }

  light_array->Reset ();

  // Add all dynamic lights to the array (if CS_NLIGHT_DYNAMIC is set).
  if (flags & CS_NLIGHT_DYNAMIC)
  {
    csDynLight *dl = first_dyn_lights;
    while (dl)
    {
      if (dl->GetSector () == sector)
      {
        sqdist = csSquaredDist::PointPoint (pos, dl->GetCenter ());
#ifdef CS_USE_NEW_RENDERER
        if (sqdist < dl->GetInfluenceRadiusSq ())
#else
        if (sqdist < dl->GetSquaredRadius ())
#endif
        {
          csRef<iLight> il (SCF_QUERY_INTERFACE (dl, iLight));
          light_array->AddLight (il, sqdist);
        }
      }

      dl = dl->GetNext ();
    }
  }

  // Add all static lights to the array (if CS_NLIGHT_STATIC is set).
  if (flags & CS_NLIGHT_STATIC)
  {
    csKDTree* kdtree = sector->GetPrivateObject ()->GetLightKDTree ();
    csVector3 position = pos;
    kdtree->Front2Back (pos, FindLightPos_Front2Back, &position, 0);
  }

  if (light_array->num_lights <= max_num_lights)
  {
    // The number of lights that we found is smaller than what fits
    // in the array given us by the user. So we just copy them all
    // and don't need to sort.
    for (i = 0; i < light_array->num_lights; i++)
      lights[i] = light_array->GetLight (i);
    return light_array->num_lights;
  }
  else
  {
    // We found more lights than we can put in the given array
    // so we sort the lights and then return the nearest.
    qsort (
      light_array->array,
      light_array->num_lights,
      sizeof (LightAndDist),
      compare_light);
    for (i = 0; i < max_num_lights; i++)
      lights[i] = light_array->GetLight (i);
    return max_num_lights;
  }
}

int csEngine::GetNearbyLights (
  iSector *sector,
  const csBox3 &box,
  uint32 flags,
  iLight **lights,
  int max_num_lights)
{
  int i;
  float sqdist;

  if (!light_array)
  {
    light_array = new csLightArray ();
    csEngine::current_engine->cleanup.Push (light_array);
    light_array->DecRef ();
  }

  light_array->Reset ();

  // Add all dynamic lights to the array (if CS_NLIGHT_DYNAMIC is set).
  if (flags & CS_NLIGHT_DYNAMIC)
  {
    csDynLight *dl = first_dyn_lights;
    while (dl)
    {
      if (dl->GetSector () == sector)
      {
        csBox3 b (box.Min () - dl->GetCenter (), box.Max () - dl->GetCenter ());
        sqdist = b.SquaredOriginDist ();
#ifdef CS_USE_NEW_RENDERER
        if (sqdist < dl->GetInfluenceRadiusSq ())
#else
        if (sqdist < dl->GetSquaredRadius ())
#endif
        {
          csRef<iLight> il (SCF_QUERY_INTERFACE (dl, iLight));
          light_array->AddLight (il, sqdist);
        }
      }

      dl = dl->GetNext ();
    }
  }

  // Add all static lights to the array (if CS_NLIGHT_STATIC is set).
  if (flags & CS_NLIGHT_STATIC)
  {
    csKDTree* kdtree = sector->GetPrivateObject ()->GetLightKDTree ();
    csBox3 bbox = box;
    kdtree->Front2Back (box.Min (), FindLightBox_Front2Back, &bbox, 0);
  }

  if (light_array->num_lights <= max_num_lights)
  {
    // The number of lights that we found is smaller than what fits
    // in the array given us by the user. So we just copy them all
    // and don't need to sort.
    for (i = 0; i < light_array->num_lights; i++)
      lights[i] = light_array->GetLight (i);
    return light_array->num_lights;
  }
  else
  {
    // We found more lights than we can put in the given array
    // so we sort the lights and then return the nearest.
    qsort (
      light_array->array,
      light_array->num_lights,
      sizeof (LightAndDist),
      compare_light);
    for (i = 0; i < max_num_lights; i++)
      lights[i] = light_array->GetLight (i);
    return max_num_lights;
  }
}

csPtr<iSectorIterator> csEngine::GetNearbySectors (
  iSector *sector,
  const csVector3 &pos,
  float radius)
{
  csSectorIt *it = new csSectorIt (sector, pos, radius);
  return csPtr<iSectorIterator> (it);
}

void csEngine::GetNearbyObjectList (iSector* sector,
    const csVector3& pos, float radius, csPArray<iObject>& list,
    csPArray<iSector>& visited_sectors, bool crossPortals)
{
  iVisibilityCuller* culler = sector->GetVisibilityCuller ();
  csRef<iVisibilityObjectIterator> visit = culler->VisTest (
  	csSphere (pos, radius));

  //@@@@@@@@ TODO ALSO SUPPORT LIGHTS!
  while (visit->HasNext ())
  {
    iVisibilityObject* vo = visit->Next ();
    iMeshWrapper* imw = vo->GetMeshWrapper ();
    if (imw)
    {
      list.Push (imw->QueryObject ()); 
      if (crossPortals && imw->GetMeshObject ()->GetPortalCount () > 0)
      {
        csRef<iThingState> st (
        	SCF_QUERY_INTERFACE (imw->GetMeshObject (), iThingState));
        if (st)
        {
          // Check if there are portals and if they are near the position.
          int pc = st->GetFactory ()->GetPortalCount ();
          int j;
          for (j = 0 ; j < pc ; j++)
          {
            iPolygon3D* pp = st->GetPortalPolygon (j);
	    iPolygon3DStatic* pps = pp->GetStaticData ();
            const csPlane3& wor_plane = pp->GetWorldPlane ();
            // Can we see the portal?
            if (wor_plane.Classify (pos) < -0.001)
            {
              csVector3 poly[100];	//@@@ HARDCODE
              int k;
              for (k = 0 ; k < pps->GetVertexCount () ; k++)
              {
                poly[k] = pp->GetVertexW (k);
              }
              float sqdist_portal = csSquaredDist::PointPoly (
                    pos, poly, pps->GetVertexCount (),
                    wor_plane);
              if (sqdist_portal <= radius * radius)
              {
                // Also handle objects in the destination sector unless
                // it is a warping sector.
                iPortal* portal = pps->GetPortal ();
                portal->CompleteSector (0);
                CS_ASSERT (portal != 0);
                if (sector != portal->GetSector () && portal->GetSector ()
                                && !portal->GetFlags ().Check (CS_PORTAL_WARP))
                {
                  int l;
                  bool already_visited = false;
                  for (l = 0 ; l < visited_sectors.Length () ; l++)
                  {
                    if (visited_sectors[l] == portal->GetSector ())
                    {
                      already_visited = true;
                      break;
                    }
                  }
                  if (!already_visited)
                  {
                    visited_sectors.Push (portal->GetSector ());
                    GetNearbyObjectList (portal->GetSector (), pos, radius, list,
                                         visited_sectors);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

csPtr<iObjectIterator> csEngine::GetNearbyObjects (
  iSector *sector,
  const csVector3 &pos,
  float radius,
  bool crossPortals)
{
  csPArray<iObject>* list = new csPArray<iObject>;
  csPArray<iSector> visited_sectors;
  visited_sectors.Push (sector);
  GetNearbyObjectList (sector, pos, radius, *list, visited_sectors, crossPortals);
  csObjectListIt *it = new csObjectListIt (list);
  return csPtr<iObjectIterator> (it);
}

csPtr<iObjectIterator> csEngine::GetVisibleObjects (
  iSector* /*sector*/,
  const csVector3& /*pos*/)
{
  // @@@ Not implemented yet.
  return 0;
}

csPtr<iObjectIterator> csEngine::GetVisibleObjects (
  iSector* /*sector*/,
  const csFrustum& /*frustum*/)
{
  // @@@ Not implemented yet.
  return 0;
}

void csEngine::GetNearbyMeshList (iSector* sector,
    const csVector3& pos, float radius, csPArray<iMeshWrapper>& list,
    csPArray<iSector>& visited_sectors, bool crossPortals)
{
  iVisibilityCuller* culler = sector->GetVisibilityCuller ();
  csRef<iVisibilityObjectIterator> visit = culler->VisTest (
  	csSphere (pos, radius));

  //@@@@@@@@ TODO ALSO SUPPORT LIGHTS!
  while (visit->HasNext ())
  {
    iVisibilityObject* vo = visit->Next ();
    iMeshWrapper* imw = vo->GetMeshWrapper ();
    if (imw)
    {
      list.Push (imw); 
      if (crossPortals && imw->GetMeshObject ()->GetPortalCount () > 0)
      {
        csRef<iThingState> st = SCF_QUERY_INTERFACE (imw->GetMeshObject (), iThingState);
        if (st)
        {
          // Check if there are portals and if they are near the position.
          int pc = st->GetFactory ()->GetPortalCount ();
          int j;
          for (j = 0 ; j < pc ; j++)
          {
            iPolygon3D* pp = st->GetPortalPolygon (j);
	    iPolygon3DStatic* pps = pp->GetStaticData ();
            const csPlane3& wor_plane = pp->GetWorldPlane ();
            // Can we see the portal?
            if (wor_plane.Classify (pos) < -0.001)
            {
              csVector3 poly[100];	//@@@ HARDCODE
              int k;
              for (k = 0 ; k < pps->GetVertexCount () ; k++)
              {
                poly[k] = pp->GetVertexW (k);
              }
              float sqdist_portal = csSquaredDist::PointPoly (
                    pos, poly, pps->GetVertexCount (),
                    wor_plane);
              if (sqdist_portal <= radius * radius)
              {
                // Also handle objects in the destination sector unless
                // it is a warping sector.
                iPortal* portal = pps->GetPortal ();
                portal->CompleteSector (0);
                CS_ASSERT (portal != 0);
                if (sector != portal->GetSector () && portal->GetSector ()
                                && !portal->GetFlags ().Check (CS_PORTAL_WARP))
                {
                  int l;
                  bool already_visited = false;
                  for (l = 0 ; l < visited_sectors.Length () ; l++)
                  {
                    if (visited_sectors[l] == portal->GetSector ())
                    {
                      already_visited = true;
                      break;
                    }
                  }
                  if (!already_visited)
                  {
                    visited_sectors.Push (portal->GetSector ());
                    GetNearbyMeshList (portal->GetSector (), pos, radius, list,
                                         visited_sectors);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

csPtr<iMeshWrapperIterator> csEngine::GetNearbyMeshes (
  iSector *sector,
  const csVector3 &pos,
  float radius,
  bool crossPortals)
{
  csPArray<iMeshWrapper>* list = new csPArray<iMeshWrapper>;
  csPArray<iSector> visited_sectors;
  visited_sectors.Push (sector);
  GetNearbyMeshList (sector, pos, radius, *list, visited_sectors, crossPortals);
  csMeshListIt *it = new csMeshListIt (list);
  return csPtr<iMeshWrapperIterator> (it);
}

csPtr<iMeshWrapperIterator> csEngine::GetVisibleMeshes (
  iSector* /*sector*/,
  const csVector3& /*pos*/)
{
  // @@@ Not implemented yet.
  return 0;
}

csPtr<iMeshWrapperIterator> csEngine::GetVisibleMeshes (
  iSector* /*sector*/,
  const csFrustum& /*frustum*/)
{
  // @@@ Not implemented yet.
  return 0;
}

int csEngine::GetTextureFormat () const
{
  iTextureManager *txtmgr = G3D->GetTextureManager ();
  return txtmgr->GetTextureFormat ();
}

iRegion* csEngine::CreateRegion (const char *name)
{
  iRegion* region = regions.FindByName (name);
  if (!region)
  {
    csRegion *r = new csRegion (this);
    region = &r->scfiRegion;
    r->SetName (name);
    regions.Push (region);
    r->DecRef ();
  }
  return region;
}

iTextureWrapper *csEngine::CreateTexture (
  const char *iName,
  const char *iFileName,
  csColor *iTransp,
  int iFlags)
{
  if (!ImageLoader) return 0;

  // First of all, load the image file
  csRef<iDataBuffer> data = VFS->ReadFile (iFileName);
  if (!data || !data->GetSize ())
  {
    Warn ("Cannot read image file \"%s\" from VFS.", iFileName);
    return 0;
  }

  csRef<iImage> ifile (ImageLoader->Load (
      data->GetUint8 (),
      data->GetSize (),
      CS_IMGFMT_TRUECOLOR)); // GetTextureFormat ());

  if (!ifile)
  {
    Warn ("Unknown image file format: \"%s\".", iFileName);
    return 0;
  }

  csRef<iDataBuffer> xname = VFS->ExpandPath (iFileName);
  ifile->SetName (**xname);

  // Okay, now create the respective texture handle object
  iTextureWrapper *tex = GetTextures ()->FindByName (iName);
  if (tex)
    tex->SetImageFile (ifile);
  else
    tex = GetTextures ()->NewTexture (ifile);

  tex->SetFlags (iFlags);
  tex->QueryObject ()->SetName (iName);

  if (iTransp)
    tex->SetKeyColor (
        QInt (iTransp->red * 255.2),
        QInt (iTransp->green * 255.2),
        QInt (iTransp->blue * 255.2));

  return tex;
}

iTextureWrapper *csEngine::CreateBlackTexture (
  const char *name,
  int w, int h,
  csColor *iTransp,
  int iFlags)
{
  if (!ImageLoader) return 0;

  csRef<iImage> ifile = csPtr<iImage>(new csImageMemory (w, h));
  ifile->SetName (name);

  // Okay, now create the respective texture handle object
  iTextureWrapper *tex = GetTextures ()->FindByName (name);
  if (tex)
    tex->SetImageFile (ifile);
  else
    tex = GetTextures ()->NewTexture (ifile);

  tex->SetFlags (iFlags);
  tex->QueryObject ()->SetName (name);

  if (iTransp)
    tex->SetKeyColor (
        QInt (iTransp->red * 255.2),
        QInt (iTransp->green * 255.2),
        QInt (iTransp->blue * 255.2));

  return tex;
}

iMaterialWrapper *csEngine::CreateMaterial (
  const char *iName,
  iTextureWrapper *texture)
{
  csMaterial *mat = new csMaterial (texture);
  iMaterialWrapper *wrapper = materials->NewMaterial (mat);
  wrapper->QueryObject ()->SetName (iName);
  mat->DecRef ();
  return wrapper;
}

csPtr<iMeshWrapper> csEngine::CreateThingMesh (
  iSector *sector,
  const char *name)
{
  csRef<iMeshWrapper> thing_wrap (CreateMeshWrapper (
  	"crystalspace.mesh.object.thing", name, sector));
  thing_wrap->SetZBufMode (CS_ZBUF_USE);
  thing_wrap->SetRenderPriority (GetObjectRenderPriority ());
  return csPtr<iMeshWrapper> (thing_wrap);
}

csPtr<iMeshWrapper> csEngine::CreateSectorWallsMesh (
  iSector *sector,
  const char *name)
{
  csRef<iMeshWrapper> thing_wrap (CreateMeshWrapper (
  	"crystalspace.mesh.object.thing", name, sector));
  thing_wrap->GetFlags ().Set (CS_ENTITY_CONVEX);
  thing_wrap->SetZBufMode (CS_ZBUF_FILL);
  thing_wrap->SetRenderPriority (GetWallRenderPriority ());
  return csPtr<iMeshWrapper> (thing_wrap);
}

iSector *csEngine::CreateSector (const char *iName)
{
  iSector *sector = &(new csSector (this))->scfiSector;
  sector->QueryObject ()->SetName (iName);
  sectors.Add (sector);
  sector->DecRef ();

  return sector;
}

csPtr<iMaterial> csEngine::CreateBaseMaterial (iTextureWrapper *txt)
{
  csMaterial *mat = new csMaterial ();
  if (txt) mat->SetTextureWrapper (txt);

  csRef<iMaterial> imat (SCF_QUERY_INTERFACE (mat, iMaterial));
  mat->DecRef ();
  return csPtr<iMaterial> (imat);
}

csPtr<iMaterial> csEngine::CreateBaseMaterial (
  iTextureWrapper *txt,
  int num_layers,
  iTextureWrapper **wrappers,
  csTextureLayer *layers)
{
  csMaterial *mat = new csMaterial ();
  if (txt) mat->SetTextureWrapper (txt);

  int i;
  for (i = 0; i < num_layers; i++)
  {
    mat->AddTextureLayer (
        wrappers[i],
        layers[i].mode,
        layers[i].uscale,
        layers[i].vscale,
        layers[i].ushift,
        layers[i].vshift);
  }

  csRef<iMaterial> imat (SCF_QUERY_INTERFACE (mat, iMaterial));
  mat->DecRef ();
  return csPtr<iMaterial> (imat);
}

iTextureList *csEngine::GetTextureList () const
{
  return &(GetTextures ()->scfiTextureList);
}

iMaterialList *csEngine::GetMaterialList () const
{
  return &(GetMaterials ()->scfiMaterialList);
}

iSharedVariableList *csEngine::GetVariableList () const
{
  return &(GetVariables()->scfiSharedVariableList);
}


iRegionList *csEngine::GetRegions ()
{
  return &(regions.scfiRegionList);
}

csPtr<iCamera> csEngine::CreateCamera ()
{
  return csPtr<iCamera> (&(new csCamera ())->scfiCamera);
}

csPtr<iStatLight> csEngine::CreateLight (
  const char *name,
  const csVector3 &pos,
  float radius,
  const csColor &color,
  bool pseudoDyn)
{
  csStatLight *light = new csStatLight (
      pos.x,
      pos.y,
      pos.z,
      radius,
      color.red,
      color.green,
      color.blue,
      pseudoDyn);
  if (name) light->SetName (name);

  csRef<iStatLight> il (SCF_QUERY_INTERFACE (light, iStatLight));
  light->DecRef ();
  return csPtr<iStatLight> (il);
}

csPtr<iDynLight> csEngine::CreateDynLight (
  const csVector3 &pos,
  float radius,
  const csColor &color)
{
  csDynLight *light = new csDynLight (
      pos.x,
      pos.y,
      pos.z,
      radius,
      color.red,
      color.green,
      color.blue);
  AddDynLight (light);

  return csPtr<iDynLight> (&(light->scfiDynLight));
}

void csEngine::RemoveDynLight (iDynLight *light)
{
  RemoveDynLight (light->GetPrivateObject ());
}

csPtr<iMeshFactoryWrapper> csEngine::CreateMeshFactory (
  const char *classId,
  const char *name)
{
  // WARNING! In the past this routine checked if the factory
  // was already created. This is wrong! This routine should not do
  // this and instead allow duplicate factories (with the same name).
  // That's because that duplicate factory can still be in another
  // region. And even if this is not the case then factories with same
  // name are still allowed.
  csRef<iPluginManager> plugin_mgr (
  	CS_QUERY_REGISTRY (object_reg, iPluginManager));
  csRef<iMeshObjectType> type (CS_QUERY_PLUGIN_CLASS (
      plugin_mgr,
      classId,
      iMeshObjectType));
  if (!type) type = CS_LOAD_PLUGIN (plugin_mgr, classId, iMeshObjectType);
  if (!type) return 0;

  csRef<iMeshObjectFactory> fact (type->NewFactory ());
  if (!fact) return 0;

  // don't pass the name to avoid a second search
  csRef<iMeshFactoryWrapper> fwrap (CreateMeshFactory (fact, 0));
  if (fwrap && name) fwrap->QueryObject ()->SetName (name);
  return csPtr<iMeshFactoryWrapper> (fwrap);
}

csPtr<iMeshFactoryWrapper> csEngine::CreateMeshFactory (
  iMeshObjectFactory *fact,
  const char *name)
{
  // WARNING! In the past this routine checked if the factory
  // was already created. This is wrong! This routine should not do
  // this and instead allow duplicate factories (with the same name).
  // That's because that duplicate factory can still be in another
  // region. And even if this is not the case then factories with same
  // name are still allowed.
  csMeshFactoryWrapper *mfactwrap = new csMeshFactoryWrapper (fact);
  if (name) mfactwrap->SetName (name);
  GetMeshFactories ()->Add (&(mfactwrap->scfiMeshFactoryWrapper));
  fact->SetLogicalParent (mfactwrap);
  return csPtr<iMeshFactoryWrapper> (&mfactwrap->scfiMeshFactoryWrapper);
}

csPtr<iMeshFactoryWrapper> csEngine::CreateMeshFactory (const char *name)
{
  // WARNING! In the past this routine checked if the factory
  // was already created. This is wrong! This routine should not do
  // this and instead allow duplicate factories (with the same name).
  // That's because that duplicate factory can still be in another
  // region. And even if this is not the case then factories with same
  // name are still allowed.
  csMeshFactoryWrapper *mfactwrap = new csMeshFactoryWrapper ();
  if (name) mfactwrap->SetName (name);
  GetMeshFactories ()->Add (&(mfactwrap->scfiMeshFactoryWrapper));
  return csPtr<iMeshFactoryWrapper> (&mfactwrap->scfiMeshFactoryWrapper);
}

//------------------------------------------------------------------------

/*
 * Loader context class for the engine.
 */
class EngineLoaderContext : public iLoaderContext
{
private:
  iEngine* Engine;
  iRegion* region;
  bool curRegOnly;

public:
  EngineLoaderContext (iEngine* Engine, iRegion* region, bool curRegOnly);
  virtual ~EngineLoaderContext ();

  SCF_DECLARE_IBASE;

  virtual iSector* FindSector (const char* name);
  virtual iMaterialWrapper* FindMaterial (const char* name);
  virtual iMeshFactoryWrapper* FindMeshFactory (const char* name);
  virtual iMeshWrapper* FindMeshObject (const char* name);
  virtual iTextureWrapper* FindTexture (const char* name);
  virtual iLight* FindLight(const char *name);
  virtual bool CheckDupes () const { return false; }
  virtual iRegion* GetRegion () const { return region; }
  virtual bool CurrentRegionOnly () const { return curRegOnly; }
};

SCF_IMPLEMENT_IBASE(EngineLoaderContext);
  SCF_IMPLEMENTS_INTERFACE(iLoaderContext);
SCF_IMPLEMENT_IBASE_END;

EngineLoaderContext::EngineLoaderContext (iEngine* Engine,
	iRegion* region, bool curRegOnly)
{
  SCF_CONSTRUCT_IBASE (0);
  EngineLoaderContext::Engine = Engine;
  EngineLoaderContext::region = region;
  EngineLoaderContext::curRegOnly = curRegOnly;
}

EngineLoaderContext::~EngineLoaderContext ()
{
}

iSector* EngineLoaderContext::FindSector (const char* name)
{
  return Engine->FindSector (name, curRegOnly ? region : 0);
}

iMaterialWrapper* EngineLoaderContext::FindMaterial (const char* name)
{
  return Engine->FindMaterial (name, curRegOnly ? region : 0);
}

iMeshFactoryWrapper* EngineLoaderContext::FindMeshFactory (const char* name)
{
  return Engine->FindMeshFactory (name, curRegOnly ? region : 0);
}

iMeshWrapper* EngineLoaderContext::FindMeshObject (const char* name)
{
  return Engine->FindMeshObject (name, curRegOnly ? region : 0);
}

iTextureWrapper* EngineLoaderContext::FindTexture (const char* name)
{
  return Engine->FindTexture (name, curRegOnly ? region : 0);
}

iLight* EngineLoaderContext::FindLight(const char *name)
{
  // This function is necessary because Engine::FindLight returns iStatLight
  // and not iLight.
  csRef<iLightIterator> li = Engine->GetLightIterator (
  	curRegOnly ? region : 0);
  iLight *light;

  while (li->HasNext ())
  {
    light = li->Next ();
    if (!strcmp (light->QueryObject ()->GetName (),name))
      return light;
  }
  return 0;
}

//------------------------------------------------------------------------

csPtr<iLoaderContext> csEngine::CreateLoaderContext (iRegion* region,
	bool curRegOnly)
{
  return csPtr<iLoaderContext> (new EngineLoaderContext (this, region,
  	curRegOnly));
}

//------------------------------------------------------------------------

csPtr<iMeshFactoryWrapper> csEngine::LoadMeshFactory (
  const char *name,
  const char *loaderClassId,
  iDataBuffer *input)
{
  csRef<iDocumentSystem> xml (
    	CS_QUERY_REGISTRY (object_reg, iDocumentSystem));
  if (!xml) xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
  csRef<iDocument> doc = xml->CreateDocument ();
  const char* error = doc->Parse (input);
  if (error != 0)
  {
    // @@@ Report error?
    return 0;
  }

  csRef<iPluginManager> plugin_mgr (
  	CS_QUERY_REGISTRY (object_reg, iPluginManager));
  csRef<iLoaderPlugin> plug (CS_QUERY_PLUGIN_CLASS (
      plugin_mgr,
      loaderClassId,
      iLoaderPlugin));
  if (!plug)
    plug = CS_LOAD_PLUGIN (plugin_mgr, loaderClassId, iLoaderPlugin);
  if (!plug) return 0;

  csRef<iMeshFactoryWrapper> fact (CreateMeshFactory (name));
  if (!fact) return 0;

  csRef<iLoaderContext> elctxt (CreateLoaderContext (0, true));
  csRef<iBase> mof (plug->Parse (
      doc->GetRoot (), elctxt, fact->GetMeshObjectFactory ()));
  if (!mof)
  {
    GetMeshFactories ()->Remove (fact);
    return 0;
  }

  csRef<iMeshObjectFactory> mof2 (
  	SCF_QUERY_INTERFACE (mof, iMeshObjectFactory));
  if (!mof2)
  {
    // @@@ ERROR?
    GetMeshFactories ()->Remove (fact);
    return 0;
  }

  fact->SetMeshObjectFactory (mof2);
  mof2->SetLogicalParent (fact);

  return csPtr<iMeshFactoryWrapper> (fact);
}

csPtr<iMeshWrapper> csEngine::LoadMeshWrapper (
  const char *name,
  const char *loaderClassId,
  iDataBuffer *input,
  iSector *sector,
  const csVector3 &pos)
{
  csRef<iDocumentSystem> xml (
    	CS_QUERY_REGISTRY (object_reg, iDocumentSystem));
  if (!xml) xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
  csRef<iDocument> doc = xml->CreateDocument ();
  const char* error = doc->Parse (input);
  if (error != 0)
  {
    // @@@ Report error?
    return 0;
  }

  csRef<iPluginManager> plugin_mgr (
  	CS_QUERY_REGISTRY (object_reg, iPluginManager));
  csRef<iLoaderPlugin> plug (CS_QUERY_PLUGIN_CLASS (
      plugin_mgr,
      loaderClassId,
      iLoaderPlugin));
  if (!plug)
    plug = CS_LOAD_PLUGIN (plugin_mgr, loaderClassId, iLoaderPlugin);
  if (!plug) return 0;

  csMeshWrapper *meshwrap = new csMeshWrapper (0);
  if (name) meshwrap->SetName (name);

  iMeshWrapper *imw = &(meshwrap->scfiMeshWrapper);
  GetMeshes ()->Add (imw);
  imw->DecRef (); // the ref is now stored in the MeshList
  if (sector)
  {
    meshwrap->GetMovable ().SetSector (sector);
    meshwrap->GetMovable ().SetPosition (pos);
    meshwrap->GetMovable ().UpdateMove ();
  }

  csRef<iLoaderContext> elctxt (CreateLoaderContext (0, true));
  csRef<iBase> mof (plug->Parse (doc->GetRoot (), elctxt, imw));
  if (!mof)
  {
    GetMeshes ()->Remove (imw);
    return 0;
  }

  csRef<iMeshObject> mof2 (SCF_QUERY_INTERFACE (mof, iMeshObject));
  meshwrap->SetMeshObject (mof2);
  return csPtr<iMeshWrapper> (imw);
}

csPtr<iMeshWrapper> csEngine::CreateMeshWrapper (
  iMeshFactoryWrapper *factory,
  const char *name,
  iSector *sector,
  const csVector3 &pos)
{
  iMeshWrapper *mesh = factory->CreateMeshWrapper ();
  if (name) mesh->QueryObject ()->SetName (name);
  GetMeshes ()->Add (mesh);
  if (sector)
  {
    mesh->GetMovable ()->SetSector (sector);
    mesh->GetMovable ()->SetPosition (pos);
    mesh->GetMovable ()->UpdateMove ();
  }

  mesh->GetMeshObject ()->SetLogicalParent (mesh);
  return csPtr<iMeshWrapper> (mesh);
}

csPtr<iMeshWrapper> csEngine::CreateMeshWrapper (
  iMeshObject *mesh,
  const char *name,
  iSector *sector,
  const csVector3 &pos)
{
  csMeshWrapper *meshwrap = new csMeshWrapper (0, mesh);
  if (name) meshwrap->SetName (name);
  GetMeshes ()->Add (&(meshwrap->scfiMeshWrapper));
  if (sector)
  {
    meshwrap->GetMovable ().SetSector (sector);
    meshwrap->GetMovable ().SetPosition (pos);
    meshwrap->GetMovable ().UpdateMove ();
  }

  mesh->SetLogicalParent (meshwrap);
  return csPtr<iMeshWrapper> (&meshwrap->scfiMeshWrapper);
}

csPtr<iMeshWrapper> csEngine::CreateMeshWrapper (const char *name)
{
  csMeshWrapper *meshwrap = new csMeshWrapper (0);
  if (name) meshwrap->SetName (name);
  GetMeshes ()->Add (&(meshwrap->scfiMeshWrapper));
  return csPtr<iMeshWrapper> (&meshwrap->scfiMeshWrapper);
}

csPtr<iMeshWrapper> csEngine::CreateMeshWrapper (
  const char *classId,
  const char *name,
  iSector *sector,
  const csVector3 &pos)
{
  csRef<iPluginManager> plugin_mgr (
  	CS_QUERY_REGISTRY (object_reg, iPluginManager));
  csRef<iMeshObjectType> type (CS_QUERY_PLUGIN_CLASS (
      plugin_mgr, classId, iMeshObjectType));
  if (!type) type = CS_LOAD_PLUGIN (plugin_mgr, classId, iMeshObjectType);
  if (!type) return 0;

  csRef<iMeshObjectFactory> fact (type->NewFactory ());
  if (!fact) return 0;

  csRef<iMeshObject> mo (SCF_QUERY_INTERFACE (fact, iMeshObject));
  if (!mo)
  {
    // The factory is not itself a mesh object. Let's see if the
    // factory can return a working mesh object.
    mo = fact->NewInstance ();
    if (mo)
      return CreateMeshWrapper (mo, name, sector, pos);
    return 0;
  }

  return CreateMeshWrapper (mo, name, sector, pos);
}

bool csEngine::RemoveObject (iBase *object)
{
  {
    csRef<iSector> sector (SCF_QUERY_INTERFACE (object, iSector));
    if (sector)
    {
      // Remove from region it might be in.
      if (sector->QueryObject ()->GetObjectParent ())
      {
	sector->QueryObject ()->GetObjectParent ()->ObjRemove (
		sector->QueryObject ());
      }
      sectors.Remove (sector);
      return true;
    }
  }
  {
    csRef<iCameraPosition> cp (SCF_QUERY_INTERFACE (object, iCameraPosition));
    if (cp)
    {
      // Remove from region it might be in.
      if (cp->QueryObject ()->GetObjectParent ())
      {
	cp->QueryObject ()->GetObjectParent ()->ObjRemove (
		cp->QueryObject ());
      }
      camera_positions.scfiCameraPositionList.Remove (cp);
      return true;
    }
  }
  {
    csRef<iDynLight> dl (SCF_QUERY_INTERFACE (object, iDynLight));
    if (dl)
    {
      // Remove from region it might be in.
      if (dl->QueryObject ()->GetObjectParent ())
      {
	dl->QueryObject ()->GetObjectParent ()->ObjRemove (
		dl->QueryObject ());
      }
      RemoveDynLight (dl);
      return true;
    }
  }
  {
    csRef<iCollection> col (SCF_QUERY_INTERFACE (object, iCollection));
    if (col)
    {
      // Remove from region it might be in.
      if (col->QueryObject ()->GetObjectParent ())
      {
	col->QueryObject ()->GetObjectParent ()->ObjRemove (
		col->QueryObject ());
      }
      collections.scfiCollectionList.Remove (col);
      return true;
    }
  }
  {
    csRef<iTextureWrapper> txt (SCF_QUERY_INTERFACE (object, iTextureWrapper));
    if (txt)
    {
      // Remove from region it might be in.
      if (txt->QueryObject ()->GetObjectParent ())
      {
	txt->QueryObject ()->GetObjectParent ()->ObjRemove (
		txt->QueryObject ());
      }
      GetTextureList ()->Remove (txt);
      return true;
    }
  }
  {
    csRef<iMaterialWrapper> mat (SCF_QUERY_INTERFACE (
        object,
        iMaterialWrapper));
    if (mat)
    {
      // Remove from region it might be in.
      if (mat->QueryObject ()->GetObjectParent ())
      {
	mat->QueryObject ()->GetObjectParent ()->ObjRemove (
		mat->QueryObject ());
      }
      GetMaterialList ()->Remove (mat);
      return true;
    }
  }
  {
    csRef<iMeshFactoryWrapper> factwrap (SCF_QUERY_INTERFACE (
        object,
        iMeshFactoryWrapper));
    if (factwrap)
    {
      // Remove from region it might be in.
      if (factwrap->QueryObject ()->GetObjectParent ())
      {
	factwrap->QueryObject ()->GetObjectParent ()->ObjRemove (
		factwrap->QueryObject ());
      }
      mesh_factories.Remove (factwrap);
      return true;
    }
  }
  {
    csRef<iMeshWrapper> meshwrap (SCF_QUERY_INTERFACE (object, iMeshWrapper));
    if (meshwrap)
    {
      // Remove from region it might be in.
      if (meshwrap->QueryObject ()->GetObjectParent ())
      {
	meshwrap->QueryObject ()->GetObjectParent ()->ObjRemove (
		meshwrap->QueryObject ());
      }
      meshes.Remove (meshwrap);
      return true;
    }
  }

  return false;
}

void csEngine::Resize ()
{
  frame_width = G3D->GetWidth ();
  frame_height = G3D->GetHeight ();
}

void csEngine::SetContext (iTextureHandle *txthandle)
{
  if (render_context != txthandle)
  {
    render_context = txthandle;
    if (render_context)
    {
      render_context->GetMipMapDimensions (0, frame_width, frame_height);
    }
    else
    {
      frame_width = G3D->GetWidth ();
      frame_height = G3D->GetHeight ();
    }
  }
}

iTextureHandle *csEngine::GetContext () const
{
  return render_context;
}

iClipper2D *csEngine::GetTopLevelClipper () const
{
  return top_clipper;
}

void csEngine::SetAmbientLight (const csColor &c)
{
  csLight::ambient_red = int (c.red * 255.0f);
  csLight::ambient_green = int (c.green * 255.0f);
  csLight::ambient_blue = int (c.blue * 255.0f);
}

void csEngine::GetAmbientLight (csColor &c) const
{
  c.red = csLight::ambient_red / 255.0f;
  c.green = csLight::ambient_green / 255.0f;
  c.blue = csLight::ambient_blue / 255.0f;
}

void csEngine::GetDefaultAmbientLight (csColor &c) const
{   
  c.red = default_ambient_red / 255.0f;
  c.green = default_ambient_green / 255.0f;
  c.blue = default_ambient_blue / 255.0f;
}

csPtr<iFrustumView> csEngine::CreateFrustumView ()
{
  csFrustumView* lview = new csFrustumView ();
  lview->EnableThingShadows (CS_LIGHT_THINGSHADOWS);
  lview->SetShadowMask (CS_ENTITY_NOSHADOWS, 0);
  lview->SetProcessMask (CS_ENTITY_NOLIGHTING, 0);
  return csPtr<iFrustumView> (lview);
}

csPtr<iObjectWatcher> csEngine::CreateObjectWatcher ()
{
  csObjectWatcher* watch = new csObjectWatcher ();
  return csPtr<iObjectWatcher> (watch);
}

void csEngine::WantToDie (iMeshWrapper* mesh)
{
  want_to_die.Add (mesh);
}

bool csEngine::DebugCommand (const char* cmd)
{
  if (!strcasecmp (cmd, "toggle_cullstat"))
  {
    return true;
  }
  return false;
}

//-------------------End-Multi-Context-Support--------------------------------

#ifdef CS_USE_NEW_RENDERER
// ======================================================================
// Render loop stuff
// ======================================================================
  
iRenderLoopManager* csEngine::GetRenderLoopManager ()
{
  return renderLoopManager;
}

iRenderLoop* csEngine::GetCurrentDefaultRenderloop ()
{
  return defaultRenderLoop;
}

bool csEngine::SetCurrentDefaultRenderloop (iRenderLoop* loop)
{
  if (loop == 0) return false;
  defaultRenderLoop = loop;
  return true;
}

#endif
