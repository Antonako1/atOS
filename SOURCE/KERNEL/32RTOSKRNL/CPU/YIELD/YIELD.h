/*
0x81 Yield Interrupt Vector
notifies scheduler to perform a context switch and halt current process execution
*/
#ifndef CPU_YIELD_H
#define CPU_YIELD_H

#define YIELD_VECTOR 0x81

#include <STD/TYPEDEF.h>
#include <STD/ASM.h>

#define YIELD_PROCESS() ASM_VOLATILE("int $" _STR(YIELD_VECTOR))

void yield_handler(I32 vector, U32 errcode);

#endif // CPU_YIELD_H