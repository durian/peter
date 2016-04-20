// $Id: Socket.cc 9 2007-02-23 09:26:02Z pberck $

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
  \file Socket.cc
  \author Peter Berck
  \date 2004
*/

#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <algorithm>

#include "Socket.h"
#include "Scheduler.h"
#include "util.h"
#include "expat_parser.h"
#include "XMLNode.h"
#include "Config.h"
#include "otp.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

Socket::Socket( const std::string& h, int p ) {

  sock_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  if( sock_fd < 0 ) {
    status = 1;
  }

  (void)set_opt( SOL_SOCKET, SO_REUSEADDR, 1 );
  //void* sigpipe = (void*)signal( SIGPIPE, SIG_IGN );

  status = 0;
  
  sock_addr.sin_family      = AF_INET;
  sock_addr.sin_port        = htons( p );
  sock_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr( h.c_str() );

  memset(&(sock_addr.sin_zero), '\0', 8);

  int res = bind( sock_fd, (struct sockaddr*)&sock_addr,
		  sizeof(struct sockaddr) );
  if ( res != 0 ) {
    status = 1;
  }

  buf_pos  = 0;
  buf_size = 10240;
  buf      = new char[buf_size];
}

Socket::Socket( int p ) {
  status = 0;

  sock_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  if( sock_fd < 0 ) {
    perror("socket");
    status = 1;
  }

  if ( ! set_opt( SOL_SOCKET, SO_REUSEADDR, 1 ) ) {
    perror("socket");
    status = 1;
  }
  
  sock_addr.sin_family      = AF_INET;
  sock_addr.sin_port        = htons( p );
  sock_addr.sin_addr.s_addr = INADDR_ANY;

  memset(&(sock_addr.sin_zero), '\0', 8);

  int res = bind( sock_fd, (struct sockaddr*)&sock_addr,
		  sizeof(struct sockaddr) );
  if ( res != 0 ) {
    perror("socket");
    status = 1;
  }

  buf_pos  = 0;
  buf_size = 10240;
  buf      = new char[buf_size];
}

void Socket::client() {
  int res = connect( sock_fd, (struct sockaddr*)&sock_addr,
		     sizeof(struct sockaddr) );
  if ( res != 0 ) {
    status = 2;
  }
}

void Socket::server() {
  int res = listen( sock_fd, 5 );
  if ( res != 0 ) {
    perror("socket");
    status = 4;
  }
}

void Socket::set_flag( int f ) {
  fcntl( sock_fd, F_SETFL, f );
}

int Socket::get_flag() {
  return fcntl( sock_fd, F_GETFL, 0 );
}

bool Socket::set_opt( int lev, int opt, int val ) {
  if ( setsockopt( sock_fd, lev, opt, (char*)&val, sizeof(val) ) < 0 ) {
    return false;
  }
  return true;
}

