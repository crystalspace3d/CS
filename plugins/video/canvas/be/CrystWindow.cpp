/*
    Copyright (C) 1999,2000 by Eric Sunshine <sunshine@sunshineco.com>
  
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cssysdef.h"
#include "isys/event.h"
#include "ivideo/graph2d.h"
#include "isys/system.h"
#include "CrystWindow.h"

CrystView::CrystView(BRect frame, iObjectRegistry* objreg, BBitmap* ibmap) :
  BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), object_reg(objreg), bitmap(ibmap)
{
}

CrystView::~CrystView()
{
}

void CrystView::UserAction() const
{
  iSystem* sys = CS_GET_SYSTEM (object_reg);	//@@@
  sys->PerformExtension("UserAction", Looper()->CurrentMessage());
}

void CrystView::KeyDown(char const *bytes, int32 numBytes)
{
  UserAction();
}

void CrystView::KeyUp(char const *bytes, int32 numBytes)
{
  UserAction();
}

void CrystView::MouseMoved(BPoint, uint32 transit, BMessage const*)
{
  UserAction();
}

void CrystView::MouseDown(BPoint)
{
  UserAction();
  if (!IsFocus())
    MakeFocus();
}

void CrystView::MouseUp(BPoint)
{
  UserAction();
}

void CrystView::Draw(BRect r)
{
  DrawBitmap(bitmap, r, r);
}

CrystWindow::CrystWindow(BRect frame, char const* name, CrystView* v,
  iObjectRegistry* objreg, iGraphics2D* ig2d) :
  BDirectWindow(frame, name, B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
  view(v), object_reg(objreg), g2d(ig2d)
{
  g2d->IncRef();
  view->SetViewColor(0, 0, 0);
  AddChild(view);
  SetSizeLimits(40, 2000, 40, 2000);
}

CrystWindow::~CrystWindow()
{
  Hide();
  Flush();
  g2d->DecRef();
}

bool CrystWindow::QuitRequested()
{
  iSystem* sys = CS_GET_SYSTEM (objreg);	//@@@
  sys->PerformExtension("ContextClose", g2d);
  sys->PerformExtension("Quit");
  return false; // Allow Crystal Space to close window itself.
}
