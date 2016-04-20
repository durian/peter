// $Id: Scheduler.cc 9 2007-02-23 09:26:02Z pberck $

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

// ----------------------------------------------------------------------------

/*!
  \file Scheduler.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class Scheduler
  \brief Starts programs at a certain time.

  Each Scheduler gets a schedule and some programs, and handles those.
  
*/
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <map>

#include "Scheduler.h"
#include "Config.h"
#include "WorkUnit.h"
#include "util.h"
#include "XMLNode.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

extern "C" void* scheduler( void *params ) {

  Scheduler *s    = (Scheduler*)params;
  Logfile   *l    = s->log;
  std::vector<WorkUnit*>& sched      = s->schedule;
  std::vector<Program*>&  programs   = s->programs;
  std::map<pid_t, Program*>& running = s->running;
  pid_t cpid;
  int status;
  int sl = 0; // Work In Progress, logging to syslog.

  /*
    We block SIGINT because we don't wont keyboard interrupts
    here. These are caught in main.cc which shuts down the threads.
  */
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  std::vector<WorkUnit*>::iterator wui;

  // Scheduler.
  //
  time_t now = utc();
  while ( s->mode != Scheduler::SCHED_OFF ) {
    //
    // Loop over schedule array, do what is necessary.
    //
    /*
      Two passes - first start if we have to, then erase used
      WorkUnits in the second pass.
    */
    time_t now = utc();
    for ( wui = sched.begin(); wui != sched.end(); ++wui ) {
      WorkUnit *wu = *wui;

      if ( wu->w_type == WT_EXIT ) {
	l->log( "EXIT." );
	wu->w_type = WT_NOTINIT; // Delete in next cycle.
	// Clear rest of schedule, stop all programs.
	s->mode = Scheduler::SCHED_OFF;
      } else if ( wu->w_type == WT_RUN_PROG ) {
	time_t delta = wu->t_at - now;
	if ( delta <= 0L ) {
	  // Start it.
	  wu->program->run( s );
	  wu->w_type = WT_NOTINIT; // Delete work unit in next cycle.
	  running[ wu->program->get_pid() ] = wu->program;
	}
      } else if ( wu->w_type == WT_END_PROG ) {
	time_t delta = wu->t_at - now;
	Program *p = wu->program;
	if ( delta <= 0L ) {
	  l->log( "Stopping: " + p->get_name() );
	  p->set_stop_mode( 1 ); // prevent what_next() because on purpose
	  s->elist->add_event( new Event(now, EVT_STOP, p, -1) );
	  // Make a "wait" work unit. NO
	  //
	  if ( p->send_signal() == 0 ) {
	    //WorkUnit *new_wu = new WorkUnit( WT_WAIT_PID, now, p );
	    //new_wu->set_pid( p->get_pid() );
	    //s->add_workunit( new_wu );
	  }

	  wu->w_type = WT_NOTINIT; // Delete in next cycle.
	}
      } else if ( wu->w_type == WT_NOSTART ) {
	// waiting.
	Program *p = wu->program;
	l->log( "Nostart: " + p->get_name() );
	wu->w_type = WT_NOTINIT; // Delete in next cycle.
      }

    } // foreach WU

    // Second pass: erase used WorkUnits.
    //
    for ( wui = sched.begin(); wui != sched.end(); ) {
      WorkUnit *wu = *wui;
      if ( wu->w_type == WT_NOTINIT ) {
	wui = sched.erase( wui++ );
      } else {
	wui++;
      }
    } // for


    std::vector<Program*>::iterator pri;
    for ( pri = programs.begin(); pri != programs.end(); pri++ ) {
      Program *p = *pri;
      pid_t pid =  p->get_pid();
      
      // limit
      //
      if ( p->get_pid() && p->check_runlimit( s ) ) {
	l->log( "LIMIT: maxruntime limit reached." );
	//
	int stop_sig = p->get_stop_sig();
	(void)p->send_signal( stop_sig );
	p->set_pid( 0 ); // premature, what if kill fails?
	// can't run peter from peter: signals get crossed.
	// wait _here_ for death?
	s->elist->add_event( new Event(now, EVT_LIMIT, p, 0) );
	//delay( s->rate );
      }
    }
    
    // Wait for deaths.
    // PJB: should only wait for own deaths?
    //
    //std::vector<Program*>::iterator pri;
    for ( pri = programs.begin(); pri != programs.end(); pri++ ) {
      Program *p = *pri;
      pid_t pid =  p->get_pid();

      // while ((cpid = waitpid((pid_t)0, &status, WNOHANG )) > 0 ) {
    while ((cpid = waitpid((pid_t)pid, &status, WNOHANG )) > 0 ) {

      if ( cpid != 0 ) {
	Program *program = running[ cpid ]; // find out which
	if ( program == NULL ) {
	  l->log( "NULL Program?! Exit Scheduler." );
	  return NULL;
	}
	program->set_pid( 0 );
	program->set_t_end( now );
	time_t run_time = program->get_runtime();
	running[ cpid ] = NULL;
        int sig = 0;
	int res = 0;
	if ( WIFSIGNALED(status) ) {
          sig = WTERMSIG(status); //status & 127;
	  if ( program->get_stop_mode() == 0 ) {
	    //
	    // We log a crash if not stopped by us (that's with sig15).
	    //
	    s->elist->add_event( new Event(now, EVT_CRASH, program, status) );
	  }
	} else {
	  s->elist->add_event( new Event(now, EVT_END, program, status) );
	}
	res = WEXITSTATUS(status); //result code
	l->log( "ENDED: " + program->get_name() + ": " + to_str(cpid)
		+ " " + to_str(res) + "/" + to_str(sig)
		+ " " + secs_to_str(run_time), sl );
	/* PJB: to be added
	std::string sl_str = program->get_name() + " ended " +
	  to_str(res) + "/" + to_str(sig);
	syslog( LOG_INFO, &(sl_str[0]) );
	*/
	//
	// Check what to do next.
	//
	program->what_next( res, sig, s );
	s->inc_changed();
      }
    } // while cpid
    } // temp for

    // Check LIMIT_MAXRUNTIME.
    //
    /*
    std::vector<Program*>::iterator pi;
    int p_count = 0;
    for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
      Program *p = *pi;
      if ( p->get_pid() && p->check_runlimit( s ) ) {
	//p->stop(); // With this, or next, event queue is not processed.
	/ *
	WorkUnit *wu = new WorkUnit( WT_END_PROG, now, p );
	s->add_workunit( wu );
	l->log( "LIMIT: maxruntime limit reached." );
	* /
	//
	// We kill it. Or should we do this via an event?
	//
	// ORIGINAL
	int stop_sig = p->get_stop_sig();
	(void)p->send_signal( stop_sig );
	s->elist->add_event( new Event(now, EVT_LIMIT, p, 0) );	
	delay( s->rate );
      }
    }
    */

    delay( s->rate );	
    
  } // while

  l->log( "Scheduler "+s->get_name()+" ending programs." );
  s->set_status( Scheduler::STATUS_ENDING );

  // Kill/Wait for my children here.
  // (could be another function?)
  //
  std::vector<Program*>::iterator pi;
  int p_count = 0;
  for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
    Program *p = *pi;
    if ( p->get_pid() > 0 ) {
      int stop_sig = p->get_stop_sig();
      if ( p->send_signal( stop_sig ) == 0 ) {
	l->log( "SIG/"+to_str(stop_sig)+": " + p->get_name() );
	++p_count;
      }
    }
  }

  s->set_status( Scheduler::STATUS_WAITING );

  // Wait for end.
  //
  int tries   = 10000; // 10 seconds
  int k_count = p_count;
  std::vector<Program*>::iterator pri;
  while ( ( tries-- > 0 ) && ( k_count > 0 ) ) {

    for ( pri = programs.begin(); pri != programs.end(); pri++ ) {
      Program *p = *pri;
      pid_t pid =  p->get_pid();

      cpid = waitpid((pid_t)pid, &status, WNOHANG );
      if ( cpid > 0 ) {
	Program *program = running[ cpid ]; // find out which
	if ( program == NULL ) {
	  l->log( "NULL Program?! Exit Scheduler." );
	  return NULL;
	}
	running[ cpid ] = NULL;
	l->log( "ENDED: " + program->info(), sl );
	program->set_pid( 0 );
	program->set_t_end( now );
	--p_count;
	--k_count;
      } else if ( cpid == 0 ) {
	delay( 100 ); // wait more. (s->get_rate() ? )
      } else if ( cpid < 0 ) {
	if ( errno == ECHILD ) {
	  --k_count; // but we don't know who.
	}
      }
    } // for
  }//while

  if ( p_count > 0 ) {
    l->log( "Programs left: "+to_str(p_count) );

    p_count = 0;
    for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
      Program *p = *pi;
      if ( p->get_pid() > 0 ) {
	p->send_signal( 9 ); // Again with SIGKILL
	l->log( "SIG/9: " + p->get_name() );
	++p_count;
      }
    }
    // Wait for end.
    //
    int tries   = 1000;
    int k_count = p_count;
    while ( ( tries-- > 0 ) && ( k_count > 0 ) ) {
      cpid = waitpid((pid_t)0, &status, WNOHANG );
      if ( cpid > 0 ) {
	Program *program = running[ cpid ]; // find out which
	if ( program == NULL ) {
	  l->log( "NULL Program?! Exit Scheduler." );
	  return NULL;
	}
	running[ cpid ] = NULL;
	l->log( "ENDED: " + program->info(), sl );
	program->set_pid( 0 );
	program->set_t_end( now );
	--p_count;
	--k_count;
      } else if ( cpid == 0 ) {
	delay( 100 );
      } else if ( cpid < 0 ) {
	if ( errno == ECHILD ) {
	  --k_count; // but we don't know who.
	}
      }
    }//while

  }

  s->set_status( Scheduler::STATUS_READY );

  l->log( "Scheduler exit." );

  return NULL;
}

