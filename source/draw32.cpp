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
// Purpose: 32-bit rendering functions
// Authors: Stephen McGranahan
//

#include <SDL.h>
#include <math.h>
#include "render.h"
#include "mapdata.h"


Uint32 getFogColor(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return (((r * level) << 8) & 0xff0000) |
          ((g * level) & 0xff00) |
          ((b * level) >> 8);
}

lightblend_t calcLight(float distance, float map, light_t light)
{
    lightblend_t result;

    float li, maxlight, dscale;
    Uint16 ulight, flight;
    float fog;

    // Turns out the lighting in doom is a simple linear equation
    // the minimum a light can scale on a wall is
    // 2x - 224, the maximum is 2x - 40
    // the amount added for scaling is distance * 2560 * 4
    // this is added to the value for minimum light and that is then clipped to the 
    // maximum light value and you have your result!

    maxlight = light.l_level * 2 - 40;

    if (maxlight > 256) // clip max light
        maxlight = 256;

    // calc the minimum light
    li = light.l_level * 2 - 224;

    if (maxlight < 0)
        ulight = 0;
    else
    {
        // calc the added amount
        dscale = (distance * 2560.0f * 4);

        // the increased light is added
        li += dscale;
        // and the result clipped
        li = li > maxlight ? maxlight : li < 0 ? 0 : li;

        // The scalars for cardboard are fixed point 1.6 numbers 
        ulight = abs((int)li & 0x1FF);
    }

    if (light.f_start)
    {
        fog = (distance - light.f_stop) / (light.f_start - light.f_stop);
        flight = fog > 1.0f ? 256 : fog < 0 ? 0 : (Uint16)(fog * 256);
        ulight = (int)(ulight * flight) >> 8;
        flight = 256 - flight;
    }
    else
        flight = 0;

    result.fogadd = getFogColor(flight, light.f_r, light.f_g, light.f_b);

    result.l_r = ((ulight * light.l_r) >> 8) + 1;
    result.l_g = ((ulight * light.l_g) >> 8) + 1;
    result.l_b = ((ulight * light.l_b) >> 8) + 1;

    return result;
}

slopelightblend_t calcSlopeLight(float distance, float map, light_t light)
{
    slopelightblend_t result;

    float li, maxlight, dscale;
    Uint16 ulight, flight;
    float fog;

    // Turns out the lighting in doom is a simple linear equation
    // the minimum a light can scale on a wall is
    // 2x - 224, the maximum is 2x - 40
    // the amount added for scaling is distance * 2560 * 4
    // this is added to the value for minimum light and that is then clipped to the 
    // maximum light value and you have your result!

    maxlight = light.l_level * 2 - 40;

    if (maxlight > 256) // clip max light
        maxlight = 256;

    // calc the minimum light
    li = light.l_level * 2 - 224;

    map = fabs(map);

    if (map < li)
        map = li;
    if (map > maxlight)
        map = maxlight;

    map = map < 0 ? 0 : map > 256 ? 256 : map;

    // TODO: FOG
    flight = 0;

    ulight = (Uint16)map;

    result.fogadd = getFogColor(flight, light.f_r, light.f_g, light.f_b);
    result.rf = (map * light.l_r) / 256;
    result.gf = (map * light.l_g) / 256;
    result.bf = (map * light.l_b) / 256;

    return result;
}

// -- Column drawers -- 
void drawColumn(rendercolumn_t column)
{
   Uint32 *source, *dest;
   int count;
   unsigned sstep = view.width, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.blend.l_r, g = column.blend.l_g, b = column.blend.l_b;
   Uint32 fogadd = column.blend.fogadd;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint32 *)column.tex) + column.texx;
   dest = ((Uint32 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
       Uint32 texl = source[(texy >> 16) & 0x3f];
       
       *dest = (((((texl & 0xFF) * b)
          | (((texl & 0xFF00) * g) & 0xFF0000)
          | ((texl * r) & 0xFF000000))) >> 8) + fogadd;

      dest += sstep;
      texy += ystep;
   }
}


void drawColumnChunk(rendercolumn_t *columns, void *destBuffer, int chunkWidth, int lowy, int highy, int startx)
{
   unsigned int sstep = view.width;

   for (int y = lowy; y < highy; ++y)
   {
      Uint32* dest = ((Uint32*)destBuffer) + (y * view.width) + startx;

      for (int i = 0; i < chunkWidth; ++i)
      {
         if (y < columns[i].y1 || y > columns[i].y2) goto skip;

         Uint32* source = (Uint32*)columns[i].tex;
         Uint16 r = columns[i].blend.l_r, g = columns[i].blend.l_g, b = columns[i].blend.l_b;
         Uint32 fogadd = columns[i].blend.fogadd;

         Uint32 texl = source[(columns[i].yfrac >> 16) & 0x3f];

         *dest = ((((texl & 0xFF) * b)
            | (((texl & 0xFF00) * g) & 0xFF0000)
            | ((texl * r) & 0xFF000000)) >> 8) + fogadd;

         columns[i].yfrac += columns[i].ystep;

      skip:
         dest++;
      }
   }
}


