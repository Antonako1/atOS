#ifndef RUNTIME_H
#define RUNTIME_H

#include <STD/TYPEDEF.h>

typedef void (*exit_func_t)(void);

#ifndef MAX_ON_EXIT_FUNCTIONS
#   define MAX_ON_EXIT_FUNCTIONS 3
#endif // MAX_ON_EXIT_FUNCTIONS
BOOL ON_EXIT(exit_func_t fn);

#endif // RUNTIME_H