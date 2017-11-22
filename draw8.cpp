// -- 8-bit rendering functions --
#include <SDL.h>
#include <math.h>
#include "render.h"



Uint32 getFogColor8(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return ((r * level) & 0xF800) | (((g * level) >> 5) & 0x7C0) | (((b * level) >> 11) & 0x1F);
}


void calcLight8(float distance, float map, light_t *light, lighttype_e to)
{
   int li, maxlight, dscale;
   Uint16 ulight, flight;
   float fog;

   // Turns out the lighting in doom is a simple linear equation
   // the minimum a light can scale on a wall is
   // 2x - 224, the maximum is 2x - 40
   // the amount added for scaling is distance * 2560 * 4
   // this is added to the value for minimum light and that is then clipped to the 
   // maximum light value and you have your result!

   maxlight = light->l_level * 2 - 40;

   if(maxlight < 0)
      ulight = 0;
   else
   {
      if(maxlight > 256) // clip max light
         maxlight = 256;

      // calc the minimum light
      li = light->l_level * 2 - 224;
      // calc the added amount
      dscale = (int)(distance * 2560.0f * 4);
      
      // the increased light is added
      li += dscale; 
      // and the result clipped
      li = li > maxlight ? maxlight : li < 0 ? 0 : li;

      // The scalars for cardboard are fixed point 1.6 numbers 
      ulight = abs(li & 0x1FF);
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
      column.fogadd = getFogColor8(flight, light->f_r, light->f_g, light->f_b);

      column.l_r = (ulight * light->l_r) >> 10;
      column.l_g = (ulight * light->l_g) >> 10;
      column.l_b = (ulight * light->l_b) >> 10;

      render->rcolumn = flight > 0 ? render->drawColumnFog : render->drawColumn;
   }
   else if(to == LT_SPAN)
   {
      span.fogadd = getFogColor8(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 10;
      span.l_g = (ulight * light->l_g) >> 10;
      span.l_b = (ulight * light->l_b) >> 10;

      render->rspan = flight > 0 ? render->drawSpanFog : render->drawSpan;
      render->rslopespan = flight > 0 ? render->drawSlopedSpanFog : render->drawSlopedSpan;
   }
}


// -- Column drawers -- 
void drawColumn8(void)
{
   Uint8 *source, *dest;
   int count;
   unsigned sstep = view.width, ystep = column.ystep, texy = column.yfrac;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint8 *)column.tex) + column.texx;
   dest = ((Uint8 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = source[(texy >> 16) & 0x3f];

      dest += sstep;
      texy += ystep;
   }
}



void drawColumnFog8(void)
{
   Uint8 *source, *dest;
   int count;
   unsigned sstep = view.width, ystep = column.ystep, texy = column.yfrac;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint8 *)column.tex) + column.texx;
   dest = ((Uint8 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = source[(texy >> 16) & 0x3f];

      dest += sstep;
      texy += ystep;
   }
}



// -- Span drawing --
void drawSpan8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}

void drawSpanFog8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}


void drawSlopedSpan8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}

void drawSlopedSpanFog8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * view.width) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}


void nop8(void)
{}

renderfunc_t render8 = {
getFogColor8,
calcLight8,
drawColumn8,
drawColumnFog8,
drawSpan8,
drawSpanFog8,
drawSlopedSpan8,
drawSlopedSpanFog8,
nop8,
nop8,
nop8};
