// ---------------------------------------------------------------------------
// $Id: main.cc 9 2007-02-23 09:26:02Z pberck $
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2005-2007 Peter Berck                                           *
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

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

#include "Config.h"
#include "Scheduler.h"
#include "WorkUnit.h"
#include "Program.h"
#include "Socket.h"
#include "qlog.h"
#include "util.h"
#include "expat_parser.h"
#include "XMLNode.h"

// ---------------------------------------------------------------------------
//  Detested globals
// ---------------------------------------------------------------------------

volatile int ctrl_c = 0;
extern char **environ;

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

/*!
  Handler for control.C
*/
extern "C" void ctrl_c_handler( int sig ) {
  //printf( "{%i}\n", sig );
  ctrl_c = sig;
}

// ---------------------------------------------------------------------------
//  Subroutines
// ---------------------------------------------------------------------------

bool not_ready( Scheduler *s ) {
  return  s->get_status() != Scheduler::STATUS_READY ;
}

/*!
  \brief Main.
*/
int main( int argc, char* argv[] ) {

  //openlog( argv[0], LOG_PID, LOG_USER ); 
  //syslog( LOG_INFO, "PETeR started." );

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " config-filename" << std::endl;
    //syslog( LOG_INFO, "PETeR ended. No arguments supplied." );
    return 1;
  }

  /*
    Set signal handlers for SIGINT, SIGHUP and SIGTERM.
  */
  struct sigaction sig_a;
  sigset_t sig_m;

  sigemptyset( &sig_m );
  sig_a.sa_handler = ctrl_c_handler;
  sig_a.sa_mask    = sig_m;
  sig_a.sa_flags   = 0;

  if ( sigaction( SIGINT, &sig_a, NULL ) == -1 ) {
    std::cerr << "Cannot install SIGINT handler." << std::endl;
    return 1;
  }
  if ( sigaction( SIGHUP, &sig_a, NULL ) == -1 ) {
    std::cerr << "Cannot install SIGHUP handler." << std::endl;
    return 1;
  }
  if ( sigaction( SIGTERM, &sig_a, NULL ) == -1 ) {
    std::cerr << "Cannot install SIGTERM handler." << std::endl;
    return 1;
  }

  // PJB: make rest into subroutines, so we can "restart" from self.

  Logfile *l = new Logfile();
  l->log( "PETeR (" + std::string(VERSION) + ") started." );
  l->log( " PID: "+to_str(getpid(),6,' ')+" PPID: "+to_str(getppid(),6,' ') );
  l->log( " UID: "+to_str(getuid(),6,' ')+" EUID: "+to_str(geteuid(),6,' ') );
  l->log( " GID: "+to_str(getgid(),6,' ')+" EGID: "+to_str(getegid(),6,' ') );

  Config *c = new Config( l );
  c->read_config( argv[1] );
  l->log( "Config file processed." );
  c->set_pid( getpid() );

  // Privileges... in Scheduler? Program/child
  // Needs work. Needs to be before first thread/socket.
  //
  if ( c->get_s_gid() != 0 ) {
    if ( setgid( c->get_s_gid()) == 0 ) {
      l->log( "Changed gid to " + to_str( c->get_s_gid()));
    } else {
      l->log( "Changing gid to " + to_str( c->get_s_gid()) + " failed." );
      return 2;
    }
    l->log( " GID: "+to_str(getgid(),6,' ')+" EGID: "+to_str(getegid(),6,' '));
  }
  if ( c->get_s_uid() != 0 ) {
    if ( setuid( c->get_s_uid()) == 0 ) {
      l->log( "Changed uid to " + to_str( c->get_s_uid()));
    } else {
      l->log( "Changing uid to " + to_str( c->get_s_uid()) + " failed." );
      return 2;
    }
    l->log( " UID: "+to_str(getuid(),6,' ')+" EUID: "+to_str(geteuid(),6,' '));
    l->log( " GID: "+to_str(getgid(),6,' ')+" EGID: "+to_str(getegid(),6,' '));
  }

  // Environment... (OS X has no clearenv and gives bus error on setenv?!)
  //
#ifdef HAVE_CLEARENV
  if ( c->get_cl_env() == true ) {
    l->log( "Clearing environment." );
    if ( clearenv() != 0 ) {
      l->log( "Clearing environment again." );
      environ = NULL;
    }
    l->log( "Setting PATH to /bin:/usr/bin:/usr/local/bin" );
    setenv( "PATH", "/bin:/usr/bin:/usr/local/bin", 1 );
    setenv( "IFS", " \t\n", 1 );
    /*
      char **e = environ;
      while(*e)
      printf("%s\n", *e++);
    */
  }
#endif

  if ( c->get_port() > 0 ) {
    l->log( "Starting socket on port " + to_str(c->get_port()) + "." );
    Socket *sock = new Socket( c->get_port() );
    sock->set_config( c );
    sock->start();
    if ( sock->get_status() != 0 ) {
      l->log( "WARNING: no socket." );
    }
  } else {
    l->log( "WARNING: no socket defined." );
  }

  std::vector<Scheduler*>& schedulers = c->get_schedulers();
  std::vector<Scheduler*>::iterator si;
  l->log( "Starting Schedulers." );
  l->inc_prefix();
  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
    Scheduler *s = *si;
    l->log( "Starting scheduler "+s->get_name() );
    s->start();

    // Count workunits (the startup WUs), when there are no
    // changes in the WU count, the scheduler is ready, and
    // we start the next one.
    //
    int prev_wuc = s->wu_count();
    while ( s->wu_count() != prev_wuc ) {
      delay(1000);
      prev_wuc = s->wu_count();
    }
  }
  l->dec_prefix();

  // Everything started ----

  l->log( "Starting main loop." );

  /*
    Wait until all the Schedulers are ready.
  */
  bool update_pidsfile;
  while ( true ) {
    delay( 100000 );

    // We need to write pids to pid file when something changes.
    //
    update_pidsfile = false;
    for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
      Scheduler *s = *si;
      if ( s->get_changed() > 0 ) {
	update_pidsfile = true;
	break;
      }
    }
    if ( update_pidsfile ) {
      std::ofstream pidfile_out( (c->get_pidfile()).c_str(), std::ios::out );
      if ( pidfile_out ) {
	XMLNode *res = new XMLNode( "peter" );
	for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	  Scheduler *s = *si;
	  res->add_child( s->command( "info" ) );
	}
	pidfile_out << *res;
	pidfile_out.close();
	delete res;
      }
    }

    if ( ctrl_c ) {
      l->log("ctrl-c/"+to_str(ctrl_c));
      for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
	Scheduler *s = *si;
	s->set_mode( Scheduler::SCHED_OFF ); // s->stop();
      }
      ctrl_c = 0;
    }
    si = find_if( schedulers.begin( ),
		  schedulers.end( ),
		  not_ready );
    if ( si == schedulers.end() ) {
      // all ready.
      break;
    }

  }

  l->log( "Removing PID file." );
  remove( (c->get_pidfile()).c_str() );

  l->log("PETeR Ready.");

  //syslog( LOG_INFO, "PETeR ended normally." ); 
  return 0;
}

// ---------------------------------------------------------------------------
