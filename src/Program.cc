// $Id: Program.cc 9 2007-02-23 09:26:02Z pberck $

/*****************************************************************************
 * Copyright 2005-2009 Peter Berck                                           *
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
  \file Program.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class Program
*/
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include "WorkUnit.h"
#include "Scheduler.h"
#include "Program.h"
#include "util.h"
#include "XMLNode.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------


Program::Program( const std::string& name ) {
  t_birth    = utc();
  this->name = name;
  cmd        = "sleep 10";
  log        = new Logfile();
  home       = "./";
  stop_sig   = 15;
  starts     = 0;
  pid        = 0;
  uid        = 0; // not used if 0.
  gid        = 0; // not used if 0.
  t_start    = 0;
  t_end      = 0;
  stop_mode  = 0;
  seq_idx    = 0;
  seq_len    = 0;

  limits.reserve(3);
  limits[LIMIT_STARTS]     = -1;
  limits[LIMIT_MINRUNTIME] = -1;
  limits[LIMIT_MAXRUNTIME] = -1;
}

void Program::run( Scheduler* s ) {

  // If we are running, we exit (or start another one?)
  //
  if ( pid != 0 ) {
    log->log( "WARNING: " + name + " is already running with pid "
	      + to_str(pid) + "." );
    return;
  }


  if ( gid > 0 ) {
    if ( setgid( gid ) == 0 ) {
      log->log( "Set gid to " + to_str(gid) );
    }
  }
  if ( uid > 0 ) {
    if ( setuid( uid ) == 0 ) {
      log->log( "Set uid to " + to_str(uid) );
    }
  }


  std::string sl_str = "START: " + name;
  log->log( sl_str );

  // Check if binary in path. If not, we check the home dir as well.
  // What if we find more than one? What if we set/change/clear environ?
  // Show which we'll be starting.
  //
  const char* p = getenv( "PATH" );
  if ( p == NULL ) {
    log->log( "getenv returned NULL.");
  }
  std::vector<std::string> tokens;
  if ( p != NULL ) {
    Tokenize( p, tokens, ':' );
  }

  log->inc_prefix();
  log->log( "[" + home + "]" );

  // replace these from $RES e.d. in calling string?
  //
  //log->log( "res/sig: " + get_s_attrib( "res/sig" ) );
  //log->log( "name: " + get_s_attrib( "name" ) );

  // Generalize, loop over attributes.
  // Is this fast enough?
  //
  cmd = replacein( cmd, "NAME", get_s_attrib( "name" ) );
  cmd = replacein( cmd, "RES",  get_s_attrib( "res" ) );
  cmd = replacein( cmd, "SIG",  get_s_attrib( "sig" ) );
  cmd = replacein( cmd, "UTC",  the_date_time_utc( now() ) );

  if ( check_dir( home ) == false ) {
    log->log( "ERROR: directory [" + home + "] inaccessible." );
    // We should gracefully exit here - stop other progs. etc.
    // Make a "quit" workunit? Call what_next from here?!
    pid = 0;
    // Add this to local schedule first, merge on return in Scheduler.
    WorkUnit *wu = new WorkUnit( WT_EXIT, now(), this );
    s->add_workunit( wu );
    log->dec_prefix();
    return;
  }

  // Check if binary exists, and we can run it.
  // In sequence: when next?
  //
  cmd = seq[ seq_idx ];
  if ( seq_len == 1 ) {
    log->log( "[" + cmd + "]" );
  } else {
    log->log( "[" + cmd + "][" + to_str(seq_idx+1)+"/"+to_str(seq_len)+"]" );
  }
  std::string bin = cmd;
  std::string arg = cmd;
  int pos = bin.find( " " );
  if ( pos != std::string::npos ) {
    bin = bin.substr(0, pos);
    arg = cmd.substr(pos+1);
    log->log(bin);
    log->log(arg);
  }

  // What if absolute path?
  //
  struct stat filestat;
  if ( (bin[0] == '/') || ((bin[0]=='.') && (bin[1] == '/')) ) { // absolute.
    int res = stat( bin.c_str(), &filestat );
    if ( res == 0 ) {
      //log->log( "BINARY: " + bin ); // OK, no need to shout about it.
    } else {
      //
      // We cannot find binary.
      //
      log->log( "ERROR: " + bin +" not found." );
      pid = 0;
      // Add this to local schedule first, merge on return in Scheduler.
      WorkUnit *wu = new WorkUnit( WT_EXIT, now(), this );
      s->add_workunit( wu );
      log->dec_prefix();
      return;
    }
  } else { // not absolute, look for it in PATH &c.
    int num_bin  = 0;
    int real_bin = 0;
    for ( int i = 0; i < tokens.size(); i++ ) {
      std::string path = tokens[i] + '/' + bin;
      int res = lstat( path.c_str(), &filestat );
      if ( res == 0 ) {
	if ( S_ISLNK(filestat.st_mode) ) {
	  log->log( "S_LINK: " + path );
	  ++num_bin;
	} else {
	  log->log( "BINARY: " + path );
	  ++num_bin;
	  ++real_bin;
	}
      }
    }
    if ( num_bin == 0) { // Still not found.
      // check home? no use - it should be in PATH!
      // 
      std::string path = home + '/' + bin;
      int res = stat( path.c_str(), &filestat );
      if ( res == 0 ) {
	log->log( "BINARY: " + path );
      } else {
	log->log( "ERROR: " + bin +" not found." );
	pid = 0;
	WorkUnit *wu = new WorkUnit( WT_EXIT, now(), this );
	s->add_workunit( wu );
	log->dec_prefix();
	return;
      }
    }
    if ( real_bin > 1 ) {
      log->log( "WARNING: more than one binary found." );
    }
  }
  
  // Check accessibility of logfiles before we fork.
  // When forked, we have no opportunity to log errors because
  // the streams will be closed when fropen fails.
  //
  int fp;
  if ( (fp = open(stderrfile.c_str(), O_CREAT, S_IRUSR | S_IWUSR)) == -1 ) {
    log->log( "ERROR opening stderr ["+stderrfile+"]." );
    pid = 0;
    log->dec_prefix();
    return;
  } else {
    close(fp);
  }
  if ( (fp = open(stdoutfile.c_str(), O_CREAT, S_IRUSR | S_IWUSR)) == -1 ) {
    log->log( "ERROR opening stdout ["+stdoutfile+"]." );
    pid = 0;
    log->dec_prefix();
    return;
  } else {
    close(fp);
  }

  // Fork.
  //
  if ( (pid = fork()) < 0 ) {
    perror( "fork" );
    exit(1);
  }

  if ( pid == 0 ) { //child

    // HAIRY code.
    //
    if ( freopen(stderrfile.c_str(), "a", stderr) == NULL ) {
      _exit( 27 );
    }
    if ( freopen(stdoutfile.c_str(), "a", stdout) == NULL ) {
      _exit( 27 );
    }

    // Change directory to program's home.
    // It should not fail because we checked above.
    //
    int res = chdir( home.c_str() );
    if ( res != 0 ) {
      // chdir to home failed. We cannot print (closed/redirected streams).
      _exit( 27 );
    }

    char *args[64];
    char *cmdcp = strdup( cmd.c_str() );
    char *bincp = strdup( bin.c_str() );
    char *argcp = strdup( arg.c_str() );
    //arg_split( cmdcp, args );
    //args[0][0] = '\0';

    //execvp( *args, args ); // replace us.
    execlp( bincp, cmdcp, (char*)NULL ); // replace us.
    perror( *args );
    _exit(17);
  } // pid == 0

  t_start = utc();
  t_end   = 0;
  ++starts;
  log_info();
  s->running[ pid ] = this;
  log->dec_prefix();
  s->elist->add_event( new Event(t_start, EVT_START, this, -1) );
}

