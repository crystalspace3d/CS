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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define CS_SYSDEF_PROVIDE_ALLOCA
#define CS_SYSDEF_PROVIDE_PATH
#include "cssysdef.h"
#include "cssys/system.h"
#include "cssys/sysdriv.h"
#include "cssys/csshlib.h"
#include "csutil/csevent.h"
#include "csutil/cseventq.h"
#include "csutil/csinput.h"
#include "csutil/cfgfile.h"
#include "csutil/cfgmgr.h"
#include "csutil/cfgacc.h"
#include "csutil/cmdline.h"
#include "csutil/prfxcfg.h"
#include "csutil/util.h"
#include "iutil/eventq.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "isys/vfs.h"
#include "ivaria/conout.h"
#include "inetwork/driver.h"
#include "iutil/config.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "iengine/motion.h"
#include "ivaria/reporter.h"

void csSystemDriver::ReportSys (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  iReporter* rep = CS_QUERY_REGISTRY (&object_reg, iReporter);
  if (rep)
  {
    rep->ReportV (severity, "crystalspace.system", msg, arg);
  }
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

//------------------------------------------------------ csPlugin class -----//

csSystemDriver::csPlugin::csPlugin (iComponent *iObject, const char *iClassID,
  const char *iFuncID)
{
  Plugin = iObject;
  ClassID = csStrNew (iClassID);
  FuncID = csStrNew (iFuncID);
}

csSystemDriver::csPlugin::~csPlugin ()
{
  delete [] ClassID;
  delete [] FuncID;
  Plugin->DecRef ();
}

//---------------------- A private class used to keep a list of plugins -----//

struct csPluginLoadRec
{
  char *FuncID;
  char *ClassID;

  csPluginLoadRec (const char *iFuncID, const char *iClassID)
  { FuncID = csStrNew (iFuncID); ClassID = csStrNew (iClassID); }
  ~csPluginLoadRec ()
  { delete [] ClassID; delete [] FuncID; }
};

class csPluginList : public csVector
{
public:
  virtual ~csPluginList ()
  { DeleteAll (); }
  bool Sort (csSystemDriver *iSys);
  csPluginLoadRec &Get (int idx)
  { return *(csPluginLoadRec *)csVector::Get (idx); }
  virtual bool FreeItem (csSome Item)
  { delete (csPluginLoadRec *)Item; return true; }
private:
  bool RecurseSort (csSystemDriver*, int row, char *order, char *loop,
    bool *matrix);
};

/**
 * Since every plugin can depend on another one, the plugin loader should be
 * able to sort them by their preferences. Thus, if some plugin A wants some
 * other plugins B and C to be loaded before him, the plugin loader should
 * sort the list of loaded plugins such that plugin A comes after B and C.
 * <p>
 * Of course it is possible that some plugin A depends on B and B depends on A,
 * or even worse A on B, B on C and C on A. The sort algorithm should detect
 * this case and type an error message if it is detected.
 * <p>
 * The alogorithm works as follows. First, a dependency matrix is built. Here
 * is a example of a simple dependency matrix:
 * <pre>
 *                iEngine      iVFS     iGraphics3D iGraphics2D
 *             +-----------+-----------+-----------+-----------+
 * iEngine     |           |     X     |     X     |     X     |
 *             +-----------+-----------+-----------+-----------+
 * iVFS        |           |           |           |           |
 *             +-----------+-----------+-----------+-----------+
 * iGraphics3D |           |     X     |           |     X     |
 *             +-----------+-----------+-----------+-----------+
 * iGraphics2D |           |     X     |           |           |
 *             +-----------+-----------+-----------+-----------+
 * </pre>
 * Thus, we see that the iEngine plugin depends on iVFS, iGraphics3D and
 * iGraphics2D plugins (this is an abstract example, in reality the
 * things are simpler), iVFS does not depend on anything, iGraphics3D
 * wants the iVFS and the iGraphics2D plugins, and finally iGraphics2D
 * wants just the iVFS.
 * <p>
 * The sort algorithm works as follows: we take each plugin, one by one
 * starting from first (iEngine) and examine each of them. If some plugin
 * depends on others, we recursively launch this algorithm on those plugins.
 * If we don't have any more dependencies, we put the plugin into the
 * load list and return to the previous recursion level. To detect loops
 * we need to maintain an "recurse list", thus if we found that iEngine
 * depends on iGraphics3D, iGraphics3D depends on iGraphics2D and we're
 * examining iGraphics2D for dependencies, we have the following
 * loop-detection array: iEngine, iGraphics3D, iGraphics2D. If we find that
 * iGraphics2D depends on anyone that is in the loop array, we found a loop.
 * If we find that the plugin depends on anyone that is already in the load
 * list, its not a loop but just an already-fullfilled dependency.
 * Thus, the above table will be traversed this way (to the left is the
 * load list, to the right is the loop detection list):
 * <pre><ol>
 *   <li> []                                  [iEngine]
 *   <li> []                                  [iEngine,iVFS]
 *   <li> [iVFS]                              [iEngine]
 *   <li> [iVFS]                              [iEngine,iGraphics3D]
 *   <li> [iVFS]                              [iEngine,iGraphics3D,iGraphics2D]
 *   <li> [iVFS,iGraphics2D]                  [iEngine,iGraphics3D]
 *   <li> [iVFS,iGraphics2D,iGraphics3D]      [iEngine]
 *   <li> [iVFS,iGraphics2D,iGraphics3D,iEngine] []
 * </ol></pre>
 * In this example we traversed all plugins in one go. If we didn't, we
 * just take the next one (iEngine, iVFS, iGraphics3D, iGraphics2D) and if
 * it is not already in the load list, recursively traverse it.
 */
bool csPluginList::Sort (csSystemDriver *iSys)
{
  int row, col, len = Length ();

  // We'll use char for speed reasons
  if (len > 255)
  {
    iSys->ReportSys (CS_REPORTER_SEVERITY_ERROR,
    	"PLUGIN LOADER: Too many plugins requested (%d, max 255)\n", len);
    return false;
  }

  // Build the dependency matrix
  bool *matrix = (bool *)alloca (len * len * sizeof (bool));
  memset (matrix, 0, len * len * sizeof (bool));
  for (row = 0; row < len; row++)
  {
    const char *dep = iSCF::SCF->GetClassDependencies (Get (row).ClassID);
    while (dep && *dep)
    {
      char tmp [100];
      const char *comma = strchr (dep, ',');
      if (!comma)
        comma = strchr (dep, 0);
      size_t sl = comma - dep;
      if (sl >= sizeof (tmp))
        sl = sizeof (tmp) - 1;
      memcpy (tmp, dep, sl);
      while (sl && ((tmp [sl - 1] == ' ') || (tmp [sl - 1] == '\t')))
        sl--;
      tmp [sl] = 0;
      if (!sl)
        break;
      bool wildcard = tmp [sl - 1] == '.';
      for (col = 0; col < len; col++)
        if ((col != row)
         && (wildcard ? strncmp (tmp, Get (col).ClassID, sl) :
             strcmp (tmp, Get (col).ClassID)) == 0)
          matrix [row * len + col] = true;
      dep = comma;
      while (*dep == ',' || *dep == ' ' || *dep == '\t')
        dep++;
    }
  }

  // Go through dependency matrix and put all plugins into an array
  bool error = false;
  char *order = (char *)alloca (len + 1);
  *order = 0;
  char *loop = (char *)alloca (len + 1);
  *loop = 0;

  for (row = 0; row < len; row++)
    if (!RecurseSort (iSys, row, order, loop, matrix))
      error = true;

  // Reorder plugin list according to "order" array
  csSome *newroot = (csSome *)malloc (len * sizeof (csSome));
  for (row = 0; row < len; row++)
    newroot [row] = root [order [row] - 1];
  free (root); root = newroot;

  return !error;
}

bool csPluginList::RecurseSort (csSystemDriver *iSys, int row, char *order,
  char *loop, bool *matrix)
{
  // If the plugin is already in the load list, skip it
  if (strchr (order, row + 1))
    return true;

  int len = Length ();
  bool *dep = matrix + row * len;
  bool error = false;
  char *loopp = strchr (loop, 0);
  *loopp++ = row + 1; *loopp = 0;
  int col, x;
  for (col = 0; col < len; col++)
    if (*dep++)
    {
      // If the plugin is already loaded, skip
      if (strchr (order, col + 1))
        continue;

      char *already = strchr (loop, col + 1);
      if (already)
      {
        iSys->ReportSys (CS_REPORTER_SEVERITY_ERROR,
		"PLUGIN LOADER: Cyclic dependency detected!\n");
        int startx = int (already - loop);
        for (x = startx; loop [x]; x++)
          iSys->ReportSys (CS_REPORTER_SEVERITY_ERROR, "   %s %s\n",
            x == startx ? "+->" : loop [x + 1] ? "| |" : "<-+",
            Get (loop [x] - 1).ClassID);
        error = true;
        break;
      }

      bool recurse_error = !RecurseSort (iSys, col, order, loop, matrix);

      // Drop recursive loop dependency since it has already been ordered.
      *loopp = 0;

      if (recurse_error)
      {
        error = true;
        break;
      }
    }

  // Put current plugin into the list
  char *orderp = strchr (order, 0);
  *orderp++ = row + 1; *orderp = 0;

  return !error;
}

//--------------------------------------------------- The System Driver -----//

SCF_IMPLEMENT_IBASE (csSystemDriver)
  SCF_IMPLEMENTS_INTERFACE (iSystem)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iPluginManager)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iVirtualClock)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iEventHandler)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSystemDriver::PluginManager)
  SCF_IMPLEMENTS_INTERFACE (iPluginManager)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSystemDriver::VirtualClock)
  SCF_IMPLEMENTS_INTERFACE (iVirtualClock)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSystemDriver::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csSystemDriver::eiEventHandler)
  SCF_IMPLEMENTS_INTERFACE (iEventHandler)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csSystemDriver::csSystemDriver () :
  VFS(0), EventQueue(0), Plugins (8, 8), OptionList (16, 16)
{
  SCF_CONSTRUCT_IBASE (NULL);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiPluginManager);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiVirtualClock);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiComponent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiEventHandler);

  Shutdown = false;
  CurrentTime = csTicks (-1);

  // Register the shared event queue.
  iEventQueue* q = new csEventQueue (&object_reg);
  object_reg.Register (q, NULL);
  q->DecRef();

  object_reg.Register (&scfiPluginManager, NULL);
  object_reg.Register (&scfiVirtualClock, NULL);
  iCommandLineParser* cmdline = new csCommandLineParser ();
  object_reg.Register (cmdline, "CommandLine");
  //@@@ cmdline->DecRef (); Uncomment when object registry moves out
  // of system driver.

  //@@@ We temporarily register iSystem with the object registry too.
  // That allows us to remove the usage of iSystem everywhere except in
  // a few places where this is still needed. This is only transitionary.
  object_reg.Register (this, "System");

  // Initialize Shared Class Facility|
  char scfconfigpath [MAXPATHLEN + 1];

