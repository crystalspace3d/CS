/*
    Crystal Space PicoGUI Server Plugin
    (C) 2003 Mat Sutcliffe <oktal@gmx.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
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
#include "pgserver.h"
#include "videodrv.h"
#include "inputdrv.h"
#include "fonteng.h"
#include "iutil/objreg.h"
#include "iutil/event.h"
#include "iutil/evdefs.h"
#include "iutil/eventq.h"
#include "ivideo/graph2d.h"
#include "ivideo/fontserv.h"

extern "C"
{
  #include <picogui/types.h>
  #include <pgserver/common.h>
  #include <pgserver/init.h>
  #include <pgserver/os.h>
  #include <pgserver/configfile.h>
}

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_IBASE (csPicoGUIServer)
  SCF_IMPLEMENTS_INTERFACE (iGUIServer)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iEventHandler)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csPicoGUIServer::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csPicoGUIServer::eiEventHandler)
  SCF_IMPLEMENTS_INTERFACE (iEventHandler)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_FACTORY (csPicoGUIServer)

csPicoGUIServer::csPicoGUIServer (iBase *parent)
{
  SCF_CONSTRUCT_IBASE (parent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiComponent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiEventHandler);
}

inline bool csPicoGUIServer::Initialize (iObjectRegistry *objreg)
{
  csRef<iEventQueue> evq = CS_QUERY_REGISTRY (objreg, iEventQueue);
  if (! evq) return false;

  csRef<iGraphics2D> g2d = CS_QUERY_REGISTRY (objreg, iGraphics2D);
  if (! g2d) return false;

  csRef<iFontServer> fsv = CS_QUERY_REGISTRY (objreg, iFontServer);
  if (! fsv) return false;

  if (! csPGVideoDriver::Construct (g2d)) return false;

  if (! csPGFontEngine::Construct (fsv)) return false;

  if (! csPGInputDriver::Construct (evq)) return false;

  //append_param_str("pgserver", "themes", " ", "lucid.th");

  g_error e = pgserver_init (PGINIT_NO_COMMANDLINE|PGINIT_NO_CONFIGFILE, 0, 0);
  if (iserror (e)) {
    os_show_error (e);
    return false;
  }

  evq->RegisterListener (& scfiEventHandler, CSMASK_Nothing);

  pgserver_mainloop_start ();
  return true;
}

csPicoGUIServer::~csPicoGUIServer ()
{
  pgserver_mainloop_stop ();
  pgserver_shutdown ();
}

bool csPicoGUIServer::HandleEvent (iEvent &ev)
{
  if (ev.Command.Code == cscmdPostProcess && pgserver_mainloop_is_running ())
  {
    pgserver_mainloop_iteration ();
    return true;
  }
  else
    return false;
}
