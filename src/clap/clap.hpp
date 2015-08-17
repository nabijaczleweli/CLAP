#ifndef CLAP_CLAP_H
#define CLAP_CLAP_H

#include <vector>
#include <string>
#include <unordered_map>

class CLAP {
public:
  CLAP(char *info);
  ~CLAP();
  bool       *is_set(char *name);
  void       *get_param(char *name, int index=0);
  bool        get_bool_param(char *name, int index=0);
  int         get_int_param(char *name, int index=0);
  float       get_float_param(char *name, int index=0);
  std::string get_string_param(char *name, int index=0);
protected:
private:
  enum type {
    bool_t,
    int_t,
    float_t,
    string_t
  };
  
  std::unordered_map<char*, std::vector<type>> map;
};

#endif // CLAP_CLAP_H