extern "C" void* socket_thread( void *params ) {
  Socket    *s         = (Socket*)params;

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  Logfile *lf = s->get_config()->get_logfile();
  s->server();
  int res;
  while (1) {
    delay(1000000);
    if ( s->have_client() ) {
      //s->spew_data();
      /*
	PROTOCOL.
	
	client: send ID.
	server: check ID, send "1" or "0"; (challenge ID from file?).
	client: on "1" send msg.
	server: read msg.
	client: wait for answer.
	server: send answer.
	close.
      */
      int client_fd;
      client_fd = s->read_data(); // length indication as argument?
      std::string s1 = s->get_buffer();
      //lf->log( "[" + s1 + "]" );
      if ( s1 != "pcmd" ) {
	//
	// We could have a "gui" ID as well, PETeR will send
	// status info without further ado (or less logging).
	//
        if ( s1 != "pgui" ) {
	    lf->log( "WARNING: bad ID received. " + s->get_peer() );
	    (void)s->close_client();
	    continue;
        }
	//
	// handle pgui.
	//
        Config *c = s->get_config();
        std::vector<Scheduler*>& schedulers = c->get_schedulers();
        std::vector<Scheduler*>::iterator si;
        std::vector<std::string> rv;
	std::vector<std::string>::iterator rvi;
        for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
          Scheduler *sched = *si;
          sched->gui_command( "ginfo", rv );
        }
        for ( rvi = rv.begin(); rvi != rv.end(); rvi++ ) {
          std::string r = *rvi + '\n';
          if ( send( s->get_client(), r.c_str(), r.length(), 0) == -1 ) {
            lf->log( "WARNING: could not send over socket." );
            (void)s->close_client();
            continue;
          }
        }
        (void)s->close_client();
        continue;
      }
      std::string a1 = "1";
      if ( send( s->get_client(), a1.c_str(), a1.length(), 0) == -1 ) {
	lf->log( "WARNING: could not send over socket." );
	(void)s->close_client();
	continue;
      }

      // msg;
      //
      s->read_more( client_fd );
      std::string cmd = s->get_buffer();
      XMLNode *result = new XMLNode("peter");
      int pos = cmd.find("<otp ");
      if ( pos != std::string::npos ) {
	lf->log( s->get_peer() + ": [OTP encrypted]" );
      } else {
	if ( s->get_config()->get_otp_only() ) {
	  lf->log( "WARNING: accepting encrypted communication only." );
	  (void)s->close_client();
	  continue;
	}
	//
	// Check if OTP only, fail here if true.
	//
	lf->log( s->get_peer() + ": " + cmd );
      }
      if ( ! s->peer_allowed() ) {
	lf->log( s->get_peer() + ": NO ACCESS." );
	XMLNode *res = new XMLNode( "error" );
	res->add_attribute( "type", "no access" );
	result->add_child( res );
	std::string tmp = result->to_string();
	if ( send( s->get_client(), tmp.c_str(), tmp.length(), 0) == -1 ) {
	  lf->log( "WARNING: could not send over socket." );
	}
	delete result;
	(void)s->close_client();
	continue;
      }

      int otp_mode = 0;
      long offset  = 0L;
      std::string keyfile;
      try {
	XMLNode *cmd_xml = parse_string( cmd );

	// First of all - is it encrypted?
	// <otp keyfile="foo.otp" offset="1230">message</otp>
	//
	if ( cmd_xml->get_name() == "otp" ) {
	  otp_mode = 1;
	  
	  keyfile = cmd_xml->get_attribute( "keyfile", "otp.txt");
	  //lf->log( "(" + keyfile +")" );
	  offset  = stol(cmd_xml->get_attribute( "offset", "0" ));

	  // check if keyfile exists.
	  // if so, open it, check length if ok for msg.
	  // if it is, decode.
	  //
	  std::string plaintext;
	  std::string bincoded = cmd_xml->get_contents();
	  std::string cyphertext = cmd_xml->get_contents();;
	  //bin_d( bincoded, cyphertext );
	  //lf->log( bincoded + "["+cyphertext+"]" );
	  res = otp_d( keyfile, offset, cyphertext, plaintext );
	  offset += bincoded.size();
	  lf->log( "Decrypted: " + plaintext );
	  // clear strings?
	  cmd_xml = parse_string( plaintext );
	}

	// Catches wrong commands also, unless "all".
	//
	if ( ! s->cmd_allowed( cmd_xml->get_name() )) {
	  lf->log( "ERROR: command is not allowed or unknown." );
	  XMLNode *res = new XMLNode( "error" );
	  res->add_attribute( "type", "not allowed or unknown" );
	  result->add_child( res );
	  std::string tmp = result->to_string();
	  if ( send( s->get_client(), tmp.c_str(), tmp.length(), 0) == -1 ) {
	    lf->log( "WARNING: could not send over socket." );
	  }
	  delete result;
	  (void)s->close_client();
	  continue;
	}

	if ( cmd_xml->get_name() == "status" ) {
	  XMLNode *res = new XMLNode( "running" );
	  res->add_attribute( "pid", to_str(s->get_config()->get_pid()) );
	  res->add_attribute( "age", secs_to_str( now() - 
				  s->get_config()->get_t_start()) );
	  result->add_child( res );
	} else if ( cmd_xml->get_name() == "scheduler" ) {
	  //
	  // <scheduler><info /></scheduler>
	  //
	  std::string s_name = cmd_xml->get_attribute( "name", "default" );
	  Scheduler *scheduler = s->get_scheduler( s_name );
	  if ( scheduler != NULL ) {
	    XMLNode *c_xml = cmd_xml->get_first_child();
	    while ( c_xml != NULL ) {
	      std::string s_cmd = c_xml->get_name();
	      scheduler->command( s->get_client(), s_cmd );
	      c_xml = cmd_xml->get_next_child();
	    }
	  }
	  
	} else if ( cmd_xml->get_name() == "info" ) { // <info />
	  Config *c = s->get_config();
	  std::vector<Scheduler*>& schedulers = c->get_schedulers();
	  std::vector<Scheduler*>::iterator si;
	  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	    Scheduler *sched = *si;
	    XMLNode *res = sched->command( "info" );
	    result->add_child(res);
	  }
	} else if ( cmd_xml->get_name() == "quit" ) { // <quit />
	  Config *c = s->get_config();
	  std::vector<Scheduler*>& schedulers = c->get_schedulers();
	  std::vector<Scheduler*>::iterator si;
	  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	    Scheduler *sched = *si;
	    XMLNode *res = sched->command( "quit" );
	    result->add_child( res );
	  }
	} else 	if ( cmd_xml->get_name() == "start" ) { //<start name="" />
	  std::string name = cmd_xml->get_attribute( "name", "" );
	  Config *c = s->get_config();
	  std::vector<Scheduler*>& schedulers = c->get_schedulers();
	  std::vector<Scheduler*>::iterator si;
	  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	    Scheduler *sched = *si;
	    Program *p = sched->get_program_by_name( name );
	    if ( p != NULL ) {
	      WorkUnit *wu = new WorkUnit( WT_RUN_PROG, now(), p );
	      sched->add_workunit( wu );
	      result->add_child( new XMLNode( "accepted" ) );
	    } else {
	      result->add_child( new XMLNode( "error" ) );
	    }
	  }
	} else if ( cmd_xml->get_name() == "stop" ) { //<stop name="" />
	  std::string name = cmd_xml->get_attribute( "name", "" );
	  Config *c = s->get_config();
	  std::vector<Scheduler*>& schedulers = c->get_schedulers();
	  std::vector<Scheduler*>::iterator si;
	  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	    Scheduler *sched = *si;
	    Program *p = sched->get_program_by_name( name );
	    if ( p != NULL ) {	      
	      WorkUnit *wu = new WorkUnit( WT_END_PROG, now(), p );
	      sched->add_workunit( wu );
	      result->add_child( new XMLNode( "accepted" ) );
	    } else {
	      result->add_child( new XMLNode( "error" ) );
	    }
	  }
	} else if ( cmd_xml->get_name() == "log" ) {
	  Config *c = s->get_config();
	  std::vector<Scheduler*>& schedulers = c->get_schedulers();
	  std::vector<Scheduler*>::iterator si;
	  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	    Scheduler *sched = *si;
	    XMLNode *res = sched->command( "log" );
	    result->add_child( res );
	  }
	} else {
	  XMLNode *res = new XMLNode( "error" );
	  res->add_attribute( "type", "unknown command" );
	  result->add_child( res );
	}
	
	std::string tmp = result->to_string();

	if ( otp_mode ) {
	  //
	  std::string pt;
	  int res = otp_e( keyfile, offset, tmp, pt );

	  if ( res > 0 ) {
	    // Didn't work.
	    lf->log( "ERROR: encryption failed ("+to_str(res)+")." );
	    std::string err = "2";
	    if ( send( s->get_client(), err.c_str(), err.length(), 0) == -1 ) {
	      // error
	    }
	  } else {
	    std::string msg = "<otp keyfile=\""+keyfile+"\" offset=\"" + 
	      to_str(offset)+"\">"+pt+"</otp>";
	    //lf->log( msg );
	    //lf->log( tmp );
	    /*
	    if (send( s->get_client(), msg.c_str(), msg.length(), 0) == -1 ) {
	      // error
	    }
	    */
	    s->send_all( s->get_client(), msg.c_str(), msg.length() );
	  }
	} else {
	  if ( send( s->get_client(), tmp.c_str(), tmp.length(), 0) == -1 ) {
	    // error
	  }
	}
	delete result;
      }
      catch ( ParseError &e ) {
	// Parse error.
	XMLNode *res = new XMLNode( "error" );
	res->add_attribute( "type", "XML error" );
	result->add_child( res );
	std::string tmp = result->to_string();

	// Encrypt if necessary.
	//
	if ( otp_mode ) {
	  //
	} else {
	  if ( send( s->get_client(), tmp.c_str(), tmp.length(), 0) == -1 ) {
	    // error
	  }
	}
	delete result;
      }
      
      (void)s->close_client();
    } // have_client()
  }
  return NULL;
}

