/*
    Copyright (C) 1998 by Jorrit Tyberghein
    Written by Brandon Ehle

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

#ifndef __CSPYTHON_H__
#define __CSPYTHON_H__

#include "ivaria/script.h"
#include "isys/plugin.h"
#include "isys/system.h"
#include "cssys/csinput.h"
#include "csutil/csvector.h"

class csPython : public iScript {
public:
  csPython(iBase *iParent);
  virtual ~csPython();

  static csPython* shared_instance;
  iSystem* Sys;
  int Mode;
  void* Storage;

  bool Initialize(iSystem* iSys);
  bool RunText(const char *Text);
  bool LoadModule(const char *Text);
	bool Store(const char* type, const char* name, void* data);
  void ShowError();
  void Print(bool Error, const char *msg);

  SCF_DECLARE_IBASE;

  // Implement iPlugIn interface.
  struct eiPlugIn : public iPlugIn
  {
    SCF_DECLARE_EMBEDDED_IBASE(csPython);
    virtual bool Initialize (iSystem* p) { return scfParent->Initialize(p); }
    virtual bool HandleEvent (iEvent&) { return false; }
  } scfiPlugIn;
};

void InitPytocs();
#endif





