/*
Internal RTOSKRNL functions. Some important functions like panic() are here,
and some not-so-important functions are here.
*/
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/PAGING/PAGING.h>
#include <DRIVERS/ATAPI/ATAPI.h>
#include <DRIVERS/ATA_PIO/ATA_PIO.h>
#include <DRIVERS/VESA/VBE.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <DRIVERS/AC97/AC97.h>

#include <FS/ISO9660/ISO9660.h>
#include <FS/FAT/FAT.h>

#include <STD/STRING.h>
#include <STD/ASM.h>

#include <MEMORY/HEAP/KHEAP.h>

#include <CPU/PIT/PIT.h>
#include <CPU/ISR/ISR.h> // for regs struct

#include <ACPI/ACPI.h>
#include <PROC/PROC.h>
#include <ERROR/ERROR.h>

#define INC_rki_row(rki_row) (rki_row += VBE_CHAR_HEIGHT + 2)
#define DEC_rki_row(rki_row) (rki_row -= VBE_CHAR_HEIGHT + 2)
static U32 rki_row __attribute__((section(".data"))) = 0;

void set_rki_row(U32 row) {
    if (row == U32_MAX)
    {
        INC_rki_row(rki_row);
    }
    else
    {
        rki_row = row;
    }
}

static inline U32 ASM_READ_ESP(void) {
    U32 esp;
    ASM_VOLATILE("mov %%esp, %0" : "=r"(esp));
    return esp;
}

static inline U32 ASM_GET_EIP(void) {
    U32 eip;
    ASM_VOLATILE("call 1f; 1: pop %0" : "=r"(eip));
    return eip;
}


static inline regs ASM_READ_REGS(void) {
    regs regs;
    ASM_VOLATILE("mov %%eax, %0" : "=r"(regs.eax));
    ASM_VOLATILE("mov %%ebx, %0" : "=r"(regs.ebx));
    ASM_VOLATILE("mov %%ecx, %0" : "=r"(regs.ecx));
    ASM_VOLATILE("mov %%edx, %0" : "=r"(regs.edx));
    ASM_VOLATILE("mov %%esi, %0" : "=r"(regs.esi));
    ASM_VOLATILE("mov %%edi, %0" : "=r"(regs.edi));
    ASM_VOLATILE("mov %%ebp, %0" : "=r"(regs.ebp));
    ASM_VOLATILE("mov %%esp, %0" : "=r"(regs.esp));
    return regs;
}

#define PANIC_COLOUR VBE_WHITE, VBE_BLUE
#define PANIC_DEBUG_COLOUR VBE_RED, VBE_LIGHT_CYAN

void DUMP_CALLER_STACK(U32 count) {
    U8 buf[20];
    VBE_DRAW_STRING(0, rki_row, "Caller stack dump:", PANIC_COLOUR);
    INC_rki_row(rki_row);
    U32 *ebp;
    ASM_VOLATILE("mov %%ebp, %0" : "=r"(ebp));
    for (U32 i = 0; i < count && ebp; i++) {
        ITOA(ebp[1], buf, 16); // return address
        ebp = (U32*)ebp[0];    // next EBP
        VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);
        INC_rki_row(rki_row);
    }

    VBE_UPDATE_VRAM();
}

void DUMP_STACK(U32 esp, U32 count) {
    U32 *stack = (U32*)esp;
    U8 buf[20];
    VBE_DRAW_STRING(0, rki_row, "Stack dump:", PANIC_COLOUR);
    INC_rki_row(rki_row);
    for(U32 i = 0; i < count; i++) {
        ITOA((U32)(stack[i]), buf, 16);
        VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);
        INC_rki_row(rki_row);
    }
    VBE_UPDATE_VRAM();
}