/*!
  Sends the stop signal to the module. Use stop() for a nice
  clean stop.
*/
int Program::send_signal() {
  return send_signal( stop_sig );
}

/*!
  Sends the killing signal to the module. use stop() for a nice
  clean stop.
*/
int Program::send_signal( int signal ) {

  if ( pid == 0 ) {
    log->log( "Program "+name+" is not running.");
    return -1;
  } 

  // Kill with given signal (15 default).
  //
  int result = kill( pid, signal );
  return result;
}

void Program::stop() {
  stop( stop_sig );
}

/*
  Send stop signal and catch.
*/
void Program::stop( int signal ) {

  if ( pid == 0 ) {
    log->log( "Program "+name+" is not running.");
    return;
  } 

  if ( send_signal( signal ) == -1 ) {
    log->log( "Cannot signal program "+name+".");
    return;
  }

  int cpid, status;
  int counter = 200; // wait 200 * 1000 us in total.

  while ( ((cpid = waitpid(pid, &status, WNOHANG )) == 0) &&
	  (counter-- > 0) ) {
    delay( 1000 ); // 1/1000th of a second.
  }
  if ( cpid == 0 ) {   // 0 means not dead.
    log->log( "Cannot terminate program "+name+".");
    return;
  }
  if ( cpid < 0 ) {   // -1 means no dead children.
    log->log( "No child?" );
    pid = 0;
    return;
  }
  if ( cpid != pid ) {
    log->log( "WARNING: waitpid returned unexpected cpid value in stop()." );
    log->log( "WARNING: pid="+to_str(pid)+" cpid="+to_str(cpid));
  }
  pid   = 0;
  t_end = utc();
}

