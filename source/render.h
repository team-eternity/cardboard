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

#ifndef RENDER_H
#define RENDER_H

#include "light.h"
#include "matrix.h"
#include "video.h"

#define MAX_WIDTH  1280
#define MAX_HEIGHT 1024

// -- Rendering --
typedef struct rendercolumn_s
{
   int x;
   int y1, y2;
   int yfrac, ystep;
   int texx;

   Uint16  l_r, l_g, l_b;
   Uint32  fogadd;

   void *tex, *screen;
} rendercolumn_t;



typedef struct renderspan_s
{
   int x1, x2, y;
   int xfrac, yfrac, xstep, ystep;

   Uint16 l_r, l_g, l_b;
   Uint32 fogadd;

   float rf, gf, bf;

   void *tex, *screen;
} renderspan_t;


typedef struct rslopespan_s
{
   int x1, x2, y;
   float iufrac, ivfrac, idfrac;
   float iustep, ivstep, idstep;

   int rfrac, gfrac, bfrac;
   int rstep, gstep, bstep;

   void *src, *dest;
} rslopespan_t;


extern rendercolumn_t column;
extern renderspan_t span;

// -- bit-specific functions --
typedef enum
{
   LT_COLUMN,
   LT_SPAN,
   LT_SLOPE,
} lighttype_e;

// TODO: Get rid of these
extern void (*rcolumn)(void);
extern void (*rspan)(void);
extern void (*rslopespan)(rslopespan_t);

Uint32 getFogColor(Uint16 level, Uint8 r, Uint8 g, Uint8 b);
void calcLight(float distance, float map, light_t* light, lighttype_e to);
void drawColumn(void);
void drawColumnFog(void);
void drawSpan(void);
void drawSpanFog(void);
void drawSlopedSpan(rslopespan_t slopespan);
void drawSlopedSpanFog(rslopespan_t slopespan);

// -- Camera --
typedef struct
{
   // rotmat is a matrix which is used to perform rotation operations on vertices
   matrix2d  rotmat;
   float     angle;
   float     x, y, z;
} camera_t;

extern camera_t camera;


typedef struct
{
   float xcenter, ycenter, xfoc, yfoc, focratio, fov;
   float sin, cos, tan;
   float leftangle, anglestep;
   int   width, height;
} viewport_t;

extern viewport_t view;

// Screen buffer pointer
extern   vidDriver *screen;
extern   vidDriver *texture;

// Much like in doom. The screen clipping array starts out open and closes up as walls
// are rendered.
extern float    cliptop[MAX_WIDTH], clipbot[MAX_WIDTH];

float safeCos(float ang);
void wrapAngle(float *ang);
void moveCamera(float speed);
void strafeCamera(float speed);
void rotateCamera(float deltaangle);
void flyCamera(float delta);
void renderScene(void);
void loadTextures(void);
void initRenderer(void);

#endif
