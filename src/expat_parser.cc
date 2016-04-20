// $Id: expat_parser.cc 7 2006-05-09 07:04:57Z pberck $
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

/*! \file expat_parser.cc
  \brief XML parser using expat.

*/

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------


#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <expat.h>
#include "XMLNode.h"
#include "util.h"
#include "expat_parser.h"
  
// ---------------------------------------------------------------------------
//  Implementation.
// ---------------------------------------------------------------------------

static void start(void *data, const char *el, const char **attr)
{
  int i;
  XML_data *dat = (XML_data *)data;

  XMLNode *this_node = new XMLNode( el );
  this_node->set_contents("");

  for (i = 0; attr[i]; i += 2) {
    this_node->add_attribute( attr[i], attr[i + 1] );
  }

  if ( dat->current_node != NULL ) {
    dat->current_node->add_child( this_node );
  }
  dat->current_node = this_node;
  dat->level++;
}

static void end(void *data, const char *el)
{
  XML_data *dat = (XML_data *)data;
  if ( dat->level > 1 ) {
    dat->current_node = dat->current_node->get_parent();
  }
  dat->level--;
}


static void contents(void *data, const XML_Char *s, int len) {
  XML_data *dat = (XML_data *)data;

  std::string n;
  n.append(s, len);
  dat->current_node->set_contents( n );
}

XMLNode *parse_string(const std::string s) throw(ParseError) {
  if ( s.length() < 3 ) {
    throw ParseError( "Parse error!" );
  }
  XML_Parser p = XML_ParserCreate("UTF-8");
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }

  XML_SetElementHandler(p, start, end);
  XML_SetCharacterDataHandler(p, contents);

  XML_data *dat;
  dat = (XML_data *) malloc(sizeof(XML_data));
  dat->level = 0;
  dat->current_node = NULL;
  XML_SetUserData(p,(void*)dat);

  if (XML_Parse(p, s.c_str(), s.size(), true) == XML_STATUS_ERROR) {
    /*
    fprintf(stderr, "Parse error at line %d:\n%s\n",
	    XML_GetCurrentLineNumber(p),
	    XML_ErrorString(XML_GetErrorCode(p)));
    */
    XML_ParserFree(p);
    throw ParseError( "Parse error at line "+to_str(XML_GetCurrentLineNumber(p)));
    //exit(-1);
  }
  XML_ParserFree(p);
  
  return dat->current_node;
}

XMLNode *parse_file(const std::string filename) {
  XML_Parser p = XML_ParserCreate("UTF-8");
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }

  XML_SetElementHandler(p, start, end);
  XML_SetCharacterDataHandler(p, contents);

  XML_data *dat;
  dat = (XML_data *) malloc(sizeof(XML_data));
  dat->level = 0;
  dat->current_node = NULL;
  XML_SetUserData(p,(void*)dat);

  std::string foo;
  foo = read_file( filename.c_str() );

  if (XML_Parse(p, foo.c_str(), foo.size(), true) == XML_STATUS_ERROR) {
    fprintf(stderr, "Parse error at line %d:\n%s\n",
	    XML_GetCurrentLineNumber(p),
	    XML_ErrorString(XML_GetErrorCode(p)));
    exit(-1);
  }
  //std::cout << *dat->current_node;


  XML_ParserFree(p);
  
  return dat->current_node;
}

/*
#include <fstream>
#include <iostream>
#include <iterator>

int main() {
  std::ifstream file("test.txt");
  std::istreambuf_iterator<char> first(file), last;
  std::string content(first, last);
  std::cout << "size: " << content.size() << ", content:\n" << content <<
'\n';
}
*/

std::string read_file( std::string filename ) {
  std::ifstream file( filename.c_str() );
  if ( ! file ) {
    std::cerr << "ERROR: cannot load file." << std::endl;
    exit(-1);
  }
  std::istreambuf_iterator<char> first(file), last;
  std::string buf(first, last);
  file.close();
  return buf;
}

std::string fileToString(std::string const& name) {
  std::ifstream in(name.c_str());
  return std::string(std::istreambuf_iterator<char>(in),
		     std::istreambuf_iterator<char>());
}
std::string fileToString1(std::string const& name) {
  std::ifstream in(name.c_str());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

