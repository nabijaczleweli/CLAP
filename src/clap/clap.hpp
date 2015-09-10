#ifndef CLAP_CLAP_H
#define CLAP_CLAP_H

#include <vector>
#include <string>
#include <unordered_map>

class CLAP {
public:
  CLAP(const std::string info, int argc, char **argv);
  ~CLAP();
  bool        is_set(const std::string name);
  bool        get_bool_param(const std::string name, unsigned int index=0);
  int         get_int_param(const std::string name, unsigned int index=0);
  float       get_float_param(const std::string name, unsigned int index=0);
  std::string get_string_param(const std::string name, unsigned int index=0);

  void        print_help();
protected:
private:
  enum Type {
    bool_t,
    int_t,
    float_t,
    string_t
  };

  void error_usage(std::string errMsg);
  bool clean_param(std::string &param);

  void *_get_param(const std::string fname, CLAP::Type t,
		   const std::string pname, unsigned int index);

  bool parse_int(std::string &arg, int &val);
  bool parse_bool(std::string &arg, bool &val);
  bool parse_float(std::string &arg, float &val);

  struct Param {
    Param(const std::string info);
    ~Param();
    std::string name;
    Type  t;
    void *val;
  };

  struct Option {
    Option(const std::string info);
    ~Option();
    bool legal_name(std::string name);
    std::string name;
    std::string short_name;
    bool is_set;
    bool do_break;
    std::vector<Param> params;
  };

  std::string exec_name;
  std::vector<Option> options;
  std::unordered_map<std::string, unsigned int> map;
  
  std::vector<Param> params;
};

#endif // CLAP_CLAP_H
