# clap
A command line argument parser written in C.

***

## Features

* Easy to use interface
* Long flags
* Short flags
* Subcommands
* Resettable parser
* Automatic error messages

***

## Getting Started
1. Start by cloning the repository with `git clone https://github.com/Noxmor/clap`.

2. Include `clap.h` found in `clap/clap.h`:

3. Done!

***

## How the parser works

**Long flags**
* Indicated by two hyphens: `--`
* Requiered arguments can be connected via a space or equals sign: `--foo bar` or `--foo=bar`
* Optional arguments must be connected via an equals sign: `--foo=bar`

**Short flags**
* Indicated by one hyphen: `-`
* Can be grouped: `-abc` is the same as `-a -b -c`
* The last grouped flag can take an argument: `-abcfoo` or `-abc foo`
* Requiered arguments can connected directly or via a space: `-fbar` or `-f bar`
* Optional arguments must be connected directly: `-fbar`

**Non-options**
* Non-options are basically any argument that is not a flag or subcommand.
* If an non-option argument looks like a flag, demarcate it with a double hyphen: `-- --foo`

**Program flags**
* A program flag is a global flag, which is not connected to any subcommand.
* Program flags are **always** specified **before** any subcommand.

**Subcommands**
* A subcommand is an alphabetical word.
* Subcommands can have their own flags.
* Only one subcommand can be parsed per command line.
* If multiple subcommands are specified within one command line, only the first one will be parsed as a subcommand, the others as non-option arguments.

**Subcommand flags**
* A subcommand flag is always connected to a specific subcommand.
* Subcommand flags are **always** specified **after** the subcommand.

### Example:
The input `program --help foo -abc --bar=123 -- --test` would achieve the following:
* Parse the program flag "help".
* Run the subcommand "foo" (if "foo" is a valid subcommand, otherwise it will be parsed as an non-option argument).
* Parse the subcommand "foo" flags 'a', 'b' and 'c'.
* Parse the subcommand "foo" flag "bar" with the argument "123".
* Parse the non-option argument "test", because "--test" was demarcated.

***

## How to use

In order to use clap, you will need to include `clap/clap.h`.
The internal parser holds a buffer for flags and subcommands.
By default, each buffer is big enough to store `256` elements.
If you want to modify these buffer's capacities, simply `define the following macros` with your own capacity before including the header:
* `#define CLAP_FLAG_CAPACITY`
* `#define CLAP_SUBCOMMAND_CAPACITY`

Now you will need to register your flags and subcommands to the parser, so the parser knows how to handle each argument when parsing.
Flags are registered as follows:
* `void clapRegisterFlag(const char* long_name, char short_name, CLAPflagArgType arg_type, const char* subcommand_name)`
* **long_name**: An alphanumerical name for the flag. Needs to be at least 2 characters long. Words are separated by a hypen.
* **short_name**: An alphabetical name for the flag.
* **arg_type**: Type of the flag's argument depending on whether it:
  * requieres an argument: `CLAP_FLAG_REQ_ARG`
  * accepts an optional argument: `CLAP_FLAG_OPT_ARG`
  * accepts no argument: `CLAP_FLAG_NO_ARG`
* **subcommand_name**: Name of the subcommand, if this flag is not a program flag but a specific subcommand flag.

Subcommands are registered as follows:
* `void clapRegisterSubcommand(const char* name)`
* **name**: The name of the subcommand.

**Note**: There are macros defined for better code readability:
* `CLAP_FLAG_NO_LONG` if the flag has no long name.
* `CLAP_FLAG_NO_SHORT` if the flag has no short name.

### Example:
```c
clapRegisterFlag("foo", CLAP_FLAG_NO_SHORT, CLAP_FLAG_REQ_ARG, NULL);
clapRegisterFlag("bar", 'b ', CLAP_FLAG_NO_ARG, NULL);
```
This will register two flags, namely "foo" and "bar". The "foo" flag has no short name, whereas the "bar" flag has the short name 'b'.
The "foo" flag requieres an argument parsed, while the "bar" flag accepts no argument.
Both flags have no subcommand name defined, thus they are global program flags.

