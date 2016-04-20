// $Id: Config.cc 7 2006-05-09 07:04:57Z pberck $
//

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
  \file Config.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class Config
  \brief Holds all the information from a config file.
*/

#include <dirent.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <algorithm>

#include "expat_parser.h"
#include "Config.h"
#include "XMLNode.h"
#include "qlog.h"
#include "util.h"

#include "Program.h"
#include "Socket.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

Config::Config() {
  programs.reserve(20);
  Scheduler *sched = new Scheduler( "default" );
  schedulers.push_back( sched );
  pidfile = "pids.xml";
  t_start = now();
  s_uid = 0;
  s_gid = 0;
  cl_env = false;
  otp_only = false;
}

Config::Config( Logfile *l ) {
  lf = l;
  programs.reserve(20);
  Scheduler *sched = new Scheduler( "default" );
  schedulers.push_back( sched );
  pidfile = "pids.xml";
  t_start = now();
  s_uid = 0;
  s_gid = 0;
  cl_env = false;
  otp_only = false;
}

Scheduler* Config::get_scheduler( const std::string& name ) {
  std::vector<Scheduler*>::iterator si;
  for ( si = schedulers.begin(); si != schedulers.end(); si++ ) {
    Scheduler *s = *si;
    if ( s->get_name() == name ) {
      return s;
    }
  }
  return NULL;
}


