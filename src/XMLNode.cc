// $Id: XMLNode.cc 7 2006-05-09 07:04:57Z pberck $
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

/*! \file XMLNode.cpp
  \brief XML processing.

  Manipulations on the tree created with the SAX parser.
*/

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>

// Import some stuff into our namespace.
//
using std::cout;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::ofstream;
using std::string;

// C includes
//
#include <sys/time.h>
#include <unistd.h>

#include "XMLNode.h"

// ---------------------------------------------------------------------------
//  Local class
// ---------------------------------------------------------------------------

/*!
  Local class to keep the attributes.

  Consists of a string name and a string value.
*/
AttVal::AttVal(const std::string& a, const std::string& v) {
  att = a;
  val = v;
}

AttVal::~AttVal() {
}

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

/*!
  Empty node.
*/
XMLNode::XMLNode() {
  node_type = XMLNode::XML_UNDEF;
  current_child = 0;
}

/*!
  Destructor, destroys pointed-to objects in children.
*/
XMLNode::~XMLNode() {
  if ( ! children.empty()) {
    std::vector<XMLNode*>::iterator ci;
    for (ci = children.begin(); ci != children.end(); ++ci) {
      delete *ci;
    }
  }
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    delete *si;
  }
  name = "";//.clear();
  contents = "";//.clear();
  children.clear();
  attributes.clear();
}

/*!
  Clone the node and children.
*/
XMLNode *XMLNode::clone() {
  XMLNode *nn    = new XMLNode();
  nn->name       = name;
  nn->contents   = contents;
  nn->node_type  = node_type;
  nn->parent     = parent;

  //nn->attributes = attributes;
  if ( ! children.empty()) {
    std::vector<XMLNode*>::iterator ci;
    for (ci = children.begin(); ci != children.end(); ++ci) {
      XMLNode *tmp = *ci;
      XMLNode *nnn = tmp->clone();
      nn->add_child(nnn);
    }
  }
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    nn->add_attribute(tmp->att, tmp->val);
  }

  return(nn);
}

/*!
  Create a leaf node: <n>c</n>
*/
XMLNode::XMLNode(const std::string& n, const std::string& c) {
  name      = n;
  contents  = c;
  node_type = XMLNode::XML_LEAF;
  current_child = 0;
}

/*!
  Create a node node: <n></n>
*/
XMLNode::XMLNode(const std::string& n) {
  name      = n;
  node_type = XMLNode::XML_UNDEF;
  current_child = 0;
}

// Attribute stuff.
//

/*!
  Returns true if this named attribute exists.
*/
bool::XMLNode::has_attribute(const std::string& a) {
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    if (tmp->att == a) {
      return true;
    }
  }
  return false;
}

/*!
  Adds the specified attribute to the node. Does not check
  if already present.
*/
void XMLNode::add_attribute(AttVal *av) {
  attributes.push_back(av);
}

/*!
  Creates a new attribute from the string arguments and adds
  it to the node.
*/
void XMLNode::add_attribute(const std::string& a, const std::string& v) {
  AttVal *av = new AttVal(a, v);
  attributes.push_back(av);
}

/*!
  Add/replace. If present, update value, else add.
*/
void XMLNode::add_rep_attribute(const std::string& a, const std::string& v) {
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    if (tmp->att == a) {
      tmp->val = v;
      return;
    }
  }
  AttVal *av = new AttVal(a, v);
  attributes.push_back(av);
}

/*!
  Returns the value of the specified attribute.
*/
std::string XMLNode::get_attribute(const std::string& att_name) throw(XMLError) {
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    if (tmp->att == att_name) {
      return tmp->val ;
    }
  }
  throw XMLError( "attribute not found ("+name+" \""+att_name+"\")" );
}

/*!
  Returns the value of the specified attribute, or default.
*/
std::string XMLNode::get_attribute(const std::string& att_name,
				   const std::string& def_name) {
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    if (tmp->att == att_name) {
      return tmp->val ;
    }
  }
  return def_name;
}

/*!
  Sets the new value of the specified attribute.

  Does nothing if the attribute is not present.

  \note Perhaps we should add it in that case...?
*/
void XMLNode::set_attribute(const std::string& att_name,
			    const std::string& att_value) {
  std::vector<AttVal*>::iterator si;
  for (si = attributes.begin(); si != attributes.end(); ++si) {
    AttVal *tmp = *si;
    if (tmp->att == att_name) {
      tmp->val = att_value;
      return;
    }
  }
}

/*!
  Add a child to the XML Node.

  A \c XML_LEAF node to which a child is added is changed into a 
  \c XML_NODE node.

  \note It just adds the pointer of the child.
*/
void XMLNode::add_child(XMLNode *x) {
  children.push_back(x);
  x->parent = this;
  node_type = XMLNode::XML_NODE;
}

