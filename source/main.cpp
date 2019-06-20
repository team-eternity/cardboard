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

//
// Cardboard - an experiment in doom-style projection and texture mapping
//
#include <SDL.h>
#include "render.h"
#include "mapdata.h"

vidDriver *screen;

// MaxW: Win32 doesn't need SDL main.
// TODO: platform.h?
#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#undef main
#endif

int main (int argc, char **argv)
{
   // Initialize the SDL Library.
   int result;
   float fps[30], total;
   int   index = -1, i;

   SDL_setenv("SDL_VIDEO_WINDOW_POS", "center", true);
   SDL_setenv("SDL_VIDEO_CENTERED", "1", true);
   result = SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO);

   if(result == -1)
      return -1;

   screen = vidDriver::setVideoMode(800, 600, 32, 0);
   
   SDL_Event e;
   bool update = true, up = false, down = false;
   bool left = false, right = false, pgup = false, pgdn = false;
   bool deletekey = false, endkey = false;

   hackMapData();
   loadTextures();

   moveCamera(-192.0f);
   //rotateCamera(3.14f * 0.5f);
   //moveCamera(-64.0f);
   flyCamera(-39.0f);

   initRenderer();

   while(1)
   {
      static Uint32 intics = SDL_GetTicks(), movetics;

      if(SDL_PollEvent(&e))
      {
         // FIXME/TODO
         //if(e.type == SDL_ACTIVEEVENT && e.active.gain == 1)
         //   update = true;

         if(e.type == SDL_KEYDOWN)
         {
            switch(e.key.keysym.scancode)
            {
               case SDL_SCANCODE_UP:
                  up = true;
                  break;
               case SDL_SCANCODE_DOWN:
                  down = true;
                  break;
               case SDL_SCANCODE_LEFT:
                  left = true;
                  break;
               case SDL_SCANCODE_RIGHT:
                  right = true;
                  break;
               case SDL_SCANCODE_PAGEUP:
                  pgup = true;
                  break;
               case SDL_SCANCODE_PAGEDOWN:
                  pgdn = true;
                  break;
               case SDL_SCANCODE_DELETE:
                  deletekey = true;
                  break;
               case SDL_SCANCODE_END:
                  endkey = true;
                  break;

               case SDL_SCANCODE_ESCAPE:
                  return 0;
                  break;
            }
         }
         if(e.type == SDL_KEYUP)
         {
            switch(e.key.keysym.scancode)
            {
               case SDL_SCANCODE_UP:
                  up = false;
                  break;
               case SDL_SCANCODE_DOWN:
                  down = false;
                  break;
               case SDL_SCANCODE_LEFT:
                  left = false;
                  break;
               case SDL_SCANCODE_RIGHT:
                  right = false;
                  break;
               case SDL_SCANCODE_PAGEUP:
                  pgup = false;
                  break;
               case SDL_SCANCODE_PAGEDOWN:
                  pgdn = false;
                  break;
               case SDL_SCANCODE_DELETE:
                  deletekey = false;
                  break;
               case SDL_SCANCODE_END:
                  endkey = false;
                  break;
            }
         }

         if(e.type == SDL_QUIT)
            return 0;
      }

      movetics = SDL_GetTicks() - intics;
      intics = SDL_GetTicks();

      if(up)
      {
         moveCamera(0.2f * movetics);
         update = true;
      }
      if(down)
      {
         moveCamera(-0.2f * movetics);
         update = true;
      }
      if(left)
      {
         rotateCamera(-.002f * movetics);
         update = true;
      }
      if(right)
      {
         rotateCamera(.002f * movetics);
         update = true;
      }
      if(pgup)
      {
         flyCamera(0.1f * movetics);
         update = true;
      }
      if(pgdn)
      {
         flyCamera(-0.1f * movetics);
         update = true;
      }
      if(deletekey)
      {
         strafeCamera(-0.1f * movetics);
         update = true;
      }
      if(endkey)
      {
         strafeCamera(0.1f * movetics);
         update = true;
      }


      Uint32 ticks = SDL_GetTicks();
      renderScene();
      ticks = SDL_GetTicks() - ticks;

      if(index == -1)
      {
         for(index = 0; index < 30; index++)
            fps[index] = 1000.0f / (float)(ticks);
         index = 0;
      }
      else
      {
         fps[index ++] = 1000.0f / (float)(ticks);
         index %= 30;
      }

      if(frameid & 0x10)
      {
         char title[256];

         total = 0;
         for(i = 0; i < 30; i++)
            total += fps[i];

         if((int)(total / 30.0f) == 0)
            sprintf(title, "cardboard 0.0.2 by Stephen McGranahan (1000 < fps)");
         else
            sprintf(title, "cardboard 0.0.2 by Stephen McGranahan (%i fps)", (int)(total / 30.0f));

         vidDriver::updateWindowTitle(title);
      }

      SDL_Delay(1);
   }

   return 0;
}
