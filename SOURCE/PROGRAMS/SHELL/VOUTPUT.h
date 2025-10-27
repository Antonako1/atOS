// When compiling, include VIDEODRIVER.c, VBE.c, and VESA.c
// This is a shell-level text output driver using VBE graphics mode
// integrated part of atOShell, not a separate library! Do not use
// in other programs.
#ifndef VOUTPUT_H
#define VOUTPUT_H
#include <DRIVERS/VIDEO/VBE.h>
#include <STD/TYPEDEF.h>
#include <DRIVERS/PS2/KEYBOARD.h>

typedef enum {
    CURSOR_BLOCK = 0,
    CURSOR_UNDERLINE = 1,
    CURSOR_BAR = 2
} CURSOR_STYLE_ENUM;

typedef struct {
    U32 Column;
    U32 Row;
    VBE_PIXEL_COLOUR bgColor;
    VBE_PIXEL_COLOUR fgColor;
    CURSOR_STYLE_ENUM CURSOR_STYLE;
    BOOLEAN CURSOR_BLINK;
    BOOLEAN CURSOR_VISIBLE;
    BOOLEAN INSERT_MODE;
    
    // Screen size in characters
    U32 ROWS;
    U32 COLUMNS;
    
    // Screen width and height in pixels
    U32 SWIDTH;
    U32 SHEIGHT; 

    U32 X_POS; // Left-most pixel of the shell area
    U32 Y_POS; // Top-most pixel of the shell area
} OutputInfo;

typedef OutputInfo* OutputHandle;

OutputHandle GetOutputHandle(void);

#define PATH_END_CHAR " $ "
#define LEND "\r\n" // Line end (CR+LF)
#undef  SCREEN_WIDTH
#undef  SCREEN_HEIGHT
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768

#define CHAR_WIDTH    8
#define CHAR_HEIGHT   16

#define CHAR_SPACING  2
#define AMOUNT_OF_COLS (SCREEN_WIDTH / (CHAR_WIDTH + CHAR_SPACING))
#define AMOUNT_OF_ROWS (SCREEN_HEIGHT / (CHAR_HEIGHT + CHAR_SPACING))
#define CHARACTERS 256 // Extended ASCII characters
#define CUR_LINE_MAX_LENGTH (AMOUNT_OF_ROWS*AMOUNT_OF_COLS)
#define CMD_LINE_HISTORY 25

U8 *GET_CURRENT_LINE();
VOID REDRAW_CURRENT_LINE();
VOID PRINTNEWLINE(VOID);
U0 INIT_SHELL_VOUTPUT(VOID);
// Increases the cursor column by one, moving to next row if at end
U0 COLUMN_INC(U0);
// Decreases the cursor column by one, if not at the start
U0 COLUMN_DEC(U0);
// Increases the cursor row by one, moving to next column if at end
U0 ROW_INC(U0);
// Decreases the cursor row by one, if not at the top
U0 ROW_DEC(U0);
// Sets the background color
U0 SET_BG_COLOR(VBE_PIXEL_COLOUR color);
// Sets the foreground (text) color
U0 SET_FG_COLOR(VBE_PIXEL_COLOUR color);
// Sets the cursor position to (col, row)
U0 SET_CURSOR_POS(U32 col, U32 row);
// Gets the current cursor position as a single U32, with high 16 bits as row and low 16 bits as column
U32 GET_CURSOR_POS(U0);
// Sets the visibility of the cursor
U0 SET_CURSOR_VISIBLE(BOOLEAN visible);
// Enables or disables cursor blinking
U0 SET_CURSOR_BLINK(BOOLEAN blink);
// Enables or disables automatic scrolling when the cursor moves beyond the last row
U0 SET_AUTO_SCROLL(BOOLEAN enable);
// Enables or disables automatic line wrapping when the end of a line is reached
U0 SET_AUTO_WRAP(BOOLEAN enable);
// Writes a null-terminated string to the screen
U32 PUTS(U8 *str);
// Draws a single character at (x, y) in pixels
U32 PUTC(U8 c);
// Prints a U32 as BIN
U32 PUT_BIN(U32 n);
// Prints a U32 as HEX
U32 PUT_HEX(U32 n);
// Prints a U32 as DEC
U32 PUT_DEC(U32 n);
void RESTORE_CURSOR_UNDERNEATH(U32 col, U32 row);
// Moves the cursor to the start of the next row
VOID NEW_ROW(VOID);

// Draws the shell line start (current path and "> ")
VOID PUT_SHELL_START(VOID);

// Outputs the current working directory path to the screen
VOID PUT_CURRENT_PATH(VOID);

// Scrolls the text buffer up by one row
VOID BLINK_CURSOR(VOID);

VOID DELETE_CHAR_AT_CURSOR(VOID);
VOID DELETE_CHAR_AT_CURSOR();

// Handles a backspace key press, deleting the character before the cursor
VOID HANDLE_BACKSPACE(VOID);

// Clears the screen and resets cursor position
VOID CLS(U0);
// Immediately updates the screen with the current text buffer
VOID FLUSH_SCREEN(U0);
// Converts a text column number to pixel X coordinate
U32 COL_TO_PIX(U32 col);
// Converts a text row number to pixel Y coordinate
U32 ROW_TO_PIX(U32 row);

VOID DRAW_TEXT_BUFFER(VOID);
VOID MOVE_BUFFER_CONTENTS_LEFT_FROM(U32 col, U32 row);
VOID CLEAR_TEXTBUFFER(VOID);
VOID REDRAW_CHAR(U32 col, U32 row);

VOID HANDLE_KEY_ARROW_LEFT();
VOID HANDLE_KEY_ARROW_RIGHT();
VOID HANDLE_KEY_ARROW_UP();
VOID HANDLE_KEY_ARROW_DOWN();
VOID HANDLE_KEY_DELETE();
VOID HANDLE_KEY_ENTER();
VOID HANDLE_CTRL_C();
VOID TOGGLE_INSERT_MODE();
VOID SET_CURSOR_BLINK(BOOLEAN blink);
VOID TOGGLE_INSERT_MODE();
VOID HANDLE_LE_ENTER();
VOID HANDLE_LE_BACKSPACE();
VOID HANDLE_LE_DELETE();
VOID HANDLE_LE_ARROW_LEFT();
VOID HANDLE_LE_ARROW_RIGHT();
VOID HANDLE_LE_ARROW_DOWN();
VOID HANDLE_LE_ARROW_UP();
VOID HANDLE_LE_DEFAULT(KEYPRESS *kp, MODIFIERS *mod);


void HANDLE_LE_CTRL_RIGHT();
void HANDLE_LE_CTRL_LEFT();
void HANDLE_LE_HOME();
void HANDLE_LE_END();
void HANDLE_LE_CTRL_A();
void HANDLE_LE_CTRL_E();
void HANDLE_LE_CTRL_U();
void HANDLE_LE_CTRL_K();
void HANDLE_LE_CTRL_W();
void HANDLE_LE_CTRL_Y();
void HANDLE_LE_CTRL_L();
void HANDLE_LE_CTRL_TAB();
void HANDLE_LE_SHIFT_ALT_DEL();
#endif // VOUTPUT_H