/*
    Copyright (C) 2010 by Jelle Hellemans

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

#ifndef __TERRAINED_H__
#define __TERRAINED_H__

#include "cstool/demoapplication.h"
#include "imesh/terrain2.h"
#include "ivaria/decal.h"

struct iTerrainModifier;

class TerrainEd : public CS::Utility::DemoApplication
{
private:
  csRef<iTerrainFactory> terrainFactory;
  csRef<iTerrainSystem> terrain;
  csRef<iTerrainModifier> modifier;
  float rectSize;
  float rectHeight;

  csTicks lastUpdate;
  csVector3 lastPosition;

  int mouse_x, mouse_y;

  csRefArray<iTerrainModifier> undoStack;

  // Decal textures
  csRef<iDecalManager> decalManager;
  csRef<iDecalTemplate> decalTemplate;
  iDecal* decal;

private:
  void UpdateModifier (bool checkPosition = false);
  void RemoveModifier ();

public:
  TerrainEd ();
  ~TerrainEd ();

  bool OnInitialize (int argc, char* argv[]);
  bool Application ();

  bool CreateRoom ();
    
  void Frame ();
  bool OnKeyboard (iEvent&);
  bool OnMouseClick (iEvent& ev);
  bool OnMouseDown (iEvent& ev);

  CS_EVENTHANDLER_PHASE_LOGIC("application.terrained")
};

#endif // __TERRAINED_H__