void Socket::start() {
  pthread_attr_t attr;
  
  pthread_attr_init( &attr );
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
  int res = pthread_create( &thread_id, &attr, &socket_thread, this );
  if ( res != 0 ) {
    status = 8;
    exit(3);
  }
  pthread_attr_destroy( &attr );
}

/*
  Check if someone is talking to us.
*/
int Socket::have_client() {
  pollfd p_fd = { sock_fd, POLLIN, 0 };
  int res = poll( &p_fd, 1, 1000 );// 1000ms
  return res;
}

int Socket::close_client() {
    int e = close( client_fd );
    if ( e == -1 ) {
      std::cerr << "close: " << errno << std::endl;
    }
    return e;
}

void Socket::spew_data() {
  socklen_t          sin_size = sizeof(struct sockaddr_in);
  struct sockaddr_in client_addr;

  client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &sin_size);
  for ( int i = 0; i < 60; i++ ) {
    if ( send( client_fd, "Hello, world!\n", 14, 0 ) == -1 ) {
      return;
    }
    delay(500000);
  }
  close(client_fd);
}

/*
  Should append until read out?
*/
int Socket::read_data() {
  socklen_t          sin_size = sizeof(struct sockaddr_in);
  struct sockaddr_in client_addr;
  struct sockaddr_in  temp;
  socklen_t templen = sizeof(struct sockaddr_in);
  ssize_t res;

  client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &sin_size);
  if ( client_fd < 0 ) {
    perror("socket");
    return -1;
  }

  getpeername(client_fd, (struct sockaddr *) &temp, &templen);
  peer = inet_ntoa(temp.sin_addr);

  //set_flag( O_NONBLOCK | O_NDELAY );
  //set_opt( IPPROTO_TCP, TCP_NODELAY, 1 );

  buf_pos  = 0;
  int read = 1;

  bzero( &buf[0], buf_size );

  while ( read ) {
    res = recv( client_fd, &buf[buf_pos], buf_size-2, 0 );

    if ( res == 0 ) {
      // finished?
      read = 0;
    } else if ( res > 0 ) {
      buf_pos += res; // check >buf_size
    }

    // Check for '>'?
    int foo = buf_pos;
    while ( read && (foo > 0) ) {
      if ( buf[foo] == '>' ) {
	read = 0;
	buf_pos = foo;
      } else if ( buf[foo] == '\n' ) {
	read = 0;
	buf_pos = foo;
	buf[foo] = '\0';
      }
      --foo;
    }

  }
  buf[buf_pos+1] = '\0';
  return client_fd;
}

