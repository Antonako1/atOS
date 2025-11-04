#include <CPU/FPU/FPU.h>
#include <CPU/PIC/PIC.h>

void disable_fpu(void) {
    // Clear TS bit in CR0
    U32 cr0;
    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 3); // clear TS
    ASM_VOLATILE("mov %0, %%cr0" :: "r"(cr0));

    PIC_Mask(13); // mask FPU IRQ
}
void fpu_enable_exceptions(void) {
    U16 cw;
    ASM_VOLATILE("fstcw %0" : "=m"(cw));
    cw &= ~0x3F;  // clear exception masks
    ASM_VOLATILE("fldcw %0" :: "m"(cw));
}

void fpu87_init(void) {
    ASM_VOLATILE("fninit"); // Initialize FPU
    fpu_enable_exceptions();
}

void fpu_save(void *state) {
    ASM_VOLATILE("fxsave (%0)" :: "r"(state) : "memory");
}

void fpu_restore(void *state) {
    ASM_VOLATILE("fxrstor (%0)" :: "r"(state) : "memory");
}