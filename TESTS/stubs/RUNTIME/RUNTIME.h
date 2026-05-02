#ifndef RUNTIME_H
#define RUNTIME_H

#include <STD/TYPEDEF.h>

typedef void (*exit_func_t)(void);

BOOL ON_EXIT(exit_func_t fn);

#endif
