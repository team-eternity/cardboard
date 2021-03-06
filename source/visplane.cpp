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
#include <math.h>
#include <limits.h>
#include "error.h"
#include "mapdata.h"
#include "visplane.h"
#include "video.h"
#include "render.h"
#include "vectors.h"




// -- Flats --
// Could also use a bucket array, but this is good enough for now.
const int maxVisplanes = 1024;
visplane_t visplanes[maxVisplanes];
int nextFreeVisplane = 0;


void clearPlane(visplane_t *plane)
{
   plane->x2 = -1;
   plane->x1 = MAX_WIDTH;

   plane->child = NULL;
}



void unlinkPlanes()
{
   nextFreeVisplane = 0;
}


visplane_t *findVisplane(float z, light_t light, pslope_t *slope)
{
   for(int i = 0; i < nextFreeVisplane; ++i)
   {
      auto rover = visplanes + i;

      if(fabs(rover->z - z) < 0.00001f && !memcmp(&light, &rover->light, sizeof(light_t)) && rover->slope == slope)
         return rover;
   }

   if (nextFreeVisplane == maxVisplanes)
   {
      fatalError::Throw("Ran out of free visplanes");
   }

   visplane_t *result = visplanes + nextFreeVisplane;
   nextFreeVisplane++;

   clearPlane(result);

   memcpy(&result->light, &light, sizeof(light_t));
   result->z = z;
   result->slope = slope;

   return result;
}


visplane_t *checkVisplane(visplane_t *check, int x1, int x2)
{
   visplane_t *ret;
   int openleft, openright;

   if(check->x1 > check->x2)
   {
      check->x1 = openleft = x1;
      check->x2 = openright = x2;

      ret = check;
   }
   else if(x1 > check->x2)
   {
      openleft = check->x2 + 1;
      openright = x2;

      check->x2 = x2;
      ret = check;
   }
   else if(x2 < check->x1)
   {
      openleft = x1;
      openright = check->x1 - 1;

      check->x1 = x1;
      ret = check;
   }
   else
   {
      // No such luck, so check/create a child
      if(check->child)
         return checkVisplane(check->child, x1, x2);

      if (nextFreeVisplane == maxVisplanes)
      {
         fatalError::Throw("Ran out of visplanes");
      }

      visplane_t *child = visplanes + nextFreeVisplane;
      nextFreeVisplane++;

      clearPlane(child);

      memcpy(&child->light, &check->light, sizeof(light_t));
      child->z = check->z;
      child->slope = check->slope;
      child->child = check->child;
      check->child = child;
      
      child->x1 = openleft = x1;
      child->x2 = openright = x2; 
   
      ret = child;
   }

   for(;openleft <= openright; openleft++)
      ret->top[openleft] = INT_MAX;

   return ret;
}

sv_t sv;

#define DOOMFLATORI
#define NUMCOLORMAPS 256

void renderSlopedSpan(int x1, int x2, int y, visplane_t *plane)
{
   pslope_t *pslope = plane->slope;
   int   rdelta, gdelta, bdelta;
   int   count = x2 - x1;
   float map;
   float base;

   vector3f s(x1 - view.xcenter, y - view.ycenter, view.xfoc);

   rslopespan_t slopespan;

   // Premultiply the u and v values by the texture width
   slopespan.iufrac = vector3f::getDot(s, sv.a) * 64.0f;
   slopespan.ivfrac = vector3f::getDot(s, sv.b) * 64.0f;
   slopespan.idfrac = vector3f::getDot(s, sv.c);

   slopespan.iustep = sv.a.x * 64.0f;
   slopespan.ivstep = sv.b.x * 64.0f;
   slopespan.idstep = sv.c.x;

   slopespan.src = texture->getBuffer();
   slopespan.dest = screen->getBuffer();

   slopespan.x1 = x1;
   slopespan.x2 = x2;
   slopespan.y = y;

   // Setup lighting
   base = 4*(plane->light.l_level) - 448;
   map = base - (NUMCOLORMAPS - (sv.shade - sv.plight * slopespan.idfrac));

   slopelightblend_t blend = calcSlopeLight(0.0f, map, plane->light);
   slopespan.rfrac = (int)(blend.rf * 65536.0f);
   slopespan.gfrac = (int)(blend.gf * 65536.0f);
   slopespan.bfrac = (int)(blend.bf * 65536.0f);

   if(count > 0)
   {
      float id = slopespan.idfrac + slopespan.idstep * (x2 - x1);

      map = base - (NUMCOLORMAPS - (sv.shade - sv.plight * id));

      blend = calcSlopeLight(0.0f, map, plane->light);
      rdelta = (int)(blend.rf * 65536.0f) - slopespan.rfrac;
      gdelta = (int)(blend.gf * 65536.0f) - slopespan.gfrac;
      bdelta = (int)(blend.bf * 65536.0f) - slopespan.bfrac;

      slopespan.rstep = rdelta / count;
      slopespan.gstep = gdelta / count;
      slopespan.bstep = bdelta / count;
   }
   else
      slopespan.rstep = slopespan.gstep = slopespan.bstep = 0;

   drawSlopedSpan(slopespan);
}





