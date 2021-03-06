// $Id: util.h 7 2006-05-09 07:04:57Z pberck $
//

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

#ifndef _util_h
#define _util_h

#include <stdexcept>

// ----------------------------------------------------------------------------

std::string secs_to_str(long);
std::string secs_to_str(long, long);
std::string to_str(int);
std::string to_str(int, int);
std::string to_str(int, int, char);
long now();
long utc();
std::string the_date_time();
std::string the_date_time(long);
std::string the_date_time_utc(long);
long parse_date_time( const std::string& );
void set_flag(unsigned long& var, unsigned long& f);
void clear_flag(unsigned long& var, unsigned long& f);
bool is_set(unsigned long& var, unsigned long& f);
void delay(long);
void udelay(long);
void arg_split(char*, char**);
inline std::string &replacein(std::string &s, const std::string &sub,
			      const std::string &other) {
  size_t b = 0;
  for (;;) {
    b = s.find(sub, b);
    if (b == s.npos) break;
    s.replace(b, sub.size(), other);
    b += other.size();
  }
  return s;
}

std::string askenv(const std::string& key) /*throw(std::invalid_argument)*/;

std::string itoa(int i,const char* form);

bool check_dir( const std::string& );
void Tokenize(const std::string& buffer, std::vector<std::string>& tokens,
              const char delimiter);
std::string status_to_str(int);

#endif

