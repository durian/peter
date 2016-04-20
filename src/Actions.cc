// $Id: Actions.cc 7 2006-05-09 07:04:57Z pberck $

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
  \file Actions.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class Actions

  Actions are called when a program terminates. There are exit actions
  (normal exit) and signal actions (on a crash).
*/

#include <map>
#include <string>

#include "Actions.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

Action::Action( const unsigned int a ) {
  action_type = a;
}

void Action::set_type( const unsigned int a ) {
  action_type = a;
}

unsigned int Action::get_type() {
  return action_type;
}

void Action::add_s_attrib( const std::string& k, const std::string& v ) {
  s_attribs[k] = v;
}

void Action::add_l_attrib( const std::string& k, long l ) {
  l_attribs[k] = l;
}

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

ActionList::ActionList() {
  // Wat is dit eigenlijk
}

void ActionList::add_action( Action* a ) {
  actions.push_back( a );
}
