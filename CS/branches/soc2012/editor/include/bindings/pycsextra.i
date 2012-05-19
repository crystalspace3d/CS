/*
    Copyright (C) 2006 Pablo Martin <caedesv@users.sourceforge.net>

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
/*
  SWIG interface for Crystal Space Python Csextra Plugin
*/

%module pycsextra
%import "bindings/cspace.i"

%{
#include "crystalspace.h"
#include "include/qtwin.h"

struct _csPyEventHandler;
%}

INTERFACE_PRE(iQtWindow);
%include "qtwin.h"
INTERFACE_POST(iQtWindow);

// ivideo/qtwin.h
%extend iQtWindow
{
  void SetParentAddress(long address)
  {
     self->SetParent(reinterpret_cast<QWidget*>(address));
  }

  long GetWindowAddress()
  {
     return reinterpret_cast<long>(self->GetWindow());
  }
}

