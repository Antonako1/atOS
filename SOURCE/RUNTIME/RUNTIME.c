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

static exit_func_t exit_funcs[MAX_ON_EXIT_FUNCTIONS] ATTRIB_DATA = { 0 };
static U32 exit_func_count ATTRIB_DATA = 0;

BOOL ON_EXIT(exit_func_t fn);
static void call_exit_functions(void);
U32 main(U32 argc, PPU8 argv);

void _start(U32 argc, PPU8 argv) {
    NOP;
    NOP;
    NOP;
    NOP;
    NOP;
#ifdef RUNTIME_GUI
    PRIC_INIT_GRAPHICAL();
#else
    PROC_INIT_CONSOLE();
#endif

    while (!IS_PROC_INITIALIZED());

    U32 code = main(argc, argv);

    call_exit_functions();

    EXIT(code);
    HLT;
}

static void call_exit_functions(void) {
    while (exit_func_count > 0) {
        exit_func_count--;
        exit_func_t fn = exit_funcs[exit_func_count];
        if (fn) fn();
    }
}
BOOL ON_EXIT(exit_func_t fn) {
    if (exit_func_count >= MAX_ON_EXIT_FUNCTIONS) return FALSE;
    exit_funcs[exit_func_count++] = fn;
    return TRUE;
}