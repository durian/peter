// $Id: otpcmd.cc 9 2007-02-23 09:26:02Z pberck $

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

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

#include "qlog.h"
#include "util.h"
#include "otp.h"
#include "Socket.h"
#include "expat_parser.h"
#include "XMLNode.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

/*!
  \brief Main.
*/
int main( int argc, char* argv[] ) {

  /*
  char foo[2];
  for ( int i = 0; i < 256; i++ ) {
    bin_e_int(i, foo);
    std::cout << i << ": [" << foo[0] << "][" << foo[1] << "] -> ";
    std::cout << bin_d_int( foo ) << std::endl;
  }
  */
  //test_e( "PETERberc~", "ABCDEFGHIJ" );
  //test_d( "QGWIWhlzl(", "ABCDEFGHIJ" );
  /*
  std::string r;
  otp_e_char( "PE TE<>R", "ABCDEFGH", r );
  std::cout << "res=" << r << std::endl;
  r = "";
  otp_d_char( "IAHGFCIHHJHBHEIJ", "ABCDEFGH", r );
  std::cout << "res=" << r << std::endl;
  return 0;
  */
  int sockfd, numbytes;  
  struct hostent *he;
  struct sockaddr_in their_addr; 

  int buf_pos  = 0;
  int buf_size = 10240;
  char* buf;
  buf = new char[buf_size];

  int         port    = 2048;
  std::string host    = "localhost";
  std::string keyfile = "1meg.txt";
  std::string posfile = keyfile + ".pos";
  std::ofstream posfile_out;

  int c;
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"keyfile", 1, 0, 0},
      {"host", 1, 0, 0},
      {"port", 1, 0, 0},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "k:h:p:", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch ( c ) {
      
    case 0:
      if ( long_options[option_index].name == "keyfile" ) {
	keyfile = std::string( optarg );
      }
      if ( long_options[option_index].name == "host" ) {
	host = std::string( optarg );
      }
      if ( long_options[option_index].name == "port" ) {
	port = stoi(std::string( optarg ));
      }
      break;
      
    case 'k':
      keyfile = std::string( optarg );
      break;
      
    case 'h':
      host = std::string( optarg );
      break;

    case 'p':
      port = stoi(std::string( optarg ));
      break;

    default:
      //printf ("?? getopt returned character code 0%o ??\n", c);
      exit(1);
    }

  } // while
  
  posfile = keyfile + ".pos";

  if ((he=gethostbyname(host.c_str())) == NULL) {  // get the host info 
    perror("gethostbyname");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

  if (connect(sockfd, (struct sockaddr *)&their_addr,
	      sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
  }
  
  // Read pos from file here.
  //
  long of = 0L;
  std::ifstream posfile_in( posfile.c_str() );
  if ( posfile_in ) {
    posfile_in >> of;
  }

  // Handshake, send "pcmd" to PETeR.
  //
  std::string m1 = "pcmd\n";
  if ( send( sockfd, m1.c_str(), m1.length(), 0) == -1 ) {
    exit(2);
  }

  // Read answer.
  //
  buf_pos  = 0;
  int read = 1;
  while ( read ) {
    int res = recv( sockfd, &buf[buf_pos], buf_size-2, 0 );
    if ( res == 0 ) {
      // finished?
      read = 0;
    } else if ( res > 0 ) {
      buf_pos += res; // check >buf_size
    }
    if ( res == 1 ) {
      read = 0;
    }
  }
  buf[buf_pos+1] = '\0';

  // stdin.
  //
  char aux[1025];
  fgets( aux , sizeof( aux ) - 1 , stdin );

  // Encrypt it, then encode it.
  //
  std::string pt;
  std::string cy = std::string(aux);
  cy = cy.substr( 0, cy.size()-1 );
  int res = otp_e( keyfile, of, cy, pt );
  if ( res > 0 ) {
    std::cerr << "Keyfile problem.\n";
    exit(3);
  }

  // Construct message.
  //
  std::string msg = "<otp keyfile=\""+keyfile+"\" offset=\""+to_str(of)+"\">" + pt + "</otp>";
  if (send(sockfd, msg.c_str(), msg.size(), 0) == -1) {
    perror("send");
    exit(1);
  }

  // Update pos file here.
  //
  of += cy.size();
  posfile_out.open( posfile.c_str(), std::ios::out );
  posfile_out << of << std::endl;
  posfile_out.close();     

  // Read answer and decrypt.
  //
  int cont = 1;
  std::string answer;
  while ( cont ) {

    if ((numbytes=recv(sockfd, buf, 10240-1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    //printf( "%s", buf );
    answer += std::string( buf );
    if ( numbytes == 0 ) {
      cont = 0;
    }
  }
  close(sockfd);

  // Errors.
  //
  if ( answer == "2" ) {
    std::cout << "ERROR: keyfile too short.\n";
    exit(1);
  }
 
  XMLNode *cmd_xml;
  try {
    cmd_xml = parse_string( answer );
  } catch ( ParseError &e ) {
    std::cout << "ERROR: did not get parsable result.\n";
    exit(1);
  }
  if ( cmd_xml->get_name() == "otp" ) {
    keyfile = cmd_xml->get_attribute( "keyfile", "otp.txt" );
    of      = stol(cmd_xml->get_attribute( "offset", "0" ));

    // check if keyfile exists.
    // if so, open it, check length if ok for msg.
    // if it is, decode.
    //
    std::string plaintext;
    std::string cyphertext = cmd_xml->get_contents();;
    res = otp_d( keyfile, of, cyphertext, plaintext );
    if ( res > 0 ) {
      std::cout << "Encryption error.\n";
    } else {
      of += cyphertext.size();
      std::cout << plaintext ;
    }
  }

  // Update pos file here.
  //
  posfile_out.open( posfile.c_str(), std::ios::out );
  posfile_out << of << std::endl;
  posfile_out.close();     

  return 0;
}


// ---------------------------------------------------------------------------
