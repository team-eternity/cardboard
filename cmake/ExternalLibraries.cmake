## Copyright (C) 2020 Alex Mayfield
## Copyright (C) 2020 Max Waine
##
## This software is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This software is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor,
## Boston, MA  02110-1301  USA
##

### SDL2 ###

if(WIN32 AND (NOT DEFINED SDL2_LIBRARY OR NOT DEFINED SDL2_INCLUDE_DIR))
   if(MSVC)
      file(DOWNLOAD
         "https://www.libsdl.org/release/SDL2-devel-2.0.14-VC.zip"
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
         EXPECTED_HASH SHA256=232071cf7d40546cde9daeddd0ec30e8a13254c3431be1f60e1cdab35a968824)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.14")
      set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(SDL2_LIBRARY "${SDL2_DIR}/lib/x64/SDL2.lib" CACHE FILEPATH "")
         set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x64/SDL2main.lib" CACHE FILEPATH "")
      else()
         set(SDL2_LIBRARY "${SDL2_DIR}/lib/x86/SDL2.lib" CACHE FILEPATH "")
         set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x86/SDL2main.lib" CACHE FILEPATH "")
      endif()
   else()
      file(DOWNLOAD
         "https://www.libsdl.org/release/SDL2-devel-2.0.14-mingw.tar.gz"
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
         EXPECTED_HASH SHA256=405eaff3eb18f2e08fe669ef9e63bc9a8710b7d343756f238619761e9b60407d)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.14/x86_64-w64-mingw32")
      else()
         set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.14/i686-w64-mingw32")
      endif()
      set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include/SDL2" CACHE PATH "")
      set(SDL2_LIBRARY "${SDL2_DIR}/lib/libSDL2.dll.a" CACHE FILEPATH "")
      set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/libSDL2main.a" CACHE FILEPATH "")
   endif()
endif()

find_package(SDL2)

if(SDL2_FOUND)
   # SDL2 target.
   add_library(SDL2::SDL2 UNKNOWN IMPORTED GLOBAL)
   set_target_properties(SDL2::SDL2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SDL2_LIBRARY}")

   if(SDL2MAIN_LIBRARY)
      # SDL2main target.
      if(MINGW)
         # Gross hack to get mingw32 first in the linker order.
         add_library(SDL2::_SDL2main_detail UNKNOWN IMPORTED GLOBAL)
         set_target_properties(SDL2::_SDL2main_detail PROPERTIES
            IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")

         # Ensure that SDL2main comes before SDL2 in the linker order.  CMake
         # isn't smart enough to keep proper ordering for indirect dependencies
         # so we have to spell it out here.
         target_link_libraries(SDL2::_SDL2main_detail INTERFACE SDL2::SDL2)

         add_library(SDL2::SDL2main INTERFACE IMPORTED GLOBAL)
         set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LIBNAME mingw32)
         target_link_libraries(SDL2::SDL2main INTERFACE SDL2::_SDL2main_detail)
      else()
         add_library(SDL2::SDL2main UNKNOWN IMPORTED GLOBAL)
         set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")
      endif()
   endif()
endif()

## EOF