***

### Typical program flag parsing
```c
#include <clap/clap.h>

int main(int argc, char** argv)
{
  clapRegisterFlag("foo", 'f', CLAP_FLAG_REQ_ARG, NULL);
  clapRegisterFlag("bar", 'b', CLAP_FLAG_OPT_ARG, NULL);
  clapRegisterFlag("test", CLAP_FLAG_NO_SHORT, CLAP_FLAG_NO_ARG, NULL);
  
  while(clapParse(argc, argv))
  {
    if(clapParsedFlag("foo", 'f'))
    {
      const char* arg = clapGetArg();
      //Further handle foo flag
    }
    
    if(clapParsedFlag("bar", 'b'))
    {
      const char* arg = clapGetArg();
      if(arg)
      {
        //...
      }
      
      //Further handle bar flag
    }
    
    if(clapParsedFlag("test", CLAP_FLAG_NO_SHORT))
    {
      //Further handle test flag
    }
    
    const char* non_opt = NULL;
    if(clapParsedNonOpt(&non_opt))
    {
      //Further handle non-option argument
    }
  }
  
  return 0;
}
```
All it takes is the registration process and a simple while loop.
While parsing, `clapParse()` will return `CLAP_TRUE`. If the parser is done, `clapParse()` will return `CLAP_FALSE`.

**Note**: `clapParsedFlag()` will only check for program flags. See below, if you want to know how to check for subcommand flags.

**Note**: Retrieving the optional argument from a flag requieres a NULL check, because the flag could be parsed without any argument.
Flags with requiered arguments do not need to perform a NULL check, because if there is no argument specified, the flag will not be parsed.

**Note**: `clapGetArg()` will always return the argument as a string (const char*). It is up to the user how to further interpret the argument and handle it. If used within a flag that does not accept any argument, `NULL` will be returned.

***

### Typical subcommand parsing (with flags)
```c
#include <clap/clap.h>

int main(int argc, char** argv)
{
  clapRegisterSubcommand("foo");
  
  clapRegisterFlag("bar", 'b', CLAP_FLAG_OPT_ARG, "foo");
  clapRegisterFlag("test", CLAP_FLAG_NO_SHORT, CLAP_FLAG_NO_ARG, NULL);
  
  while(clapParse(argc, argv))
  {
    if(clapParsedSubcommandFlag("foo", "bar", 'b'))
    {
      const char* arg = clapGetArg();
      if(arg)
      {
        //...
      }
      
      //Further handle bar flag
    }
    
    if(clapParsedFlag("test", CLAP_FLAG_NO_SHORT))
    {
      //Further handle test flag
    }
    
    const char* non_opt = NULL;
    if(clapParsedNonOpt(&non_opt))
    {
      //Further handle non-option argument
    }
  }
  
  if(clapParsedSubcommand("foo"))
  {
    //Run subcommand foo
  }
  
  return 0;
}
```
Registering subcommands is in fact even simpler than flags.

**Note**: In order to register the "bar" flag as a subcommand flag, we needed to specify the name of the subcommand at the end. When handling subcommand flags, we will use the `clapParsedSubcommandFlag()` function instead of `clapParsedFlag()`. The intention is to setup all necessary information for a subcommand with `clapParsedSubcommandFlag()` and after all parsing is done, we can check after the while loop if a subcommand was entered with `clapParsedSubcommand()` and finally run the subcommand.

***

## Reset
If you want to use the parser more than once in your program, you can reset the parser.
* To reset everything: `clapReset()`
* To reset only the parser: `clapResetParser()`
* To reset all flags: `clapResetFlags()`
* To reset all subcommands: `clapResetSubcommands()`

***

## Known bugs
Currently none.