#ifndef CS_STATIC_LINKED
  // Add both installpath and installpath/lib dirs to search for plugins
  csGetInstallPath (scfconfigpath, sizeof (scfconfigpath));
  csAddLibraryPath (scfconfigpath);
  strcat (scfconfigpath, "lib");   
  int scfconfiglen = strlen(scfconfigpath);
  scfconfigpath[scfconfiglen] = PATH_SEPARATOR;
  scfconfigpath[scfconfiglen+1] = 0;
  csAddLibraryPath (scfconfigpath);
#endif

  // Find scf.cfg and initialize SCF
  csGetInstallPath (scfconfigpath, sizeof (scfconfigpath));
  strcat (scfconfigpath, "scf.cfg");
  csConfigFile scfconfig (scfconfigpath);
  scfInitialize (&scfconfig);

  iConfigFile *cfg = new csConfigFile();
  iConfigManager* Config = new csConfigManager(cfg, true);
  object_reg.Register (Config, NULL);
  Config->DecRef ();
  cfg->DecRef ();
}

csSystemDriver::~csSystemDriver ()
{
  Close ();

  ReportSys (CS_REPORTER_SEVERITY_DEBUG,
  	"*** System driver is going to shut down now!\n");

  // Free all plugin options (also decrefs their iConfig interfaces)
  OptionList.DeleteAll ();

  // Deregister all known drivers and plugins

  // @@ either comment the following out or the appropriate CHECK () in UnloadPlugins
  // i'm not sure whats nicer. I'd prefer to decref it here but then a special
  // treating of VFS is still needed in UnloadPlugin. Since the system class wont 
  // live forever, we can simply wait for the biological solution.
  //  if (VFS) VFS->DecRef ();

  // Free all plugins.
  int i;
  for (i = Plugins.Length()-1; i >= 0; i--)
      UnloadPlugin((iComponent *)Plugins.Get(i)->Plugin);
     
  if (EventQueue != 0)
    EventQueue->DecRef();

  // Explicitly clear the object registry before its destruction since some
  // objects being cleared from it may need to query it for other objects, and
  // such queries can fail (depending upon the compiler) if they are made while
  // the registry itself it being destroyed.  Furthermore, such objects may may
  // SCF queries as they are destroyed, so this must occur before SCF is
  // finalized (see below).
  object_reg.Clear();

  iSCF::SCF->Finish();
}

