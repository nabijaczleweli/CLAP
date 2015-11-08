#include <unordered_map>
#include <iostream>

#include "clap.hpp"

/////////////////////////////
// Config
#define USAGE_LINE_MAX_LENGTH 100
#define USAGE_MIN_DESC_WIDTH   15

/////////////////////////////
// Helper functions

/*
 * Tokenize a string (split by white space)
 */
void tokenize(const std::string string, std::vector<std::string> &tokens) {
  std::string cur("");
  unsigned int i;
  bool str = false;
  for(i = 0; string[i]; i++) {
    if(string[i] == '\'') {
      if(str) {
	cur += "'";
	tokens.push_back(cur);
	cur = "";
      }
      else {
	if(cur.length() > 0)
	  tokens.push_back(cur);
	cur = "'";
      }
      str = !str;
    }
    else if(str)
      cur += string[i];
    else if(isspace(string[i])) {
      if(cur.length() > 0)
	tokens.push_back(cur);
      cur = "";
    }
    else
      cur += string[i];
  }
  if(cur.length() > 0)
    tokens.push_back(cur);
}

/*
 * Trips a string (removes leading and trailing white space)
 */
void trim(std::string &string) {
  unsigned int b = 0, e = string.length();
  while(b < e && isspace(string[b])) b++;
  while(e > b && isspace(string[e-1])) e--;
  string = (e == b ? "" : string.substr(b,e));
}

/*
 * Checks if a string is a legal option/param name, i.e.
 * starts with a letter and contains only letters, numbers,
 * '-' and '_'
 */
bool legal_name(const std::string name) {
  unsigned int i;
  // First must be alpha
  if(name.length() == 0 || !isalpha(name[0]))
    return false;
  for(i = 1; i < name.length(); i++)
    if(!isalnum(name[i]) && name[i] != '-' && name[i] != '_')
      return false;
  return true;
}

/////////////////////////////
// Public functions

/* 
 * CLAP Constructor
 */
