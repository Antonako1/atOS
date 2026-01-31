# SYS_PROGS -- System programs found inside /ATOS

Why SYS_PROGS instead of PROGRAMS?

SYS_PROGS is a bit quicker to setup, so you don't have to spend too much time creating the CMake scripts.
Also you don't have to modify the path everytime you create a new program since all of these are copied into /ATOS which is already in the user's path by default.

## Creating program:

 1. Create PROGNAME.SOURCES
    - This file will hold the relative path to your program source files
    - This can also hold non-source files and will just be copied into /ATOS folder
    - If you use already existing and used folder, no need to add it to your .SOURCES file
 2. Create PROGNAME.LIBS
    - This file will hold the relative path to the libraries SOURCE files if used
 3. Create PROGNAME.BDEF
    - This file will hold the Build DEFinitions for your program
    - These are GCC compiler flags, placed one per line
    - To create a GUI program, add the following line:
      -DRUNTIME_GUI
 3. Create source files
 4. Compile and run