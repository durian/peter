// $Id: Events.cc 7 2006-05-09 07:04:57Z pberck $

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
  \file Event.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class Event.cc
*/


#include <string>

#include "Events.h"
#include "Program.h"
#include "XMLNode.h"
#include "util.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

Event::Event( time_t t, const unsigned int et, Program *p, int s ) {
  t_stamp = t;
  event_type = et;
  program = p;
  status = s;
}

std::ostream& operator<<(std::ostream& s, Event& e) {
  s << the_date_time(e.t_stamp) << " " <<
    event_text[e.event_type] << " " << 
    e.program->get_name() << " ";
  if ( e.status != -1 ) {
    s << e.status;
  }
  return s;
}

XMLNode* Event::event_xml() {
  XMLNode *e = new XMLNode( "event" );
  e->add_attribute( "time", the_date_time(t_stamp) );
  e->add_attribute( "tstamp", to_str(t_stamp) );
  e->add_attribute( "type", to_str(event_type) );
  e->add_attribute( "program", program->get_name() );
  if ( status <= 0 ) {
    e->add_attribute( "status", "0/0" );
  } else {
    e->add_attribute( "status", status_to_str(status) );
  }

  return e;
}

EventList::EventList() {
}

void EventList::add_event( Event *e ) {
  events.push_back( e );
}

void EventList::dump() {
  std::vector<Event*>::iterator ei;
  for ( ei = events.begin(); ei != events.end(); ei++ ) {
    Event *e = *ei;
    std::cout << *e << std::endl;
  }
}

std::vector<Event*>& EventList::get_events() {
  return events;
}
