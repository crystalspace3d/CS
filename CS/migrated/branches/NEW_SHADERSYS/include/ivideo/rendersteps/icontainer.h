/*
    Crystal Space 3D engine
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

#ifndef __CS_IVIDEO_RENDERSTEPS_ICONTAINER_H__
#define __CS_IVIDEO_RENDERSTEPS_ICONTAINER_H__

#include "csutil/scf.h"

struct iRenderStep;

SCF_VERSION (iRenderStepContainer, 0, 0, 1);

/**
 * Container for render steps.
 * \todo Add more step management methods.
 */
struct iRenderStepContainer : public iBase
{
  virtual int AddStep (iRenderStep* step) = 0;
  virtual int GetStepCount () = 0;
};

#endif
