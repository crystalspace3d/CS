/*
  Copyright (C) 2008 by Julian Mautner

  This application is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This application is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this application; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CSCLOUDRENDERER_PLUGIN_H__
#define __CSCLOUDRENDERER_PLUGIN_H__

#include "imesh/clouds.h"

//Cloud-Renderer class
class csCloudsRenderer : public scfImplementation<csCloudsRenderer, iCloudsRenderer>
{
private:


public:
	csCloudsRenderer(iBase* pParent);
	~csCloudsRenderer();

	virtual const bool Render(const i3DScalarField<double>& aaaMixingRatios /*, const csMatrix& mTransformation */);
};

#endif // __CSCLOUDRENDERER_PLUGIN_H__