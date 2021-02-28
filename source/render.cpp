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
// Purpose: Renderer
// Authors: Stephen McGranahan
//

#include <SDL.h>
#include "video.h"
#include "matrix.h"
#include "error.h"
#include "vectors.h"
#include "render.h"
#include "pixelmath.h"
#include "mapdata.h"
#include "visplane.h"
#include "vectors.h"


// -- Camera --

camera_t camera;
viewport_t view;


// -- Rendering -- 
// set of functions for bit-specific rendering operations
// renderfunc_t *render;

// Screen buffer pointer
vidDriver *texture = NULL;



// -- Clipping -- 
// Much like in doom. The screen clipping array starts out open and closes up as walls
// are rendered.
float    cliptop[MAX_WIDTH], clipbot[MAX_WIDTH];



void loadTextures(void)
{
   texture = vidDriver::loadBMPFile("../texture8.bmp");
   if(!texture)
      exit(0);

   texture->convertFormat(*screen);
}



float safeCos(float ang)
{
   // cosf spits out garbage at these angles
   if(ang == pi / 2 || ang == 3 * pi/2)
      return 0;
   else
      return cosf(ang);
}



void wrapAngle(float *ang)
{
   while(*ang > 2*pi)
      *ang -= 2*pi;
   while(*ang < 0)
      *ang += 2*pi;
}


void initRenderer(void)
{
   float fov = 90.0f, ratio, slopet;

   view.xcenter = screen->getWidth() / 2.0f;
   view.ycenter = screen->getHeight() / 2.0f;

   view.width = screen->getWidth();
   view.height = screen->getHeight();

   view.fov = fov;
   fov = fov * pi / 180.0f;
   view.tan = (float)tan(fov/2.0f);

   ratio = 1.6f / ((float)view.width / (float)view.height);
   view.xfoc = view.xcenter / view.tan;
   view.yfoc = view.xfoc * ratio;
   view.focratio = view.yfoc / view.xfoc;

   // Thanks to 'Randi' of Zdoom fame!
   slopet = tan((90.0f + view.fov / 2.0f) * pi / 180.0f);
   sv.vis = 8.0f * slopet * 16.0f * 320.0f / (float)view.width;
}



void setupFrame(void)
{
   camera.rotmat.setIdentity();
   camera.rotmat.rotateAngle(camera.angle);

   // keep the camera angle sane so the special cases for the trig functions can be easily
   // caught.
   wrapAngle(&camera.angle);
   view.cos = safeCos(camera.angle);
   // viewsin does not seem to suffer from the same problem.
   view.sin = sinf(camera.angle);

   view.anglestep = (view.fov * pi) / (180.0f * view.width);
   view.leftangle = camera.angle - view.xcenter * view.anglestep;


   // reset the clipping arrays
   for(int i = 0; i < MAX_WIDTH; i++)
   {
      cliptop[i] = 0.0f;
      clipbot[i] = view.height - 1;
   }

   auto bytesPerPixel = screen->getFormat().BytesPerPixel;
   switch(bytesPerPixel)
   {
      case 4:
         break;
      default:
         fatalError::Throw("%i bytes per pixel is unsupported. Currently only 32-bit color is supported.", bytesPerPixel);
         break;
   }


   unlinkPlanes();

   nextFrameID();
}



void moveCamera(float speed)
{
   camera.x += sinf(camera.angle) * speed;
   camera.y += cosf(camera.angle) * speed;
}


void strafeCamera(float speed)
{
   camera.x += cosf(camera.angle) * speed;
   camera.y -= sinf(camera.angle) * speed;
}



void rotateCamera(float deltaangle)
{
   camera.angle += deltaangle;
}


void flyCamera(float delta)
{
   camera.z += delta;
}


struct wall_t
{
   int   x1, x2;

   float top, high;
   float topstep, highstep;
   float tpeg, tpegstep;

   float low, bottom;
   float lowstep, bottomstep;
   float lpeg, lpegstep;

   float dist, diststep, length;
   float len, lenstep;
   float xoffset, yoffset;
   float xscale, yscale;

   mapsector_t *sector;

   bool twosided, upper, middle, lower;


   bool markfloor, markceiling;
   visplane_t *floorp, *ceilingp;
};



