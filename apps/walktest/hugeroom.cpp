/*
    Copyright (C) 1998 by Jorrit Tyberghein

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

#include "sysdef.h"
#include "walktest/hugeroom.h"
#include "walktest/walktest.h"
#include "csengine/sector.h"
#include "csengine/polygon.h"
#include "csengine/world.h"
#include "csengine/texture.h"
#include "csengine/light.h"
#include "csengine/thing.h"
#include "csobject/nameobj.h"

extern WalkTest* Sys;

extern float rand1 (float max);
extern float rand2 (float max);

static int pol_nr = 0;

HugeRoom::HugeRoom ()
{
  wall_dim = 100;
  wall_num_tris = 10;
  wall_min_red = .78;
  wall_min_green = .78;
  wall_min_blue = .78;
  wall_max_red = .82;
  wall_max_green = .82;
  wall_max_blue = .82;
  thing_max_x = 50;
  thing_max_y = 50;
  thing_max_z = 50;
  thing_min_poly = 10;
  thing_max_poly = 20;
  thing_cityblock_dim = 4;
  sector_min_thing = 5;
  sector_max_thing = 12;
  sector_min_thing_x = 30;
  sector_min_thing_y = 30;
  sector_min_thing_z = 30;
  sector_max_thing_x = 90;
  sector_max_thing_y = 90;
  sector_max_thing_z = 90;
  sector_min_lights = 2;
  sector_max_lights = 4;
  sector_light_max_pos = 90;
  sector_light_min_radius = 30;
  sector_light_max_radius = 40;
  sector_light_min_red = .4;
  sector_light_min_green = .4;
  sector_light_min_blue = .4;
  sector_light_max_red = 1;
  sector_light_max_green = 1;
  sector_light_max_blue = 1;
  seed = 1654594509;
}

void HugeRoom::create_wall (csSector* sector, csPolygonSet* thing,
	const csVector3& p1, const csVector3& p2, const csVector3& p3,
	const csVector3& p4, int hor_res, int ver_res, int txt)
{
  int i, j;
  for (i = 0 ; i < hor_res ; i++)
    for (j = 0 ; j < ver_res ; j++)
    {
      csVector3 v12a = p1 + ((float)i/(float)hor_res) * (p2-p1);
      csVector3 v43a = p4 + ((float)i/(float)hor_res) * (p3-p4);
      csVector3 v12b = p1 + ((float)(i+1)/(float)hor_res) * (p2-p1);
      csVector3 v43b = p4 + ((float)(i+1)/(float)hor_res) * (p3-p4);
      csVector3 v1 = v12a + ((float)j/(float)ver_res) * (v43a-v12a);
      csVector3 v2 = v12b + ((float)j/(float)ver_res) * (v43b-v12b);
      csVector3 v3 = v12b + ((float)(j+1)/(float)ver_res) * (v43b-v12b);
      csVector3 v4 = v12a + ((float)(j+1)/(float)ver_res) * (v43a-v12a);
      create_polygon (sector, thing, v1, v2, v3, txt);
      create_polygon (sector, thing, v1, v3, v4, txt);
    }
}

csPolygon3D* HugeRoom::create_polygon (csSector* sector, csPolygonSet* thing,
	const csVector3& p1, const csVector3& p2, const csVector3& p3,
	int txt)
{
  csMatrix3 t_m;
  csVector3 t_v (0, 0, 0);

  csTextureHandle* tm = NULL;
  switch (txt)
  {
    case 0: tm = NULL; break;
    case 1: tm = world->GetTextures ()->GetTextureMM ("txt"); break;
    case 2: tm = world->GetTextures ()->GetTextureMM ("txt2"); break;
  }

  CHK (csPolygon3D* p = new csPolygon3D (tm));
  char polname[10];
  sprintf (polname, "p%d", pol_nr);
  csNameObject::AddName (*p, polname); 
  pol_nr++;
  p->SetSector (sector);
  p->SetParent (thing);
  thing->AddPolygon (p);

  p->AddVertex (p1);
  p->AddVertex (p2);
  p->AddVertex (p3);

  p->SetTextureType (POLYTXT_GOURAUD);
  csGouraudShaded* gs = p->GetGouraudInfo ();
  gs->Setup (p->GetVertices ().GetNumVertices ());
  gs->EnableGouraud (true);
  gs->SetUV (0, 0, 0);
  gs->SetUV (1, 1, 0);
  gs->SetUV (2, 0, 1);
  if (txt == 0)
    p->SetFlatColor (
    	rand1 (wall_max_red-wall_min_red)+wall_min_red,
    	rand1 (wall_max_green-wall_min_green)+wall_min_green,
    	rand1 (wall_max_blue-wall_min_blue)+wall_min_blue);

  p->SetTextureSpace (t_m, t_v);

  return p;
}

//#define ROOM_PURE_RANDOM
//#define ROOM_RANDOM_WALLS
#define ROOM_CITYBLOCKS

csThing* HugeRoom::create_thing (csSector* sector, const csVector3& pos)
{
  CHK (csThing* thing = new csThing ());
  csNameObject::AddName (*thing, "t"); 

#ifdef ROOM_CITYBLOCKS
  float y_low = -wall_dim;
  float y_high = y_low + rand1 (wall_dim) + 5;
  int txt = (rand () & 0x8) ? 1 : 2;
  csVector3 p1 (-thing_cityblock_dim/2,y_low,thing_cityblock_dim/2);
  csVector3 p2 (thing_cityblock_dim/2,y_low,thing_cityblock_dim/2);
  csVector3 p3 (thing_cityblock_dim/2,y_low,-thing_cityblock_dim/2);
  csVector3 p4 (-thing_cityblock_dim/2,y_low,-thing_cityblock_dim/2);
  csVector3 p5 (-thing_cityblock_dim/2,y_high,thing_cityblock_dim/2);
  csVector3 p6 (thing_cityblock_dim/2,y_high,thing_cityblock_dim/2);
  csVector3 p7 (thing_cityblock_dim/2,y_high,-thing_cityblock_dim/2);
  csVector3 p8 (-thing_cityblock_dim/2,y_high,-thing_cityblock_dim/2);
  create_wall (sector, thing, p5, p6, p7, p8, 3, 3, txt);	// Top
  create_wall (sector, thing, p8, p7, p3, p4, 3, 3, txt);	// Front
  create_wall (sector, thing, p7, p6, p2, p3, 3, 3, txt);	// Right
  create_wall (sector, thing, p5, p8, p4, p1, 3, 3, txt);	// Left
  create_wall (sector, thing, p6, p5, p1, p2, 3, 3, txt);	// Back
#endif
#ifdef ROOM_RANDOM_WALLS
  thing_min_poly = 3;
  thing_min_poly = 8;
  int num = ((rand () >> 3) % (thing_max_poly-thing_min_poly+1)) + thing_min_poly;
  int i;
  for (i = 0 ; i < num ; i++)
  {
    int txt = (rand () & 0x8) ? 1 : 2;
    csVector3 p1 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
    csVector3 p2 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
    csVector3 p3 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
    csVector3 p4 = p2 + (p1-p2) + (p3-p2);
    create_wall (sector, thing, p1, p2, p3, p4, 4, 4, txt);
  }
#endif
#ifdef ROOM_PURE_RANDOM
  int num = ((rand () >> 3) % (thing_max_poly-thing_min_poly+1)) + thing_min_poly;
  int i;
  csVector3 p1 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
  csVector3 p2 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
  csVector3 p3 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
  for (i = 0 ; i < num ; i++)
  {
    int txt = (rand () & 0x8) ? 1 : 2;
    create_polygon (sector, thing, p1, p2, p3, txt);
    create_polygon (sector, thing, p3, p2, p1, txt);
    p1 = p2;
    p2 = p3;
    p3 = csVector3 (rand2 (thing_max_x), rand2 (thing_max_y), rand2 (thing_max_z));
  }
#endif

  thing->SetSector (sector);
  sector->AddThing (thing);
  csReversibleTransform obj;
  obj.SetT2O (csMatrix3 ());
  obj.SetOrigin (pos);
  thing->SetTransform (obj);
  thing->Transform ();

  return thing;
}

csSector* HugeRoom::create_huge_world (csWorld* world)
{
  this->world = world;
  csSector* room = world->NewSector();
  csNameObject::AddName (*room, "sector");

  if (seed == 0) seed = rand ();
  srand (seed);
  Sys->Printf (MSG_INITIALIZATION, "Used seed %u.\n", seed);

  int i, num;

#ifdef ROOM_CITYBLOCKS
  float x, y;
  //float cnt = wall_dim/thing_cityblock_dim;
  //for (x = -cnt/2 ; x < cnt/2 ; x++)
    //for (y = -cnt/2 ; y < cnt/2 ; y++)
  for (x = -3 ; x < 3 ; x++)
    for (y = -3 ; y < 3 ; y++)
    {
      if ((rand () & 0xc) == 0)
        create_thing (room, csVector3 (x*thing_cityblock_dim, 0, y*thing_cityblock_dim));
    }
#else
  num = ((rand () >> 3) % (sector_max_thing-sector_min_thing+1)) + sector_min_thing;
  for (i = 0 ; i < num ; i++)
  {
    float x = rand1 (sector_max_thing_x-sector_min_thing_x+1)+sector_min_thing_x;
    if (rand () & 0x8) x = -x;
    float y = rand1 (sector_max_thing_y-sector_min_thing_y+1)+sector_min_thing_y;
    if (rand () & 0x8) y = -y;
    float z = rand1 (sector_max_thing_z-sector_min_thing_z+1)+sector_min_thing_z;
    if (rand () & 0x8) z = -z;
    create_thing (room, csVector3 (x, y, z));
  }
#endif

  num = ((rand () >> 3) % (sector_max_lights-sector_min_lights+1)) + sector_min_lights;
  for (i = 0 ; i < num ; i++)
  {
    CHK (csStatLight* light = new csStatLight (
    	rand2 (sector_light_max_pos), rand2 (sector_light_max_pos), rand2 (sector_light_max_pos),
    	sector_light_min_radius+rand1 (sector_light_max_radius-sector_light_min_radius+1),
    	rand1 (sector_light_max_red-sector_light_min_red)+sector_light_min_red,
    	rand1 (sector_light_max_green-sector_light_min_green)+sector_light_min_green,
    	rand1 (sector_light_max_blue-sector_light_min_blue)+sector_light_min_blue, false));
    room->AddLight (light);
  }

  CHK (csThing* floorthing = new csThing ());
  csNameObject::AddName (*floorthing, "floor"); 
  create_wall (room, floorthing, csVector3 (-3, -1, 3), csVector3 (3, -1, 3),
  	csVector3 (3, -1, -3), csVector3 (-3, -1, -3), 4, 4, 0);
  create_wall (room, floorthing, csVector3 (-3, -1, -3), csVector3 (3, -1, -3),
  	csVector3 (3, -1, 3), csVector3 (-3, -1, 3), 4, 4, 0);
  floorthing->SetSector (room);
  room->AddThing (floorthing);
  floorthing->Transform ();

  create_wall (room, room,
  	csVector3 (-wall_dim,wall_dim,wall_dim), csVector3 (wall_dim,wall_dim,wall_dim),
	csVector3 (wall_dim,-wall_dim,wall_dim), csVector3 (-wall_dim,-wall_dim,wall_dim),
	wall_num_tris, wall_num_tris, 0);
  create_wall (room, room,
  	csVector3 (wall_dim,wall_dim,-wall_dim), csVector3 (-wall_dim,wall_dim,-wall_dim),
	csVector3 (-wall_dim,-wall_dim,-wall_dim), csVector3 (wall_dim,-wall_dim,-wall_dim),
	wall_num_tris, wall_num_tris, 0);
  create_wall (room, room,
  	csVector3 (-wall_dim,wall_dim,-wall_dim), csVector3 (-wall_dim,wall_dim,wall_dim),
	csVector3 (-wall_dim,-wall_dim,wall_dim), csVector3 (-wall_dim,-wall_dim,-wall_dim),
	wall_num_tris, wall_num_tris, 0);
  create_wall (room, room,
  	csVector3 (wall_dim,wall_dim,wall_dim), csVector3 (wall_dim,wall_dim,-wall_dim),
	csVector3 (wall_dim,-wall_dim,-wall_dim), csVector3 (wall_dim,-wall_dim,wall_dim),
	wall_num_tris, wall_num_tris, 0);
  create_wall (room, room,
  	csVector3 (-wall_dim,-wall_dim,wall_dim), csVector3 (wall_dim,-wall_dim,wall_dim),
	csVector3 (wall_dim,-wall_dim,-wall_dim), csVector3 (-wall_dim,-wall_dim,-wall_dim),
	wall_num_tris, wall_num_tris, 0);
  create_wall (room, room,
  	csVector3 (-wall_dim,wall_dim,-wall_dim), csVector3 (wall_dim,wall_dim,-wall_dim),
	csVector3 (wall_dim,wall_dim,wall_dim), csVector3 (-wall_dim,wall_dim,wall_dim),
	wall_num_tris, wall_num_tris, 0);

  Sys->Printf (MSG_INITIALIZATION, "Number of polygons: %d\n", pol_nr);
  room->UseStaticTree (BSP_ALMOST_MINIMIZE_SPLITS, true);
  Sys->Printf (MSG_INITIALIZATION, "Number of polygons (after BSP): %d+%d=%d\n",
  	room->GetStaticThing ()->GetNumPolygons (), room->GetNumPolygons (),
	room->GetStaticThing ()->GetNumPolygons ()+room->GetNumPolygons ());

  return room;
}

