// $Id: otp.cc 9 2007-02-23 09:26:02Z pberck $
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

#include <dirent.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "otp.h"

// ----------------------------------------------------------------------------

/*
  Encrypt.
*/
int otp_e(std::string kf, long of, std::string pt, std::string& cy ) {

  // Check if keyfile exists, if so, open it and read size.
  //
  std::ifstream file( kf.c_str(), std::ios::ate );
  if ( ! file ) {
    return 1;// no keyfile.
  }
  long endpos = file.tellg(); 
  file.seekg( of, std::ios::beg );

  // Read plaintext length number of bytes from keyfile.
  //
  std::string::size_type len = pt.size();
  char *buffer = new char[len];
  file.read( buffer, len );
  if ( file.eof() ) {
    file.close();
    //std::cerr << "No data in keyfile (" << len << " needed).\n";
    return 2; // no data.
  }
  file.close();

  otp_e_char( pt.c_str(), buffer, cy );

  return 0;
}

int otp_d(std::string kf, long of, std::string cy, std::string& pt ) {

  std::ifstream file( kf.c_str() );
  if ( ! file ) {
    return 1;// no keyfile.
  }
  file.seekg( of, std::ios::beg );

  std::string::size_type len = cy.size();

  char *buffer = new char[len];
  file.read( buffer, len );
  file.close();

  buffer[len] = '\0';

  otp_d_char( cy.c_str(), buffer, pt );

  return 0;
}

/*
  Simple binary encoding - eight bits into two times four, 64 added
  to make printable ascii. Easier on the XML transfer, no need for
  ugly [CDATA[...]] contents.
*/
int bin_e( std::string src, std::string& res ) {
  int len = src.size();
  char *encoded = new char[len*2];
  char tmp[2];
  for ( int i = 0; i < len; i++ ) {
    bin_e_int( src[i], tmp );
    encoded[i*2]     = tmp[0];
    encoded[(i*2)+1] = tmp[1];
    /*
    unsigned char c  = src[i];
    unsigned char cl = (c>>4) & 15 ;
    unsigned char cr = c & 15;
    encoded[i*2]     = cl + 64;
    encoded[(i*2)+1] = cr + 64;
    */
  }
  encoded[len*2] = '\0';
  res = std::string( encoded );
  return 0;
}
void bin_e_int( int i, char (&r)[2] ) {
  char cl = (i>>4) & 15 ;
  char cr = i & 15;
  r[0]    = cl + 64;
  r[1]    = cr + 64;
}

int bin_d( std::string src, std::string& res ) {
  int len = src.size();
  char *decoded = new char[(len/2)+1];
  
  int j = 0;
  for ( int i = 0; i < len; i += 2 ) {
    unsigned char cl = src[i]   - 64;
    unsigned char cr = src[i+1] - 64;
    unsigned char c = (cl<<4) + cr;
    decoded[j++] = c;
  }  
  decoded[j] = '\0';

  res = std::string( decoded );
  return 0;
}
int bin_d_int( char c1, char c2 ) {
    int cl = c1 - 64;
    int cr = c2 - 64;
    int c = (cl<<4) + cr;
    return c;
}

int get_keyfiles(std::string keydirname, std::vector<std::string>& keyfiles) {

  // Read dir and store files.
  //
  DIR *dirp;
  struct dirent *direntp;
  dirp = opendir( keydirname.c_str() );
  while ( (direntp = readdir( dirp )) != NULL ) {
    // add to keyfiles. Maybe grep on extension
    if ( DT_REG == direntp->d_type ) {
      //(void)printf( "%s\n", direntp->d_name );
      keyfiles.push_back( std::string( direntp->d_name ) );
    }
  }
  (void)closedir( dirp );

  return keyfiles.size();
}

// Precalculated encryption matrix.
//
void create_e_matrix( int (&m)[256][256] ) {
  for ( int i = 0; i < 256; i++ ) {
    for ( int j = 0; j < 256; j++ ) {
      m[i][j] = l_add(i, j);//(i + j) % 256;
    }//j
  }//i
}

// Precalculated decryption matrix.
//
void create_d_matrix( int (&m)[256][256] ) {
  for ( int i = 0; i < 256; i++ ) {
    for ( int j = 0; j < 256; j++ ) {
      m[i][j] = l_sub(i, j);
      /*i - j;
      if ( m[i][j] < 0 ) {
	m[i][j] + 256;
	}*/
    }//j
  }//i
}

int l_add(int i, int j) {
  int r = (i + j) % 256;
  //int r = ( (i + j) % 96 ) + 32; // Space too small.
  return r;
}
int l_sub(int i, int j) {
  int r = i - j;
  if ( r < 0 ) {
    r += 256;
  }
  /*
  int r = i - j;
  if ( r < 96 ) {
    r += 96;
  }
  r -= 32;
  */
  return r;
}

// ASCII encrypt - returns two printable ASCII chars for
// each excrypted byte.
//
void otp_e_char(const char* pt, const char* otp, std::string& res) {
  int m[256][256];
  create_e_matrix( m );
  char tmp[2];
  
  int i = 0;
  while ( pt[i] ) {
    int c = m[pt[i]][otp[i]];
    bin_e_int( c, tmp );
    res += tmp[0];
    res += tmp[1];
    ++i;
  }
}

// ASCII decrypt - decodes two printable chars to one byte, decrypts
// them.
//
void otp_d_char(const char* ct, const char* otp, std::string& res) {
  int m[256][256];
  create_d_matrix( m );
  
  int i = 0;
  int j = 0;
  while ( ct[i] ) {
    int c = bin_d_int( ct[i], ct[i+1] );
    res += char( m[c][otp[j]] );
    i += 2;
    j++;
  }
}

// ----------------------------------------------------------------------------
