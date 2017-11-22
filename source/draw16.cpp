// -- 16-bit rendering functions --
#include <SDL.h>
#include <math.h>
#include "render.h"



Uint32 getFogColor16(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return ((r * level) & 0xF800) | (((g * level) >> 5) & 0x7C0) | (((b * level) >> 11) & 0x1F);
}


void calcLight16(float distance, float map, light_t *light, lighttype_e to)
{
   int maxlight, dscale;
   Uint16 ulight, flight;
   float fog, li;

   // Turns out the lighting in doom is a simple linear equation
   // the minimum a light can scale on a wall is
   // 2x - 224, the maximum is 2x - 40
   // the amount added for scaling is distance * 2560 * 4
   // this is added to the value for minimum light and that is then clipped to the 
   // maximum light value and you have your result!

   maxlight = light->l_level * 2 - 40;

   if(maxlight > 256) // clip max light
      maxlight = 256;

   // calc the minimum light
   li = light->l_level * 2 - 224;


   if(to == LT_SLOPE)
   {
      if(map < li)
         map = li;
      if(map > maxlight)
         map = maxlight;

      map = map < 0 ? 0 : map > 256 ? 256 : map;

      // TODO: FOG
      flight = 0;

      ulight = (Uint16)map;

      span.fogadd = getFogColor16(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 8;
      span.l_g = (ulight * light->l_g) >> 8;
      span.l_b = (ulight * light->l_b) >> 8;
      span.rf = (map * light->l_r) / 256;
      span.gf = (map * light->l_g) / 256;
      span.bf = (map * light->l_b) / 256;
      render->rslopespan = flight > 0 ? render->drawSlopedSpanFog : render->drawSlopedSpan;
      return;
   }

   if(maxlight < 0)
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

   if(light->f_start)
   {
      fog = (distance - light->f_stop) / (light->f_start - light->f_stop);
      flight = fog > 1.0f ? 256 : fog < 0 ? 0 : (Uint16)(fog * 256);
      ulight = (int)(ulight * flight) >> 8;
      flight = 256 - flight;
   }
   else
      flight = 0;

   if(to == LT_COLUMN)
   {
      column.fogadd = getFogColor16(flight, light->f_r, light->f_g, light->f_b);

      column.l_r = (ulight * light->l_r) >> 8;
      column.l_g = (ulight * light->l_g) >> 8;
      column.l_b = (ulight * light->l_b) >> 8;

      render->rcolumn = flight > 0 ? render->drawColumnFog : render->drawColumn;
   }
   else if(to == LT_SPAN)
   {
      span.fogadd = getFogColor16(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 8;
      span.l_g = (ulight * light->l_g) >> 8;
      span.l_b = (ulight * light->l_b) >> 8;

      render->rspan = flight > 0 ? render->drawSpanFog : render->drawSpan;
   }
}


// -- Column drawers -- 
void drawColumn16(void)
{
   Uint16 *source, *dest;
   int count;
   unsigned sstep = view.width, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint16 *)column.tex) + column.texx;
   dest = ((Uint16 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = (((source[(texy >> 16) & 0x3f] & 0x1F) * b)
         | (((source[(texy >> 16) & 0x3f] & 0x07C0) * g) & 0x7C000)
         | (((source[(texy >> 16) & 0x3f] & 0xF800) * r) & 0xF80000)) >> 8;

      dest += sstep;
      texy += ystep;
   }
}



void drawColumnFog16(void)
{
   register Uint16 *source, *dest;
   int count;
   unsigned sstep = view.width, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b, fogadd = (Uint16)column.fogadd;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint16 *)column.tex) + column.texx;
   dest = ((Uint16 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = ((((source[(texy >> 16) & 0x3f] & 0x1F) * b)
         | (((source[(texy >> 16) & 0x3f] & 0x07C0) * g) & 0x7C000)
         | (((source[(texy >> 16) & 0x3f] & 0xF800) * r) & 0xF80000)) >> 8) + fogadd;

      dest += sstep;
      texy += ystep;
   }
}



// -- Span drawing --
void drawSpan16(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint16 *source = (Uint16 *)span.tex;
   Uint16 *dest = (Uint16 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest = (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x1F) * b)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x07C0) * g) & 0x7C000)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xF800) * r) & 0xF80000)) >> 8;
      dest++;
      xf += xs;
      yf += ys;
   }
}

void drawSpanFog16(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b, fogadd = (Uint16)span.fogadd;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint16 *source = (Uint16 *)span.tex;
   Uint16 *dest = (Uint16 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest = ((((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x1F) * b)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x07C0) * g) & 0x7C000)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xF800) * r) & 0xF80000)) >> 8) + fogadd;
      dest++;
      xf += xs;
      yf += ys;
   }
}



void drawSlopedSpan16(void)
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

   Uint16 *src = (Uint16 *)slopespan.src;
   Uint16 *dest = (Uint16 *)slopespan.dest + (slopespan.y * view.width) + slopespan.x1;

#if 0
   // Perfect *slow* render
   while(count--)
   {
      float mul = 1.0f / id;

      int u = (int)(64.0f * iu * mul);
      int v = (int)(64.0f * iv * mul);
      Uint16 texl = source[(v & 63) * 64 + (u & 63)];

      *dest = (((texl & 0x1F) * (b>>16))
         | (((texl & 0x07C0) * (g>>16)) & 0x7C000)
         | (((texl & 0xF800) * (r>>16)) & 0xF80000)) >> 8;
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
         Uint16 texl = src[((vfrac >> 10) & 0xFC0) + ((ufrac >> 16) & 0x3F)];

         *dest = (((texl & 0x1F) * (b>>16))
            | (((texl & 0x07C0) * (g>>16)) & 0x7C000)
            | (((texl & 0xF800) * (r>>16)) & 0xF80000)) >> 8;
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

         *dest = (((texl & 0x1F) * (b>>16))
            | (((texl & 0x07C0) * (g>>16)) & 0x7C000)
            | (((texl & 0xF800) * (r>>16)) & 0xF80000)) >> 8;
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

void drawSlopedSpanFog16(void)
{
   /*unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b, fogadd = (Uint16)span.fogadd;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint16 *source = (Uint16 *)span.tex;
   Uint16 *dest = (Uint16 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest = ((((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x1F) * b)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x07C0) * g) & 0x7C000)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xF800) * r) & 0xF80000)) >> 8) + fogadd;
      dest++;
      xf += xs;
      yf += ys;
   }*/
}


void nop16(void)
{}

renderfunc_t render16 = {
getFogColor16,
calcLight16,
drawColumn16,
drawColumnFog16,
drawSpan16,
drawSpanFog16,
drawSlopedSpan16,
drawSlopedSpanFog16,
nop16,
nop16,
nop16};
