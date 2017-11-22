#ifndef MAPDATA_H
#define MAPDATA_H

#include "light.h"

// ----- Map data -----
typedef unsigned int frameid_t;
extern frameid_t frameid;

typedef struct mapsector_s mapsector_t;

// -- Map vertex --
// The map vertices store the x and y (which translates to horizontal and distance in 3D)
// as a speed booster, the projected x and distance are stored in each vertex from the frame currently being rendered
// so each vertex only has to be projected once, except in cases where the vertex is clipped to the view plane, in which case
// the projection values will not be stored.
typedef struct
{
   float x, y;

   frameid_t   frameid;
   float proj_x, distance, idistance;
   float t_x, t_y;
} mapvertex_t;



// -- Map lineside --
typedef struct
{
   float xoffset, yoffset;
   float xscale, yscale;

   mapsector_t *sector;
} mapside_t;



// -- Map line --
typedef struct
{
   mapvertex_t    *v1, *v2;
   float          length;

   mapside_t      *side[2];
   mapsector_t    *sector1, *sector2;
} mapline_t;



// -- Map sector --

// A planeslope equasion consists of a 2d vertex, 2d directional vector, and a 
// unit slope  (that is, z delta per map unit traveled along the directional 
// vector in 2d space), 
typedef struct pslope_s
{
   float xori, yori, zori;

   // Used for rendering against vertical walls.
   float xvec, yvec;
   float zdelta, slope;

   // Used for texturing. Normalized plane vector. The plane crosses through the
   // origin above.
   float px, py, pz;

   bool  isceiling;
} pslope_t;


// For my purposes now I'm going to assume that all sectors are closed and convex.
struct mapsector_s
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

#endif //MAPDATA_H
