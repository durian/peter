// $Id: Scheduler.h 9 2007-02-23 09:26:02Z pberck $

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
  \file Scheduler.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Scheduler
  \brief Starts programs
*/

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <pthread.h>

#include <string>
#include <vector>
#include <map>

#include "qlog.h"
#include "WorkUnit.h"
#include "Scheduler.h"
#include "Program.h"
#include "Events.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class XMLNode;

class Scheduler {

 public:
  std::string            id;          /*!< Who am I? */
  std::string            name;        /*!< Who am I? */
  std::vector<WorkUnit*> schedule;    /*!< List of programs to start */
  std::vector<Program*>  programs;    /*!< List of programs to take care of */
  std::map<pid_t, Program*> running;  /*!< What is running with pid */
  pthread_t              thread_id;   /*!< ID of thread */
  Logfile               *log;         /*!< Logger object */
  long                   rate;        /*!< Polling (rather delay) rate */
  unsigned int           mode;        /*!< Run modus */
  pthread_mutex_t        mtx;
  unsigned int           status;      /*!< What am I doing */
  unsigned int           changed;     /*!< Flag set if something changed */
  unsigned int           syslog;      /*!< Log to syslog or not */
  EventList             *elist;       /*!< Log what has happened */

  //! Constructor.
  //!
  Scheduler( const std::string& );

  //! Destructor.
  //!
  ~Scheduler();

  const std::string& get_name() const { return name; }
  void start();
  void set_status( unsigned int );
  void add_workunit( WorkUnit* );
  void add_program( Program* );
  Program* get_program_by_name( const std::string& );
  void set_rate( long );
  void set_mode( const unsigned int );
  unsigned int get_status() { return status; }
  void command( int, const std::string& );
  XMLNode* command( const std::string& );
  void gui_command( const std::string&, std::vector<std::string>& ); 
  int wu_count();
  void add_pids( std::vector<pid_t>& pids );
  unsigned int get_changed();
  void set_changed( unsigned int c ) { changed = c; }
  void inc_changed() { changed++; }
  EventList* get_elist() { return elist; }

  const static unsigned int SCHED_OFF = 0;
  const static unsigned int SCHED_ON  = 1;
  
  const static unsigned int STATUS_INITIALISED =  1;
  const static unsigned int STATUS_RUNNING     =  2;
  const static unsigned int STATUS_ENDING      =  4;
  const static unsigned int STATUS_WAITING     =  8;
  const static unsigned int STATUS_READY       = 16;
  const static unsigned int STATUS_STARTING    = 32;
};


extern "C" void* scheduler( void* );

#endif