CLAP::CLAP(const std::string info, unsigned int argc, char **argv) : exec_name(argv[0]), desc("") {
  try {
    // Split info string
    std::vector<std::string> lines;
    std::string cur;
    unsigned int i, j, n;
    for(i = 0; info[i]; i++) {
      if(info[i]=='\n') {
	trim(cur);
	lines.push_back(cur);
	cur = "";
      }
      else {
	cur += info[i];
      }
    }
    if(cur.length() > 0) {
      trim(cur);
      lines.push_back(cur);
    }
  
    // Only use the name part of exec_name
    i = this->exec_name.length()-1;
    while(i >= 0 && this->exec_name[i] != '/') {i--;}
    this->exec_name = this->exec_name.substr(i+1);
  
    // Build description
    if(lines[0] == "DESCRIPTION:") {
      this->desc = "";
      for(i = 1; i < lines.size() && lines[i] != "OPTIONS:"; i++)
	this->desc += (i==1?"":"\n") + lines[i];
    }

    // Check first line is correct
    if(i == lines.size())
      throw SetupError("CLAP::CLAP - missing OPTIONS section");

    // Add default options
    CLAP::Option help("-h --help br 'Prints this help'");
    this->options.push_back(help);
    map.insert({"--"+help.name, 0});
    map.insert({"-"+help.short_name, 0});
  
    // Parse lines
    for(i++; i < lines.size() && lines[i] != "PARAMETERS:"; i++) {
      CLAP::Option o(lines[i]);
      std::string name = "--"+o.name, short_name = "-"+o.short_name;
      if(this->map.find(name)!=this->map.end() || this->map.find(short_name)!=this->map.end())
	throw SetupError("CLAP::CLAP - Same parameter defined twice ["+o.name+","+o.short_name+"]");
      this->options.push_back(o);
      map.insert({name, this->options.size()-1});
      map.insert({short_name, this->options.size()-1});
    }

    if(i == lines.size() || lines[i] != "PARAMETERS:")
      throw SetupError("CLAP::CLAP - missing PARAMETERS section");
    for(i++; i < lines.size(); i++) {
      std::vector<std::string> tokens;
      tokenize(lines[i], tokens);
    
      // Check that the pattern is not a redefinition
      for(j = 0; j < this->patterns.size(); j++)
	if(this->patterns[j].size() == tokens.size())
	  throw SetupError("CLAP::CLAP - same pattern defined twice");
    
      this->patterns.push_back(std::vector<CLAP::Param>());
      std::vector<CLAP::Param> &pattern = this->patterns.back();
    
      // Build pattern
      for(j = 0; j < tokens.size(); j++) {
	Param p(tokens[j]);
	pattern.push_back(p);
      }
    }
    
    // Parse options
    bool do_break = false;
    for(i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if(!this->is_option(arg))
	break;
    
      // Check if option is valid
      if(this->map.find(arg)==this->map.end())
	throw InputError("invalid option '"+std::string(argv[i])+"'");
    
      Option &o = this->options[this->map[arg]];
      o.is_set++;
      if(o.do_break) {
	do_break = true;
	break;
      }
      n = o.params.size();
      if(i+n >= argc)
	throw InputError("missing parameter(s) for option '"+arg+"'");
      for(j = 0; j < n; j++) {
	Param &p = o.params[j];
	std::string arg = argv[i+j+1];
	
	try { o.args.push_back(Value(p.t, arg)); }
	catch(InputError ie) { throw InputError("Option "+o.name+": parameter "+p.name+" "+ie.msg); }
      }
      i += j;
    }
  
    if(!do_break) {
      // Detect correct pattern
      this->sel_pattern = -1;
      for(j = 0; j < this->patterns.size(); j++)
	if(this->patterns[j].size() == argc-i) {
	  this->sel_pattern = j;
	  break;
	}
      if(this->sel_pattern == -1)
	throw InputError("wrong number of parameters");
      std::vector<CLAP::Param> &params = this->patterns[this->sel_pattern];
    
      // Parse remaining parameters
      for(j = 0; i < argc; i++, j++) {
	std::string param(argv[i]);
	Param &p = params[j];
      
	try { this->args.push_back(Value(p.t, param)); }
	catch(InputError ie) { throw InputError("Parameter "+p.name+" "+ie.msg); }
      }
    }
  }
  catch(InputError ie) {
    this->error_usage(ie.msg);
  }
  catch(SetupError se) { std::cerr << se.msg << std::endl; throw 2; }

  if(this->is_set("h")) {
    this->print_help();
    throw 0;
  }
}

/* 
 * CLAP Destructor
 */
CLAP::~CLAP() {
  
}

/* 
 * Implementation of CLAP::is_set(...)
 */
unsigned int CLAP::is_set(const std::string name) {
  std::string sname = "-"+name;
  // Check for short name
  if(this->map.find(sname)==this->map.end()) {
    // Check for long name
    sname = "-"+sname;
    if(this->map.find(sname)==this->map.end())
      throw SetupError("CLAP::is_set - invalid option '"+name+"'"); 
  }
  
  unsigned int i = this->map[sname];
  return this->options[i].is_set;
}

/* 
 * Implementation of CLAP::get_bool_param(...)
 */
bool CLAP::get_bool_param(const std::string name, unsigned int n, unsigned int m) {
  return *(bool*)this->_get_param("get_bool_param", CLAP::Type::bool_t, name, n, m);
}

/* 
 * Implementation of CLAP::get_int_param(...)
 */
int CLAP::get_int_param(const std::string name, unsigned int n, unsigned int m) {
  return *(int*)this->_get_param("get_int_param", CLAP::Type::int_t, name, n, m);
}

/* 
 * Implementation of CLAP::get_float_param(...)
 */
float CLAP::get_float_param(const std::string name, unsigned int n, unsigned int m) {
  return *(float*)this->_get_param("get_float_param", CLAP::Type::float_t, name, n, m);
}

