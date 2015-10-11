#ifndef CLAP_CLAP_H
#define CLAP_CLAP_H

#include <vector>
#include <string>
#include <unordered_map>

/**
 * @class CLAP
 * @author Stephan Lorenzen
 *
 * The [C]ommand [L]ine [A]rgument [P]arser, used for easily configuring a program
 * for parsing command line arguments.
 */
class CLAP {
public:
  /**
   * CLAP Constructor
   * Constructs the CLAP from a configuration string and the input arguments.
   * The configuration string must have the following format: 
   *
   *       +--------------------------------------------------------+
   *       | OPTIONS:                                               |
   *       | -o1 --option1 [br]  [var1:(i|f|b|s)] [var2: ...] ...   |
   *       |  .                                                     |
   *       |  .                                                     |
   *       |  .                                                     |
   *       | PARAMETERS:                                            |
   *       | par1:(i|f|b|s) [par2:(i|f|b|s)] ...                    |
   *       |  .                                                     |
   *       |  .                                                     |
   *       |  .                                                     |
   *       +--------------------------------------------------------+
   *
   * Options are defined in the OPTIONS section.
   * 'o1' is the short name for an option, 'option1' is the long name for that same option.
   * 'br' is optional - if set, the parser will stop without error when encountering this option.
   * 'var1', 'var2', ... are the (optional) parameters for the option. The type follows the ':'
   * and must be either 'i' = integer, 'f' = float, 'b' = boolean, 's' = string.
   * 
   * Mandatory parameters are defined in the PARAMETERS section. Here, several accepted patterns
   * can be defined. Leave an empty line (or no lines) for no parameters.
   * 'par1' is the name of the first parameter. Again, the type is specified after the ':'.
   *
   * Both the OPTIONS and the PARAMETERS sections may be left empty.
   *
   * @param info The configuration string. This string must have the format given above.
   * @param argc Size of argv array.
   * @param argv Array containing command line arguments.
   */
  CLAP(const std::string info, unsigned int argc, char **argv);
  
  /**
   * Destructor
   * Cleans up the CLAP structure.
   */
  ~CLAP();

  /**
   * Checks if an option/parameter is set.
   *
   * @param name The name of the option/parameter
   * @return integer The number of times the parameter has been passed
   */
  unsigned int is_set(const std::string name);

  /**
   * Gets a boolean parameter of some option.
   *
   * @param name The name of the option
   * @param n The n'th parameter of the option
   * @param m The m'th occurence of the option
   * @return bool The value given
   */
  bool get_bool_param(const std::string name, unsigned int n=0, unsigned int m=0);
  
  /**
   * Gets an integer parameter of some option.
   *
   * @param name The name of the option
   * @param n The n'th parameter of the option
   * @param m The m'th occurence of the option
   * @return int The value given
   */
  int get_int_param(const std::string name, unsigned int n=0, unsigned int m=0);
  
  /**
   * Gets a floating point parameter of some option.
   *
   * @param name The name of the option
   * @param n The n'th parameter of the option
   * @param m The m'th occurence of the option
   * @return float The value given
   */
  float get_float_param(const std::string name, unsigned int n=0, unsigned int m=0);
  
  /**
   * Gets a string parameter of some option.
   *
   * @param name The name of the option
   * @param n The n'th parameter of the option
   * @param m The m'th occurence of the option
   * @return string The value given
   */
  std::string get_string_param(const std::string name, unsigned int n=0, unsigned int m=0);

  /**
   * Get the chosen pattern. If a break has occured, -1 is returned.
   *
   * @return int The chosen pattern
   */
  int get_chosen_pattern();

  /**
   * Prints the help text.
   */
  void print_help();

  /**
   * Prints a usage error
   */
  void error_usage(std::string msg);
  
protected:
private:
  /**
   * Parameter types.
   */
  enum Type {
    bool_t,
    int_t,
    float_t,
    string_t
  };
  
  /**
   * Checks if a string is an option
   * 
   * @param arg The argument to be cleaned up.
   * @return True if '-' or '--' removed, otherwise false.
   */
  bool is_option(std::string &arg);

