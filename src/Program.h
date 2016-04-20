// $Id: Program.h 7 2006-05-09 07:04:57Z pberck $

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

/*!
  \file Program.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Program
*/

#ifndef _PROGRAM_H
#define _PROGRAM_H

#include <string>
#include <vector>

#include "qlog.h"
#include "Actions.h"
#include "WorkUnit.h"
#include "Program.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Scheduler;
class XMLNode;

class Program {
 private:
  std::string          id;          /*!< Who am I? */
  std::string          name;        /*!< Name of the program */
  int                  type;        /*!< Type of program */
  uid_t                uid;         /*!< Run as... */
  gid_t                gid;         /*!< Run as... */
  std::string          cmd;         /*!< How to start */
  int                  seq_idx;     /*!< Sequence index */
  int                  seq_len;     /*!< Sequence length */
  std::vector<std::string> seq;     /*!< Sequence of commands */
  std::string          labels;      /*!< Labels to group programs by */
  std::string          home;        /*!< chdir here first */
  std::string          stderrfile;  /*!< Redirected stderr/out */
  std::string          stdoutfile;  /*!< Redirected stderr/out */
  time_t               t_start;     /*!< Module's startup time */
  time_t               t_end;       /*!< Module's ending time */
  pid_t                pid;         /*!< Process ID */
  time_t               t_birth;     /*!< Running since... */
  int                  stop_sig;    /*!< Signal used to stop program */
  Logfile             *log;         /*!< Logging object */
  std::vector<WorkUnit*> schedule;  /*!< Local schedule */
  std::map<int, ActionList*> exit_actions;
  std::map<int, ActionList*> sgnl_actions;
  std::map<std::string, std::string> s_attribs; /*!< Extra attributes */
  int                  starts;      /*!< Number of starts */
  int                  stop_mode;   /*!< Were we stopped by hand? */
  std::vector<int>     limits;   /*!< Limit starts/time, etc */

public:

  //! Constructor.
  //!
  Program( const std::string& );

  //! Destructor.
  //!
  ~Program();

  const std::string& get_name() const { return name; }
  const pid_t get_pid() const { return pid; }
  void set_pid(pid_t p) { pid = p; }
  const uid_t get_uiod() const { return uid; }
  void set_uid(uid_t p) { uid = p; }
  const gid_t get_gid() const { return gid; }
  void set_gid(gid_t p) { gid = p; }
  void set_cmd( const std::string& c ) { cmd = c; }
  const std::string& get_cmd() const { return cmd; }
  void set_home( const std::string& h ) { home = h; }
  const std::string& get_home() const { return home; }
  void set_labels( const std::string& l ) { labels = l; }
  const std::string& get_labels() const { return labels; }
  void set_stderrfile( const std::string& f ) { stderrfile = f; }
  const std::string& get_stderrfile() const { return stderrfile; }
  void set_stdoutfile( const std::string& f ) { stdoutfile = f; }
  const std::string& get_stdoutfile() const { return stdoutfile; }
  void set_t_end( time_t t ) { t_end = t; }
  const time_t get_t_start() const { return t_start; }
  void set_stop_mode( int m ) { stop_mode = m; }
  const int get_stop_mode() const { return stop_mode; }
  void set_stop_sig( int m ) { stop_sig = m; }
  const int get_stop_sig() const { return stop_sig; }
  void run( Scheduler* );
  int  send_signal();
  int  send_signal( int );
  void stop();
  void stop( int );
  void log_info();
  time_t get_runtime();
  time_t get_offtime();
  std::string info();
  XMLNode *xml_info();
  void add_res_action( int, ActionList* );
  void add_sig_action( int, ActionList* );
  ActionList *get_actions( int, int );
  void what_next( int, int, Scheduler* );
  void add_s_attrib( const std::string& k, const std::string& v ) { s_attribs[k] = v;}
  const std::string& get_s_attrib( const std::string& a ) { return s_attribs[a]; }
  void set_limit( int limit, int value ) { limits[limit] = value; }
  const int get_limit( int limit ) { return limits[limit]; }
  void inc_seq_idx();
  void add_seq_cmd( std::string );
  bool check_runlimit( Scheduler* );
};

const static unsigned int PTYPE_IGNORE  = 0;
const static unsigned int PTYPE_NORMAL  = 1;
const static unsigned int PTYPE_PRECOND = 2;

const static unsigned int LIMIT_STARTS     = 0; // should be re- or auto-starts
const static unsigned int LIMIT_MINRUNTIME = 1;
const static unsigned int LIMIT_MAXRUNTIME = 2;

#endif