/*!
  Replace contents if already present, else add.
*/
void XMLNode::update_child(XMLNode *x) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == x->name) {
      if (temp->node_type == XMLNode::XML_LEAF) {
	temp->contents = x->contents;
      }
      //
      // We replace the node with the new one. Note that only a pointer
      // to x is stored, don't delete x somewhere else. Clone?
      //
      if (temp->node_type == XMLNode::XML_NODE) {
	delete temp;
	children[current_child] = x;//x->clone();
      }
      return;
    }
    temp = get_next_child();
  }
  children.push_back(x);
}

/*!
  Returns a reference to the name of the node.

  In case of an \c XML_NODE or \c XML_LEAF this is the
  name of the tag.
*/
std::string &XMLNode::get_name() {
  return name;
}

/*!
  Returns a reference to the contents of the node.
*/
std::string &XMLNode::get_contents() {
  return contents;
}

/*!
  Returns parent.
*/
XMLNode *XMLNode::get_parent() {
  return parent;
}

/*!
  Returns the number of children.
*/
int XMLNode::get_num_children() {
  return children.size();
}

/*!
  Set the \c name field of a node.
*/
void XMLNode::set_name(const std::string& new_n) {
  name = new_n;
}

/*!
  Set the \c contents field of a node.
*/
void XMLNode::set_contents(const std::string& new_c) {
  contents = new_c;
  node_type = XMLNode::XML_LEAF;
}

/*!
  Returns a reference to the vector of children.

  This allows "manipulation" of the XML tree.
*/
std::vector<XMLNode*> &XMLNode::get_children() {
  return children;
}

/*!
  Prints debugging information.
*/
void XMLNode::print_info(std::ostream& s) {
  s << "type:       " << node_type << endl;
  s << "name:       " << name << endl;
  s << "contents:   " << contents << endl;
  s << "attributes: " << attributes.size() << endl;
  s << "children:   " << children.size() << endl;
}

/*!
  Pretty print the node. Needs a rewrite.
*/
void XMLNode::pp(std::ostream& s, std::string indent) {
  static char quote = '\"';
  static bool lf = true;
  static std::string indent_increment = "  ";
  //static bool lf = false;
  //static std::string indent_increment = "";
  //indent = "";

  if (node_type == XMLNode::XML_COMMENT) {
    s << "<!--" << contents << "-->";
    if (lf) {
      s << endl;
    }
    return;
  }

  if (lf) {
    s << indent;
  }
  s << "<" << name;

  // Attributes
  //
  if ( ! attributes.empty()) {
    std::vector<AttVal*>::iterator si;
    std::vector<AttVal*> atts = attributes;
    for (si = atts.begin(); si != atts.end(); ++si) {
      AttVal *av = *si;
      s << " " << av->att << "=" << quote << av->val << quote;
    }
  }

  if ( (node_type == XMLNode::XML_LEAF) && (contents == "") ) {
    s << " />";
    if (lf) {
     s << endl;
    }
    return;
  }
  s << ">";
  
  // Children?
  //
  if ( ! children.empty()) {
    if (lf) {
      s << endl;
    }
    std::vector<XMLNode*>::iterator ci;
    std::vector<XMLNode*> cs = children;
    for (ci = cs.begin(); ci != cs.end(); ++ci) {
      (*ci)->pp(s, indent + indent_increment);
    }
    if (lf) {
      s << indent;
    }
  } else {
    s << contents;
  }

  s << "</" << name << ">";
  if (lf) {
    s << endl;
  }
}

/*!
  Pretty print the node.
*/
void XMLNode::pp(std::ostream& s) {
  static char quote = '\"';
  static bool lf = false;
  static std::string indent_increment = "";
  std::string indent = "";

  if (node_type == XMLNode::XML_COMMENT) {
    s << "<!--" << contents << "-->";
    return;
  }

  s << "<" << name;

  // Attributes
  //
  if ( ! attributes.empty()) {
    std::vector<AttVal*>::iterator si;
    std::vector<AttVal*> atts = attributes;
    for (si = atts.begin(); si != atts.end(); ++si) {
      AttVal *av = *si;
      s << " " << av->att << "=" << quote << av->val << quote;
    }
  }

  if ( (node_type == XMLNode::XML_LEAF) && (contents == "") ) {
    s << " />";
    return;
  }
  s << ">";
  
  // Children?
  //
  if ( ! children.empty()) {
    std::vector<XMLNode*>::iterator ci;
    std::vector<XMLNode*> cs = children;
    for (ci = cs.begin(); ci != cs.end(); ++ci) {
      (*ci)->pp(s);
    }
  } else {
    s << contents;
  }
  
  s << "</" << name << ">";
}

