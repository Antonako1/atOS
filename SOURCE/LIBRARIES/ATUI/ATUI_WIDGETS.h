#ifndef ATUI_WIDGETS_H
#define ATUI_WIDGETS_H

#include <LIBRARIES/ATUI/ATUI.h>

// Ready-made widgets for ATUI

// Message box (modal): returns 0=Cancel, 1=OK
INT ATUI_MSGBOX_OK(const CHAR *title, const CHAR *message);
// Yes/No box: returns 1=Yes, 0=No
INT ATUI_MSGBOX_YESNO(const CHAR *title, const CHAR *message);
// Input box: returns 1=OK, 0=Cancel, writes result to buf
INT ATUI_MSGBOX_INPUT(const CHAR *title, const CHAR *message, CHAR *buf, U32 buflen);
// List box: returns selected index or -1 for cancel
INT ATUI_MSGBOX_LIST(const CHAR *title, const CHAR *message, const CHAR **items, U32 item_count);
// Progress bar dialog: returns when done
VOID ATUI_MSGBOX_PROGRESS(const CHAR *title, const CHAR *message, U32 *progress, U32 max);

// Color picker: returns selected color index or -1
INT ATUI_COLOR_PICKER(const CHAR *title, VBE_COLOUR *out_color);
// File picker: returns 1=OK, 0=Cancel, writes path to buf
INT ATUI_FILE_PICKER(const CHAR *title, CHAR *buf, U32 buflen);

// Yes/No prompt (alias for programs that call ATUI_FORM_YESNO)
INT ATUI_FORM_YESNO(const CHAR *question);

// Additional ready-made dialogs
INT ATUI_MSGBOX_ERROR(const CHAR *title, const CHAR *message);
INT ATUI_MSGBOX_INFO(const CHAR *title, const CHAR *message);
INT ATUI_MSGBOX_WARN(const CHAR *title, const CHAR *message);
INT ATUI_MSGBOX_CONFIRM(const CHAR *title, const CHAR *message);
// Spinbox: integer up/down selector
INT ATUI_SPINBOX(const CHAR *title, const CHAR *message, INT min, INT max, INT step, INT *out_val);
// Slider: value selection with left/right
INT ATUI_SLIDER(const CHAR *title, const CHAR *message, INT min, INT max, INT *out_val);
// Keybind picker: waits for a key and returns its code
INT ATUI_KEYBIND_PICKER(const CHAR *title, U32 *out_key);
// Font picker: returns 1=OK, 0=Cancel, writes path to buf
INT ATUI_FONT_PICKER(const CHAR *title, CHAR *buf, U32 buflen);
// About box
VOID ATUI_ABOUT_BOX(const CHAR *app, const CHAR *author, const CHAR *desc);

#endif // ATUI_WIDGETS_H
