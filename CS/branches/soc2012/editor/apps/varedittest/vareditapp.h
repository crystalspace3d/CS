/*
  Copyright (C) 2011 Christian Van Brussel, Eutyche Mukuama, Dodzi de Souza
      Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef GRAPH_BEHAVIOURAPP_H
#define GRAPH_BEHAVIOURAPP_H

#include "wx/app.h"
#include "wx/wx.h"
#include "vareditframe.h"
#include <stdarg.h>

#include "csutil/sysfunc.h"
#include "csutil/event.h"
#include "csutil/common_handlers.h"
#include "csutil/cfgfile.h"
#include "iutil/eventq.h"
#include "iutil/event.h"
#include "iutil/objreg.h"
#include "iengine/engine.h"
#include "iengine/camera.h"
#include "iengine/light.h"
#include "iengine/texture.h"
#include "iengine/mesh.h"
#include "csutil/event.h"
#include "cstool/initapp.h"

class VarEditTestApp : public wxApp
{
public:
  /**
   * Generates some test properties for GUI generation and sets up
   * the CS environment.
   */
  virtual bool OnInit();
  virtual int  OnExit();

private:
  iObjectRegistry*  object_reg;
  // csRef<iEngine>    engine;
   
};

#endif
