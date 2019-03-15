// $Id: qlog.cc 8 2007-02-23 09:25:00Z pberck $

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

/*! \class Logfile

  \brief Simple logging class.
  \author Peter Berck

*/

// ----------------------------------------------------------------------------
//  Includes.
// ----------------------------------------------------------------------------
//
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

// C includes
//
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>

// Local includes
//
#include "qlog.h"
#include "util.h"

// Import some stuff into our namespace.
//
using std::cout;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::ofstream;
using std::string;

// ----------------------------------------------------------------------------
// Code
//-----------------------------------------------------------------------------

pthread_mutex_t Logfile::mtx = PTHREAD_MUTEX_INITIALIZER;

/*!
  \brief Creates a logfile which prints to cout.
*/
Logfile::Logfile() {
  init();
}

Logfile::~Logfile() {
}

// time_format looks like: 20010401 13:45:36
// Thousands are automatically added.
//
void Logfile::init() {
  char      timestring[32];
  struct tm *t;
  
  time_format = std::string("%Y%m%d %H:%M:%S");
}

/*!
  \brief Append the string argument to the logfile.

  Writes the string to the logfile, prefixed by the current time,
  and (if set), the prefix string.
  
  \param string The string to write to the log file. Will be prepended
  by a timestamp.

  \todo Fix fixed length of maximum temporary buffer for strftime().
*/
void Logfile::append(std::string s) {
  char      timestring[32];
  timeval   tv;
  struct tm *t;
  std::ostringstream ostr;

  gettimeofday(&tv, 0);
  t = localtime(&tv.tv_sec);
  
  strftime(timestring, 32, time_format.c_str(),  t);

  // We only display with milli-seconds accuracy.
  //
  int msec = (tv.tv_usec + 500) / 1000;

  ostr << timestring << "." << std::setfill('0')
       << std::setw(3) << msec << ": ";
  ostr << prefix;
  ostr << s << endl; // endl slow but flushes.

  cout << ostr.str();
}

/*!
  \brief Print to cout only, no flush.
*/
void Logfile::log( std::string s ) {
  char      timestring[32];
  timeval   tv;
  struct tm *t;

  pthread_mutex_lock( &mtx );

  gettimeofday( &tv, 0 );
  t = localtime( &tv.tv_sec );
  
  strftime( timestring, 32, time_format.c_str(),  t );

  // We only display with centi-seconds accuracy.
  //
  int msec = tv.tv_usec / 10000;

  std::cout << timestring << "." << std::setfill( '0' )
	    << std::setw( 2 ) << msec << ": " << prefix;
  //std::cout << s << "\n";
  //std::cout << "\033[1;33m";//This is yellow\033[m" ;
  std::cout << s << std::endl;

  pthread_mutex_unlock( &mtx );
}
 void Logfile::log( std::string s, const std::string& esc_code  ) {
  char      timestring[32];
  timeval   tv;
  struct tm *t;

  pthread_mutex_lock( &mtx );

  gettimeofday( &tv, 0 );
  t = localtime( &tv.tv_sec );
  
  strftime( timestring, 32, time_format.c_str(),  t );

  // We only display with centi-seconds accuracy.
  //
  int msec = tv.tv_usec / 10000;

  std::cout << timestring << "." << std::setfill( '0' )
	    << std::setw( 2 ) << msec << ": " << prefix;
  //std::cout << s << "\n";
  //std::cout << "\033[1;33m";//This is yellow\033[m" ;
  std::cout << esc_code << s << "\033[m" << std::endl;

  pthread_mutex_unlock( &mtx );
}

/*!
  Log to syslog.
*/
void Logfile::log( std::string s, int sl ) {
  log( s );
  if ( sl != 0 ) {
    syslog( LOG_INFO, "%s", s.c_str() );
  }
}

/*!
  \brief Print to cout only, no flush.
*/
void Logfile::log_begin( std::string s ) {
  char      timestring[32];
  timeval   tv;
  struct tm *t;

  gettimeofday( &tv, 0 );
  t = localtime( &tv.tv_sec );
  
  strftime( timestring, 32, time_format.c_str(),  t );

  // We only display with centi-seconds accuracy.
  //
  int msec = tv.tv_usec / 10000;

  std::cout << timestring << "." << std::setfill( '0' )
	    << std::setw( 2 ) << msec << ": " << prefix;
  std::cout << s;
}
void Logfile::log_end( std::string s ) {
  std::cout << s << std::endl;
}


/*!
  \brief Sets the prefix string.

  Sets the string that will be printed after the timestamp,
  but before the log message.

  \param prefix The name of the new prefix string.
 */
void Logfile::set_prefix(std::string a_prefix) {
  prefix = a_prefix;
}

/*!
  \brief Gets the prefix string.

  Gets the string that will be printed after the timestamp,
  but before the log message.
 */
std::string Logfile::get_prefix() {
  return prefix ;
}

void Logfile::inc_prefix() {
  prefix += " ";
}

void Logfile::dec_prefix() {
  if (prefix.length() > 0) {
    prefix = prefix.substr(0, prefix.length()-1);
  }
}

void Logfile::lock() {
    pthread_mutex_lock( &mtx );
}

void Logfile::unlock() {
  pthread_mutex_unlock( &mtx );
}

// ----------------------------------------------------------------------------
