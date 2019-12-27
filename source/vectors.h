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

#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

const float pi = 3.14159265f;

class vector2f
{
   public:
   vector2f() {x = y = 0;}
   vector2f(float xval, float yval) : x(xval), y(yval) {}

   inline void normalize()
   {
      lastlen = sqrtf(x * x + y * y);

      if(!lastlen)
         return;

      x /= lastlen;
      y /= lastlen;
   }

   inline void scale(float amount)
   {
      x *= amount;
      y *= amount;
   }

   inline void divide(float amount)
   {
      x /= amount;
      y /= amount;
   }

   inline void add(vector2f &vec)
   {
      x += vec.x;
      y += vec.y;
   }

   inline void sub(vector2f &vec)
   {
      x -= vec.x;
      y -= vec.y;
   }


   float    getLength() {return (lastlen = sqrtf(x * x + y * y));}
   vector2f getNormal()
   {
      lastlen = sqrtf(x * x + y * y);
      if(!lastlen)
         return vector2f(0, 0);

      return vector2f(x / lastlen, y / lastlen);
   }

   vector2f operator- (const vector2f &other) const
   {
      return vector2f(x - other.x, y - other.y);
   }


   vector2f operator+ (const vector2f &other) const
   {
      return vector2f(x + other.x, y + other.y);
   }
   float x, y, lastlen;
};



class vector3f
{
   public:
   vector3f() {x = y = z = 0;}
   vector3f(float xval, float yval, float zval) {x = xval; y = yval; z = zval;}

   inline void normalize()
   {
      float dist = sqrtf(x * x + y * y + z * z);

      if(!dist)
         return;

      dist = 1.0f / dist;
      x *= dist;
      y *= dist;
      z *= dist;
   }

   inline float getLength()
   {
      return sqrtf(x * x + y * y + z * z);
   }

   inline vector3f getNormal()
   {
      float dist = getLength();
      if(!dist)
         return vector3f();

      dist = 1.0f / dist;

      return vector3f(x * dist, y * dist, z * dist);
   }

   static inline float getDot(const vector3f &v1, const vector3f &v2)
   {
      return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
   }

   static inline vector3f getCross(const vector3f &v1, const vector3f &v2)
   {
      return vector3f((v1.y * v2.z) - (v1.z * v2.y),
                      (v1.z * v2.x) - (v1.x * v2.z),
                      (v1.x * v2.y) - (v1.y * v2.x));
   }

   
   vector3f operator -(const vector3f &v2) const {return vector3f(x - v2.x, y - v2.y, z - v2.z);}
   vector3f operator +(const vector3f &v2) const {return vector3f(x + v2.x, y + v2.y, z + v2.z);}

   float x, y, z;
};



class point3f
{
   public:
   point3f() {x = y = z = 0.0f;}
   point3f(float xval, float yval, float zval) : x(xval), y(yval), z(zval) {}

   vector3f operator -(const point3f &p2) const {return vector3f(x - p2.x, y - p2.y, z - p2.z);}
   point3f operator +(const vector3f &v) const {return point3f(x + v.x, y + v.y, z + v.z);}

   float x, y, z;
};
#endif
