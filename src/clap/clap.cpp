#include <unordered_map>
#include <iostream>

#include "clap.hpp"

/////////////////////////////
// Helper functions

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

void error_setup(const std::string errMsg) {
  std::cout << "CLAP setup error: " << errMsg << std::endl;
  exit(1);
}

void trim(std::string &string) {
  unsigned int b = 0, e = string.length();
  while(isspace(string[b])) b++;
  while(isspace(string[e-1])) e--;
  string = (e <= b ? "" : string.substr(b,e));
}

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
 * Constructor
 */
CLAP::CLAP(const std::string info, int argc, char **argv) : exec_name(argv[0]) {
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
  map.insert({help.name, 0});
  map.insert({help.short_name, 0});
  
  // Parse lines
  for(i = 1; i < lines.size() && lines[i] != "PARAMETERS:"; i++) {
    CLAP::Option o(lines[i]);
    if(this->map.find(o.name)!=this->map.end() || this->map.find(o.short_name)!=this->map.end())
      error_setup("CLAP::CLAP - Same parameter defined twice ["+o.name+","+o.short_name+"]");
    this->options.push_back(o);
    map.insert({o.name, this->options.size()-1});
    map.insert({o.short_name, this->options.size()-1});
  }

  if(i == lines.size() || lines[i] != "PARAMETERS:")
    error_setup("CLAP::CLAP - missing PARAMETERS section");
  for(i++; i < lines.size(); i++) {
    Param p(lines[i]);
    this->params.push_back(p);
  }
  
  // Parse arguments
  bool do_break = false;
  for(i = 1; i < (unsigned int)argc; i++) {
    std::string arg(argv[i]);
    if(!this->clean_param(arg))
      break;
    
    // Check if option is valid
    if(this->map.find(arg)==this->map.end())
      this->error_usage("invalid option '"+std::string(argv[i])+"'");
    
    Option &o = this->options[this->map[arg]];
    o.is_set = true;
    if(o.do_break) {
      do_break = true;
      break;
    }
    n = o.params.size();
    if(i+n >= (unsigned int)argc)
      this->error_usage("missing parameter(s) for option '"+arg+"'");
    for(j = 0; j < n; j++) {
      Param &p = o.params[j];
      std::string arg = argv[i+j+1];
      switch(p.t) {
      case CLAP::Type::int_t:
	p.val = new int();
	if(!this->parse_int(arg, *(int*)p.val))
	  this->error_usage("invalid integer argument '"+arg+"' for option '"+o.name+"' - must be an integer");
	break;
      case CLAP::Type::bool_t:
	p.val = new bool();
	if(!this->parse_bool(arg, *(bool*)p.val))
	  this->error_usage("invalid boolean argument '"+arg+"' for option '"+o.name+"' - must be either 0 or 1");
	break;
      case CLAP::Type::float_t:
	p.val = new float();
	if(!this->parse_float(arg, *(float*)p.val))
	  this->error_usage("invalid float argument '"+arg+"' for option '"+o.name+"' - must be a floating point value");
	break;
      case CLAP::Type::string_t:
	p.val = new std::string(arg);
	break;
      }
    }
    i += j;
  }
  
  if(!do_break) {
    if((unsigned int)argc-i < this->params.size())
      this->error_usage("missing mandatory parameters");
    // Parse remaining parameters
    for(j = 0; i < (unsigned int)argc; i++, j++) {
      std::string param(argv[i]);
      Param &p = this->params[j];
      switch(p.t) {
      case CLAP::Type::int_t:
	p.val = new int();
	if(!this->parse_int(param, *(int*)p.val))
	  this->error_usage("invalid integer value '"+param+"' for argument '"+p.name+"'");
	break;
      case CLAP::Type::bool_t:
	p.val = new bool();
	if(!this->parse_bool(param, *(bool*)p.val))
	  this->error_usage("invalid boolean value '"+param+"' for argument '"+p.name+"' - must be either 0 or 1");
	break;
      case CLAP::Type::float_t:
	p.val = new float();
	if(!this->parse_float(param, *(float*)p.val))
	  this->error_usage("invalid float value '"+param+"' for argument '"+p.name+"'");
	break;
      case CLAP::Type::string_t:
	p.val = new std::string(param);
	break;
      default: error_setup("CLAP::CLAP - Unknown type: '"+param+"'");
      }
    }
  }
  
  if(this->is_set("h")) {
    this->print_help();
    exit(0);
  }
}