void renderSpan(int x1, int x2, int y, visplane_t *plane)
{
   float iscale, dy, xstep, ystep, realy, height;
   renderspan_t span;

   // offset the height based on the camera
   height = plane->z - camera.z;
   if(!height) return; // avoid divide by 0

   // if you recall from the equations with wall height to screen y projection
   //                       (height - camz) * yfoc
   //  screeny = ycenter - ------------------------
   //                               y

   // solving this equation for y you get
   //      (height - camz) * yfoc
   // y = ----------------------------
   //         ycenter - screeny

   // .01 is added to make sure that when screeny == ycenter there is no divide by 0
   if(view.ycenter == y)
      dy = 0.01f;
   else if(y < view.ycenter)
      dy = fabsf(view.ycenter - y) - 1.0f;
   else
      dy = fabsf(view.ycenter - y) + 1.0f;

   iscale = fabsf(height/dy);
   realy = iscale * view.yfoc;

   iscale *= view.focratio;

   // steps are calculated using trig and the slope ratio
   xstep = view.cos * iscale;
   ystep = -view.sin * iscale;

   span.blend = calcLight(1.0f / realy, 0, plane->light);

   // the texture coordinates are first calculated at the center of the screen and
   // then offsetted using the step values to x1.
   // these values are then converted to 16.16 fixed point for a major speed increase
   span.xfrac = (int)((camera.x + (view.sin * realy) + ((x1 - view.xcenter) * xstep)) * 65536.0f);
   span.yfrac = (int)((camera.y + (view.cos * realy) + ((x1 - view.xcenter) * ystep)) * 65536.0f);
   span.xstep = (int)(xstep * 65536.0f);
   span.ystep = (int)(ystep * 65536.0f);
   span.x1 = x1;
   span.x2 = x2;
   span.y = y;
   span.tex = texture->getBuffer();
   span.screen = screen->getBuffer();

   drawSpan(span);
}



int spanstart[MAX_HEIGHT];



void translateVector3f(vector3f &v)
{
   float tx, ty, tz;

   tx = v.x - camera.x;
   ty = camera.z - v.y;
   tz = v.z - camera.y;

   // Just like wall projection.
   v.x = (tx * view.cos) - (tz * view.sin);
   v.z = (tz * view.cos) + (tx * view.sin);
   v.y = ty;
}


