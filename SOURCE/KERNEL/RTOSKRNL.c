#ifndef RTOS_KERNEL
#define RTOS_KERNEL
#endif
#include <RTOSKRNL.h>

__attribute__((noreturn))
void rtos_kernel(U0) {
    CLI;
    KDEBUG_INIT();
    KDEBUG_PUTS("[atOS] rtos_kernel start\n");
    debug_vram_start(); // Start in early mode, using direct framebuffer access
    // These are called here, to fix address issues and to set up new values
    GDT_INIT();
    IDT_INIT();
    IRQ_INIT();
    SETUP_ISR_HANDLERS();
    disable_fpu();
    panic_if(!vesa_check(), PANIC_TEXT("Failed to initialize VESA"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] VESA/VBE OK\n");
    
    panic_if(!vbe_check(), PANIC_TEXT("Failed to initialize VBE"), PANIC_INITIALIZATION_FAILED);

    panic_if(INITIALIZE_ATAPI() == ATA_FAILED, PANIC_TEXT("Failed to initialize ATAPI interface!"), PANIC_INITIALIZATION_FAILED);
    panic_if(!E820_INIT(), PANIC_TEXT("Failed to initialize E820 memory map!"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] E820 OK\n");
    panic_if(!PAGEFRAME_INIT(), PANIC_TEXT("Failed to initialize page frame! Possibly not enough memory."), PANIC_INITIALIZATION_FAILED);
    panic_if(!KHEAP_INIT(KHEAP_MAX_SIZE_PAGES), PANIC_TEXT("Failed to initialize kheap. Not enough memory or pageframe issue."), PANIC_INITIALIZATION_FAILED);    
    KDEBUG_PUTS("[atOS] KHEAP OK\n");
    // For some reason, PAGEFRAME or something?
    //   corrupts or zeroes the E820 entries, so we re-init it here
    panic_if(!REINIT_E820(), PANIC_TEXT("Failed to re-initialize E820!"), PANIC_INITIALIZATION_FAILED);
    panic_if(!PAGING_INIT(), PANIC_TEXT("Failed to initialize paging!"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] PAGING OK\n");

    
    panic_if(!PS2_KEYBOARD_INIT(), PANIC_TEXT("Failed to initialize PS2 keyboard"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] PS2 keyboard OK\n");
    
    PCI_INITIALIZE();
    KDEBUG_PUTS("[atOS] PCI enumerated\n");
    // panic_if(!ATA_PIIX3_INIT(), PANIC_TEXT("Failed to initialize ATA DMA"), PANIC_INITIALIZATION_FAILED);
    if (AC97_INIT()) {
        KDEBUG_PUTS("[atOS] AC97 init OK\n");
    } else {
        KDEBUG_PUTS("[atOS] AC97 init FAILED\n");
    }
    // No immediate tone here; melody will play after PIT is ready

    debug_vram_end(); // End early mode, now using task framebuffers
    init_multitasking();
    INIT_RTC();
    STI;

    // No startup melody
    
    panic_if(!ATA_PIO_INIT(), PANIC_TEXT("Failed to initialize ATA PIO"), PANIC_INITIALIZATION_FAILED);
    
    if (RTL8139_START()) {
        KDEBUG_PUTS("[atOS] RTL8139 started\n");
    } else {
        KDEBUG_PUTS("[atOS] RTL8139 start skipped/failed\n");
    }

    // Copies files from ISO to HDD
    panic_if(!initialize_filestructure(), PANIC_TEXT("Failed to initialize FAT on disk"), PANIC_INITIALIZATION_FAILED);

    LOAD_AND_RUN_KERNEL_SHELL();
    RTOSKRNL_LOOP();
    HLT; // just in case
    NOP;
}

__attribute__((noreturn, section(".text")))
void _start(U0) {
    rtos_kernel();
    HLT;
}
