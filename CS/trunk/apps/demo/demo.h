/*
    Copyright (C) 2001 by Jorrit Tyberghein

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

#ifndef DEMO_H
#define DEMO_H

#include <stdarg.h>
#include "cssys/sysdriv.h"
#include "csgeom/math2d.h"
#include "csgeom/math3d.h"

struct iEngine;
struct iSector;
struct iView;
struct iImageLoader;
struct iLoaderPlugIn;
class DemoSequenceManager;

class Demo : public SysSystemDriver
{
  typedef SysSystemDriver superclass;

public:
  iEngine* engine;
  iSector* room;
  iView* view;
  DemoSequenceManager* seqmgr;

private:
  void LoadMaterial (const char* matname, const char* filename);
  void LoadFactory (const char* factname, const char* filename,
  	const char* classId, iLoaderPlugIn* plug);

public:
  Demo ();
  virtual ~Demo ();

  virtual bool Initialize (int argc, const char* const argv[],
    const char *iConfigName);
  virtual void NextFrame ();
  virtual bool HandleEvent (iEvent &Event);

  void SetupFactories ();
  void SetupMaterials ();
  void SetupSector ();
  void SetupObjects ();
};

#endif // DEMO_H
