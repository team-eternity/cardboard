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


// -- Fog and Light -- 
// These are stored in a single struct for easier handling.
struct light_t
{
   // -- Light --
   // The color multipliers work on fixed point scheme of sorts. The multipliers
   // are stored as 1.8 fixed point numbers (max should always be 256)
   // the actual pixel values are multiplied by this integer and then shifted back down
   // discarding the newly formed fractional bits and effectivly scaling the pixel value.

   // l_level controls the distance fading calculations and l_r, l_g, and l_b are the 
   // individual color multipliers. These values are multiplied by whatever brightness value
   // is calculated for a wall column or floor span and are then used as multipliers on the colors. 
   Uint16 l_level;
   Uint16 l_r, l_g, l_b;

   // -- Fog --
   // The fog system works much like the lighting system except the lighting system fades the colors
   // gradually to 0, the fog system fades colors gradually to another single color. The system
   // works on almost the exact same concept (pixel color values are multiplied by fixed point
   // multipliers and shifted) but additional color is then added (is effectivly 1 - scale)
   // thus facilitating the fade to a color.
   float f_start, f_stop;
   Uint8 f_r, f_g, f_b;
};

