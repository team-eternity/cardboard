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

#ifndef VISPLANE_H
#define VISPLANE_H

#include "render.h"

typedef struct visplane_s
{
   struct visplane_s *next, *child;

   float z;
   light_t light;

   pslope_t *slope;

   int x1, x2;
   int pad1, top[MAX_WIDTH], pad2, pad3, bot[MAX_WIDTH], pad4;
} visplane_t;



typedef struct sv_s
{
   // Magic vectors!
   vector3f  m, n, p;
   vector3f  a, b, c;

   float sinslope;
   float zat;

   float pscale;
   float vis;
   float plight;
   float shade;
} sv_t;


extern sv_t sv;




visplane_t *findVisplane(float z, light_t *light, pslope_t *slope);
visplane_t *checkVisplane(visplane_t *check, int x1, int x2);

void renderVisplanes();

void unlinkPlanes();
#endif
