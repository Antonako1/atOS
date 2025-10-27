#ifndef SHELL_H
#define SHELL_H
#include <STD/TYPEDEF.h>
#include <PROGRAMS/SHELL/VOUTPUT.h>
#include <PROGRAMS/SHELL/BATSH.h>
#include <DRIVERS/PS2/KEYBOARD.h>
#include <FAT/FAT.h>

typedef enum {
    STATE_CMD_INTERFACE, // A focused PROC is running 
    STATE_EDIT_LINE, // Editing current command line
} SHELL_STATES;

typedef struct {
    U8 *PATH;
} ENV_VARS;

typedef struct {
    U32 focused_pid; // PID of the currently focused process.
    U32 previously_focused_pid;
    OutputHandle cursor;
    SHELL_STATES state;
    BOOLEAN aborted;    
    struct fat_info {
        U32 current_cluster;
        DIR_ENTRY current_path_dir_entry;
        U8 path[FAT_MAX_PATH];
        U8 prev_path[FAT_MAX_PATH];
    } fat_info;
    ENV_VARS VARS;
} SHELL_INSTANCE;

SHELL_INSTANCE *GET_SHNDL(VOID);
U0 SWITCH_PROC_MODE(VOID);


U0 SWITCH_CMD_INTERFACE_MODE(VOID);
VOID HANDLE_KB_CMDI(KEYPRESS *kp, MODIFIERS *mod);
VOID CMD_INTERFACE_LOOP();
U0 EDIT_LINE_LOOP();
U0 EDIT_LINE_MSG_LOOP();
VOID HANDLE_KB_EDIT_LINE(KEYPRESS *kp, MODIFIERS *mod);


void INITIALIZE_DIR(SHELL_INSTANCE *shndl);
PU8 GET_PATH();
PU8 GET_PREV_PATH();
BOOLEAN CD_PREV();
BOOLEAN CD_INTO(PU8 path);
BOOLEAN CD_BACKWARDS_DIR();
PU8 PARSE_CD_RAW_LINE(PU8 line, U32 cut_index);

BOOLEAN MAKE_DIR(PU8 path);
BOOLEAN REMOVE_DIR(PU8 path);

BOOLEAN ResolvePath(PU8 path, PU8 out_buffer, size_t out_buffer_size);

VOID PRINT_CONTENTS(FAT_LFN_ENTRY *dir);
VOID PRINT_CONTENTS_PATH(PU8 path);

#endif // SHELL_H