void Program::add_res_action( int res, ActionList* al ) {
  exit_actions[res] = al;
}

void Program::add_sig_action( int sig, ActionList* al ) {
  sgnl_actions[sig] = al;
}

void Program::log_info() {
  log->log( info() );
}

/*
  Return how long a program is/was running.
*/
time_t Program::get_runtime() {
  if ( t_end == 0 ) {
    if ( t_start == 0 ) {
      return 0;
    }
    return utc() - t_start;
  }
  return t_end - t_start;
}

/*
  Return how long a program is idle.
*/
time_t Program::get_offtime() {
  if ( t_end == 0 ) {
    return 0;
  }
  return utc() - t_end;
}

/*
  Return an info string for the logile.
*/
std::string Program::info() {
  std::string part1 = name+" P:"+to_str(pid)+" S:"+to_str(starts)
    + " A:" + secs_to_str( get_runtime() );
  if ( pid == 0 ) {
    part1 += " D:" + secs_to_str( get_offtime() );
  }
  return part1;
}

XMLNode* Program::xml_info() {
  XMLNode *res = new XMLNode("program");
  res->add_attribute( "name", name );
  res->add_attribute( "pid", to_str(pid) );
  res->add_attribute( "starts", to_str(starts) );
  res->add_attribute( "age", secs_to_str(get_runtime()) );
  if ( pid == 0 ) {
   res->add_attribute( "off", secs_to_str(get_offtime()) );
  }
  return res;
}

/*
  This needs to be torn apart in a function which takes an
  ActionList and decides what goes next.
  i.e.:
    ActionList * what_next( res, sig );
  and
    actionlist_to_workunits( Scheduler );
*/
ActionList* Program::get_actions( int res, int sig ) {
  ActionList *al = NULL;
  if ( sig > 0 ) {
    //
    // Got signal, handle that.
    //
    al = sgnl_actions[sig]; // That's a map - do we get NULL if empty?
    if ( al == NULL ) {
      al = sgnl_actions[0]; // default crash actions.
    }
  } else {
    al = exit_actions[res];
    if ( al == NULL ) {
      al = NULL; // default action list defined somewhere?
    }
  }
  return al;
}