std::ostream& operator<<(std::ostream& s, XMLNode& x) {
  x.pp(s, "");
  return s;
}

std::ostream& operator<<(std::ostream& s, XMLDoc& x) {
  //
  // Print DOC info here.
  //
  s << "<?xml version=\"1.0\" encoding=\"" << x.encoding << "\" standalone=\"";
  if (x.standalone == 1) {
    s << "yes";
  } else {
    s << "no";
  }
  s << "\"?>" << endl;
  if (x.dtd != "") {
    s << "<!DOCTYPE " << x.dtd;
    if (x.pub_id != "") {
      s << " PUBLIC " << "\"" << x.pub_id << "\"";
    }
    if (x.sys_id != "") {
      s << " SYSTEM " << "\"" << x.sys_id << "\"";
    }
    s << ">" << endl;
  }

  XMLNode *rt = x.get_root();
  if (rt != NULL) {
    rt->pp(s, "");
  }
  return s;
}

std::string XMLNode::to_string() {
  std::ostringstream ostr;
  ostr << *this;
  return ostr.str();
}

// Siblings

/*!
  Get the first child of the node. Returns NULL if none.
*/
XMLNode *XMLNode::get_first_child() {
  current_child = 0;
  if ( ! children.empty()) {
    return children[0];
  }
  return NULL ;
}

/*!
  Get the first child of the node. Returns NULL if none.
*/
XMLNode *XMLNode::get_first_child(const std::string& n) {
  current_child = 0;
  for ( current_child = 0; current_child < children.size()-1; current_child++ ) {
    if ( children[current_child]->get_name() == n ) {
      return children[current_child];
    }
  }
  return NULL ;
}

/*!
  Get the "current" child - only makes sense from the \c get_first_child()
  and \c get_next_child() perspective.
*/
XMLNode *XMLNode::get_current_child() {
  if ( ! children.empty()) {
    return children[current_child];
  }
  return NULL ;
}

/*!
  Return the next child, or NULL if none.
*/
XMLNode *XMLNode::get_next_child() {
  if (current_child < children.size()-1) {
    ++current_child;
    return children[current_child];
  }
  return NULL ;
}

/*!
  Return the next child, or NULL if none.
*/
XMLNode *XMLNode::get_next_child(const std::string& n) {
  for (++current_child; current_child < children.size(); current_child++ ) {
    if ( children[current_child]->get_name() == n ) {
      return children[current_child];
    }
  }
  return NULL ;
}

/*!
  Get the first child of specified type of the node. Returns NULL if none.
*/
XMLNode *XMLNode::get_first_child(int type) {
  current_child = -1;
  int nc = children.size() - 1;
  while (current_child < nc) {
    ++current_child;
    if (children[current_child]->node_type == type) {
      return children[current_child];
    }
  }
  return NULL ;
}

/*!
  Return the next child of specified type, or NULL if none.
*/
XMLNode *XMLNode::get_next_child(int type) {
  while (current_child < children.size()-1) {
    ++current_child;
    if (children[current_child]->node_type == type) {
      return children[current_child];
    }
  }
  return NULL ;
}

/*!
  Breadth-first search for named node. Only first level children.
*/
std::string XMLNode::find_contents(const std::string& node_name) throw(XMLError) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == node_name) {
      return temp->contents ;
    }
    temp = get_next_child();
  }
  /*
  temp = get_first_child();
  while (temp != NULL) {
    std::string res = temp->find_contents(node_name);
    return res ;
  }
    temp = get_next_child();
  }
  */
  throw XMLError( "node not found ("+name+"/"+node_name+")" );
}

/*!
  Returns contents specified like "peter general redirect"
*/
std::string XMLNode::get_contents(const std::string& node_path) throw(XMLError) {
  std::string::size_type s_pos = 0;
  std::string::size_type s_pos_p = 0;
  XMLNode *tmp = this;

  while ( (s_pos = node_path.find( " ", s_pos_p )) != std::string::npos) {
    tmp = tmp->get_named_child(node_path.substr(s_pos_p, s_pos-s_pos_p));
    s_pos_p = s_pos+1;
  }
  return tmp->find_contents(node_path.substr(s_pos_p));
}

/*!
  Returns node specified like "peter general redirect"
*/
XMLNode *XMLNode::get_child(const std::string& node_path) throw(XMLError) {
  std::string::size_type s_pos = 0;
  std::string::size_type s_pos_p = 0;
  XMLNode *tmp = this;

  while ( (s_pos = node_path.find( " ", s_pos_p )) != std::string::npos) {
    tmp = tmp->get_named_child(node_path.substr(s_pos_p, s_pos-s_pos_p));
    s_pos_p = s_pos+1;
  }
  return tmp->get_named_child(node_path.substr(s_pos_p));
}