void renderWall1s(wall_t wall)
{
   float basescale, yscale, xscale;
   int t, b;
   int ctop, cbot;

   rendercolumn_t column;

   column.screen = (void *)screen->getBuffer();
   column.tex = (void *)texture->getBuffer();

   for(int i = wall.x1; i <= wall.x2; i++)
   {
      int m;

      if(cliptop[i] >= clipbot[i])
         goto skip;

      ctop = (int)cliptop[i];
      cbot = (int)clipbot[i];

      t = wall.top < ctop ? ctop : (int)wall.top;
      b = wall.bottom > cbot ? cbot : (int)wall.bottom;
      
      m = t - 1 < cbot ? t-1 : cbot;
      if(wall.markceiling && wall.ceilingp && m > ctop)
      {

         wall.ceilingp->top[i] = ctop;
         wall.ceilingp->bot[i] = m;
      }

      m = b + 1 > ctop ? b + 1 : ctop;
      if(wall.markfloor && wall.floorp && m < cbot)
      {
         wall.floorp->top[i] = m;
         wall.floorp->bot[i] = cbot;
      }
      
      // Close the column
      cliptop[i] = b;
      clipbot[i] = t;

      if(wall.middle)
      {
         // the actual scale is y / yfoc however, you have to divide 1 by dist (1/y)
         // get y anyway so the resulting equasion is
         //      1
         // ------------
         //   yfoc * y
         basescale = yscale = xscale = 1.0f / (wall.dist * view.yfoc);

         yscale *= wall.yscale;
         xscale *= wall.xscale;

         // Fixed numbers 16.16 format
         column.ystep = (int)(yscale * 65536.0);
         column.texx = ((int)((wall.len * xscale) + wall.xoffset) & 0x3f) * 64;
         column.x = i;

         column.blend = calcLight(wall.dist, 0, wall.sector->light);

         column.y1 = t;
         column.y2 = b;

         if(t <= b)
         {
            column.yfrac = (int)((((column.y1 - wall.tpeg + 1) * yscale) + wall.yoffset) * 65536.0);
            drawColumn(column);
         }
      }

      skip:

      wall.dist += wall.diststep;
      wall.len += wall.lenstep;
      wall.top += wall.topstep;
      wall.bottom += wall.bottomstep;
      wall.tpeg += wall.tpegstep;
   }
}


void renderWall2s(wall_t wall)
{
   float basescale, yscale, xscale;
   int h, l, t, b, m;
   int ctop, cbot;

   rendercolumn_t column;
   column.screen = (void *)screen->getBuffer();
   column.tex = (void *)texture->getBuffer();

   // fairly straight forward.
   for(int i = wall.x1; i <= wall.x2; i++)
   {
      if(cliptop[i] >= clipbot[i])
         goto skip;
      
      ctop = (int)cliptop[i];
      cbot = (int)clipbot[i];

      t = wall.top < ctop ? ctop : (int)wall.top;
      b = wall.bottom > cbot ? cbot : (int)wall.bottom;

      m = t - 1 < cbot ? t-1 : cbot;
      if(wall.markceiling && wall.ceilingp && m > ctop)
      {

         wall.ceilingp->top[i] = ctop;
         wall.ceilingp->bot[i] = m;
      }

      m = b + 1 > ctop ? b + 1 : ctop;
      if(wall.markfloor && wall.floorp && m < cbot)
      {
         wall.floorp->top[i] = m;
         wall.floorp->bot[i] = cbot;
      }
      
      if(wall.upper || wall.lower)
      {
         // the actual scale is y / yfov however, you have to divide 1 by dist (1/y)
         // get y anyway so the resulting equasion is
         //      1
         // ------------
         //   yfoc * y
         basescale = yscale = xscale = 1.0f / (wall.dist * view.yfoc);

         yscale *= wall.yscale;
         xscale *= wall.xscale;

         // Fixed numbers 16.16 format
         column.ystep = (int)(yscale * 65536.0);
         column.texx = ((int)((wall.len * xscale) + wall.xoffset) & 0x3f) * 64;
         column.x = i;

         column.blend = calcLight(wall.dist, 0, wall.sector->light);
      }

      if(wall.upper)
      {
         h = wall.high < ctop ? ctop : wall.high > cbot ? cbot : wall.high;

         column.y1 = t;
         column.y2 = h;
         
         if(t <= h)
         {
            column.yfrac = (int)((((column.y1 - wall.tpeg + 1) * yscale) + wall.yoffset) * 65536.0);
            drawColumn(column);
            cliptop[i] = h;
         }
         else
            cliptop[i] = t;
      }
      else
         cliptop[i] = t;

      if(wall.lower)
      {
         l = wall.low < ctop ? ctop : wall.low > cbot ? cbot : wall.low;

         column.y1 = l;
         column.y2 = b;

         if(l <= b)
         {
            column.yfrac = (int)((((column.y1 - wall.lpeg + 1) * yscale) + wall.yoffset) * 65536.0);
            drawColumn(column);
            clipbot[i] = l;
         }
         else
            clipbot[i] = b;
      }
      else
         clipbot[i] = b;

      skip:

      wall.dist += wall.diststep;
      wall.len += wall.lenstep;
      wall.high += wall.highstep;
      wall.low += wall.lowstep;
      wall.top += wall.topstep;
      wall.bottom += wall.bottomstep;
      wall.tpeg += wall.tpegstep;
      wall.lpeg += wall.lpegstep;
   }
}