  /**
   * Returns a void pointer to the value of the parameter.
   *
   * @param fname Function name to print in error message on error.
   * @param t Expected type of value.
   * @param pname Name of the option
   * @param m The m'th occurence of the option 
   * @param n The n'th parameter of the option
   * @return void* Pointer to the value of this parameter
   */
  void *_get_param(const std::string fname, CLAP::Type t,
		   const std::string pname, unsigned int n,
		   unsigned int m);

  /**
   * @struct CLAP::Param
   *
   * Models parameters in CLAP
   */
  struct Param {
    /**
     * Constructor for CLAP::Param. Constructs the parameter from a
     * string formatted as follows: 'name:(i|f|b|s)', where 'name'
     * is the name of the parameter and one of the types 'i' = integer,
     * 'f' = float, 'b' = boolean, 's' = string, must be chosen.
     *
     * @param info The input string, formatted as shown above.
     */
    Param(const std::string info);
    
    /**
     * Destructor for CLAP::Param
     * Cleans up the Param structure.
     */
    ~Param();
    
    /** Name of the parameter */
    std::string name;
    /** Type of the parameter */
    Type  t;
  };

  /**
   * @struct Value
   *
   * Models values in CLAP
   */
  struct Value {
    /**
     * Constructor for CLAP::Value. Sets the type and value,
     * parsed from the given param string.
     *
     * @param t The type of this value.
     * @param param The input parameter.
     */
    Value(Type &t, std::string &param);
    
    /**
     * Copy constructor for CLAP::Value
     *
     * @param other The other value.
     */
    Value(const Value &other);
    
    /**
     * Destructor for value
     */
    ~Value();
    
    /**
     * Parses an integer from a string.
     *
     * @param arg The input string
     */
    void parse_int(std::string &arg);

    /**
     * Parses a boolean from a string.
     *
     * @param arg The input string
     */
    void parse_bool(std::string &arg);

    /**
     * Parses a float from a string.
     *
     * @param arg The input string
     */
    void parse_float(std::string &arg);

    /** Holds the type of this value */
    Type t;
    /** Holds the actual value */
    void *val;
  };

  /**
   * @struct CLAP::Option
   *
   * Models options in CLAP
   */
  struct Option {
    /**
     * Constructor for CLAP::Option. Constructs the option from a
     * string formatted as follows:
     *  '-o -option [br] [var1:(i|f|b|s)] [var2:...]...', where 'o'/'option'
     * is the short/full name of the option, 'br' is optional and will set the
     * option to stop the parser if it occurs, and 'var1', 'var2', ... are the
     * parameters, with types 'i' = integer, 'f' = float, 'b' = boolean or
     * 's' = string.
     *
     * @param info The input string, formatted as shown above.
     */
    Option(const std::string info);
    
    /**
     * Destructor for CLAP::Option
     * Cleans up the Option structure.
     */
    ~Option();
    
    /** Option name */
    std::string name;
    /** Option short name */
    std::string short_name;
    /** Integer indicating number of times set */
    unsigned int is_set;
    /** Flag indicating whether the option should stop the parser */
    bool do_break;
    /** List of parameters of this option */
    std::vector<Param> params;
    /** List of arguments */
    std::vector<Value> args;
  };

  /**
   * @struct InputError
   *
   * Represents internal input errors in CLAP.
   */
  struct InputError {
    /**
     * Constructor
     *
     * @param msg The error message
     */
    InputError(std::string msg);
    
    /**
     * Destructor
     */
    ~InputError();
    
    /** Error message */
    std::string msg;
  };
  
  /**
   * @struct SetupError
   *
   * Represents internal setup errors in CLAP.
   */
  struct SetupError {
    /**
     * Constructor
     *
     * @param msg The error message
     */
    SetupError(std::string msg);
    
    /**
     * Destructor
     */
    ~SetupError();
    
    /** Error message */
    std::string msg;
  };

  /** Name of the executable */
  std::string exec_name;
  /** List of the available options for this program */
  std::vector<Option> options;
  /** Map of the options for this program */
  std::unordered_map<std::string, unsigned int> map;
  /** Selected pattern */
  int sel_pattern;
  /** List of mandatory parameter patterns for this program */
  std::vector< std::vector<Param> > patterns;
  /** List of arguments */
  std::vector<Value> args;
};

#endif // CLAP_CLAP_H
