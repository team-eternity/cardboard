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

#pragma once

#include "light.h"

// ----- Map data -----
using frameid_t = unsigned int;
extern frameid_t frameid;

struct mapsector_t;

// -- Map vertex --
// The map vertices store the x and y (which translates to horizontal and distance in 3D)
// as a speed booster, the projected x and distance are stored in each vertex from the frame currently being rendered
// so each vertex only has to be projected once, except in cases where the vertex is clipped to the view plane, in which case
// the projection values will not be stored.
struct mapvertex_t
{
   float x, y;

   frameid_t   frameid;
   float proj_x, distance, idistance;
   float t_x, t_y;
};



// -- Map lineside --
struct mapside_t
{
   float xoffset, yoffset;
   float xscale, yscale;

   mapsector_t *sector;
};



// -- Map line --
struct mapline_t
{
   mapvertex_t    *v1, *v2;
   float          length;

   mapside_t      *side[2];
   mapsector_t    *sector1, *sector2;
};



// -- Map sector --

// A planeslope equasion consists of a 2d vertex, 2d directional vector, and a 
// unit slope  (that is, z delta per map unit traveled along the directional 
// vector in 2d space), 
struct pslope_t
{
   float xori, yori, zori;

   // Used for rendering against vertical walls.
   float xvec, yvec;
   float zdelta, slope;

   // Used for texturing. Normalized plane vector. The plane crosses through the
   // origin above.
   float px, py, pz;

   bool  isceiling;
};


// For my purposes now I'm going to assume that all sectors are closed and convex.
struct mapsector_t
{
   float floorz, ceilingz;
   float f_xoff, f_yoff;
   float c_xoff, c_yoff;

   light_t light;

   Uint32 linecount;
   mapline_t **lines;

   // For slopes, a bit of extra data is needed.
   pslope_t  *fslope, *cslope;
};


extern Uint32   vertexcount;
extern mapvertex_t vertexlist[];


extern Uint32   sidecount;
extern mapside_t sidelist[];


extern Uint32   linecount;
extern mapline_t linelist[];


extern Uint32   sectorcount;
extern mapsector_t sectorlist[];



float zPositionAt(pslope_t *slope, float x, float y);


void nextFrameID(void);
void hackMapData(void);