/*!
  Breadth-first search for named node.
  
  \note Returns default if not found.
*/
std::string XMLNode::find_contents(const std::string& node_name,
				   const std::string& deflt) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == node_name) {
      return temp->contents ;
    }
    temp = get_next_child();
  }
  temp = get_first_child();
  while (temp != NULL) {
    std::string res = temp->find_contents(node_name, deflt);
    if (res != "") {
      return res ;
    }
    temp = get_next_child();
  }
  return deflt ;
}

/*!
  Returns \c true of a named tag is present. Children only.
*/
bool XMLNode::tag_present(const std::string node_name) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == node_name) {
      return true ;
    }
    temp = get_next_child();
  }
  /*
  temp = get_first_child();
  while (temp != NULL) {
    bool res = temp->tag_present(node_name);
    if (res == true) {
      return true ;
    }
    temp = get_next_child();
  }
  */
  return false ;
}

/*!
  Returns a pointer to the first child, or throws error.
*/
XMLNode *XMLNode::get_named_child(const std::string a_name) throw (XMLError) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == a_name) {
      return temp ;
    }
    temp = get_next_child();
  }
  throw XMLError( "child not found ("+name+"/"+a_name+")" );
}

/*!
  Returns a pointer to the first child, or pointer.
*/
XMLNode *XMLNode::get_named_child(const std::string a_name, XMLNode* ptr) {
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == a_name) {
      return temp ;
    }
    temp = get_next_child();
  }
  return ptr;
}


/*!
  Returns a vector with child-pointers.

  NB! compare get_children()
*/
std::vector<XMLNode*> XMLNode::get_children(const std::string a_name) {
  std::vector<XMLNode*> res_v;
  XMLNode *temp = get_first_child();
  while (temp != NULL) {
    if (temp->name == a_name) {
      res_v.push_back(temp);
    }
    temp = get_next_child();
  }
  return res_v ;
}


// ----

/*!
  Add a comment to a node.

  /note Comments shouldn't be children. Rewrite.
*/
void XMLNode::add_comment(const std::string s) {
  XMLNode *cn   = new XMLNode("comment", s);
  cn->node_type = XMLNode::XML_COMMENT;
  children.push_back(cn);
}

XMLNode* XMLNode::get_option_by_ref(const std::string ref) {
  std::vector<XMLNode*> options = get_children("option");
  std::vector<XMLNode*>::const_iterator vi;
  for (vi = options.begin(); vi != options.end(); ++vi) {
    XMLNode *opt = *vi;
    string opt_ref = opt->find_contents("ref_number");
    if (opt_ref == ref) {
      return(opt);
    }
  }
  return(NULL);
}

// ----------------------------------------------------------------------------
// Removing stuff
// ----------------------------------------------------------------------------

void XMLNode::remove_named_children(const std::string& a_name) {
   std::vector<XMLNode*>::iterator iter = children.begin();
   const std::vector<XMLNode*>::iterator end = children.end();
   while (iter != end)
   {
        std::vector<XMLNode*>::iterator next = iter;
        ++next;
	if ((*iter)->name == a_name) {
	  //cerr << "removing\n";
	  delete (*iter);
	  children.erase(iter);
	}
        iter = next;
   }

  //children.erase(remove(children.begin(), children.end(),
  //		static_cast<XMLNode*>(0)), children.end() );
  
}

// ----------------------------------------------------------------------------
// Document
// ----------------------------------------------------------------------------

/*!
  Top node.
*/
XMLDoc::XMLDoc() {
  encoding   = "UTF-8"; //"iso-8859-1";
  node_type  = XMLDoc::XML_DOC;
  standalone = 1;
  root       = NULL;
}

XMLDoc::~XMLDoc() {
  if ( root ) {
    delete root;
  }
}

XMLDoc *XMLDoc::clone() {
  XMLDoc *new_doc = new XMLDoc();

  if (root != NULL) {
    new_doc->root = root->clone();
  } else {
    new_doc->root = NULL;
  }
  new_doc->encoding   = encoding;
  new_doc->node_type  = node_type;
  new_doc->standalone = standalone;

  return new_doc ;
}

/*!
  Returns a pointer to the root element of the document.

  Returns \c NULL is the XMLDoc is not a XML_DOC.
*/
XMLNode *XMLDoc::get_root() {
  return root ;
}

/*!
  Sets the root element of the document.
*/
void XMLDoc::set_root(XMLNode *new_root) {
  root = new_root;
}

std::string XMLDoc::find_contents(const std::string node_name) {
  return root->find_contents(node_name) ;
}

bool XMLDoc::tag_present(const std::string node_name) {
  return root->tag_present(node_name) ;
}

XMLNode *XMLDoc::get_named_child(const std::string a_name) {
  return root->get_named_child(a_name) ;
}

// ---------------------------------------------------------------------------