bool csSystemDriver::Initialize (int argc, const char* const argv[],
  const char *iConfigName)
{
  EventQueue = CS_QUERY_REGISTRY (&object_reg, iEventQueue);
  EventQueue->IncRef();

  // @@@ This is ugly.  We need a better, more generalized way of doing this.
  // Hard-coding the name of the VFS plugin (crytalspace.kernel.vfs) is bad.
  // Then later ensuring that we skip over this same plugin when requested
  // by the client is even uglier.  The reason that the VFS plugin is required
  // this early is that both the application configuration file and the
  // configuration file for other plugins may (and almost always do) reside on
  // a VFS volume.

  // we first create an empty application config file, so we can create the
  // config manager at all. Then we load the VFS. After that, all config files
  // can be loaded. At the end, we make the user-and-application-specific
  // config file the dynamic one.

  ReportSys (CS_REPORTER_SEVERITY_DEBUG, "*** Initializing system driver!\n");

#if 0
  iConfigFile *cfg = new csConfigFile();
  iConfigManager* Config = new csConfigManager(cfg, true);
  object_reg.Register (Config, NULL);
  Config->DecRef ();
  cfg->DecRef ();
#endif

  iConfigManager* Config = CS_QUERY_REGISTRY (&object_reg, iConfigManager);
  iConfigFile* cfg = Config->GetDynamicDomain ();
  Config->SetDomainPriority(cfg, iConfigManager::ConfigPriorityApplication);
  VFS = CS_LOAD_PLUGIN (this, "crystalspace.kernel.vfs", CS_FUNCID_VFS, iVFS);

  // Initialize application configuration file
  if (iConfigName)
    if (!cfg->Load (iConfigName, VFS))
      ReportSys (CS_REPORTER_SEVERITY_WARNING,
	"WARNING: Failed to load configuration file `%s'\n", iConfigName);

  // look if the user-specific config domain should be used
  {
    csConfigAccess cfgacc (&object_reg, "/config/system.cfg");
    if (cfgacc->GetBool("System.UserConfig", true))
    {
      // open the user-specific, application-neutral config domain
      cfg = OpenUserConfig("Global", "User.Global");
      Config->AddDomain(cfg, iConfigManager::ConfigPriorityUserGlobal);
      cfg->DecRef();

      // open the user-and-application-specific config domain
      cfg = OpenUserConfig(cfgacc->GetStr("System.ApplicationID", "Noname"),
        "User.Application");
      Config->AddDomain(cfg, iConfigManager::ConfigPriorityUserApp);
      Config->SetDynamicDomain(cfg);
      cfg->DecRef();
    }
  }
  
  // Register some generic pseudo-plugins.  (Some day these should probably
  // become real plugins.)
  iKeyboardDriver* k = new csKeyboardDriver (&object_reg);
  iMouseDriver*    m = new csMouseDriver    (&object_reg);
  iJoystickDriver* j = new csJoystickDriver (&object_reg);
  object_reg.Register (k, NULL);
  object_reg.Register (m, NULL);
  object_reg.Register (j, NULL);
  j->DecRef();
  m->DecRef();
  k->DecRef();

  // Collect all options from command line
  iCommandLineParser* CommandLine = CS_QUERY_REGISTRY (&object_reg,
  	iCommandLineParser);
  CommandLine->Initialize (argc, argv);

  // Analyse config and command line
  SetSystemDefaults (Config);

  // The list of plugins
  csPluginList PluginList;

  // Now eat all common-for-plugins command-line switches
  bool g3d_override = false;

  const char *val;
  if ((val = CommandLine->GetOption ("video")))
  {
    // Alternate videodriver
    char temp [100];
    sprintf (temp, "crystalspace.graphics3d.%s", val);
    ReportSys (CS_REPORTER_SEVERITY_NOTIFY,
    	"Using alternative 3D driver: %s\n", temp);
    PluginList.Push (new csPluginLoadRec (CS_FUNCID_VIDEO, temp));
    g3d_override = true;
  }
  if ((val = CommandLine->GetOption ("canvas")))
  {
    char temp [100];
    if (!strchr (val, '.'))
    {
      sprintf (temp, "crystalspace.graphics2d.%s", val);
      CommandLine->ReplaceOption ("canvas", temp);
    }
  }

  // Eat all --plugin switches specified on the command line
  int n = 0;
  while ((val = CommandLine->GetOption ("plugin", n++)))
  {
    size_t sl = strlen (val);
    char temp [100];
    if (sl >= sizeof (temp)) sl = sizeof (temp) - 1;
    memcpy (temp, val, sl); temp [sl] = 0;
    char *func = strchr (temp, ':');
    if (func) *func++ = 0;
    if (g3d_override && !strcmp (CS_FUNCID_VIDEO, func)) continue;
    PluginList.Push (new csPluginLoadRec (func, temp));
  }

  // Now load and initialize all plugins
  iConfigIterator *plugin_list = Config->Enumerate ("System.Plugins.");
  if (plugin_list)
  {
    while (plugin_list->Next ())
    {
      const char *funcID = plugin_list->GetKey (true);
      // If -video was used to override 3D driver, then respect it.
      if (g3d_override && strcmp (funcID, CS_FUNCID_VIDEO) == 0)
        continue;
      const char *classID = plugin_list->GetStr ();
      if (classID)
        PluginList.Push (new csPluginLoadRec (funcID, classID));
    }
    plugin_list->DecRef ();
  }

  // Sort all plugins by their dependency lists
  if (!PluginList.Sort (this))
    return false;

  // Load all plugins
  for (n = 0; n < PluginList.Length (); n++)
  {
    const csPluginLoadRec& r = PluginList.Get(n);
    // If plugin is VFS then skip if already loaded earlier.
    if (VFS && r.FuncID && strcmp (r.FuncID, CS_FUNCID_VFS) == 0)
      continue;
    iBase *plg = LoadPlugin (r.ClassID, r.FuncID, NULL, 0);
    if (plg) plg->DecRef ();
  }

  /// Now find the drivers that are known by the system driver
  if (!VFS)
    VFS = CS_QUERY_PLUGIN_ID (this, CS_FUNCID_VFS, iVFS);

  // flush all removed config files
  Config->FlushRemoved();
  return true;
}

