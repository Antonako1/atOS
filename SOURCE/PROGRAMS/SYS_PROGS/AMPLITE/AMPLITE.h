#ifndef AMPLITE_H
#define AMPLITE_H

#include <STD/TYPEDEF.h>
#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/STRING.h>
#include <STD/DEBUG.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>

#ifdef DEBUG_MODE
#define DPRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#else
#define DPRINTF(...) do {} while(0)
#endif

#endif // AMPLITE_H