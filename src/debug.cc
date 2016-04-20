// $Id: debug.cc 9 2007-02-23 09:26:02Z pberck $

/*****************************************************************************
 * Copyright 2005 Peter Berck                                                *
 *                                                                           *
 * This file is part of PETeR.                                          *
 *                                                                           *
 * PETeR is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * PETeR is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with PETeR; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ----------------------------------------------------------------------------
//  Includes.
// ----------------------------------------------------------------------------
//
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>

// C includes
//
#include <sys/time.h>
#include <unistd.h>

// Local includes
//
#include "debug.h"

// Import some stuff into our namespace.
//
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;

// ----------------------------------------------------------------------------
// Code
//-----------------------------------------------------------------------------

/*
  Foo!
*/
void DBGLOG(std::string s) {
  char      timestring[32];
  timeval   tv;
  struct tm *t;

  gettimeofday(&tv, 0);
  t = localtime(&tv.tv_sec);

  //time_format = std::string("%Y/%m/%d %H:%M:%S");
  strftime(timestring, 32, "%H:%M:%S",  t);

  std::cout << timestring << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << ": ";
  std::cout << s << std::endl;
}

/*!
  Q&D method to get a string rep. of an integer.
*/
string DBGto_str(int i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

// ----------------------------------------------------------------------------
