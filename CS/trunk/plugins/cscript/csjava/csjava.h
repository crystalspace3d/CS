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

#ifndef __CSJAVA_H__
#define __CSJAVA_H__

#include "ivaria/script.h"
#include "isys/plugin.h"
#include "isys/system.h"
#include "cssys/csinput.h"
#include "csutil/csvector.h"

class csJava : public iScript
{
public:
  csJava(iBase *iParent);
  virtual ~csJava();

  iSystem* Sys;
  int Mode;
  void* Storage;

  bool RunText(const char *Text);
  bool LoadModule(const char *Text);
	bool Store(const char* type, const char* name, void* data);
  void ShowError();
  void Print(bool Error, const char *msg);

  SCF_DECLARE_IBASE;

  struct eiPlugIn : public iPlugIn
  {
    SCF_DECLARE_EMBEDDED_IBASE(csJava);
    virtual bool Initialize (iSystem*) { return true; }
    virtual bool HandleEvent (iEvent&) { return false; }
  } scfiPlugIn;
};

extern csJava *thisclass;

#endif
