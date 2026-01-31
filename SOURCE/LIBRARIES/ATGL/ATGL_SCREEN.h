#ifndef ATGL_SCREEN_H
#define ATGL_SCREEN_H

#include <STD/TYPEDEF.h>
#include <LIBRARIES/ATGL/ATGL_INPUT.h>
#include <LIBRARIES/ATGL/ATGL_GRAPHICS.h>
#include <LIBRARIES/ATGL/ATGL_DEFS.h>
#include <LIBRARIES/ATGL/ATGL_NODES.h>

typedef enum {
    ATGL_SA_NONE = 0x00000000,
} ATGL_SCREEN_ARGS;

typedef struct {
    VOIDPTR font_data;
    U32 font_data_sz;
    U8 font_width;
    U8 font_height; 
    U8 char_spacing;
    U8 line_spacing;
    U16 char_count;
} ATGL_FONT_DATA;

typedef struct _ATGL_SCREEN {
    PATGL_NODE root_node;      // The desktop/background
    PATGL_NODE focused_node;   // Who gets keyboard input?
    PATGL_NODE mouse_over;     // Who is being hovered?
    
    PU32 back_buffer;          // The "Compositor" buffer
    U32 width, height;

    ATGL_SCREEN_ARGS args;

    ATGL_FONT_DATA font;
} ATGL_SCREEN, *PATGL_SCREEN;

/// @brief Creates a new screen
/// @param args Screen arguments
/// @note If a screen already exists, it is destroyed first, only one screen can exist at a time
/// @return ATGL_SCREEN struct. NULLPTR on failure
PATGL_SCREEN ATGL_CREATE_SCREEN(ATGL_SCREEN_ARGS args);

/// @brief Frees all resources held by a screen
VOID ATGL_DESTROY_SCREEN();

/// @brief Gets the main screen
/// @note A screen must be first created with ATGL_CREATE_SCREEN.  
/// @return Main screen
PATGL_SCREEN ATGL_GET_MAIN_SCREEN();

/// @brief Retrieves the root node of a screen
/// @return Pointer to root node
PATGL_NODE ATGL_GET_SCREEN_ROOT_NODE();

/// @brief Is the screen running?
/// @return TRUE if running, FALSE if not
BOOLEAN8 ATGL_IS_SCREEN_RUNNING();
#endif // ATGL_SCREEN_H