void Socket::read_more(int client_fd) {
  buf_pos  = 0;
  int read = 1;
  ssize_t res;

  bzero( &buf[0], buf_size );

  while ( read ) {
    res = recv( client_fd, &buf[buf_pos], buf_size-2, 0 );
    if ( res == 0 ) {
      // finished?
      read = 0;
    } else if ( res > 0 ) {
      buf_pos += res; // check >buf_size
    }

    // Check for '>'?
    int foo = buf_pos;
    while ( read && (foo > 0) ) {
      if ( buf[foo] == '>' ) {
	read = 0;
	buf_pos = foo;
      } else if ( buf[foo] == '\n' ) {
	read = 0;
	buf_pos = foo;
	buf[foo] = '\0';
      }
      --foo;
    } 
  }
  buf[buf_pos+1] = '\0';
  return;
}

std::string Socket::get_buffer() {
  if ( strlen(buf) > 0 ) {
    // Remove the linefeed
    //buf[ strlen(buf)-1 ] = '\0';
  }
  return std::string( buf );
}

void Socket::clear_buffer() {
  buf_pos = 0;
}

void Socket::send_all(int s, const char *data, int len) {
  int total = 0;        // how many bytes we've sent
  int bytesleft = len; // how many we have left to send
  int n;

  while(total < len) {
    n = send(s, data+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
} 

bool Socket::peer_allowed() {
  std::vector<std::string> ha = config->get_h_allow();
  std::vector<std::string>::iterator si;
  si = find_if( ha.begin( ),
		ha.end( ),
		std::bind2nd( std::equal_to<std::string>(), peer) );

  if ( si == ha.end() ) {
    // not found.
    return false;
  }

  return true;
}

bool Socket::cmd_allowed( const std::string& cmd ) {
  std::vector<std::string> ca = config->get_c_allow();
  std::vector<std::string>::iterator si;
  if (ca.size() == 0 ) {
    return false;
  }
  if ( ca[0] == "all" ) {
    return true;
  }
  si = find_if( ca.begin( ),
		ca.end( ),
		std::bind2nd( std::equal_to<std::string>(), cmd) );

  if ( si == ca.end() ) {
    // not found.
    return false;
  }

  return true;
}

Scheduler* Socket::get_scheduler( const std::string& name ) {
  return config->get_scheduler( name );
}

std::string dnslookup( const std::string& hname ) {

  struct hostent    *host_ent;
 
  if ( (host_ent = gethostbyname(hname.c_str())) == NULL) {
    return std::string( hname );
  }

  std::string host = inet_ntoa(*((struct in_addr *)host_ent->h_addr));
  return host;
  
}


// ----------------------------------------------------------------------------