Scheduler::Scheduler( const std::string& n ) {
  name = n;
  log  = new Logfile();
  rate = 1000; // 1/1000th second
  schedule.reserve(20);
  pthread_mutex_init( &mtx, NULL );
  status  = STATUS_INITIALISED;
  changed = 1;
  elist   = new EventList();
}

void Scheduler::start() {
  pthread_attr_t attr;
  
  mode = SCHED_ON;

  pthread_attr_init( &attr );
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
  int res = pthread_create( &thread_id, &attr, &scheduler, this );
  if ( res != 0 ) {
    log->log( "ERROR creating thread." );
    log->log( "PETeR ended." );
    exit(3);
  }
  pthread_attr_destroy( &attr );
  status = STATUS_RUNNING;
}

void Scheduler::set_status( unsigned int s ) {
  status = s;
}

/*
  Add a WorkUnit.
*/
void Scheduler::add_workunit( WorkUnit *wu ) {
  schedule.push_back( wu );
}

/*
  Add a Program.
*/
void Scheduler::add_program( Program *prog ) {
  programs.push_back( prog );
}

/*
  Get a program by name.
*/
Program* Scheduler::get_program_by_name( const std::string& name ) {
  std::vector<Program*>::iterator pi;
  for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
    Program *p = *pi;
    if ( p->get_name() == name ) {
      return p;
    }
  }
  return NULL;
}

