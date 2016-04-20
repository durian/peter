// $Id: otp.h 9 2007-02-23 09:26:02Z pberck $
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

#ifndef _otp_h
#define _otp_h

#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

// ----------------------------------------------------------------------------

int otp_e(std::string, long, std::string, std::string& );
int otp_d(std::string, long, std::string, std::string& );

int bin_e( std::string, std::string& );
void bin_e_int( int, char (&r)[2] );

int bin_d( std::string, std::string& );
int bin_d_int( char, char );

int get_keyfiles( std::string, std::vector<std::string>& );

void create_e_matrix( int (&m)[256][256] );
void create_d_matrix( int (&m)[256][256] );
int l_add(int, int);
int l_sub(int, int);

void otp_e_char(const char* ct, const char* otp, std::string&);
void otp_d_char(const char* ct, const char* otp, std::string&);

// ----------------------------------------------------------------------------

#endif
