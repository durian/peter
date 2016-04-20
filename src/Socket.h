// $Id: Socket.h 7 2006-05-09 07:04:57Z pberck $

/*****************************************************************************
 * Copyright 2005 Peter Berck                                                *
 *                                                                           *
 * This file is part of PETeR.                                               *
 *                                                                           *
 * PETeR is free software; you can redistribute it and/or modify it          *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * PETeR is distributed in the hope that it will be useful, but WITHOUT      *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with PETeR; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ----------------------------------------------------------------------------

/*!
  \file Socket.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Socket.h
  \brief Socket wrapper.
*/

#ifndef _SOCKET_H
#define _SOCKET_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <netdb.h>

#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Scheduler;
class Config;

class Socket {
 private:
  int                port;
  std::string        peer;
  int                backlog;
  int                sock_fd;
  int                client_fd;
  struct sockaddr_in sock_addr;
  char*              buf;
  int                buf_size;
  int                buf_pos;
  int                status;
  pthread_t          thread_id;   /*!< ID of thread */
  Scheduler         *scheduler;   /*!< Scheduler this socket handles */
  Config            *config;

 public:
  Socket( const std::string&, int );
  Socket( int );

  std::string dnslookup( const std::string& );
  void client();
  void server();
  void set_flag( int );
  int  get_flag();
  bool set_opt( int, int, int );
  int  get_status() { return status; }
  void start();
  int  have_client();
  int  close_client();
  int  get_client() { return client_fd; }
  std::string  get_peer() { return peer; }
  void spew_data();
  int read_data();
  void read_more(int);
  std::string get_buffer();
  void send_all(int, const char*, int);
  bool peer_allowed();
  bool cmd_allowed( const std::string& );
  void clear_buffer();
  void set_scheduler( Scheduler *s ) { scheduler = s; }
  void set_config( Config* c ) { config = c; }
  Config* get_config() { return config; }
  Scheduler* get_scheduler( const std::string& );
};

std::string dnslookup( const std::string& );

extern "C" void* socket_thread( void* );

#endif

