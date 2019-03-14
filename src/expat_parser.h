// $Id: expat_parser.h 7 2006-05-09 07:04:57Z pberck $
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

#ifndef _EXPAT_PARSER_H
#define _EXPAT_PARSER_H

#include "XMLNode.h"
#include <string>
#include <expat.h>
#include <stdexcept>

typedef struct {
  int level;
  XMLNode *current_node;
} XML_data;


class ParseError : public std::exception {
 private:
  std::string msg;
  
 public:
  ParseError( const std::string& s ) { msg.append( s ); }
  ~ParseError() throw() {}
  const char* what() const throw() { return msg.c_str(); }
};

extern "C" {
  static void start(void *data, const char *el, const char **attr);
  static void end(void *data, const char *el);
  static void contents(void *data, const XML_Char *s, int len);
}

XMLNode *parse_string(const std::string) /*throw(ParseError)*/;
XMLNode *parse_file(const std::string);
std::string read_file(std::string);

#endif