/* 
 * Destructor
 */
CLAP::~CLAP() { }

/* 
 * 
 */
bool CLAP::is_set(const std::string name) {
  if(this->map.find(name)==this->map.end())
    error_setup("CLAP::is_set - invalid option '"+name+"'");
  
  unsigned int i = this->map[name];
  return this->options[i].is_set;
}

/* 
 * 
 */
bool CLAP::get_bool_param(const std::string name, unsigned int index) {
  return *(bool*)this->_get_param("get_bool_param", CLAP::Type::bool_t, name, index);
}

/* 
 * 
 */
int CLAP::get_int_param(const std::string name, unsigned int index) {
  return *(int*)this->_get_param("get_int_param", CLAP::Type::int_t, name, index);
}

/* 
 * 
 */
float CLAP::get_float_param(const std::string name, unsigned int index) {
  return *(float*)this->_get_param("get_float_param", CLAP::Type::float_t, name, index);
}

/* 
 * 
 */
std::string CLAP::get_string_param(const std::string name, unsigned int index) {
  return *(std::string*)this->_get_param("get_string_param", CLAP::Type::string_t, name, index);
}

void CLAP::print_help() {
  unsigned int i, j;
  std::cout << "Usage: " << this->exec_name << " [Options]";
  for(i = 0; i < this->params.size(); i++)
    std::cout << " <" << this->params[i].name << ">";
  std::cout << std::endl << std::endl;
  
  // Options
  std::cout << "Options:" << std::endl;
  for(i = 0; i < this->options.size(); i++) {
    std::cout << " -" << this->options[i].short_name
	      << "\t--" << this->options[i].name << " ";
    for(j = 0; j < this->options[i].params.size(); j++) {
      std::cout << this->options[i].params[j].name << " ";
      /*std::cout << "(";
      switch(this->options[i].params[j].t) {
      case CLAP::Type::int_t:    std::cout << "integer";        break;
      case CLAP::Type::bool_t:   std::cout << "0|1";            break;
      case CLAP::Type::float_t:  std::cout << "floating point"; break;
      case CLAP::Type::string_t: std::cout << "string";         break;
      default: break;
      }
      std::cout << "), " << std::endl;*/
    }
    std::cout << std::endl;
  }
}

/////////////////////////////
// Private functions

void CLAP::error_usage(std::string errMsg) {
  std::cout << this->exec_name << ": " << errMsg << std::endl
	    << "Try '" << this->exec_name << " --help' for more information." << std::endl;
  exit(1);
}

bool CLAP::clean_param(std::string &param) {
  if(param[0] == '-') {
    param = param.substr(param[1] == '-' ? 2 : 1);
    return true;
  }
  // Param is mandatory or error
  return false;
}

void *CLAP::_get_param(const std::string fname, CLAP::Type t,
		       const std::string pname, unsigned int index) {
  Param *p = NULL;
  
  if(this->map.find(pname)==this->map.end()) {
    // Search through mandatory params
    for(unsigned int i = 0; i < this->params.size(); i++) {
      if(this->params[i].name == pname) {
	p = &this->params[i];
	break;
      }
    }
    if(p == NULL)
      error_setup("CLAP::"+fname+" - invalid option '"+pname+"'");
  }
  else {
    // Option found
    Option &o = this->options[this->map[pname]];
    // Check index
    if(index >= o.params.size())
      error_setup("CLAP::"+fname+" - invalid index for option '"+pname+"'");
    p = &o.params[index];
  }
  if(p->t != t)
    error_setup("CLAP::"+fname+" - wrong type");
  return p->val;
}


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

bool CLAP::parse_bool(std::string &arg, bool &val) {
  if(arg.length() != 1 || (arg[0] != '0' && arg[0] != '1'))
    return false;
  val = (arg[0] == '1');
  return true;
}

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

CLAP::Option::Option(const std::string info) : is_set(false), do_break(false) {
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

CLAP::Option::~Option() { }

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
  this->val = NULL;
}

CLAP::Param::~Param() {
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