/*
  If running, we should restart.
*/
void Scheduler::set_rate( long r ) {
  rate = r;
}

/*
  Change operating modus,
*/
void Scheduler::set_mode( const unsigned int m ) {
  mode = m;
}

/*
  Execute an external command.
*/
void Scheduler::command( int sock_fd, const std::string& cmd ) {
  std::vector<Program*>::iterator pi;

  log->log( "CMD ["+cmd+"]" );
  log->inc_prefix();
  if ( cmd == "info" ) {
    for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
      Program *p = *pi;
      std::string tmp = p->info() + "\n";
      tmp = (p->xml_info())->to_string();
      if ( send( sock_fd, tmp.c_str(), tmp.length(), 0) == -1 ) {
	log->log("ERROR.");
	log->dec_prefix();
	return;
      }
    }
  } else if ( cmd == "quit" ) {
    WorkUnit *wu = new WorkUnit( WT_EXIT, now(), NULL );
    add_workunit( wu );
  }
  log->dec_prefix();
  log->log( "CMD ready." );
}

/*
  Execute an external command.
*/
XMLNode* Scheduler::command( const std::string& cmd ) {
  std::vector<Program*>::iterator pi;

  XMLNode *res = new XMLNode( "scheduler" );
  res->add_attribute( "name", name );

  if ( cmd == "info" ) {
    for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
      Program *p   = *pi;
      XMLNode *tmp = p->xml_info();
      res->add_child( tmp );
    }
  } else if ( cmd == "quit" ) {
    WorkUnit *wu = new WorkUnit( WT_EXIT, now(), NULL );
    add_workunit( wu );
    res->set_contents( "quitting" );
  } else if ( cmd == "log" ) {
    std::vector<Event*>::iterator ei;
    std::vector<Event*> events = elist->get_events();
    for ( ei = events.begin(); ei != events.end(); ei++ ) {
      Event *e = *ei;
      res->add_child( e->event_xml() );
    }
  }

  return res;
}

void Scheduler::gui_command( const std::string& cmd, std::vector<std::string>& rv ) {
  std::vector<Program*>::iterator pi;

  if ( cmd == "ginfo" ) {
    for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
      Program *p   = *pi;
      std::string r = p->info();
      rv.push_back( r );
    }
  }
}

int Scheduler::wu_count() {
  return schedule.size();
}

void Scheduler::add_pids( std::vector<pid_t>& pids ) {
  std::vector<Program*>::iterator pi;
  for ( pi = programs.begin(); pi != programs.end(); pi++ ) {
    pids.push_back( (*pi)->get_pid() );
  }
}

unsigned int Scheduler::get_changed() {
  unsigned int tmp = changed;
  changed = 0;
  return tmp;
}

// -- End of text ------------------------------------------------------------
