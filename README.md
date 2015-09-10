# CLAP - A simple Command Line Argument Parser in C++

The CLAP library consists of a single C++ class which allows for handling command line arguments easily.

A configuration string is given in to the class constructor together with the argument count ('argc') and the array of arguments ('argv').


# Configuration

To use CLAP, construct an instance of the CLAP class, like so:

```
int main(int argc, char* argv[]) {
    ...
    CLAP c(info, argc, argv);
    ...
}
``` 

The configuration string must be formatted as follows:

```
OPTIONS:
-o1 --option1 [br] [var1:(i|f|b|s)] [var2:(i|f|b|s)] ...
...
PARAMETERS:
par1:(i|f|b|s)
...
```

The PARAMETERS section contains the mandatory parameters, while the OPTIONS section defines the optional parameters (options), which may themselves have parameters. Parameters may have one of the following four types: 'i' = integer, 'f' = floating point value, 'b' = boolean and 's' = string.

E.g. the following configuration string sets up the program to take to mandatory parameters 'direction' (integer) and 'speed' (float), and options 'version' (breaks, no parameters) and 'log' (no break, parameters: 'level' (integer) and 'log-file' (string)):

```
OPTIONS:
-v --version br
-l --log         level:i log-file:s
PARAMETERS:
direction:i
speed:f
```

So one could e.g. call the program with:

```
./program -l 2 /path/to/file.txt 1 0.5
```

# Retrieving parameters

CLAP supplies several functions for checking if options/parameters is set. Continuing the example from above, we may retrieve the parameters in the following way:

```
int main(int argc, char* argv[]) {
    ...
    CLAP c(info, argc, argv);
    ...
    if(c.is_set("v")) {
       // Print version here...
       exit(0);
    }
    
    if(c.is_set("l")) {
        int lvl          = c.get_int_param("l", 0);
	std::string file = c.get_string_param("l", 1);
	// Configure log here...
    }
    
    // Mandatory parameters guaranteed to be set if no
    // breaking option is set.
    int direction = c.get_int_param("direction");
    float speed   = c.get_float_param("speed");
    // Run program here...
}
```