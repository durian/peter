// $Id: XMLNode.h 7 2006-05-09 07:04:57Z pberck $
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

#ifndef _XMLNODE_H
#define _XMLNODE_H

#include <string>
#include <vector>
#include <iostream>

// ----------------------------------------------------------------------------

static int cnt;

// ----------------------------------------------------------------------------

class XMLError : public std::exception {
 private:
  std::string msg;
  
 public:
  XMLError( const std::string& s ):msg("XMLError: ") { msg.append( s ); }
  ~XMLError() throw() {}
  const char* what() const throw() { return msg.c_str(); }
};


// ----------------------------------------------------------------------------
// Local class
// ----------------------------------------------------------------------------

class AttVal {
 private:

 public:
  AttVal(const std::string&, const std::string&);
  ~AttVal();

  std::string att;
  std::string val;
};


class XMLNode {
 private:
  std::string name;
  std::string contents;
  std::vector<AttVal*> attributes;
  std::vector<XMLNode*> children;
  int node_type;
  int current_child;
  XMLNode *parent;

 public:  
  XMLNode();
  XMLNode(const std::string&, const std::string&);
  XMLNode(const std::string&);
  ~XMLNode();

  bool has_attribute(const std::string&);
  void add_attribute(AttVal*);
  void add_attribute(const std::string&, const std::string&);
  void add_rep_attribute(const std::string&, const std::string&);
  std::string get_attribute(const std::string&) /*throw(XMLError)*/;
  std::string get_attribute(const std::string&, const std::string&);
  void set_attribute(const std::string&, const std::string&);
  void add_child(XMLNode *x);
  void set_parent(XMLNode *x);
  XMLNode *get_parent();
  void update_child(XMLNode *);

  XMLNode *clone();

  std::string &get_name();
  std::string &get_contents();
  int get_nodetype();
  int get_num_children();
  void set_name(const std::string&);
  void set_type(int);
  void set_contents(const std::string&);
  std::vector<XMLNode*> &get_children();

  void print_info(std::ostream&);
  void pp(std::ostream&, std::string);
  void pp(std::ostream&);
  std::string to_string();

  friend std::ostream& operator<<(std::ostream& s, XMLNode& x);

  XMLNode *get_first_child();
  XMLNode *get_first_child(const std::string&);
  XMLNode *get_current_child();
  XMLNode *get_next_child();
  XMLNode *get_next_child(const std::string&);
  XMLNode *get_first_child(int);
  XMLNode *get_next_child(int);
  std::string find_contents(const std::string&) /*throw (XMLError)*/;
  std::string get_contents(const std::string&) /*throw(XMLError)*/;
  XMLNode *get_child(const std::string&) /*throw(XMLError)*/;
  std::string find_contents(const std::string&, const std::string&);
  bool tag_present(const std::string);
  XMLNode *get_named_child(const std::string) /*throw (XMLError)*/;
  XMLNode *get_named_child(const std::string, XMLNode*);
  std::vector<XMLNode*> get_children(const std::string);

  XMLNode* get_option_by_ref(const std::string);

  void add_comment(const std::string);

  void remove_named_children(const std::string&);

  const static int XML_NODE     = 1;
  const static int XML_LEAF     = 2;
  const static int XML_COMMENT  = 3;
  const static int XML_UNDEF    = 4;
};

class XMLDoc {
 private:
  std::string encoding;
  std::string dtd;
  std::string pub_id;
  std::string sys_id;
  int node_type;
  int standalone;
  XMLNode* root;

 public:
  XMLDoc();
  ~XMLDoc();

  XMLDoc *clone();
  XMLNode *get_root();
  void set_root(XMLNode *);
  int get_nodetype();
  std::string find_contents(const std::string);
  bool tag_present(const std::string);
  XMLNode *get_named_child(const std::string);

  friend std::ostream& operator<<(std::ostream& s, XMLDoc& x);

  const static int XML_DOC      = 0;
};

#endif