// -- Span drawing --
void drawSpan(renderspan_t span)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.blend.l_r, g = span.blend.l_g, b = span.blend.l_b;
   Uint32 fogadd = span.blend.fogadd;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint32 *source = (Uint32 *)span.tex;
   Uint32 *dest = (Uint32 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      Uint32 texl = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];

      *dest = (((((texl & 0xFF) * b)
         | (((texl & 0xFF00) * g) & 0xFF0000)
         | ((texl * r) & 0xFF000000))) >> 8) + fogadd;

      dest++;
      xf += xs;
      yf += ys;
   }
}

void drawSlopedSpan(rslopespan_t slopespan)
{
   float iu = slopespan.iufrac, iv = slopespan.ivfrac;
   float ius = slopespan.iustep, ivs = slopespan.ivstep;
   float id = slopespan.idfrac, ids = slopespan.idstep;

   int r, rs, g, gs, b, bs;

   int count;

   if((count = slopespan.x2 - slopespan.x1 + 1) < 0)
      return;

   r = slopespan.rfrac; rs = slopespan.rstep;
   g = slopespan.gfrac; gs = slopespan.gstep;
   b = slopespan.bfrac; bs = slopespan.bstep;

   Uint32 *src = (Uint32 *)slopespan.src;
   Uint32 *dest = (Uint32 *)slopespan.dest + (slopespan.y * view.width) + slopespan.x1;

#if 0
   // Perfect *slow* render
   while(count--)
   {
      float mul = 1.0f / id;

      int u = (int)(64.0f * iu * mul);
      int v = (int)(64.0f * iv * mul);
      unsigned texl = (v & 63) * 64 + (u & 63);

      *dest = ((((src[texl] & 0xFF) * (b >> 16))
         | (((src[texl] & 0xFF00) * (g >> 16)) & 0xFF0000)
         | ((src[texl] * (r >> 16)) & 0xFF000000))) >> 8;
      dest++;

      iu += ius;
      iv += ivs;
      id += ids;
      r += rs;
      g += gs;
      b += bs;
   }
#else
   #define SPANJUMP 16
   #define INTERPSTEP (1.0f / 16.0f)
   while(count >= SPANJUMP)
   {
      float ustart, uend;
      float vstart, vend;
      float mulstart, mulend;
      int ustep, vstep;
      int incount, ufrac, vfrac;

      mulstart = 65536.0f / id;
      id += ids * SPANJUMP;
      mulend = 65536.0f / id;

      ufrac = (int)(ustart = iu * mulstart);
      vfrac = (int)(vstart = iv * mulstart);
      iu += ius * SPANJUMP;
      iv += ivs * SPANJUMP;
      uend = iu * mulend;
      vend = iv * mulend;

      ustep = (int)((uend - ustart) * INTERPSTEP);
      vstep = (int)((vend - vstart) * INTERPSTEP);

      incount = SPANJUMP;
      while(incount--)
      {
         Uint32 texl = src[((vfrac >> 10) & 0xFC0) + ((ufrac >> 16) & 0x3F)];

         *dest = ((((texl & 0xFF) * (b >> 16))
            | (((texl & 0xFF00) * (g >> 16)) & 0xFF0000)
            | ((texl * (r >> 16)) & 0xFF000000))) >> 8;
         dest++;

         ufrac += ustep;
         vfrac += vstep;
         r += rs;
         g += gs;
         b += bs;
      }

      count -= SPANJUMP;
   }
   if(count > 0)
   {
      float ustart, uend;
      float vstart, vend;
      float mulstart, mulend;
      int ustep, vstep;
      int incount, ufrac, vfrac;

      mulstart = 65536.0f / id;
      id += ids * count;
      mulend = 65536.0f / id;

      ufrac = (int)(ustart = iu * mulstart);
      vfrac = (int)(vstart = iv * mulstart);
      iu += ius * count;
      iv += ivs * count;
      uend = iu * mulend;
      vend = iv * mulend;

      ustep = (int)((uend - ustart) / count);
      vstep = (int)((vend - vstart) / count);

      incount = count;
      while(incount--)
      {
         Uint32 texl = src[((vfrac >> 10) & 0xFC0) + ((ufrac >> 16) & 0x3F)];

         *dest = ((((texl & 0xFF) * (b >> 16))
            | (((texl & 0xFF00) * (g >> 16)) & 0xFF0000)
            | ((texl * (r >> 16)) & 0xFF000000))) >> 8;
         dest++;

         ufrac += ustep;
         vfrac += vstep;
         r += rs;
         g += gs;
         b += bs;
      }
   }
#endif
}