void calcSlopeVectors(visplane_t *plane)
{
   int x, y;
   float ixscale, iyscale;

   x = (int)plane->slope->xori;
   y = (int)plane->slope->yori;

   // TODO: replace 64 with actual texture width.
   x -= x % 64;
   y -= y % 64;

   // Also todo: the slope texture vectors should really be stored in the
   // slope struct and precalculated once per frame.
   sv.p.x = (float)x;
   sv.p.z = (float)(y - 64);
   sv.p.y = zPositionAt(plane->slope, sv.p.x, sv.p.z);

#ifdef DOOMFLATORI
   // Should be texture width.. Also, todo: Rotation
   sv.m.x = sv.p.x - 64.0f;
   sv.m.z = sv.p.z;
   sv.m.y = zPositionAt(plane->slope, sv.m.x, sv.m.z);

   // Should be texture width.. Also, todo: Rotation
   sv.n.x = sv.p.x;
   sv.n.z = sv.p.z - 64.0f;
   sv.n.y = zPositionAt(plane->slope, sv.n.x, sv.n.z);
#else
   // Should be texture width.. Also, todo: Rotation
   sv.m.x = sv.p.x;
   sv.m.z = sv.p.z + 64.0f;
   sv.m.y = zPositionAt(plane->slope, sv.m.x, sv.m.z);

   // Should be texture width.. Also, todo: Rotation
   sv.n.x = sv.p.x + 64.0f;
   sv.n.z = sv.p.z;
   sv.n.y = zPositionAt(plane->slope, sv.n.x, sv.n.z);
#endif

   // SoM:
   // **WRONG** Project the vectors. **WRONG**
   // Translate the vectors.
   translateVector3f(sv.p);
   translateVector3f(sv.m);
   translateVector3f(sv.n);

   sv.m = sv.m - sv.p;
   sv.n = sv.n - sv.p;

   sv.a = vector3f::getCross(sv.p, sv.n);
   sv.b = vector3f::getCross(sv.p, sv.m);
   sv.c = vector3f::getCross(sv.m, sv.n);

   // This is helpful for removing some of the muls when calculating light.
   sv.a.x *= 0.5f;
   sv.a.y *= 0.5f / view.focratio;
   sv.a.z *= 0.5f;

   sv.b.x *= 0.5f;
   sv.b.y *= 0.5f / view.focratio;
   sv.b.z *= 0.5f;

   sv.c.x *= 0.5f;
   sv.c.y *= 0.5f / view.focratio;
   sv.c.z *= 0.5f;

   sv.zat = zPositionAt(plane->slope, camera.x, camera.y);

   // More help from randy. I was totally lost on this... 
   ixscale = 1.0f / 64.0f;
   iyscale = 1.0f / 64.0f;

   sv.plight = (sv.vis * ixscale * iyscale) / (sv.zat - camera.z);
   sv.shade = NUMCOLORMAPS * 2.0f - (plane->light.l_level + 16.0f) * NUMCOLORMAPS / 128.0f;
}


void renderVisplane(visplane_t *plane)
{
   int x, stop, t1, t2, b1, b2;
   void (*rspanfunc)(int, int, int, visplane_t*) = renderSpan;

   if(plane->slope)
   {
      calcSlopeVectors(plane);

      if(plane->slope->isceiling == true && sv.zat <= camera.z)
         return;

      if(plane->slope->isceiling == false && sv.zat >= camera.z)
         return;

      rspanfunc = renderSlopedSpan;
   }

   x = plane->x1;
   stop = plane->x2 + 1;

   plane->top[x - 1] = plane->top[stop] = INT_MAX;
   plane->bot[x - 1] = plane->bot[stop] = 0;

   for(;x <= stop; x++)
   {
      t1 = plane->top[x - 1];
      t2 = plane->top[x];
      b1 = plane->bot[x - 1];
      b2 = plane->bot[x];

      for(; t2 > t1 && t1 <= b1; t1++)
         rspanfunc(spanstart[t1], x - 1, t1, plane);
      for(; b2 < b1 && t1 <= b1; b1--)
         rspanfunc(spanstart[b1], x - 1, b1, plane);

      while(t2 < t1 && t2 <= b2)
         spanstart[t2++] = x;
      while(b2 > b1 && t2 <= b2)
         spanstart[b2--] = x;
   }

   if(plane->child)
      renderVisplane(plane->child);
}


void renderVisplanes()
{
   visplane_t *rover;

   for (int i = 0; i < nextFreeVisplane; ++i)
   {
      rover = visplanes + i;
      renderVisplane(rover);
   }
}