void DUMP_REGS(regs *r) {
    U8 buf[20];
    VBE_DRAW_STRING(0, rki_row, "EAX: ", PANIC_COLOUR);
    ITOA(r->eax, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "EBX: ", PANIC_COLOUR);
    ITOA(r->ebx, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "ECX: ", PANIC_COLOUR);
    ITOA(r->ecx, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "EDX: ", PANIC_COLOUR);
    ITOA(r->edx, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "ESI: ", PANIC_COLOUR);
    ITOA(r->esi, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "EDI: ", PANIC_COLOUR);
    ITOA(r->edi, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "EBP: ", PANIC_COLOUR);
    ITOA(r->ebp, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "ESP: ", PANIC_COLOUR);
    ITOA(r->esp, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}

void DUMP_ERRCODE(U32 errcode) {
    U8 buf[20];
    VBE_DRAW_STRING(0, rki_row, "Error code: ", PANIC_COLOUR);
    ITOA_U(errcode, buf, 16);
    VBE_DRAW_STRING(100, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}

void DUMP_U32_2(U32 x, U32 y) {
    U8 buf[20];
    ITOA_U(x, buf, 16);
    VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);
    ITOA_U(y, buf, 16);
    VBE_DRAW_STRING(200, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}

void DUMP_INTNO(U32 int_no) {
    U8 buf[20];
    VBE_DRAW_STRING(0, rki_row, "Interrupt number: ", PANIC_COLOUR);
    ITOA_U(int_no, buf, 16);
    VBE_DRAW_STRING(200, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}

void DUMP_MEMORY(U32 addr, U32 length) {
    U8 buf[20] = { 0 };
    char ascii[17] = { 0 };  // 16 chars + null terminator
    U8 *ptr = (U8*)addr;

    for (U32 i = 0; i < length; i++) {
        // Start new line every 16 bytes
        if (i % 16 == 0) {
            if (i != 0) {
                // Draw ASCII characters at end of previous line
                VBE_DRAW_STRING(100 + 16 * 30, rki_row, ascii, PANIC_COLOUR);
                INC_rki_row(rki_row);
            }

            // Print memory address at start of line
            ITOA_U((U32)(ptr + i), buf, 16);
            VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);

            MEMZERO(ascii, sizeof(ascii)); // clear ASCII buffer
        }

        // Print hex value of the byte
        ITOA_U(ptr[i], buf, 16);
        VBE_DRAW_STRING(100 + (i % 16) * 30, rki_row, buf, PANIC_COLOUR);

        // Prepare ASCII representation
        ascii[i % 16] = (ptr[i] >= 0x20 && ptr[i] <= 0x7E) ? ptr[i] : '.';
    }

    // Draw ASCII for the last line
    int lastLineBytes = length % 16;
    if (lastLineBytes == 0) lastLineBytes = 16; // full line
    ascii[lastLineBytes] = '\0';                 // null terminate
    VBE_DRAW_STRING(100 + 16 * 30, rki_row, ascii, PANIC_COLOUR);
    INC_rki_row(rki_row);

    VBE_UPDATE_VRAM();
}





void DUMP_STRING(PU8 buf) {
    VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}
void DUMP_STRINGN(PU8 buf, U32 n) {
    for(U32 i = 0; i < n; i++) {
        VBE_DRAW_CHARACTER(i * VBE_CHAR_WIDTH, rki_row, buf[i], PANIC_COLOUR);
    }
    INC_rki_row(rki_row);
    VBE_UPDATE_VRAM();
}
void DUMP_STRING_U32(PU8 str, U32 num) {
    U8 buf[16];
    ITOA_U(num, buf, 16);
    VBE_DRAW_STRING(0, rki_row, buf, PANIC_COLOUR);
    VBE_DRAW_STRING(16 * 16 + 2, rki_row, str, PANIC_COLOUR);
    INC_rki_row(rki_row);
    DUMP_MEMORY(num, 32);
    VBE_UPDATE_VRAM();
}

void panic_reg(regs *r, const U8 *msg, U32 errmsg) {
    CLI;
    debug_vram_start();
    AC97_TONE(300, 150, 14400, 8000);
    AC97_TONE(200, 200, 14400, 8000);
    VBE_COLOUR fg = VBE_WHITE;
    VBE_COLOUR bg = VBE_BLUE;
    VBE_CLEAR_SCREEN(bg);
    U8 buf[16];

    // --- Print to debug port ---
    KDEBUG_PUTS("\n=== PANIC REG ===\n");
    KDEBUG_PUTS("Message: ");
    KDEBUG_PUTS(msg);
    KDEBUG_PUTS("\nError Code: 0x");
    KDEBUG_HEX32(errmsg);
    KDEBUG_PUTS("\n");

    // --- Draw to VRAM ---
    ITOA(errmsg, buf, 16);
    VBE_DRAW_STRING(0, rki_row, "ERRORCODE: 0x", fg, bg);
    VBE_DRAW_STRING(VBE_CHAR_WIDTH*13, rki_row, buf, fg, bg);
    INC_rki_row(rki_row);

    VBE_DRAW_STRING(0, rki_row, msg, fg, bg);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "System halted, Dump as of exception raise.", fg, bg);
    INC_rki_row(rki_row);

    // --- Register dump ---
    U32 esp = r->esp;
    U32 ebp = r->ebp;
    INC_rki_row(rki_row);    
    VBE_DRAW_STRING(0, rki_row, "Registers:", fg, bg);
    INC_rki_row(rki_row);
    DUMP_REGS(r);
    INC_rki_row(rki_row);

    // Debug output for registers
    KDEBUG_PUTS("Register dump:\n");
    KDEBUG_PUTS("EAX="); KDEBUG_HEX32(r->eax); KDEBUG_PUTS(" ");
    KDEBUG_PUTS("EBX="); KDEBUG_HEX32(r->ebx); KDEBUG_PUTS(" ");
    KDEBUG_PUTS("ECX="); KDEBUG_HEX32(r->ecx); KDEBUG_PUTS(" ");
    KDEBUG_PUTS("EDX="); KDEBUG_HEX32(r->edx); KDEBUG_PUTS("\n");
    KDEBUG_PUTS("ESI="); KDEBUG_HEX32(r->esi); KDEBUG_PUTS(" ");
    KDEBUG_PUTS("EDI="); KDEBUG_HEX32(r->edi); KDEBUG_PUTS("\n");
    KDEBUG_PUTS("EBP="); KDEBUG_HEX32(ebp); KDEBUG_PUTS(" ");
    KDEBUG_PUTS("ESP="); KDEBUG_HEX32(esp); KDEBUG_PUTS("\n");
    KDEBUG_PUTS("EIP="); KDEBUG_HEX32(r->eip); KDEBUG_PUTS("\n");
    KDEBUG_PUTS("EFLAGS="); KDEBUG_HEX32(r->eflags); KDEBUG_PUTS("\n");

    INC_rki_row(rki_row);
    DUMP_CALLER_STACK(10);
    INC_rki_row(rki_row);

    VBE_DRAW_STRING(0, rki_row, "Stack dump at ESP, starting from ESP-60", fg, bg);
    INC_rki_row(rki_row);
    DUMP_STACK(esp - 60, 80);
    INC_rki_row(rki_row);

    // --- Heap info ---
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "Kernel Heap Info:", fg, bg);
    INC_rki_row(rki_row);
    KHeap *heap = KHEAP_GET_INFO();
    if (heap) {
        ITOA(heap->totalSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Total Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(heap->usedSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Used Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(heap->freeSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Free Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        // Debug output
        KDEBUG_PUTS("Heap: total=0x"); KDEBUG_HEX32(heap->totalSize);
        KDEBUG_PUTS(" used=0x"); KDEBUG_HEX32(heap->usedSize);
        KDEBUG_PUTS(" free=0x"); KDEBUG_HEX32(heap->freeSize);
        KDEBUG_PUTS("\n");
    }

    // --- Pageframe info ---
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "Pageframe Info:", fg, bg);
    INC_rki_row(rki_row);
    PAGEFRAME_INFO *pf = GET_PAGEFRAME_INFO();
    if (pf && pf->initialized) {
        ITOA(pf->size, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Total Pages: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->usedMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Used Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->freeMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Free Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->reservedMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Reserved Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        // Debug output
        KDEBUG_PUTS("PF: total=0x"); KDEBUG_HEX32(pf->size);
        KDEBUG_PUTS(" used=0x"); KDEBUG_HEX32(pf->usedMemory);
        KDEBUG_PUTS(" free=0x"); KDEBUG_HEX32(pf->freeMemory);
        KDEBUG_PUTS(" reserved=0x"); KDEBUG_HEX32(pf->reservedMemory);
        KDEBUG_PUTS("\n");
    }

    KDEBUG_PUTS("System halted.\n");
    VBE_UPDATE_VRAM();
    ASM_VOLATILE("cli; hlt");
}

void PANIC_RAW(const U8 *msg, U32 errmsg, VBE_COLOUR fg, VBE_COLOUR bg) {
    CLI;
    debug_vram_start();
    VBE_UPDATE_VRAM();
    AC97_TONE(300, 150, 14400, 8000);
    AC97_TONE(200, 200, 14400, 8000);
    rki_row = 0;
    VBE_CLEAR_SCREEN(bg);
    U8 buf[16];
    // Error code
    ITOA_U(errmsg, buf, 16);
    VBE_DRAW_STRING(0, rki_row, "ERRORCODE: 0x", fg, bg);
    VBE_DRAW_STRING(VBE_CHAR_WIDTH*13, rki_row, buf, fg, bg);
    INC_rki_row(rki_row);
    ITOA_U(get_current_tcb()->info.pid, buf, 16);
    VBE_DRAW_STRING(0, rki_row, "Current PID: 0x", fg, bg);
    VBE_DRAW_STRING(VBE_CHAR_WIDTH*16, rki_row, buf, fg, bg);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, get_current_tcb()->info.name, fg, bg);
    INC_rki_row(rki_row);
    // --- Print to debug port ---
    KDEBUG_PUTS("\n=== PANIC ===\n");
    KDEBUG_PUTS("Message: ");
    KDEBUG_PUTS(msg);
    KDEBUG_PUTS("\nError Code: 0x");
    KDEBUG_HEX32(errmsg);
    KDEBUG_PUTS("\n");
    KDEBUG_PUTS("\nCurrent PID: 0x");
    KDEBUG_HEX32(get_current_tcb()->info.pid);
    KDEBUG_PUTS("\n");
    // Last error
    VBE_DRAW_STRING(0, rki_row, "Last error: 0x", fg, bg);
    ITOA_U(GET_LAST_ERROR(), buf, 16);
    VBE_DRAW_STRING(VBE_CHAR_WIDTH*14, rki_row, buf, fg, bg);
    VBE_DRAW_CHARACTER(VBE_CHAR_WIDTH*19, rki_row, (U8)errmsg, fg, bg);
    INC_rki_row(rki_row);

    // Message
    VBE_DRAW_STRING(0, rki_row, msg, fg, bg);
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "System halted, Dump as in panic() call.", fg, bg);
    INC_rki_row(rki_row);

    // Registers
    U32 esp = ASM_READ_ESP();
    U32 eip = ASM_GET_EIP();
    INC_rki_row(rki_row);    
    VBE_DRAW_STRING(0, rki_row, "Registers:", fg, bg);
    INC_rki_row(rki_row);
    regs r = ASM_READ_REGS();
    DUMP_REGS(&r);

    VBE_DRAW_STRING(0, rki_row, "EIP:", fg, bg);
    ITOA(eip, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, fg, bg);
    INC_rki_row(rki_row);

    VBE_DRAW_STRING(0, rki_row, "ESP:", fg, bg);
    ITOA(esp, buf, 16);
    VBE_DRAW_STRING(50, rki_row, buf, fg, bg);
    INC_rki_row(rki_row);

    // Stack
    INC_rki_row(rki_row);
    DUMP_STACK(esp, 10);
    INC_rki_row(rki_row);
    DUMP_CALLER_STACK(10);
    INC_rki_row(rki_row);

    // Memory dumps
    VBE_DRAW_STRING(0, rki_row, "Memory dump at EIP", fg, bg);
    INC_rki_row(rki_row);
    DUMP_MEMORY(eip, 64);

    VBE_DRAW_STRING(0, rki_row, "Memory dump at error code.", fg, bg);
    INC_rki_row(rki_row);
    DUMP_MEMORY(errmsg, 256);

    // 🔍 Kernel Heap Info
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "Kernel Heap Info:", fg, bg);
    INC_rki_row(rki_row);
    KHeap *heap = KHEAP_GET_INFO();
    if (heap) {
        ITOA(heap->totalSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Total Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(heap->usedSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Used Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(heap->freeSize, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Free Size: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        KDEBUG_PUTS("Heap: total=0x"); KDEBUG_HEX32(heap->totalSize);
        KDEBUG_PUTS(" used=0x"); KDEBUG_HEX32(heap->usedSize);
        KDEBUG_PUTS(" free=0x"); KDEBUG_HEX32(heap->freeSize);
        KDEBUG_PUTS("\n");
    }

    // 🔍 Pageframe Info
    INC_rki_row(rki_row);
    VBE_DRAW_STRING(0, rki_row, "Pageframe Info:", fg, bg);
    INC_rki_row(rki_row);
    PAGEFRAME_INFO *pf = GET_PAGEFRAME_INFO();
    if (pf && pf->initialized) {
        ITOA(pf->size, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Total Pages: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->usedMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Used Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->freeMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Free Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

        ITOA(pf->reservedMemory, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Reserved Memory: 0x", fg, bg);
        VBE_DRAW_STRING(200, rki_row, buf, fg, bg);
        INC_rki_row(rki_row);

            KDEBUG_PUTS("PF: total=0x"); KDEBUG_HEX32(pf->size);
        KDEBUG_PUTS(" used=0x"); KDEBUG_HEX32(pf->usedMemory);
        KDEBUG_PUTS(" free=0x"); KDEBUG_HEX32(pf->freeMemory);
        KDEBUG_PUTS(" reserved=0x"); KDEBUG_HEX32(pf->reservedMemory);
        KDEBUG_PUTS("\n");
    }
    KDEBUG_PUTS("Failed on "); KDEBUG_HEX32(get_current_tcb()->info.pid);
    KDEBUG_PUTS(" ");
    KDEBUG_PUTS(get_current_tcb()->info.name);
    KDEBUG_PUTS("\n");

    VBE_UPDATE_VRAM();
    ASM_VOLATILE("cli; hlt");
}

void panic(const U8 *msg, U32 errmsg) {
    PANIC_RAW(msg, errmsg, PANIC_COLOUR);
}
void panic_if(BOOL condition, const U8 *msg, U32 errmsg) {
    if (condition) {
        panic(msg, errmsg);
    }
}

void panic_debug(const U8 *msg, U32 errmsg) {
    PANIC_RAW(msg, errmsg, PANIC_DEBUG_COLOUR);
}
void panic_debug_if(BOOL condition, const U8 *msg, U32 errmsg) {
    if (condition) {
        panic_debug(msg, errmsg);
    }
}

void system_halt(VOID) {
    ASM_VOLATILE("cli; hlt");
    NOP;
    NOP;
    NOP;
    NOP;
    NOP;
}

void system_halt_if(BOOL condition) {
    if (condition) {
        system_halt();
    }
}

void system_reboot(VOID) {
    ASM_VOLATILE(
        "cli\n"             // Disable interrupts
        "mov $0xFE, %al\n"  // Reset command
        "out %al, $0x64\n"  // Send to keyboard controller
        "hlt\n"             // Stop CPU if reset fails
    );
}
void system_reboot_if(BOOL condition) {
    if (condition) {
        system_reboot();
    }
}
void system_shutdown(VOID) {
    panic(PANIC_TEXT("System shutdown not implemented, here's a halt instead"), PANIC_NONE);   
    ACPI_SHUTDOWN_SYSTEM();
    // If ACPI shutdown fails, halt the system
    system_halt();
}
void system_shutdown_if(BOOL condition) {
    if (condition) {
        system_shutdown();
    }
}

// assert function
void assert(BOOL condition) {
    if (!condition) {
        panic(PANIC_TEXT("Assertion failed"), PANIC_UNKNOWN_ERROR);
    }
}









#define SHELL_PATH "PROGRAMS/ATOSHELL/ATOSHELL.BIN"
#define STR(x, y) x y 

void LOAD_AND_RUN_KERNEL_SHELL(VOID) {
    KDEBUG_PUTS("[atOS] Enter LOAD_AND_RUN_KERNEL_SHELL\n");
    VBE_FLUSH_SCREEN();
    FAT_LFN_ENTRY ent = { 0 };
    U32 sz = 0;
    if(!PATH_RESOLVE_ENTRY(SHELL_PATH, &ent)) {
        panic("Unable to load SHELL from FAT", PANIC_INITIALIZATION_FAILED);
    }
    VOIDPTR file = READ_FILE_CONTENTS(&sz, &ent.entry);
    if(!file) {
        panic("Unable to read SHELL from FAT", PANIC_INITIALIZATION_FAILED);
    }
    U8 *shell_argv[] = { SHELL_PATH , "-test", NULLPTR };
    panic_if(
        !RUN_BINARY(
            STR("/", SHELL_PATH), 
            file, 
            sz, 
            USER_HEAP_SIZE, 
            USER_STACK_SIZE, 
            TCB_STATE_ACTIVE | TCB_STATE_INFO_CHILD_PROC_HANDLER , 
            0,
            shell_argv,
            2
        ), 
        "PANIC: Failed to run kernel shell!", 
        PANIC_KERNEL_SHELL_GENERAL_FAILURE
    );
    KFREE(file);
    file = NULLPTR;
    KDEBUG_PUTS("[atOS] RUN_BINARY returned OK\n");
    KDEBUG_PUTS("[atOS] LOAD_AND_RUN_KERNEL_SHELL done\n");
}

BOOL initialize_filestructure(VOID) {
    if(!GET_BPB_LOADED()) {
        return LOAD_BPB();
    }
    VOIDPTR bin = NULLPTR;
    U32 sz = 0;
    bin = ISO9660_READ_FILEDATA_TO_MEMORY_QUICKLY("ATOS/DISK_VBR.BIN", &sz);
    if(!bin) {
        return FALSE;
    }
    if(!ZERO_INITIALIZE_FAT32(bin, sz)) {
        ISO9660_FREE_MEMORY(bin);
        return FALSE;
    }
    ISO9660_FREE_MEMORY(bin);
    U32 tmp;
    CREATE_CHILD_DIR(GET_ROOT_CLUSTER(), "HOME", 0, &tmp);
    CREATE_CHILD_DIR(tmp, "DOCS", 0, &tmp);
    CREATE_CHILD_DIR(GET_ROOT_CLUSTER(), "TMP", 0, &tmp);
    // HLT;
    return TRUE;
}


void RTOSKRNL_LOOP(VOID) {
    kernel_loop_init();
    while(1) {
        early_debug_tcb(get_last_pid());
        handle_kernel_messages();
    }
}