bool csSystemDriver::Open ()
{
  ReportSys (CS_REPORTER_SEVERITY_DEBUG, "*** Opening the drivers now!\n");

  // Start listening for events.  This class is interested in cscmdQuit, but
  // subclasses may be interested in any event, so listen for all types except
  // for the special one which sends cscmdPreProcess/cscmdPostProcess.
  EventQueue->RegisterListener(&scfiEventHandler, ~CSMASK_Nothing);

  // Now pass the open event to all plugins
  csEvent Event (csGetTicks (), csevBroadcast, cscmdSystemOpen);
  EventQueue->Dispatch (Event);

  return true;
}

void csSystemDriver::Close ()
{
  ReportSys (CS_REPORTER_SEVERITY_DEBUG, "*** Closing the drivers now!\n");

  // Warn all plugins the system is going down
  csEvent Event (csGetTicks (), csevBroadcast, cscmdSystemClose);
  EventQueue->Dispatch (Event);

  // Stop listening for events.
  EventQueue->RemoveListener(&scfiEventHandler);
}

void csSystemDriver::AdvanceVirtualTimeClock ()
{
  csTicks last = CurrentTime;
  CurrentTime = csGetTicks ();
  if (last == csTicks(-1))
    ElapsedTime = 0;
  else
    ElapsedTime = CurrentTime - last;
}

