#include <stdlib.h>
#include <iostream>

#include "clap/clap.hpp"

int main(int argc, char *argv[]) {
  const std::string info =
    "OPTIONS:              \n"
    "-v --version br       \n"
    "-k --kernel n:i err:s \n"
    "-b --bool   bo:s      \n"
    "PARAMETERS:           \n"
    "num_threads:i         \n";
  // i = integer, f = floating point, b = boolean, s = string

  CLAP c(info, argc, argv);

  if(c.is_set("v")) {
    printf("version\n");
    return 0;
  }

  if(c.is_set("b"))
    std::cout << "b has value: " << c.get_string_param("b") << std::endl;
  if(c.is_set("kernel"))
    std::cout << "kernel has value: " << c.get_int_param("k",0) << " and "
	      << c.get_string_param("k",1) << std::endl;

  std::cout << "Number of kernels: " << c.get_int_param("num_threads") << std::endl;

  return 0;
}
