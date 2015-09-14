#include <unordered_map>
#include <iostream>

#include "clap.hpp"

/////////////////////////////
// Helper functions

/*
 * Tokenize a string (split by white space)
 */
void tokenize(const std::string string, std::vector<std::string> &tokens) {
  std::string cur("");
  unsigned int i;
  for(i = 0; string[i]; i++) {
    if(isspace(string[i])) {
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
 * Displays error message and exits.
 * To be used on setup error.
 */
void error_setup(const std::string errMsg) {
  std::cout << "CLAP setup error: " << errMsg << std::endl;
  exit(1);
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
CLAP::CLAP(const std::string info, unsigned int argc, char **argv) : exec_name(argv[0]) {
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
  
  // Check first line is correct
  if(lines[0] != "OPTIONS:")
    error_setup("CLAP::CLAP - missing OPTIONS section");

  // Add default options
  CLAP::Option help("-h --help br");
  this->options.push_back(help);
  map.insert({"--"+help.name, 0});
  map.insert({"-"+help.short_name, 0});
  
  // Parse lines
  for(i = 1; i < lines.size() && lines[i] != "PARAMETERS:"; i++) {
    CLAP::Option o(lines[i]);
    std::string name = "--"+o.name, short_name = "-"+o.short_name;
    if(this->map.find(name)!=this->map.end() || this->map.find(short_name)!=this->map.end())
      error_setup("CLAP::CLAP - Same parameter defined twice ["+o.name+","+o.short_name+"]");
    this->options.push_back(o);
    map.insert({name, this->options.size()-1});
    map.insert({short_name, this->options.size()-1});
  }

  if(i == lines.size() || lines[i] != "PARAMETERS:")
    error_setup("CLAP::CLAP - missing PARAMETERS section");
  for(i++; i < lines.size(); i++) {
    std::vector<std::string> tokens;
    tokenize(lines[i], tokens);
    
    // Check that the pattern is not a redefinition
    for(j = 0; j < this->patterns.size(); j++)
      if(this->patterns[j].size() == tokens.size())
	error_setup("CLAP::CLAP - same pattern defined twice");
    
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
      this->error_usage("invalid option '"+std::string(argv[i])+"'");
    
    Option &o = this->options[this->map[arg]];
    o.is_set++;
    if(o.do_break) {
      do_break = true;
      break;
    }
    n = o.params.size();
    if(i+n >= argc)
      this->error_usage("missing parameter(s) for option '"+arg+"'");
    for(j = 0; j < n; j++) {
      Param &p = o.params[j];
      void *val = NULL;
      std::string arg = argv[i+j+1];
      switch(p.t) {
      case CLAP::Type::int_t:
	val = new int();
	if(!this->parse_int(arg, *(int*)val))
	  this->error_usage("invalid integer argument '"+arg+"' for option '"+o.name+"' - must be an integer");
	break;
      case CLAP::Type::bool_t:
	val = new bool();
	if(!this->parse_bool(arg, *(bool*)val))
	  this->error_usage("invalid boolean argument '"+arg+"' for option '"+o.name+"' - must be either 0 or 1");
	break;
      case CLAP::Type::float_t:
	val = new float();
	if(!this->parse_float(arg, *(float*)val))
	  this->error_usage("invalid float argument '"+arg+"' for option '"+o.name+"' - must be a floating point value");
	break;
      case CLAP::Type::string_t:
	val = new std::string(arg);
	break;
      }
      o.args.push_back(val);
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
      this->error_usage("wrong number of parameters");
    std::vector<CLAP::Param> &params = this->patterns[this->sel_pattern];
    
    // Parse remaining parameters
    for(j = 0; i < argc; i++, j++) {
      std::string param(argv[i]);
      Param &p = params[j];
      void* val = NULL;
      switch(p.t) {
      case CLAP::Type::int_t:
	val = new int();
	if(!this->parse_int(param, *(int*)val))
	  this->error_usage("invalid integer value '"+param+"' for argument '"+p.name+"'");
	break;
      case CLAP::Type::bool_t:
	val = new bool();
	if(!this->parse_bool(param, *(bool*)val))
	  this->error_usage("invalid boolean value '"+param+"' for argument '"+p.name+"' - must be either 0 or 1");
	break;
      case CLAP::Type::float_t:
	val = new float();
	if(!this->parse_float(param, *(float*)val))
	  this->error_usage("invalid float value '"+param+"' for argument '"+p.name+"'");
	break;
      case CLAP::Type::string_t:
	val = new std::string(param);
	break;
      default: error_setup("CLAP::CLAP - Unknown type: '"+param+"'");
      }
      this->args.push_back(val);
    }
  }
  
  if(this->is_set("h")) {
    this->print_help();
    exit(0);
  }
}

/* 
 * CLAP Destructor
 */
CLAP::~CLAP() { }

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
      error_setup("CLAP::is_set - invalid option '"+name+"'"); 
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
  
  // Options
  std::cout << "Options:" << std::endl;
  for(i = 0; i < this->options.size(); i++) {
    std::cout << " -" << this->options[i].short_name
	      << "\t--" << this->options[i].name << " ";
    for(j = 0; j < this->options[i].params.size(); j++)
      std::cout << "<" << this->options[i].params[j].name << "> ";
    std::cout << std::endl;
  }
}

/////////////////////////////
// Private functions

/*
 * Implementation of CLAP::error_usage(...)
 */
void CLAP::error_usage(std::string errMsg) {
  std::cout << this->exec_name << ": " << errMsg << std::endl
	    << "Try '" << this->exec_name << " --help' for more information." << std::endl;
  exit(1);
}

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
	  v = this->args[i];
	  break;
	}
      }
    }
    if(p == NULL)
      error_setup("CLAP::"+fname+" - invalid option '"+pname+"'");
  }
  else {
    // Option found
    Option &o = this->options[this->map[sname]];
    unsigned int k = o.params.size();
    // Check index
    if(n >= k)
      error_setup("CLAP::"+fname+" - invalid n for option '"+pname+"'");
    if(m >= o.is_set)
      error_setup("CLAP::"+fname+" - invalid m for option '"+pname+"'");
    p = &o.params[n];
    v = o.args[m*k+n];
  }
  if(p->t != t)
    error_setup("CLAP::"+fname+" - wrong type");
  return v;
}

