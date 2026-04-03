#ifndef RUNTIME_H
#define RUNTIME_H

#include <STD/TYPEDEF.h>

typedef void (*exit_func_t)(void);

#define MAX_ON_EXIT_FUNCTIONS 5

/// @brief Register a function to be called on program exit. Functions will be called in reverse order of registration (LIFO).
/// @param fn The function to call on exit. Must take no parameters and return void.
/// @return TRUE if the function was registered successfully, FALSE if the maximum number of exit functions has been reached.
BOOL ON_EXIT(exit_func_t fn);

#ifdef RUNTIME_ATGL
#include <LIBRARIES/ATGL/ATGL.h>
#endif // RUNTIME_ATGL

#endif // RUNTIME_H