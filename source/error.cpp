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

#include "error.h"

//
// Begin: basicError ------------------------------------------------------------------------
//
void basicError::Throw(const string str)
{
   basicError *e = new basicError();

   e->str = str;
   throw e;
}


void basicError::Throw(const char *fmt, ...)
{
   static char buf[10240];
   va_list     argptr;

   va_start (argptr,fmt);
   vsprintf (buf,fmt,argptr);
   va_end   (argptr);
   
   basicError *e = new basicError();
   e->str = buf;

   throw e;
}
//
// End: basicError --------------------------------------------------------------------------
//



//
// Begin: fatalError -------------------------------------------------------------------
//
void fatalError::Throw(const string str)
{
   fatalError *e = new fatalError();

   e->str = str;
   throw e;
}


void fatalError::Throw(const char *fmt, ...)
{
   static char buf[10240];
   va_list     argptr;

   va_start (argptr,fmt);
   vsprintf (buf,fmt,argptr);
   va_end   (argptr);
   
   fatalError *e = new fatalError();
   e->str = buf;

   throw e;
}
//
// End: fatalError ---------------------------------------------------------------------
//


//
// Begin: logStream --------------------------------------------------------------------------
//
fstream logStream::file;
string  logStream::logname;
bool    logStream::printlogs = false;


void logStream::log(const string str)
{
   if(file.is_open())
      file << str;
   if(printlogs)
      printf(str.c_str());
}


void logStream::log(const char *fmt, ...)
{
   static char buf[10240];
   va_list     argptr;

   va_start (argptr,fmt);
   vsprintf (buf,fmt,argptr);
   va_end   (argptr);

   if(file.is_open())
      file << string(buf);
   if(printlogs)
      printf(buf);
}


void logStream::enable(string filename, bool append, bool print)
{
   if(printlogs && !print)
      printf(":: End of print logStream ::\n");

   printlogs = print;

   if(file.is_open())
   {
      log("Warning: logStream file %s aborted because new logStream file %s was stared.\n", logname.c_str(), filename.c_str());
      disable();
   }

   if(!filename.length())
   {
      if(printlogs)
         printf("logStreamging enabled to the console window only.\n");

      return;
   }

   if(append)
      file.open(filename.c_str(), ios::out|ios::app);
   else
      file.open(filename.c_str(), ios::out);

   if(!file.is_open())
      log("Could not open logStream file file " + filename);

}


void logStream::disable()
{
   log("/n----------End of logStream----------/n");

   if(file.is_open())
      file.close();
   printlogs = false;
}
//
// End: logStream ----------------------------------------------------------------------------
//
