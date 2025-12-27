#ifndef CPU_FPU_H
#define CPU_FPU_H

#include <STD/TYPEDEF.h>
#include <STD/ASM.h>
#include <RTOSKRNL/PROC/PROC.h>

void fpu_disable(void);
void fpu_enable(void);

void fpu_save(TCB *t);
void fpu_restore(TCB *t);
void fpu_zero_init(TCB *t);
void fpu_init(TCB *t);
void fpu_mark_task_switched(void);

#endif // CPU_FPU_H