void projectWall(mapline_t *line)
{
   float x1, x2;
   float i1, i2;
   float istep;
   vector2f v1, v2, toffset;
   mapside_t *side, *backside;
   mapsector_t *sector, *backsector;

   // Thses are the rounded xstart and xstop column values.
   int   floorx1, floorx2;

   // These are used for slope calculations
   mapvertex_t *mv1, *mv2;
   float       leftclip = 0.0f, rightclip = 0.0f;

   vector2f  t1, t2;

   if(line->v1->frameid == frameid)
   {
      t1.x = line->v1->t_x;
      t1.y = line->v1->t_y;
   }
   else
   {
      v1.x = line->v1->x;
      v1.y = line->v1->y;

      // Translate the vertices
      t1 = v1 - vector2f(camera.x, camera.y);
      // rotate
      t1 = camera.rotmat.execute(t1);
   }

   if(line->v2->frameid == frameid)
   {
      t2.x = line->v2->t_x;
      t2.y = line->v2->t_y;
   }
   else
   {
      v2.x = line->v2->x;
      v2.y = line->v2->y;

      t2 = v2 - vector2f(camera.x, camera.y);
      t2 = camera.rotmat.execute(t2);
   }

   toffset.x = toffset.y = 0;

   // simple rejection for lines entirely behind the view plane
   if(t1.y < 1.0f && t2.y < 1.0f)
      return;

   // projection:
   //                vertex.x * xfoc
   // x = xcenter + -----------------
   //                       y

   // clip to the front view plane
   if(t1.y < 1.0f)
   {
      float move;
      vector2f movevec;

      // the y value is behind the view plane so interpolate a new x and y value for v1
      // the first step is to determine exactly how much of the line needs to be clipped.
      move = (1.0f - t1.y) * ((t2.x - t1.x) / (t2.y - t1.y));

      // offset the vertex x based on length moved
      t1.x += move;

      // determine the texture offset needed based on the clip
      movevec.x = move;
      movevec.y = 1.0f - t1.y;
      leftclip = movevec.getLength();

      // finally, y and 1/y both equal 1
      i1 = t1.y = 1.0f;
      x1 = view.xcenter + (t1.x * i1 * view.xfoc);
   }
   else
   {
      if(line->v1->frameid == frameid)
      {
         i1 = line->v1->idistance;
         x1 = line->v1->proj_x;
      }
      else
      {
         line->v1->idistance = i1 = 1.0f / t1.y; // Calculate 1/y used for oh-so-much
         line->v1->proj_x = x1 = view.xcenter + (t1.x * i1 * view.xfoc);

         line->v1->t_x = t1.x;
         line->v1->t_y = t1.y;
         line->v1->frameid = frameid;
      }
   }


   // determine if y2 is behind the plane
   if(t2.y < 1.0f)
   {
      float move;
      vector2f movevec;

      // clipping y2 is much simpler because the interpolation is from x1
      // and can be stopped at any point... except for slopes... WAH WAH...
      move = (1.0f - t2.y) * ((t2.x - t1.x) / (t2.y - t1.y));

      t2.x += move;

      // determine the texture offset needed based on the clip
      movevec.x = move;
      movevec.y = 1.0f - t2.y;
      rightclip = movevec.getLength();

      i2 = t2.y = 1.0f;
      x2 = view.xcenter + (t2.x * i2 * view.xfoc);
   }
   else
   {
      if(line->v2->frameid == frameid)
      {
         i2 = line->v2->idistance;
         x2 = line->v2->proj_x;
      }
      else
      {
         line->v2->idistance = i2 = 1.0f / t2.y;
         x2 = line->v2->proj_x = view.xcenter + (t2.x * i2 * view.xfoc);
         line->v2->t_x = t2.x;
         line->v2->t_y = t2.y;
         line->v2->frameid = frameid;
      }
   }

   // back-face rejection
   if(x2 < x1)
   {
      if(!line->side[1])
         return;

      side = line->side[1];
      backside = line->side[0];

      sector = line->sector2;
      backsector = line->sector1;

      float temp = x2;
      x2 = x1;
      x1 = temp;

      temp = i2;
      i2 = i1;
      i1 = temp;

      // Swap leftclip and rightclip too
      temp = leftclip;
      leftclip = rightclip;
      rightclip = temp;

      mv1 = line->v2;
      mv2 = line->v1;
   }
   else
   {
      if(!line->side[0])
         return;

      side = line->side[0];
      backside = line->side[1];

      sector = line->sector1;
      backsector = line->sector2;

      mv1 = line->v1;
      mv2 = line->v2;
   }

   toffset.x += leftclip;
   toffset.x *= side->xscale;

   toffset.x += side->xoffset;
   toffset.y += side->yoffset;

   // SoM: Handle the case where a wall is only occupying a single post but 
   // still needs to be rendered to keep groups of single post walls from not
   // being rendered and causing slime trails.
   floorx1 = (float)floor(x1 + 0.999f);
   floorx2 = (float)floor(x2 - 0.001f);

   // off the screen rejection
   if(floorx2 < 0 || floorx1 >= view.width)
      return;

   // determine the amount the 1/y changes each pixel to the right
   // 1/y changes linearly across the line where as y itself does not.
   if(floorx2 > floorx1)
      istep = 1.0f / (floorx2 - floorx1);
   else
      istep = 1.0f;

   wall_t wall;
   wall.length = (t2 - t1).getLength();

   wall.dist = i1;
   wall.diststep = (i2 - i1) * istep;

   // Multiply these for the heights below
   i1 *= view.yfoc; i2 *= view.yfoc;

   // wall len is the length in linnear space increased with each pixel / y
   // the full equasion would be something like
   //             endx/y2 - startx/y1
   // lenstep = ----------------------
   //                 (x2 - x1)
   // but because startx is always 0 the equasion is simplified here
   wall.len = 0;
   wall.lenstep = wall.length * i2 * istep;

   // project the heights of the wall.
   //                       (height - camz) * yfov
   //  screeny = ycenter - ------------------------
   //                               y
   
   float top1, top2, high1, high2, low1, low2, bottom1, bottom2;

   // MaxW: These need initialising, otherwise low1 gets used before assigned a value
   // FIXME: Is this correct?
   top1 = top2 = high1 = high2 = low1 = low2 = bottom1 = bottom2 = 0.0f;

   if(!backsector)
   {
      float z1, z2, peg1, peg2;
      bool planeinsite;

      if(sector->cslope && camera.z < zPositionAt(sector->cslope, camera.x, camera.y))
         wall.markceiling = true;
      else if(!sector->cslope && camera.z < sector->ceilingz)
         wall.markceiling = true;
      else
         wall.markceiling = false;

      if(sector->fslope && camera.z > zPositionAt(sector->fslope, camera.x, camera.y))
         wall.markfloor = true;
      else if(!sector->fslope && camera.z > sector->floorz)
         wall.markfloor = true;
      else
         wall.markfloor = false;

      if(sector->cslope)
      {
         float zfrac;

         z1 = zPositionAt(sector->cslope, mv1->x, mv1->y);
         z2 = zPositionAt(sector->cslope, mv2->x, mv2->y);

         zfrac = (z2 - z1) / line->length;

         if(leftclip != 0)
            z1 += leftclip * zfrac;
         if(rightclip != 0)
            z2 -= rightclip * zfrac;
      }
      else
      {
         z1 = z2 = sector->ceilingz;
      }

      high1 = top1 = view.ycenter - ((z1 - camera.z) * i1);
      high2 = view.ycenter - ((z2 - camera.z) * i2);
      wall.tpeg = peg1 = view.ycenter - ((sector->ceilingz - camera.z) * i1);
      peg2 = view.ycenter - ((sector->ceilingz - camera.z) * i2);

      if(sector->fslope)
      {
         float zfrac;

         z1 = zPositionAt(sector->fslope, mv1->x, mv1->y);
         z2 = zPositionAt(sector->fslope, mv2->x, mv2->y);

         zfrac = (z2 - z1) / line->length;

         if(leftclip != 0)
            z1 += leftclip * zfrac;
         if(rightclip != 0)
            z2 -= rightclip * zfrac;
      }
      else
      {
         z1 = z2 = sector->floorz;
      }

      low1 = bottom1 = view.ycenter - ((z1 - camera.z) * i1) - 1;
      low2 = view.ycenter - ((z2 - camera.z) * i2) - 1;

      // calculate the step values.
      wall.highstep = wall.topstep = (high2 - high1) * istep;
      wall.lowstep = wall.bottomstep = (low2 - low1) * istep;
      wall.tpegstep = (peg2 - peg1) * istep;

      wall.upper = wall.lower = false;
      wall.middle = true;

      wall.twosided = false;
   }
   else
   {
      bool lightsame = memcmp(&sector->light, &backsector->light, sizeof(light_t)) ? false : true;
      bool planeinsite;

      float frontz1, frontz2, backz1, backz2, peg1, peg2;

      // Do the sector ceiling
      if(sector->cslope)
      {
         float zfrac;

         frontz1 = zPositionAt(sector->cslope, mv1->x, mv1->y);
         frontz2 = zPositionAt(sector->cslope, mv2->x, mv2->y);

         zfrac = (frontz2 - frontz1) / line->length;

         if(leftclip != 0)
            frontz1 += leftclip * zfrac;
         if(rightclip != 0)
            frontz2 -= rightclip * zfrac;

         planeinsite = camera.z < zPositionAt(sector->cslope, camera.x, camera.y);
      }
      else
      {
         frontz1 = frontz2 = sector->ceilingz;
         planeinsite = camera.z < sector->ceilingz;
      }

      if(backsector->cslope)
      {
         float zfrac;

         backz1 = zPositionAt(backsector->cslope, mv1->x, mv1->y);
         backz2 = zPositionAt(backsector->cslope, mv2->x, mv2->y);

         zfrac = (backz2 - backz1) / line->length;

         if(leftclip != 0)
            backz1 += leftclip * zfrac;
         if(rightclip != 0)
            backz2 -= rightclip * zfrac;
      }
      else
         backz1 = backz2 = backsector->ceilingz;


      // Mark ceiling now
      if((!lightsame || frontz1 != backz1 || frontz2 != backz2 || sector->cslope != backsector->cslope) && planeinsite)
         wall.markceiling = true;
      else
         wall.markceiling = false;

      if(frontz1 > backz1 || frontz2 > backz2)
      {
         top1 = view.ycenter - ((frontz1 - camera.z) * i1);
         top2 = view.ycenter - ((frontz2 - camera.z) * i2);
         wall.topstep = (top2 - top1) * istep;

         wall.tpeg = peg1 = view.ycenter - ((backsector->ceilingz - camera.z) * i1);
         peg2 = view.ycenter - ((backsector->ceilingz - camera.z) * i2);
         wall.tpegstep = (peg2 - peg1) * istep;

         high1 = view.ycenter - ((backz1 - camera.z) * i1);
         high2 = view.ycenter - ((backz2 - camera.z) * i2);
         wall.highstep = (high2 - high1) * istep;

         wall.upper = true;
      }
      else
      {
         top1 = view.ycenter - ((frontz1 - camera.z) * i1);
         top2 = view.ycenter - ((frontz2 - camera.z) * i2);
         wall.topstep = (top2 - top1) * istep;
         wall.highstep = 0;

         wall.upper = false;
      }


      
      // Now do the floor!!
      if(sector->fslope)
      {
         float zfrac;

         frontz1 = zPositionAt(sector->fslope, mv1->x, mv1->y);
         frontz2 = zPositionAt(sector->fslope, mv2->x, mv2->y);

         zfrac = (frontz2 - frontz1) / line->length;

         if(leftclip != 0)
            frontz1 += leftclip * zfrac;
         if(rightclip != 0)
            frontz2 -= rightclip * zfrac;

         planeinsite = camera.z > zPositionAt(sector->fslope, camera.x, camera.y);
      }
      else
      {
         frontz1 = frontz2 = sector->floorz;
         planeinsite = camera.z > sector->floorz;
      }

      if(backsector->fslope)
      {
         float zfrac;

         backz1 = zPositionAt(backsector->fslope, mv1->x, mv1->y);
         backz2 = zPositionAt(backsector->fslope, mv2->x, mv2->y);

         zfrac = (backz2 - backz1) / line->length;

         if(leftclip != 0)
            backz1 += leftclip * zfrac;
         if(rightclip != 0)
            backz2 -= rightclip * zfrac;
      }
      else
         backz1 = backz2 = backsector->floorz;


      // Mark the floor now
      if((!lightsame || frontz1 != backz1 || frontz2 != backz2 || sector->fslope != backsector->fslope) && planeinsite)
         wall.markfloor = true;
      else
         wall.markfloor = false;

      if(frontz1 < backz1 || frontz2 < backz2)
      {
         low1 = view.ycenter - ((backz1 - camera.z) * i1) - 1;
         low2 = view.ycenter - ((backz2 - camera.z) * i2) - 1;
         wall.lowstep = (low2 - low1) * istep;

         wall.lpeg = peg1 = view.ycenter - ((backsector->floorz - camera.z) * i1);
         peg2 = view.ycenter - ((backsector->floorz - camera.z) * i2);
         wall.lpegstep = (peg2 - peg1) * istep;

         bottom1 = view.ycenter - ((frontz1 - camera.z) * i1) - 2;
         bottom2 = view.ycenter - ((frontz2 - camera.z) * i2) - 2;
         wall.bottomstep = (bottom2 - bottom1) * istep;

         wall.lower = true;
      }
      else
      {
         bottom1 = view.ycenter - ((frontz1 - camera.z) * i1) - 1;
         bottom2 = view.ycenter - ((frontz2 - camera.z) * i2) - 1;
         wall.bottomstep = (bottom2 - bottom1) * istep;
         wall.lowstep = 0;
         wall.lower = false;
      }

      wall.middle = false;
      wall.twosided = true;
   }


   // clip to screen
   if(x1 < 0.0f)
   {
      // when clipping x1 all the values that are increased per-pixel need to be increased
      // the amount x1 is jumping which would be newx1 - x1 but because newx1 is 0, the equasion
      // is simplified to -x1
      wall.len += wall.lenstep * -x1;
      wall.dist += wall.diststep * -x1;

      high1 += wall.highstep * -x1;
      low1 += wall.lowstep * -x1;
      top1 += wall.topstep * -x1;
      bottom1 += wall.bottomstep * -x1;

      wall.tpeg += wall.tpegstep * -x1;
      wall.lpeg += wall.lpegstep * -x1;

      x1 = 0.0f;
      floorx1 = 0;
   }

   if(x2 >= view.width)
   {
      float clip = x2 - view.width;

      x2 = view.width - 1;
      floorx2 = view.width - 1;
   }


   if(wall.markfloor)
      wall.floorp = checkVisplane(findVisplane(sector->floorz, sector->light, sector->fslope), floorx1, floorx2);
   else
      wall.floorp = NULL;

   if(wall.markceiling)
      wall.ceilingp = checkVisplane(findVisplane(sector->ceilingz, sector->light, sector->cslope), floorx1, floorx2);
   else
      wall.ceilingp = NULL;
 

   // Round the pixel values as a last step;
   wall.x1 = floorx1;
   wall.x2 = floorx2;
   wall.sector = sector;

   wall.high = high1;
   wall.low = low1;
   wall.top = top1;
   wall.bottom = bottom1;

   wall.xoffset = toffset.x;
   wall.yoffset = toffset.y;

   wall.xscale = side->xscale;
   wall.yscale = side->yscale;

   if(!backsector)
      renderWall1s(wall);
   else
      renderWall2s(wall);
}






void renderScene(void)
{
   unsigned i;

   screen->lock();
   texture->lock();

   //screen->clear();

   setupFrame();

   for(i = 0; i < linecount; i++)
      projectWall(&linelist[i]);

   renderVisplanes();

   screen->unlock();
   texture->unlock();

   screen->flipVideoPage();
}
