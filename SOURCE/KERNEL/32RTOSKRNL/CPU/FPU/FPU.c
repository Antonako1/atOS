#include <CPU/FPU/FPU.h>
#include <CPU/PIC/PIC.h>
#include <STD/MEM.h>
#include <DEBUG/KDEBUG.h>

void fpu_disable(void) {
    // Clear TS bit in CR0
    U32 cr0;
    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 3); // clear TS
    ASM_VOLATILE("mov %0, %%cr0" :: "r"(cr0));

    PIC_Mask(13); // mask FPU IRQ
}

// void fpu_enable_exceptions(void) {
//     U16 cw;
//     ASM_VOLATILE("fstcw %0" : "=m"(cw));
//     cw &= ~0x3F; // unmask all exceptions
//     ASM_VOLATILE("fldcw %0" :: "m"(cw));
// }

void fpu_cpu_init(void) {
    U32 cr0, cr4;

    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);  // EM = 0
    cr0 |=  (1 << 1);  // MP = 1
    ASM_VOLATILE("mov %0, %%cr0" :: "r"(cr0));

    ASM_VOLATILE("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);   // OSFXSR
    cr4 |= (1 << 10);  // OSXMMEXCPT (recommended)
    ASM_VOLATILE("mov %0, %%cr4" :: "r"(cr4));
}

void fpu_enable(void) {
    U32 cr0;
    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 3);  // TS = 1, force #NM on first FPU instruction
    cr0 &= ~(1 << 2); // EM = 0, enable FPU
    ASM_VOLATILE("mov %0, %%cr0" :: "r"(cr0));

    fpu_cpu_init();
    KDEBUG_PUTS("[FPU] Initialized\n");
}


void fpu_save(TCB *t) {
    ASM_VOLATILE("fxsave (%0)" :: "r"(&t->fpu) : "memory");
}

void fpu_restore(TCB *t) {
    ASM_VOLATILE("fxrstor (%0)" :: "r"(&t->fpu) : "memory");
}

void fpu_zero_init(TCB *t) {
    KDEBUG_PUTS("[FPU] Initializing FPU for ");
    KDEBUG_HEX32(t->info.pid);
    KDEBUG_PUTS("\n");
    MEMSET_OPT(&t->fpu.fxstate, 0, sizeof(FPUState));  // clear FPU buffer
    t->info.fpu_initialized = FALSE;
}

void fpu_init(TCB *t) {
    ASM_VOLATILE("fninit" ::: "memory");
    t->info.fpu_initialized = TRUE;
}

void fpu_mark_task_switched(void) {
    U32 cr0;
    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 3);   // TS = 1
    ASM_VOLATILE("mov %0, %%cr0" :: "r"(cr0));
}