void Config::read_config( const std::string filename ) {
  config_xml = parse_file( filename );
  lf->log( "Config file: " + filename );
  lf->inc_prefix();

  try {
    lf->log( "GENERAL" );
    XMLNode *general = config_xml->get_named_child( "general" );
    
    try {
      XMLNode *scheds_xml = general->get_named_child( "schedulers" );
      XMLNode *s_xml = scheds_xml->get_first_child();
      while ( s_xml != NULL ) {
	std::string s_name = s_xml->get_attribute( "name", "??" );
	Scheduler *sched = new Scheduler( s_name );
	lf->log( "Scheduler: " + s_name );
	long rate = stol( s_xml->get_attribute( "rate", "0" ) );
	if ( rate > 0L ) {
	  lf->log( "Rate: " + to_str(rate) );
	  sched->set_rate( rate );
	}
	schedulers.push_back( sched );
	s_xml = scheds_xml->get_next_child();
      }
    }
    catch ( const std::exception& e ) {
      lf->log( "Default scheduler." );
    }

    try {
      XMLNode *log_xml = general->get_named_child( "log" );
      if ( log_xml != NULL ) {
	std::string logfilename = log_xml->get_attribute( "file", "" ); 
	lf->log( "Logging to file: " + logfilename );
      }
    }
    catch ( const std::exception& e ) {
      lf->log( "Logging to screen." );
    }

    /*
      The whole program can drop privileges, change environment.
    */
    try {
      XMLNode *priv_xml = general->get_named_child( "priv" );
      if ( priv_xml != NULL ) {
	std::string priv_uid = priv_xml->get_attribute( "uid", "0" );
	if ( priv_uid != "0" ) {
	  s_uid = stoi( priv_uid );
	  lf->log( "UID: " + priv_uid );
	}
	std::string priv_gid = priv_xml->get_attribute( "gid", "0" ); 
	if ( priv_gid != "0" ) {
	  s_gid = stoi( priv_gid );
	  lf->log( "GID: " + priv_gid );
	}
#ifdef HAVE_CLEARENV
	std::string clear_env = priv_xml->get_attribute( "clearenv", "1" );
	if ( clear_env != "0" ) {
	  cl_env = true;
	  lf->log( "Will clear environment." );
	}
#endif
      }
    }
    catch ( const std::exception& e ) {
      lf->log( "No changing of privileges." );
    }

    // Keyfile for OTP
    //
    try {
      XMLNode *otp_xml = general->get_named_child( "otp" );
      if ( otp_xml != NULL ) {
	std::string keydirname = otp_xml->get_attribute( "keydir", "./" );
	lf->log( "OTP keydir: " + keydirname );
	otp_only = false;
	std::string otp = otp_xml->get_attribute( "only", "0" );
	if ( otp != "0" ) {
	  lf->log( "Accepting only encrypted communication." );
	  otp_only = true;
	}
	
	// Read dir and store files.
	//
	/*
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
	lf->log( "Keyfiles: " + to_str( keyfiles.size() ) );
	*/
      }
    }
    catch ( const std::exception& e ) {
      //lf->log( "No OTP." );
    }


    try {
      pidfile = "pids_"+to_str(getpid(),6,'0')+".xml";
      XMLNode *pf_xml = general->get_named_child( "pidfile" );
      if ( pf_xml != NULL ) {
	std::string piddir = pf_xml->get_attribute( "dir", "./" );
	if ( piddir[ piddir.length()-1 ] != '/' ) {
	  piddir = piddir + '/';
	}
	pidfile = piddir + pidfile;
      }
    }
    catch ( const std::exception& e ) {
      pidfile = "/tmp/" + pidfile;
    }
    lf->log( "PID file: " + pidfile );

    // <socket port="2048" allow="127.0.0.1, dante" commands="info" />
    //
    XMLNode *sock_xml = general->get_named_child( "socket" );
    if ( sock_xml != NULL ) {
      port = stoi( sock_xml->get_attribute( "port", "2048" ));
      if ( port > 0 ) {
	lf->log( "SOCKET: " + to_str(port) );
	std::string a = sock_xml->get_attribute( "allow", "localhost" );
	a.erase( remove(a.begin(), a.end(), ' '), a.end() );
	Tokenize( a, h_allow, ',' );
	for ( int i = 0; i < h_allow.size(); i++ ) {
	  lf->log( " Allow: " + h_allow[i] + "/" + 
		   dnslookup( h_allow[i] ) );
	  h_allow[i] = dnslookup( h_allow[i] );
	}

	a = sock_xml->get_attribute( "commands", "info,log,status" );
	a.erase( remove(a.begin(), a.end(), ' '), a.end() );
	Tokenize( a, c_allow, ',' );
	for ( int i = 0; i < c_allow.size(); i++ ) {
	  lf->log( " Allow command: " + c_allow[i] );
	}

      }
    }
  }
  catch ( const std::exception& e ) {
    lf->log( e.what() );
    lf->log( "PETeR ended." );
    exit(1);
  }
  
  try {
    lf->log( "PROGRAMS" );
    XMLNode *general = config_xml->get_named_child( "programs" );
    
    XMLNode *prog = general->get_first_child();
    while ( prog != NULL ) {
      std::string name = prog->get_attribute( "name" );
      lf->log( "PROGRAM: " + name );
      lf->inc_prefix();

      Program *p = new Program( name );

      // Sequences.
      //
      //p->set_cmd( prog->find_contents( "cmd" ) );
      XMLNode *cmd_list = prog->get_first_child();
      while ( cmd_list != NULL ) {
	if ( cmd_list->get_name() == "cmd" ) {
	  p->add_seq_cmd( cmd_list->get_contents() );
	}
	cmd_list = prog->get_next_child();
      }
      p->set_home( prog->find_contents( "home", "./" ) );
      p->set_stop_sig( stoi( prog->find_contents( "signal", "15" ) ) );

      XMLNode *rdr_xml = prog->get_named_child( "redirect", NULL );
      std::string rdr_cout = "/dev/null";
      std::string rdr_cerr = "/dev/null";
      if ( rdr_xml != NULL ) {
	rdr_cout = rdr_xml->get_attribute( "stdout", "/dev/null" );
	rdr_cerr = rdr_xml->get_attribute( "stderr", "/dev/null" );
      }
      p->set_stdoutfile( rdr_cout );
      lf->log( "Redirecting stdout to " + rdr_cout );
      p->set_stderrfile( rdr_cerr );
      lf->log( "Redirecting stderr to " + rdr_cerr );

      XMLNode *lim_xml = prog->get_named_child( "limit", NULL );
      if ( lim_xml != NULL ) {
	int limit = stoi( lim_xml->get_attribute( "restarts", "-1" ) );
	p->set_limit( LIMIT_STARTS, limit );
	if ( limit != -1 ) {
	  lf->log( "Limit restarts: " + to_str( p->get_limit(LIMIT_STARTS) ));
	}
	limit = stoi( lim_xml->get_attribute( "minruntime", "-1" ) );
	p->set_limit( LIMIT_MINRUNTIME, limit );
	if ( limit != -1 ) {
	  lf->log( "Limit min runtime: " + to_str( p->get_limit(LIMIT_MINRUNTIME) ));
	}
	limit = stoi( lim_xml->get_attribute( "maxruntime", "-1" ) );
	p->set_limit( LIMIT_MAXRUNTIME, limit );
	if ( limit != -1 ) {
	  lf->log( "Limit max runtime: " + to_str( p->get_limit(LIMIT_MAXRUNTIME) ));
	}
      }

      Scheduler *scheduler =
	get_scheduler( prog->find_contents( "scheduler", "default" ) );
      if ( scheduler == NULL ) {
	lf->log( "ERROR: unknown scheduler specified." );
	exit(1);
      }
      if ( scheduler->get_name() != "default" ) {
	lf->log( "Scheduler: " + scheduler->get_name() );
      }

      // Start up actions.
      //
      try {
	XMLNode *init_xml = prog->get_named_child( "init" );
	XMLNode *cmds_xml = init_xml->get_first_child();

	while ( cmds_xml != NULL ) {

	  WorkUnit *wu = new WorkUnit( cmds_xml );
	  wu->set_program( p );
	  if ( wu->w_type != WT_NOTINIT) {
	    scheduler->add_workunit( wu );
	  }

	  cmds_xml = init_xml->get_next_child();
	}
      } catch (...) {
	// Default init, start the program.
	lf->log( "No init actions, adding default startup." );
	WorkUnit *wu = new WorkUnit( WT_RUN_PROG, utc(), p );
	scheduler->add_workunit( wu );
      }

      // Exit actions.
      // We need result="*" or result="1,2,3" ?
      //
      try {
	XMLNode *actions_xml = prog->get_named_child( "actions" );
	lf->log( "ACTIONS" );

	XMLNode *exit_xml = actions_xml->get_first_child();

	while ( exit_xml != NULL ) {
	  int sig = -1;
	  int res = -1;

	  // First create the ActionList with Actions, then look
	  // at for which values we want it.
	  //
	  ActionList *al = new ActionList();
	  XMLNode *cmds_xml = exit_xml->get_first_child();
	  
	  while (cmds_xml != NULL ) {
	    Action *a;

	    std::string after = cmds_xml->get_attribute( "after", "0" );
	    std::string prog  = cmds_xml->get_attribute( "name", "" );
	    
	    if ( cmds_xml->get_name() == "start" ) {
	      a = new Action( AXN_EXEC );
	      a->add_l_attrib( "after", stol( after ));
	      a->add_s_attrib( "prog", prog );
	    }

	    if ( cmds_xml->get_name() == "stop" ) {
	      a = new Action( AXN_KILL );
	      a->add_l_attrib( "after", stol( after ));
	      a->add_s_attrib( "prog", prog );
	    }

	    if ( cmds_xml->get_name() == "exit" ) {
	      a = new Action( AXN_EXIT );
	    }

	    al->add_action( a );
	    cmds_xml = exit_xml->get_next_child();	    
    
	  } //cmds_xml != NULL

	  // Now look for which (range of) values we want
	  // this action to work.
	  //
	  if ( exit_xml->has_attribute( "signal" )) {
	    std::string signals = exit_xml->get_attribute("signal");
	    std::vector<std::string> signal_parts;
	    std::vector<std::string>::iterator si;
	    Tokenize( signals, signal_parts, ',' );
	    for ( si = signal_parts.begin(); si != signal_parts.end(); si++ ) {
	      std::string tmp = *si;
	      int pos = tmp.find('-');
	      if ( pos != std::string::npos ) { // a-b range
		std::vector<std::string> range_parts;
		Tokenize( tmp, range_parts, '-' );
		int range_begin = stoi( range_parts[0] );
		int range_end   = stoi( range_parts[1] );
		for ( sig = range_begin; sig <= range_end; sig++ ) {
		  p->add_sig_action( sig, al );
		}
		lf->log( "Action for result " + tmp );
	      } else {
		sig = stoi( tmp );
		p->add_sig_action( sig, al );
		lf->log( "Action for signal " + to_str(sig));
	      }
	    }
	  } else if ( exit_xml->has_attribute( "result" )) {
	    std::string results = exit_xml->get_attribute("result");
	    std::vector<std::string> result_parts;
	    std::vector<std::string>::iterator si;
	    Tokenize( results, result_parts, ',' );
	    for ( si = result_parts.begin(); si != result_parts.end(); si++ ) {
	      std::string tmp = *si;
	      int pos = tmp.find('-');
	      if ( pos != std::string::npos ) { // a-b range
		std::vector<std::string> range_parts;
		Tokenize( tmp, range_parts, '-' );
		int range_begin = stoi( range_parts[0] );
		int range_end   = stoi( range_parts[1] );
		for ( res = range_begin; res <= range_end; res++ ) {
		  p->add_res_action( res, al );
		}
		lf->log( "Action for result " + tmp );
	      } else {
		res = stoi( tmp );
		p->add_res_action( res, al );
		lf->log( "Action for result " + to_str(res));
	      }
	    }
	  } else {
	    res = 0;
	    p->add_res_action( res, al );
	    lf->log( "Action for result " + to_str(res));
	  }

	  exit_xml = actions_xml->get_next_child();

	} // exit_xml != NULL
      } catch (...) {
	// Default action restart? res=0, sig=0.
	lf->log( "Adding default actions." );
	ActionList *al = new ActionList();
	Action *a = new Action( AXN_EXEC );
	a->add_s_attrib( "prog", "" );
	a->add_l_attrib( "after", 0 );
	al->add_action( a );
	p->add_res_action( 0, al );
	p->add_sig_action( 0, al );
      }

      // Store in list. Should check if it exists.
      //
      programs.push_back( p );

      // Add to scheduler (choose?)
      //
      scheduler->add_program( p );

      lf->dec_prefix();
      prog = general->get_next_child();
    } // prog
    lf->dec_prefix();
    
  }
  catch ( const std::exception& e ) {
    lf->log( e.what() );
    lf->log( "PETeR ended." );
    exit(1);
  }
}

