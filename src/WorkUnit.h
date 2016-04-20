// $Id: WorkUnit.h 7 2006-05-09 07:04:57Z pberck $

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
  \file WorkUnit.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class WorkUnit.h
  \brief Used in Scheduler
*/

#ifndef _WORKUNIT_H
#define _WORKUNIT_H

#include <string>

#include "XMLNode.h"


// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Program;

class WorkUnit {
 public:
  unsigned int  w_type;
  time_t        t_at;
  pid_t         pid;
  Program      *program;

  WorkUnit( unsigned int, long, Program* );
  WorkUnit( XMLNode* );

  void set_program( Program *p ) { program = p; }
  void set_pid( pid_t p) { pid = p; }
};


const unsigned int WT_NOTINIT  =  0;
const unsigned int WT_EXIT     =  1;
const unsigned int WT_RUN_PROG =  2;
const unsigned int WT_END_PROG =  4;
const unsigned int WT_WAIT_PID =  8;
const unsigned int WT_NOSTART  = 16;

#endif
