//
// Copyright(C) 2007-2017 Stephen McGranahan
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
// Purpose: [TODO: DESCRIBE MODULE]
// Authors: Stephen McGranahan
//

#include <SDL.h>
#include "mapdata.h"
#include "vectors.h"

Uint32   vertexcount = 6;
mapvertex_t vertexlist[] = {
{ -64,      64},    // 0
{  64,      64},    // 1
{  96,       0},    // 2
{  96,     -512},    // 3
{ -64,     -512},    // 4
{   0,      128}};


Uint32   sidecount = 6;
mapside_t sidelist[] = {
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)2}};


Uint32   linecount = 7;
mapline_t linelist[] = {
{&vertexlist[0], &vertexlist[1], 0.0f, {&sidelist[0], &sidelist[5]}, (mapsector_t *)1, (mapsector_t *)2},
{&vertexlist[1], &vertexlist[2], 0.0f, {&sidelist[1], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[2], &vertexlist[3], 0.0f, {&sidelist[2], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[3], &vertexlist[4], 0.0f, {&sidelist[3], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[4], &vertexlist[0], 0.0f, {&sidelist[4], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[0], &vertexlist[5], 0.0f, {&sidelist[5], NULL}, (mapsector_t *)2, NULL},
{&vertexlist[5], &vertexlist[1], 0.0f, {&sidelist[5], NULL}, (mapsector_t *)2, NULL},
};



Uint32   sectorcount = 2;

pslope_t slopes[] = {
{ -72.0f, 64.0f, -48.0f,   0.0f,             -1.0f,        -0.2f},
{ -72.0f, 64.0f,  64.0f,  -1.0f,              0.0f,        -0.2f}};

mapsector_t sectorlist[] = {
{-80.0f, 64.0f, 0, 0, 0, 0, {128, 256, 256, 256, 0, 0, 90, 0, 0}, 0, NULL, &slopes[0], &slopes[1]},
{-48.0f, 32.0f, 0, 0, 0, 0, {128, 256, 256, 256, 0, 0, 90, 0, 0}, 0, NULL, NULL, NULL}};



frameid_t    frameid = 0;


void nextFrameID(void)
{
   frameid++;

   if(!frameid)
   {
      frameid = 1;

      for(unsigned i = 0; i < vertexcount; i++)
         vertexlist[i].frameid = 0;
   }
}



float zPositionAt(pslope_t *slope, float x, float y)
{
   float dist;

   // Get the distance between the origin and the point
   dist  = (x - slope->xori) * slope->xvec;
   dist += (y - slope->yori) * slope->yvec;

   return slope->zori + (dist * slope->zdelta);
}



void makeSlopePlane(pslope_t *slope, bool isceiling)
{
   vector3f  v1, v2, v3, p;

   slope->isceiling = isceiling;

   v1.x = slope->xori;
   v1.y = slope->yori;
   v1.z = slope->zori;

   v2.x = v1.x;
   v2.y = v1.y + 10.0f;
   v2.z = zPositionAt(slope, v2.x, v2.y);

   v3.x = v1.x + 10.0f;
   v3.y = v1.y;
   v3.z = zPositionAt(slope, v3.x, v3.y);

   if(isceiling)
      p = vector3f::getCross(v1 - v3, v2 - v3);
   else
      p = vector3f::getCross(v1 - v2, v3 - v2);

   p.normalize();

   slope->px = p.x;
   slope->py = p.y;
   slope->pz = p.z;

   p.x = slope->xvec;
   p.y = slope->yvec;
   p.z = slope->zdelta;
   p.normalize();

   slope->slope = p.z;
}


void hackMapData(void)
{
   Uint32 i, p;

   for(i = 0; i < vertexcount; i++)
      vertexlist[i].frameid = 0;
   
   for(i = 0; i < sidecount; i++)
      sidelist[i].sector = sectorlist + ((int)sidelist[i].sector - 1);

   for(i = 0; i < linecount; i++)
   {
      vector2f v1, v2;

      linelist[i].sector1 = 
         linelist[i].sector1 != NULL 
         ? sectorlist + ((int)linelist[i].sector1 - 1)
         : NULL;

      linelist[i].sector2 = 
         linelist[i].sector2 != NULL
         ? sectorlist + ((int)linelist[i].sector2 - 1)
         : NULL;
      
      // Generate line length
      v1.x = linelist[i].v1->x;
      v1.y = linelist[i].v1->y;

      v2.x = linelist[i].v2->x;
      v2.y = linelist[i].v2->y;

      linelist[i].length = (v2 - v1).getLength();
   }

   for(i = 0; i < sectorcount; i++)
   {
      Uint32 count = 0;
      for(p = 0; p < linecount; p++)
      {
         if(linelist[p].sector1 == sectorlist + i ||
            linelist[p].sector2 == sectorlist + i)
            count++;
      }

      sectorlist[i].lines = (mapline_t **)malloc(sizeof(mapline_t **) * count);
      sectorlist[i].linecount = count;

      count = 0;
      for(p = 0; p < linecount; p++)
      {
         if(linelist[p].sector1 == sectorlist + i ||
            linelist[p].sector2 == sectorlist + i)
            sectorlist[i].lines[count++] = linelist + p;
      }

      if(sectorlist[i].cslope)
         makeSlopePlane(sectorlist[i].cslope, true);
      if(sectorlist[i].fslope)
         makeSlopePlane(sectorlist[i].fslope, false);
   }
}