/* 
 * Implementation of CLAP::get_string_param(...)
 */
std::string CLAP::get_string_param(const std::string name, unsigned int n, unsigned int m) {
  return *(std::string*)this->_get_param("get_string_param", CLAP::Type::string_t, name, n, m);
}

/*
 * Implementation of CLAP::get_chosen_pattern()
 */
int CLAP::get_chosen_pattern() {
  return this->sel_pattern;
}

/*
 * Implementation of CLAP::print_help()
 */
void CLAP::print_help() {
  unsigned int i, j;
  std::cout << "Usage: " << std::endl;
  for(i = 0; i < this->patterns.size(); i++) {
    std::cout << " " << this->exec_name << " [Options]";
    for(j = 0; j < this->patterns[i].size(); j++)
      std::cout << " <" << this->patterns[i][j].name << ">";
    std::cout << std::endl;
  }
  std::cout << std::endl;

  unsigned int max_width = USAGE_LINE_MAX_LENGTH;
  if(this->desc.length() > 0) {
    // Add desc
    std::string desc = this->desc;
    while(desc.length() > max_width) {
      for(j = 0; j < desc.length(); j++)
	if(desc[j] == '\n') {
	  std::cout << desc.substr(0,j) << std::endl;
	  desc = desc.substr(j+1);
	}
      
      if(desc.length() > max_width) {
	for(j = max_width; j > 0; j--) {
	  if(isspace(desc[j])) break;
	}
	j = j == 0 ? max_width : j;
	std::cout << desc.substr(0,j) << std::endl;
	desc = desc.substr(j+1);
      }
    }
    std::cout << desc << std::endl << std::endl;
  }
  
  // Options
  std::cout << "Options:" << std::endl;
  unsigned int l1 = 0, l2 = 0, l3 = 0, t;
  for(i = 0; i < this->options.size(); i++) {
    l1 = l1 > this->options[i].short_name.length() ? l1 : this->options[i].short_name.length();
    l2 = l2 > this->options[i].name.length() ? l2 : this->options[i].name.length();
    t = 0;
    for(j = 0; j < this->options[i].params.size(); j++)
      t += this->options[i].params[j].name.length()+2;
    l3 = l3 > t ? l3 : t;
  }
  l1 += 4;
  l2 += 4+l1;
  l3 += l2;
  
  max_width = max_width > l3+USAGE_MIN_DESC_WIDTH ? max_width : l3+USAGE_MIN_DESC_WIDTH;
  
  for(i = 0; i < this->options.size(); i++) {
    std::string line = " -"+this->options[i].short_name+"  ";
    while(line.length() < l1) line += " ";
    line += "--"+this->options[i].name+"  ";
    while(line.length() < l2) line += " ";
    for(j = 0; j < this->options[i].params.size(); j++)
      line += this->options[i].params[j].name+" ";
    while(line.length() < l3) line += " ";
    
    // Add desc
    std::string desc = this->options[i].desc;
    while(l3+desc.length() > max_width) {
      for(j = max_width-l3-1; j > 0; j--) {
	if(isspace(desc[j])) break;
      }
      j = j == 0 ? max_width-l3-1 : j;
      line += desc.substr(0, j)+"\n";
      desc = desc.substr(j+1);
      for(j = 0; j < l3; j++) line += " ";
    }
    line += desc;
    std::cout << line << std::endl;
  }
}

/**
 * Implementation of CLAP::error_usage(...)
 */
void CLAP::error_usage(std::string msg) {
  std::cerr << this->exec_name << ": " << msg << std::endl
	    << "Try '" << this->exec_name << " --help' for more information." << std::endl;
  throw 1;
}
  

/////////////////////////////
// Private functions

/*
 * Implementation of CLAP::is_option(...)
 */
