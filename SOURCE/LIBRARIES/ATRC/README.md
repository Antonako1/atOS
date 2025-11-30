# ATRC — A Static, Header only, Resource/Configuration file library

A smaller rewrite of my old existing configuration file library. Found at [GitHub](https://github.com/Antonako1/ATRC)

The syntax is basically the same as in Microsoft .INI configuration files, but with some additional ease of life features.

All syntax and features can be seen below

File extension should be `.CNF`.

```ini
#!ATRC
# This is here to tell the parser that the file is ATRC, just a failsafe measure.
#   to system resources.

# A public variable
%env%=production

# A key like this can be found inside a block named " "
BlocklessKey=value

# A block which contains keys
[ServerConfig]
Platform=atOS

# Keys can hold references to variables
Environment=%env%

# A few 'preprocessor' directives. These are not preprocessed...
#.INCLUDE path/to/another.atrc
# We have now included all blocks, keys and variables into this file

# Raw strings. Not fancy but does their job. Define with .SR <VAR|KEY>. End with .ER

#.SR VAR
%RawStringVar%=Raw string
variable definition will include
[ThisBlockToo]
#.ER

#.SR KEY
RawString=Everything inside
the directive will be part of
the value
Including=This one!
#.ER

# Keys inside blocks get auto-assigned enum values from 0 onwards
[EnumTest]
Key1=Value #enum0
Key2=Value #enum1
Key3=Value #enum2
```