/**
 * Include this runtime file in your main.c file.
 * This allows the file to look just like c-standard:
 * 
 * U32 main(U32 argc, PPU8 argv) {
 * 
 *     return 0;
 * }
 * 
 * Everything else is setup by this runtime
 * 
 * Define RUNTIME_GUI for GUI application before including this file!
 * 
 * NOTE: Include only once in your program!
 */
#ifndef RUNTIME_H
#define RUNTIME_H

#include <STD/TYPEDEF.h>
#include <STD/PROC_COM.h>

U32 main(U32 argc, PPU8 argv);

void _start(U32 argc, PPU8 argv) {
    #ifdef RUNTIME_GUI
    PRIC_INIT_GRAPHICAL();
    #else
    PROC_INIT_CONSOLE();
    #endif
    while(!IS_PROC_INITIALIZED());

    U32 code = main(argc, argv);
    EXIT(code);
}

#endif // RUNTIME