// $Id: WorkUnit.cc 7 2006-05-09 07:04:57Z pberck $

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
  \file WorkUnit.cc
  \author Peter Berck
  \date 2004
*/

/*!
  \class WorkUnit.cc
  \brief Used in Scheduler
*/


#include <string>
#include <iostream>
#include <sstream>
#include <ctime>

#include <string.h>

#include "WorkUnit.h"
#include "Program.h"
#include "XMLNode.h"
#include "util.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

WorkUnit::WorkUnit( unsigned int w_type, long t_at, Program *program ) {
  this->w_type    = w_type;
  this->t_at      = t_at;
  this->program   = program;
}

/*
  <init>
    <start after="10" />
    <stop  after="20" />
  </init>
*/
WorkUnit::WorkUnit( XMLNode *xml ) {
  w_type = WT_NOTINIT;
  if ( xml->get_name() == "start" ) {
    w_type = WT_RUN_PROG;
    std::string at = xml->get_attribute( "at", "0" ); // 2019-03-05 05:55:28
    if ( at.length() == 19 ) {
      struct tm tm;
      memset( &tm, 0, sizeof tm );
      t_at = utc(); 
      if ( strptime(at.c_str(), "%Y-%m-%d %H:%M:%S", &tm) ) {
	t_at = mktime( &tm ); // PJB FIXME check is in the past?
      }
    } else {
      std::string after = xml->get_attribute( "after", "0" );
      t_at = utc() + std::stol( after );
    }
  }
  if ( xml->get_name() == "stop" ) {
    w_type = WT_END_PROG;
    std::string at = xml->get_attribute( "at", "0" ); // 2019-03-05 05:55:28
    if ( at.length() == 19 ) {
      struct tm tm;
      memset( &tm, 0, sizeof tm );
      t_at = utc(); 
      if ( strptime(at.c_str(), "%Y-%m-%d %H:%M:%S", &tm) ) {
	t_at = mktime( &tm );
      }
    } else {
      std::string after = xml->get_attribute( "after", "0" );
      t_at = utc() + std::stol( after ); // SHOULD BE RELATIVE TO START TIME
    }
  }
  if ( xml->get_name() == "nostart" ) {
    w_type = WT_NOSTART;
    t_at = utc();
  }
  program = NULL;
}



