/**
 * atOS C-Runtime
 * 
 * - Sets up environment.
 * - Calls U32 main(U32, PPU8)
 * - Registers up to MAX_ON_EXIT_FUNCTIONS exit functions
 * - Exits once returned from main
 */
#include <RUNTIME/RUNTIME.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>
#include <STD/IO.h>
static exit_func_t exit_funcs[MAX_ON_EXIT_FUNCTIONS] ATTRIB_DATA = { 0 };
static U32 exit_func_count ATTRIB_DATA = 0;

BOOL ON_EXIT(exit_func_t fn);
static void call_exit_functions(void);

#ifndef RUNTIME_ATGL
U32 main(U32 argc, PPU8 argv);
void _start(U32 argc, PPU8 argv) 
{
    DEBUG_PRINTF("[RUNTIME] Entered _start with argc:%d\n", argc);

    #ifdef RUNTIME_GUI
    PRIC_INIT_GRAPHICAL();
    #else
    PROC_INIT_CONSOLE();
    #endif
    U32 timeout = U32_MAX;
    #ifdef RUNTIME_GUI
    while(!IS_PROC_GUI_INITIALIZED()) 
    {
    #else
    while (!IS_PROC_INITIALIZED()) 
    {
    #endif
        if(timeout-- == 0) 
        {
            DEBUG_PRINTF("[RUNTIME] Process initialization timed out!\n");
            EXIT(-1);
        };
    }
    DEBUG_PRINTF("[RUNTIME] Entering main!\n");
    KB_MS_INIT();
    U32 code = main(argc, argv);

    call_exit_functions();
    DEBUG_PRINTF("[RUNTIME] Exiting with code %d\n", code);
    EXIT(code);
}

#else // RUNTIME_ATGL
#include <LIBRARIES/ATGL/ATGL.h>
U32 ATGL_MAIN(U32 argc, PPU8 argv);
VOID ATGL_GRAPHICS_LOOP(U32 ticks);
VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev);

U32 freq ATTRIB_DATA = 24;
VOID SET_GRAPHIC_LOOP_CALL_FREQUENCY(U32 frequency) {
    freq = frequency;
}

void _start(U32 argc, PPU8 argv) 
{
    DEBUG_PRINTF("[RUNTIME_ATGL] Entered _start with argc:%d\n",argc);
    PRIC_INIT_GRAPHICAL();
    U32 timeout = U32_MAX;
    while(!IS_PROC_GUI_INITIALIZED()) 
    {
        if(timeout-- == 0) 
        {
            DEBUG_PRINTF("[RUNTIME_ATGL] Process initialization timed out!\n");
            EXIT(-1);
        };
    }
    DEBUG_PRINTF("[RUNTIME_ATGL] Entering ATGL_MAIN!\n");
    KB_MS_INIT();
    U32 code = ATGL_MAIN(argc, argv);
    if(code != 0) {
        DEBUG_PRINTF("[RUNTIME_ATGL] ATGL_MAIN returned error code %d, exiting immediately.\n", code);
        EXIT(code);
    }
    
    U32 current_tick;
    U32 last_frame_tick = GET_PIT_TICKS();

    while(ATGL_IS_SCREEN_RUNNING()) 
    {
        ATGL_EVENT ev;
        ATGL_POLL_EVENT(&ev);
        if(ev.type != ATGL_EV_NONE) ATGL_EVENT_LOOP(&ev);

        current_tick = GET_PIT_TICKS();
        
        // Use >= to handle cases where the PIT might skip a count 
        // if Event Loop took too long.
        if(current_tick - last_frame_tick >= freq) {
            ATGL_GRAPHICS_LOOP(current_tick);
            
            // Increment by freq rather than setting to current_tick
            // to prevent "timing drift" over long periods.
            last_frame_tick += freq; 
        }
    }
    call_exit_functions();
    DEBUG_PRINTF("[RUNTIME_ATGL] Exiting with code %d\n", code);
    EXIT(code);
}
#endif // RUNTIME_ATGL


static void call_exit_functions(void) {
    while (exit_func_count > 0) {
        exit_func_count--;
        exit_func_t fn = exit_funcs[exit_func_count];
        if (fn) fn();
        DEBUG_PRINTF("[RUNTIME] Calling exit function[%d]!\n", exit_func_count);
    }
}
BOOL ON_EXIT(exit_func_t fn) {
    if (exit_func_count >= MAX_ON_EXIT_FUNCTIONS) return FALSE;
    exit_funcs[exit_func_count++] = fn;
    return TRUE;
}