/*
  Convert the Actions to WorkUnits.
*/
void Program::what_next( int res, int sig, Scheduler *s ) {

  // If we were stopped by hand, return. Clear flag.
  //
  if ( stop_mode == 1 ) {
    stop_mode = 0;
    return;
  }

  // When crash, we get the next one now!
  // Maybe we need Actions for the sequences.
  //
  if ( seq_idx == seq_len-1 ) { // last one finished.
    //std::cerr << "!\n";
    inc_seq_idx();
  } else {
    inc_seq_idx();// SEQ: Should this be here?
    WorkUnit *wu = new WorkUnit( WT_RUN_PROG, now(), this );
    s->add_workunit( wu );
    return;
  }
  
  // Get a list of Actions for this res/sig exit.
  // If nothing, we return without doing anything.
  //
  ActionList *al = get_actions( res, sig );
  if ( al == NULL ) {
    log->log( "No actions for "+to_str(res)+"/"+to_str(sig)+" exit." );
    return;
  }

  // We have Actions, but, first check if we have reached a limit
  // which prevents us from continuing.
  //
  bool limit_reached = false;
  if ( (limits[LIMIT_MINRUNTIME] != -1) &&
       (get_runtime() < limits[LIMIT_MINRUNTIME]) ) {
    log->log( "LIMIT: minruntime limit not reached." );
    s->elist->add_event( new Event(now(), EVT_LIMIT, this, 0) );
    limit_reached = true;
  }
  if ( (limits[LIMIT_STARTS] != -1) &&
       (starts > limits[LIMIT_STARTS]) ) {
    log->log( "LIMIT: startlimit reached." );
    s->elist->add_event( new Event(now(), EVT_LIMIT, this, 0) );
    limit_reached = true;
  }

  // Now go through it. Put WUs in local schedule!
  // An Action contains a type, and string and number arguments.
  //
  std::vector<Action*>::iterator ai;
  std::vector<Action*>& actions = al->actions;
  for ( ai = actions.begin(); ai != actions.end(); ai++ ) {
    Action *a = *ai;
    //log->log( "Action: " + to_str( a->get_type() ));

    long               after = a->get_l_attrib( "after" );
    const std::string& prog  = a->get_s_attrib( "prog" );

    Program *p = s->get_program_by_name( prog );
    if ( p == NULL ) {
      p = this; // if nothing is specified, apply to self
    }

    // These can be used in the run() command.
    //
    p->add_s_attrib( "res", to_str(res) );
    p->add_s_attrib( "sig", to_str(sig) );
    p->add_s_attrib( "name", name );

    // <start />
    // We check limits here, but still do the other actions.
    //
    if ( a->get_type() == AXN_EXEC ) {
      if ( limit_reached && (p == this) ) {
	continue; // skip this one
      }
      if ( (p == this) && (after == 0L) ) {
	run( s ); // start direct
      } else {
	WorkUnit *wu = new WorkUnit( WT_RUN_PROG, now() + after, p );
	s->add_workunit( wu );
      }
    }
    // <stop name=..." />
    if ( a->get_type() == AXN_KILL ) {
      if ( ( p != NULL ) && ( p != this ) ){
	WorkUnit *wu = new WorkUnit( WT_END_PROG, now() + after, p );
	s->add_workunit( wu );
      }
    }
    // <exit /> whole Scheduler
    if ( a->get_type() == AXN_EXIT ) {
      if ( p != NULL ) {
	WorkUnit *wu = new WorkUnit( WT_EXIT, now(), p );
	s->add_workunit( wu );
      }
    }
  }
}

void Program::inc_seq_idx() {
  seq_idx = (seq_idx + 1) % seq_len;
}

void Program::add_seq_cmd( std::string cmd ) {
    seq.push_back( cmd );
    ++seq_len;
}

bool Program::check_runlimit( Scheduler *s ) {
  if ( pid && 
       (limits[LIMIT_MAXRUNTIME] != -1) &&
       (get_runtime() > limits[LIMIT_MAXRUNTIME]) ) {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