/*
 * Implementation of CLAP::parse_int(...)
 */
bool CLAP::parse_int(std::string &arg, int &val) {
  unsigned int i = (arg[0] == '-' ? 1 : 0);
  if(arg.size() == i)
    // Do not allow '-'
    return false;
  if(arg[i] == '0' && arg.size() > i+1)
    // No leading zeros
    return false;
  for(; i < arg.length(); i++)
    if(arg[i] < '0' || arg[i] > '9')
      return false;
  val = std::stoi(arg);
  return true;
}

/*
 * Implementation of CLAP::parse_bool(...)
 */
bool CLAP::parse_bool(std::string &arg, bool &val) {
  if(arg.length() != 1 || (arg[0] != '0' && arg[0] != '1'))
    return false;
  val = (arg[0] == '1');
  return true;
}

/*
 * Implementation of CLAP::parse_float(...)
 */
bool CLAP::parse_float(std::string &arg, float &val) {
  unsigned int i = (arg[0] == '-' ? 1 : 0);
  if(arg.size() == i)
    // Do not allow '-'
    return false;
  for(; i < arg.length(); i++) {
    if((arg[i] < '0' || arg[i] > '9') && arg[i] != '.')
      return false;
  }
  val = std::stof(arg);
  return true;
}

/*
 * CLAP::Option constructor
 */
CLAP::Option::Option(const std::string info) : is_set(0), do_break(false) {
  std::vector<std::string> tokens;
  unsigned int i;
  tokenize(info, tokens);
  if(tokens.size() < 2)
    error_setup("Option::Option - missing long name");

  if(tokens[0].length() < 2 || tokens[0][0] != '-' || !legal_name(tokens[0].substr(1)))
    error_setup("Option::Option - error in short name '"+tokens[0]+"'");
  if(tokens[1].length() < 3 || tokens[1][0] != '-'
     || tokens[1][1] != '-' || !legal_name(tokens[1].substr(2)))
    error_setup("Option::Option - error in long name '"+tokens[1]+"'");
  this->short_name = tokens[0].substr(1);
  this->name       = tokens[1].substr(2);
  
  i = 2;
  if(tokens.size() > 2 && tokens[2] == "br") {
    this->do_break = true;
    i++;
  }
  for(; i < tokens.size(); i++) {
    CLAP::Param p(tokens[i]);
    this->params.push_back(p);
  }
}

/*
 * CLAP::Option destructor
 */
CLAP::Option::~Option() {
  for(unsigned int i = 0; i < this->args.size(); i++) {
    if(this->args[i] != NULL) {
      switch(this->params[i].t) {
      case CLAP::Type::int_t:
	delete (int*)this->args[i];
	break;
      case CLAP::Type::bool_t:
	delete (bool*)this->args[i];
	break;
      case CLAP::Type::float_t:
	delete (float*)this->args[i];
	break;
      case CLAP::Type::string_t:
	delete (std::string*)this->args[i];
	break;
      }
    }
  }
}

/*
 * CLAP::Param constructor
 */
CLAP::Param::Param(const std::string info) {
  unsigned int l = info.length();
  this->name = info.substr(0,l-2);
  if(!legal_name(this->name))
    error_setup("Param::Param - invalid parameter name '"+name+"'");
  if(info[l-2] != ':')
    error_setup("Param::Param - parsing error, missing ':' in '"+info+"'");
  switch(info[l-1]) {
  case 'i': this->t = CLAP::Type::int_t;    break;
  case 'f': this->t = CLAP::Type::float_t;  break;
  case 'b': this->t = CLAP::Type::bool_t;   break;
  case 's': this->t = CLAP::Type::string_t; break;
  default: error_setup("Param::Param - Unknown type: '"+info+"'");
  }
}

/*
 * CLAP::Param destructor
 */
CLAP::Param::~Param() { }
