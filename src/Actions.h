// $Id: Actions.h 7 2006-05-09 07:04:57Z pberck $

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
  \file Actions.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Actions
*/

#ifndef _ACTIONS_H
#define _ACTIONS_H

#include <string>
#include <map>
#include <vector>

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

/*
        <exec prog="two" at="13:30" />
        <exec after="20" prog="three" />
        <start />
        <start at="12:30.55" />
        <start after="100" />
	<mail to="..." subject="..." ... />
	...
*/
class Action {
 private:
  unsigned int                       action_type; //start, exec, etc.
  std::map<std::string, std::string> s_attribs; 
  std::map<std::string, long>        l_attribs; 

 public:
  Action( const unsigned int );

  void set_type( const unsigned int );
  unsigned int get_type();
  void add_s_attrib( const std::string&, const std::string& );
  const std::string& get_s_attrib( const std::string& a ) { return s_attribs[a]; }
  void add_l_attrib( const std::string&, long );
  long get_l_attrib( const std::string& a ) { return l_attribs[a]; }
};

class ActionList {
 public:
  std::vector<Action*> actions;

  ActionList();
  
  void add_action( Action * );
};

const unsigned int AXN_NOP  = 0;
const unsigned int AXN_EXEC = 1;
const unsigned int AXN_WAIT = 2;
const unsigned int AXN_KILL = 4;
const unsigned int AXN_EXIT = 8;

#endif
