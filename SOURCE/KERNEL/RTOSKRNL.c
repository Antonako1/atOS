#ifndef RTOS_KERNEL
#define RTOS_KERNEL
#endif
#include <RTOSKRNL.h>

#include <DRIVERS/CMOS/CMOS.h>

#define dbg(x, y, colour) do { \
    VBE_DRAW_LINE(x, y, x + 5, y + 5, colour); \
    VBE_STOP_DRAWING(); \
} while(0)

static U16 row = 1;
void strd(const char *str) {

    static U16 pg = 0;
    RTC_DATE_TIME dt = GET_SYS_TIME();
    U8 str222[100];
    SPRINTF(str222, "%02d/%02d/%02d, %02d:%02d.%02d", 
        dt.day_of_month,
        dt.month,
        dt.year,
        dt.hours,
        dt.minutes,
        dt.seconds
    );
    VBE_DRAW_STRING(10, row * 10, str222, VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(200, row * 10, str, VBE_WHITE, VBE_BLACK);
    VBE_STOP_DRAWING();
    row++;
    if(row >= 74) {
        row = 0;
        VBE_CLEAR_SCREEN(VBE_BLACK);
        pg++;
    }
    U8 buf[20];
    ITOA_U(pg, buf, 10);
    VBE_DRAW_STRING(10, row * 10, buf, VBE_WHITE, VBE_BLACK);
}

__attribute__((noreturn))
void rtos_kernel(U0) {
    debug_vram_start(); // Start in early mode, using direct framebuffer access
    VBE_CLEAR_SCREEN(VBE_BLACK);
    VBE_DRAW_TRIANGLE(10, 10, 30, 10, 20, 30, VBE_BLACK); // Green triangle for early graphics test
    VBE_STOP_DRAWING();
    
    CLI;
    // SERIAL_INIT();
    // SERIAL_WRITE_DATA(COM1_PORT, "Serial ports initialized1\n", 24);
    // SERIAL_WRITE_DATA(COM2_PORT, "Serial ports initialized2\n", 24);
    KDEBUG_PUTS("[atOS] Serial ports initialized\n");
    KDEBUG_PUTS("[atOS] rtos_kernel start\n");
    // These are called here, to fix address issues and to set up new values
    GDT_INIT();

    KDEBUG_PUTS("[atOS] GDT OK\n");
    IDT_INIT();
    KDEBUG_PUTS("[atOS] IDT OK\n");
    IRQ_INIT();
    KDEBUG_PUTS("[atOS] IRQ OK\n");
    SETUP_ISR_HANDLERS();
    KDEBUG_PUTS("[atOS] ISR OK\n");
    // fpu_disable();
    fpu_enable();
    // strd("FPU enabled");
    KDEBUG_PUTS("[atOS] FPU OK\n");
    
    panic_if(!vesa_check(), PANIC_TEXT("Failed to initialize VESA"), PANIC_INITIALIZATION_FAILED);
    strd("VESA detected and mode set");
    panic_if(!vbe_check(), PANIC_TEXT("Failed to initialize VBE"), PANIC_INITIALIZATION_FAILED);
    strd("VESA/VBE initialized");
    KDEBUG_PUTS("[atOS] VESA/VBE OK\n");
    panic_if(INITIALIZE_ATAPI() == ATA_FAILED, PANIC_TEXT("Failed to initialize ATAPI interface!"), PANIC_INITIALIZATION_FAILED);
    strd("ATAPI interface initialized");
    panic_if(!E820_INIT(), PANIC_TEXT("Failed to initialize E820 memory map!"), PANIC_INITIALIZATION_FAILED);
    strd("E820 memory map initialized");
    KDEBUG_PUTS("[atOS] E820 OK\n");

    panic_if(!PAGEFRAME_INIT(), PANIC_TEXT("Failed to initialize page frame! Possibly not enough memory."), PANIC_INITIALIZATION_FAILED);
    strd("Pageframe initialized");
    KDEBUG_PUTS("[atOS] PAGEFRAME OK\n");
    panic_if(!KHEAP_INIT(KHEAP_MAX_SIZE_PAGES), PANIC_TEXT("Failed to initialize kheap. Not enough memory or pageframe issue."), PANIC_INITIALIZATION_FAILED);    
    KDEBUG_PUTS("[atOS] KHEAP OK\n");
    // For some reason, PAGEFRAME or something? -
    //   corrupts or zeroes the E820 entries, so we re-init it here
    strd("KHEAP initialized");
    panic_if(!REINIT_E820(), PANIC_TEXT("Failed to re-initialize E820!"), PANIC_INITIALIZATION_FAILED);
    strd("E820 entries re-initialized after pageframe setup");
    KDEBUG_PUTS("[atOS] E820 REINIT OK\n");
    panic_if(!PAGING_INIT(), PANIC_TEXT("Failed to initialize paging!"), PANIC_INITIALIZATION_FAILED);
    strd("Paging initialized");
    KDEBUG_PUTS("[atOS] PAGING OK\n");
    
    strd("Basic hardware initialization complete");
    panic_if(!PS2_KEYBOARD_INIT(), PANIC_TEXT("Failed to initialize PS2 keyboard"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] PS2 keyboard OK\n");
    strd("PS/2 keyboard initialized");
    panic_if(!PS2_MOUSE_INIT(), PANIC_TEXT("Failed to initialize PS2 mouse"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] PS2 mouse OK\n");
    strd("PS/2 keyboard and mouse initialized");
    PCI_INITIALIZE();
    strd("PCI bus enumerated");
    KDEBUG_PUTS("[atOS] PCI enumerated\n");
    panic_if(!ATA_PIIX3_INIT(), PANIC_TEXT("Failed to initialize ATA DMA"), PANIC_INITIALIZATION_FAILED);
    strd("ATA PIIX3 DMA initialized");
    KDEBUG_PUTS("[atOS] ATA PIIX3 DMA OK\n");

    if (AC97_INIT()) {
        KDEBUG_PUTS("[atOS] AC97 init OK\n");
        strd("AC97 audio initialized");
    } else {
        strd("AC97 audio initialization failed, audio disabled");
        KDEBUG_PUTS("[atOS] AC97 init FAILED\n");
    }
    // No immediate tone here; melody will play after PIT is ready
    // debug_vram_end(); // End early mode, now using task framebuffers
    CLI; // INIT_RTC enables interrupts internally; re-disable until fully ready
    STI;

    // No startup melody
    strd("Initialization complete, launching kernel shell...");
    
    panic_if(!ATA_PIO_INIT(), PANIC_TEXT("Failed to initialize ATA PIO"), PANIC_INITIALIZATION_FAILED);
    KDEBUG_PUTS("[atOS] ATA PIO init OK\n");
    strd("ATA PIO initialized");

    // if (RTL8139_START()) {
    //     KDEBUG_PUTS("[atOS] RTL8139 started\n");
    //     strd("RTL8139 network card initialized");
    // } else {
    //     KDEBUG_PUTS("[atOS] RTL8139 start skipped/failed\n");
    //     strd("RTL8139 network card initialization failed, network disabled");
    // }

    // Copies files from ISO to HDD
    strd("Setting up filesystem structure on disk...");
    panic_if(!initialize_filestructure(), PANIC_TEXT("Failed to initialize FAT on disk"), PANIC_INITIALIZATION_FAILED);
    strd("Filesystem structure initialized on disk");
    strd("Initialization complete, launching kernel shell...");

    init_multitasking(); // start multitasking only after all hardware is ready
    INIT_RTC();
    
    LOAD_AND_RUN_KERNEL_SHELL();
    RTOSKRNL_LOOP();
    HLT; // just in case
    NOP; // debug nop
}

__attribute__((noreturn, section(".text")))
void _start(U0) {
    rtos_kernel();
    HLT;
}