void csSystemDriver::NextFrame ()
{
  // Update elapsed time first
  AdvanceVirtualTimeClock ();

  // Process the event queue.
  EventQueue->Process();
}

void csSystemDriver::Loop ()
{
  while (!Shutdown)
    NextFrame ();
}

bool csSystemDriver::HandleEvent (iEvent& e)
{
  if (e.Type == csevBroadcast && e.Command.Code == cscmdQuit)
  {
    Shutdown = true;
    return true;
  }
  return false;
}

void csSystemDriver::SetSystemDefaults (iConfigManager*)
{
  // First look in .cfg file
  csConfigAccess cfg;
  cfg.AddConfig (&object_reg, "/config/system.cfg");

  // Now analyze command line
  iCommandLineParser* CommandLine = CS_QUERY_REGISTRY (&object_reg,
  	iCommandLineParser);
}

iConfigFile *csSystemDriver::OpenUserConfig(const char *ApplicationID,
  const char *Alias)
{
  // the default implementation does not make a difference between different
  // users. It always uses /config/user.cfg, with the application ID as prefix.
  return new csPrefixConfig("/config/user.cfg", VFS, ApplicationID, Alias);
}

void csSystemDriver::QueryOptions (iComponent *iObject)
{
  iCommandLineParser* CommandLine = CS_QUERY_REGISTRY (&object_reg,
  	iCommandLineParser);

  iConfig *Config = SCF_QUERY_INTERFACE (iObject, iConfig);
  if (Config)
  {
    int on = OptionList.Length ();
	int i;
    for (i = 0 ; ; i++)
    {
      csOptionDescription option;
      if (!Config->GetOptionDescription (i, &option))
        break;
      OptionList.Push (new csPluginOption (option.name, option.type, option.id,
        (option.type == CSVAR_BOOL) || (option.type == CSVAR_CMD), Config));
      if (option.type == CSVAR_BOOL)
      {
        char buf[100];
        strcpy (buf, "no");
        strcpy (buf + 2, option.name);
        OptionList.Push (new csPluginOption (buf, option.type, option.id,
          false, Config));
      }
    } /* endfor */

    // Parse the command line for plugin options and pass them to plugin
    for (; on < OptionList.Length (); on++)
    {
      csPluginOption *pio = (csPluginOption *)OptionList.Get (on);
      const char *val;
      if ((val = CommandLine->GetOption (pio->Name)))
      {
        csVariant optval;
        switch (pio->Type)
        {
          case CSVAR_CMD:
	    optval.SetCommand ();
	    break;
          case CSVAR_BOOL:
            optval.SetBool (pio->Value);
            break;
          case CSVAR_LONG:
            if (!val) continue;
            optval.SetLong (atol (val));
            break;
          case CSVAR_FLOAT:
            if (!val) continue;
            optval.SetFloat (atof (val));
            break;
	  case CSVAR_STRING:
	    if (!val) continue;
	    optval.SetString (val);
	    break;
        }
        pio->Config->SetOption (pio->ID, &optval);
      }
    }
    Config->DecRef ();
  }
}