bool CLAP::is_option(std::string &param) {
  if(param.size() < 2)
    return false;
  if(param[0] == '-') {
    return legal_name(param.substr(param[1] == '-' ? 2 : 1));
  }
  // Param is mandatory or error
  return false;
}

/*
 * Implementation of CLAP::_get_param(...)
 */
void *CLAP::_get_param(const std::string fname, CLAP::Type t,
		       const std::string pname, unsigned int n,
		       unsigned int m) {
  Param *p = NULL;
  void  *v = NULL;

  bool is_option = true;
  std::string sname = "-"+pname;
  // Check for short name
  if(this->map.find(sname)==this->map.end()) {
    // Check for long name
    sname = "-"+sname;
    if(this->map.find(sname)==this->map.end())
      is_option = false;
  }
  if(!is_option) {
    // Search through mandatory params (if no break)
    if(this->sel_pattern != -1) {
      std::vector<CLAP::Param> &params = this->patterns[this->sel_pattern];
      for(unsigned int i = 0; i < params.size(); i++) {
	if(params[i].name == pname) {
	  p = &params[i];
	  v = this->args[i].val;
	  break;
	}
      }
    }
    if(p == NULL)
      throw SetupError("CLAP::"+fname+" - invalid option '"+pname+"'");
  }
  else {
    // Option found
    Option &o = this->options[this->map[sname]];
    unsigned int k = o.params.size();
    // Check index
    if(n >= k)
      throw SetupError("CLAP::"+fname+" - invalid n for option '"+pname+"'");
    if(m >= o.is_set)
      throw SetupError("CLAP::"+fname+" - invalid m for option '"+pname+"'");
    p = &o.params[n];
    v = o.args[m*k+n].val;
  }
  if(p->t != t)
    throw SetupError("CLAP::"+fname+" - wrong type");
  return v;
}

/*
 * CLAP::Option constructor
 */
CLAP::Option::Option(const std::string info) : is_set(0), do_break(false) {
  std::vector<std::string> tokens;
  unsigned int i;
  tokenize(info, tokens);
  if(tokens.size() < 3)
    throw SetupError("Option::Option - missing parameter");
  
  if(tokens[0].length() < 2 || tokens[0][0] != '-' || !legal_name(tokens[0].substr(1)))
    throw SetupError("Option::Option - error in short name '"+tokens[0]+"'");
  if(tokens[1].length() < 3 || tokens[1][0] != '-'
     || tokens[1][1] != '-' || !legal_name(tokens[1].substr(2)))
    throw SetupError("Option::Option - error in long name '"+tokens[1]+"'");
  this->short_name = tokens[0].substr(1);
  this->name       = tokens[1].substr(2);
  
  i = 2;
  if(tokens.size() > 3 && tokens[2] == "br") {
    this->do_break = true;
    i++;
  }
  for(; i < tokens.size()-1; i++) {
    CLAP::Param p(tokens[i]);
    this->params.push_back(p);
  }
  std::string &desc = tokens[i];
  if(desc.length() < 2 || desc[0] != '\'' || desc[desc.length()-1] != '\'')
    throw SetupError("Option::Option - error in description format - missing ' delimiters");
  this->desc = desc.substr(1,desc.length()-2);
}

/*
 * CLAP::Option destructor
 */
CLAP::Option::~Option() { }

/*
 * CLAP::Param constructor
 */
CLAP::Param::Param(const std::string info) {
  unsigned int l = info.length();
  this->name = info.substr(0,l-2);
  if(!legal_name(this->name))
    throw SetupError("Param::Param - invalid parameter name '"+name+"'");
  if(info[l-2] != ':')
    throw SetupError("Param::Param - parsing error, missing ':' in '"+info+"'");
  switch(info[l-1]) {
  case 'i': this->t = CLAP::Type::int_t;    break;
  case 'f': this->t = CLAP::Type::float_t;  break;
  case 'b': this->t = CLAP::Type::bool_t;   break;
  case 's': this->t = CLAP::Type::string_t; break;
  default: throw SetupError("Param::Param - Unknown type: '"+info+"'");
  }
}

