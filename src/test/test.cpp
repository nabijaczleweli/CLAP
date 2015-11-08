#include <stdlib.h>
#include <iostream>

#include "clap/clap.hpp"

int main(int argc, char *argv[]) {
  try {
    const std::string info =
      "DESCRIPTION:                       \n"
      "This is a test of the CLAP library.\n"
      "  Try it out...                    \n"
      "OPTIONS:                           \n"
      "-v --version br        'print version' \n"
      "-k --kernel n:i err:s  'Set kernel and error message' \n"
      "-b --bool   bo:s       ''          \n"
      "PARAMETERS:                        \n"
      "                                   \n"
      "num_threads:i                      \n"
      "my_name:s over_18:b                \n";
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
    if(c.is_set("kernel") == 2)
      std::cout << "kernel 2 has value: " << c.get_int_param("k",0,1) << " and "
		<< c.get_string_param("k",1,1) << std::endl;
    if(c.is_set("kernel") == 3)
      std::cout << "kernel 3 has value: " << c.get_int_param("k",0,2) << " and "
		<< c.get_string_param("k",1,2) << std::endl;
  
    std::cout << "Chosen pattern: " << c.get_chosen_pattern() << std::endl;

    if(c.get_chosen_pattern() == 1)
      std::cout << "Number of kernels: " << c.get_int_param("num_threads") << std::endl;

    std::cout << "Almost done" << std::endl;

    return 0;
  } catch(int e) { return e; }
}