void csSystemDriver::RequestPlugin (const char *iPluginName)
{
  // @@@ WARNING we have to query for the commandline by tag name
  // because SCF is not yet initialized at the point we come here.
  iCommandLineParser* CommandLine = (iCommandLineParser*)(
  	CS_QUERY_REGISTRY_TAG (&object_reg, "CommandLine"));
  CommandLine->AddOption ("plugin", iPluginName);
}

//-------------------------------- iSystem interface for csSystemDriver -----//

iBase *csSystemDriver::LoadPlugin (const char *iClassID, const char *iFuncID,
  const char *iInterface, int iVersion)
{
  iComponent *p = SCF_CREATE_INSTANCE (iClassID, iComponent);
  if (!p)
    ReportSys (CS_REPORTER_SEVERITY_WARNING,
    	"WARNING: could not load plugin `%s'\n", iClassID);
  else
  {
    int index = Plugins.Push (new csPlugin (p, iClassID, iFuncID));
    if (p->Initialize (&object_reg))
    {
      iBase *ret;
      if (iInterface)
        ret = (iBase *)p->QueryInterface (
	  iSCF::SCF->GetInterfaceID (iInterface), iVersion);
      else
        (ret = p)->IncRef();
      if (ret)
      {
        QueryOptions (p);
        return ret;
      }
    }
    ReportSys (CS_REPORTER_SEVERITY_WARNING,
    	"WARNING: failed to initialize plugin `%s'\n", iClassID);
    Plugins.Delete (index);
  }
  return NULL;
}

