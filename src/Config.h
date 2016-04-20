// $Id: Config.h 7 2006-05-09 07:04:57Z pberck $
//

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
  \file Config.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Config
  \brief Holds all the information from a config file.
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#include <string>
#include <vector>

#include "XMLNode.h"
#include "Program.h"
#include "Scheduler.h"
#include "qlog.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Config {
 private:
  std::string             id;         /*!< Who am I? */
  uid_t                   uid;        /*!< Who am I? */
  uid_t                   s_uid;       /*!< Who shall I be? */
  gid_t                   s_gid;       /*!< Who shall I be? */
  pid_t                   pid;        /*!< Who am I? */
  time_t                  t_start;    /*!< Startup time */
  XMLNode                *config_xml; /*!< The configuration document */
  Logfile                *lf;         /*!< Logfile object */
  std::vector<Program*>   programs;   /*!< List of programs to take care of */
  std::vector<Scheduler*> schedulers; /*!< List of schedulers */
  std::vector<std::string>h_allow;    /*!< Allowed hosts for socket */
  std::vector<std::string>c_allow;    /*!< Allowed commands for socket */
  std::vector<std::string>keyfiles;   /*!< Files with OTPs */
  std::string             host;
  int                     port;
  std::string             pidfile;    /*!< File where PIDs are stored */
  bool                    cl_env;     /*!< Clear environment? */
  bool                    otp_only;   /*!< Accept only encrypted comm */

public:

  //! Constructor.
  //!
  Config();
  Config( Logfile * );

  //! Destructor.
  //!
  ~Config();

  Scheduler* get_scheduler( const std::string& );
  void read_config( const std::string );
  pid_t get_pid() { return pid; }
  pid_t get_s_uid() { return s_uid; }
  gid_t get_s_gid() { return s_gid; }
  bool  get_cl_env() { return cl_env; }
  bool  get_otp_only() { return otp_only; }
  void set_pid(pid_t p) { pid = p; }
  Logfile *get_logfile() { return lf; }
  time_t get_t_start() { return t_start; }
  std::vector<Program*>& get_programs() { return programs; }
  std::vector<Scheduler*>& get_schedulers() { return schedulers; }
  std::vector<std::string>& get_h_allow() { return h_allow; }
  std::vector<std::string>& get_c_allow() { return c_allow; }
  const std::string get_host() { return host; }
  int get_port() { return port; }
  const std::string get_pidfile() const { return pidfile; }
};

#endif
