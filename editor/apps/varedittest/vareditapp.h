/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Bârsan

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

#ifndef __VAREDIT_APP_H__
#define __VAREDIT_APP_H__

#include "wx/wx.h"
#include "iutil/objreg.h"

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
};

#endif // __VAREDIT_APP_H__
