/*
    Copyright (C) 2003 by Jorrit Tyberghein
	      (C) 2003 by Frank Richter

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

#include "csutil/scf.h"
#include "iutil/comp.h"
#include "iutil/document.h"

struct iObjectRegistry;

class csBinaryDocumentSystem : public iDocumentSystem, public iComponent
{
public:
  SCF_DECLARE_IBASE;
  
  csBinaryDocumentSystem (iBase* parent = 0);
  virtual ~csBinaryDocumentSystem ();
	
  virtual bool Initialize (iObjectRegistry* objreg);

  csRef<iDocument> CreateDocument ();
};