bool csSystemDriver::RegisterPlugin (const char *iClassID,
  const char *iFuncID, iComponent *iObject)
{
  int index = Plugins.Push (new csPlugin (iObject, iClassID, iFuncID));
  if (iObject->Initialize (&object_reg))
  {
    QueryOptions (iObject);
    iObject->IncRef ();
    return true;
  }
  else
  {
    ReportSys (CS_REPORTER_SEVERITY_WARNING,
    	"WARNING: failed to initialize plugin `%s'\n", iClassID);
    Plugins.Delete (index);
    return false;
  }
}

int csSystemDriver::GetPluginCount ()
{
  return Plugins.Length ();
}

iBase* csSystemDriver::GetPlugin (int idx)
{
  csPlugin* pl = Plugins.Get (idx);
  return pl->Plugin;
}

iBase *csSystemDriver::QueryPlugin (const char *iInterface, int iVersion)
{
  scfInterfaceID ifID = iSCF::SCF->GetInterfaceID (iInterface);
  int i;
  for (i = 0; i < Plugins.Length (); i++)
  {
    iBase *ret =
      (iBase *)Plugins.Get (i)->Plugin->QueryInterface (ifID, iVersion);
    if (ret)
      return ret;
  }
  return NULL;
}

iBase *csSystemDriver::QueryPlugin (
  const char *iFuncID, const char *iInterface, int iVersion)
{
  int idx = Plugins.FindKey (iFuncID, 1);
  if (idx < 0)
    return NULL;

  return (iBase *)Plugins.Get (idx)->Plugin->QueryInterface (
    iSCF::SCF->GetInterfaceID (iInterface), iVersion);
}

iBase *csSystemDriver::QueryPlugin (const char* iClassID, const char *iFuncID, 
				    const char *iInterface, int iVersion)
{
  scfInterfaceID ifID = iSCF::SCF->GetInterfaceID (iInterface);
  int i;
  for (i = 0 ; i < Plugins.Length () ; i++)
  {
    csPlugin* pl = Plugins.Get (i);
    if (pl->ClassID && pl->FuncID)
      if (pl->ClassID == iClassID || !strcmp (pl->ClassID, iClassID))
      {
	if (pl->FuncID == iFuncID || !strcmp (pl->FuncID, iFuncID))
	{
	  return (iBase*)Plugins.Get(i)->Plugin->QueryInterface(ifID,iVersion);
	}
      }
  }
  return NULL;
}

bool csSystemDriver::UnloadPlugin (iComponent *iObject)
{
  int idx = Plugins.FindKey (iObject);
  if (idx < 0)
    return false;

  iConfig *config = SCF_QUERY_INTERFACE (iObject, iConfig);
  if (config)
  {
	int i;
    for (i = OptionList.Length () - 1; i >= 0; i--) 
    {
      csPluginOption *pio = (csPluginOption *)OptionList.Get (i);
      if (pio->Config == config)
        OptionList.Delete (i);
    }
    config->DecRef ();
  }

  csPlugin *p = Plugins.Get (idx);

#define CHECK(Var,Func)						\
  if (p->FuncID && !strcmp (p->FuncID, Func)) { Var->DecRef (); Var = NULL; }

  CHECK (VFS, CS_FUNCID_VFS)

#undef CHECK

  object_reg.Unregister ((iBase *)iObject, NULL);
  return Plugins.Delete (idx);
}

