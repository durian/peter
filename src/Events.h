// $Id: Events.h 7 2006-05-09 07:04:57Z pberck $

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
  \file Events.h
  \author Peter Berck
  \date 2005
*/

/*!
  \class Events
*/

#ifndef _EVENTS_H
#define _EVENTS_H

#include <string>
#include <vector>

#include "Program.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Event {
 private:
  time_t           t_stamp;
  unsigned int     event_type;
  Program         *program;
  int              status;

  // what else it triggered? no, will be logged.

 public:
  Event( time_t, const unsigned int, Program *, int status );
  XMLNode* event_xml();

  friend std::ostream& operator<<(std::ostream& s, Event& e);
};

class EventList {
 public:
  std::vector<Event*> events;

  EventList();
  
  void add_event( Event * );
  void dump();
  std::vector<Event*>& get_events();
};

const unsigned int EVT_START  = 0;
const unsigned int EVT_END    = 1;
const unsigned int EVT_STOP   = 2;
const unsigned int EVT_CRASH  = 4;
const unsigned int EVT_LIMIT  = 8;

const std::string event_text[2] = { "started", "stopped" };

#endif