/*
 * CLAP::Param destructor
 */
CLAP::Param::~Param() { }

/*
 * CLAP::Value constructor
 */
CLAP::Value::Value(Type &t, std::string &param) {
  this->t   = t;
  this->val = NULL;
  switch(t) {
  case CLAP::Type::int_t:   this->parse_int(param);   break;
  case CLAP::Type::bool_t:  this->parse_bool(param);  break;
  case CLAP::Type::float_t: this->parse_float(param); break;
  case CLAP::Type::string_t:
    this->val = new std::string(param);
    break;
    default: throw SetupError("CLAP::CLAP - Unknown type: '"+param+"'");
  }
}

/*
 * CLAP::Value copy-constructor
 */
CLAP::Value::Value(const CLAP::Value &other) {
  this->t   = other.t;
  this->val = NULL;
  switch(t) {
  case CLAP::Type::int_t:
    this->val = new int(*((int*)other.val));
    break;
  case CLAP::Type::bool_t:
    this->val = new bool(*((bool*)other.val));
    break;
  case CLAP::Type::float_t:
    this->val = new float(*((float*)other.val));
    break;
  case CLAP::Type::string_t:
    this->val = new std::string(*((std::string*)other.val));
    break;
  }
}

/*
 * CLAP::Value destructor
 */
CLAP::Value::~Value() {
  if(this->val != NULL) {
    switch(this->t) {
    case CLAP::Type::int_t:
      delete (int*)this->val;
      break;
    case CLAP::Type::bool_t:
      delete (bool*)this->val;
      break;
    case CLAP::Type::float_t:
      delete (float*)this->val;
      break;
    case CLAP::Type::string_t:
      delete (std::string*)this->val;
      break;
    }
  }
}

/*
 * Implementation of CLAP::Value::parse_int(...)
 */
void CLAP::Value::parse_int(std::string &arg) {
  if(arg.size() == 0)
    throw InputError("must be an integer");
  unsigned int i = (arg[0] == '-' ? 1 : 0);
  if(arg.size() == i)
    // Do not allow '-'
    throw InputError("must be an integer");
  if(arg[i] == '0' && arg.size() > i+1)
    // No leading zeros
    throw InputError("must be an integer");
  for(; i < arg.length(); i++)
    if(arg[i] < '0' || arg[i] > '9')
      throw InputError("must be an integer");
  this->val = new int(std::stoi(arg));
}

/*
 * Implementation of CLAP::Value::parse_bool(...)
 */
void CLAP::Value::parse_bool(std::string &arg) {
  if(arg.length() != 1 || (arg[0] != '0' && arg[0] != '1'))
    throw InputError("must be a boolean (0 or 1)");
  this->val = new bool(arg[0] == '1');
}

/*
 * Implementation of CLAP::Value::parse_float(...)
 */
void CLAP::Value::parse_float(std::string &arg) {
  if(arg.size() == 0)
    throw InputError("must be a float");
  unsigned int i = (arg[0] == '-' ? 1 : 0);
  if(arg.size() == i)
    // Do not allow '-'
    throw InputError("must be a float");
  for(; i < arg.length(); i++) {
    if((arg[i] < '0' || arg[i] > '9') && arg[i] != '.')
      throw InputError("must be a float");
  }
  this->val = new float(std::stof(arg));
}

/*
 * Implementation of CLAP::InputError constructor
 */
CLAP::InputError::InputError(std::string msg) : msg(msg) { }

/*
 * Implementation of CLAP::InputError destructor
 */
CLAP::InputError::~InputError() { }

/*
 * Implementation of CLAP::SetupError constructor
 */
CLAP::SetupError::SetupError(std::string msg) : msg(msg) { }

/*
 * Implementation of CLAP::SetupError destructor
 */
CLAP::SetupError::~SetupError() { }
