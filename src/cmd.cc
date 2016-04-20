// $Id: cmd.cc 7 2006-05-09 07:04:57Z pberck $

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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "qlog.h"
#include "util.h"
#include "Socket.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

/*!
  \brief Main.
*/
int main( int argc, char* argv[] ) {
  int sockfd, numbytes;  
  struct hostent *he;
  struct sockaddr_in their_addr; 

  int buf_pos  = 0;
  int buf_size = 10240;
  char* buf;
  buf  = new char[buf_size];

  std::string host = "localhost";
  int port = 2048;

  if ( argc == 2 ) {
    port = atoi( argv[1] );
  }
  if ( argc == 3 ) {
    port = atoi( argv[2] );
    host = argv[1];
  }

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

  std::string m1 = "pcmd\n";
  if ( send( sockfd, m1.c_str(), m1.length(), 0) == -1 ) {
    //
  }
  //  sleep(1);

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
  
  char aux[1025];
  fgets( aux , sizeof( aux ) - 1 , stdin );

  if (send(sockfd, aux, strlen(aux), 0) == -1) {
    perror("send");
  }

  int cont = 1;
  while ( cont ) {

    if ((numbytes=recv(sockfd, buf, 10240-1, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf( "%s", buf );
    if ( numbytes == 0 ) {
      cont = 0;
    }
  }

  close(sockfd);
  return 0;
}

// ---------------------------------------------------------------------------
