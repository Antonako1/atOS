/*+++
    LIBRARIES/ATGL/ATGL_THEME.c - Theme management

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Provides a classic Win32-style default theme and runtime theme switching.
---*/
#include <LIBRARIES/ATGL/ATGL_INTERNAL.h>
#include <STD/MEM.h>

ATGL_THEME ATGL_DEFAULT_THEME(VOID)
{
    ATGL_THEME t;

    /* ---- Classic Win32 silver ---- */
    t.bg             = VBE_COLOUR(192, 192, 192);
    t.fg             = VBE_BLACK;

    t.widget_bg      = VBE_COLOUR(192, 192, 192);
    t.widget_fg      = VBE_BLACK;
    t.widget_border  = VBE_COLOUR(128, 128, 128);

    /* 3-D borders */
    t.highlight      = VBE_WHITE;
    t.shadow         = VBE_COLOUR(128, 128, 128);
    t.dark_shadow    = VBE_BLACK;

    /* Button */
    t.btn_bg         = VBE_COLOUR(192, 192, 192);
    t.btn_fg         = VBE_BLACK;
    t.btn_hover      = VBE_COLOUR(210, 210, 210);
    t.btn_pressed    = VBE_COLOUR(160, 160, 160);
    t.btn_disabled_bg = VBE_COLOUR(192, 192, 192);
    t.btn_disabled_fg = VBE_COLOUR(128, 128, 128);

    /* Text Input */
    t.input_bg          = VBE_WHITE;
    t.input_fg          = VBE_BLACK;
    t.input_border      = VBE_COLOUR(128, 128, 128);
    t.input_focus       = VBE_BLUE;
    t.input_placeholder = VBE_COLOUR(160, 160, 160);

    /* Checkbox / Radio */
    t.check_bg   = VBE_WHITE;
    t.check_fg   = VBE_BLACK;
    t.check_mark = VBE_BLACK;

    /* Slider */
    t.slider_track = VBE_COLOUR(192, 192, 192);
    t.slider_fill  = VBE_BLUE;
    t.slider_thumb = VBE_COLOUR(192, 192, 192);

    /* Progress Bar */
    t.progress_bg   = VBE_WHITE;
    t.progress_fill = VBE_BLUE;
    t.progress_text = VBE_BLACK;
    t.progress_text_bg = VBE_SEE_THROUGH; // Let the fill color show through behind the text

    /* Listbox */
    t.list_bg     = VBE_WHITE;
    t.list_fg     = VBE_BLACK;
    t.list_sel_bg = VBE_BLUE;
    t.list_sel_fg = VBE_WHITE;

    /* Focus */
    t.focus_ring = VBE_COLOUR(100, 100, 150);  /* Subtle blue-grey instead of black */

    /* Metrics (8×8 built-in VBE font) */
    t.border_width = 1;
    t.padding      = 4;
    t.char_w       = 8;
    t.char_h       = 8;

    return t;
}

VOID ATGL_SET_THEME(ATGL_THEME *theme)
{
    if (!theme) return;
    MEMCPY_OPT(&atgl.theme, theme, sizeof(ATGL_THEME));

    /* Force full repaint */
    if (atgl.root) {
        ATGL_NODE_INVALIDATE(atgl.root);
    }
}

ATGL_THEME *ATGL_GET_THEME(VOID)
{
    return &atgl.theme;
}
