#include "AMPLITE.h"
#include <STD/AUDIO.h>
#include <RUNTIME/RUNTIME.h>

ATGL_NODE *root_node ATTRIB_DATA = NULLPTR;
static PATGL_NODE window_panel ATTRIB_DATA = NULLPTR;
static PATGL_NODE status_label ATTRIB_DATA = NULLPTR;
static PATGL_NODE freq_label ATTRIB_DATA = NULLPTR;
static PATGL_NODE duration_label ATTRIB_DATA = NULLPTR;
static PATGL_NODE amp_label ATTRIB_DATA = NULLPTR;
static PATGL_NODE freq_slider ATTRIB_DATA = NULLPTR;
static PATGL_NODE duration_slider ATTRIB_DATA = NULLPTR;
static PATGL_NODE amp_slider ATTRIB_DATA = NULLPTR;
static PATGL_NODE loop_checkbox ATTRIB_DATA = NULLPTR;
static PATGL_NODE multiple_tones_checkbox ATTRIB_DATA = NULLPTR;

static VOID AMPLITE_UPDATE_LABELS(VOID) {
    CHAR buf[64];
    BOOL loop_mode;

    if (!freq_slider || !duration_slider || !amp_slider || !loop_checkbox)
        return;

    loop_mode = ATGL_CHECKBOX_GET(loop_checkbox);

    SNPRINTF(buf, sizeof(buf), "Frequency: %d Hz", ATGL_SLIDER_GET_VALUE(freq_slider));
    ATGL_NODE_SET_TEXT(freq_label, (PU8)buf);

    if (loop_mode) {
        ATGL_NODE_SET_TEXT(duration_label, (PU8)"Duration: loop");
    } else {
        SNPRINTF(buf, sizeof(buf), "Duration: %d ms", ATGL_SLIDER_GET_VALUE(duration_slider));
        ATGL_NODE_SET_TEXT(duration_label, (PU8)buf);
    }

    SNPRINTF(buf, sizeof(buf), "Amplitude: %d", ATGL_SLIDER_GET_VALUE(amp_slider));
    ATGL_NODE_SET_TEXT(amp_label, (PU8)buf);

    ATGL_NODE_SET_ENABLED(duration_slider, !loop_mode);
}

static VOID AMPLITE_SET_MULTIPLE_TONES_MODE(BOOL enabled) {
    if (!multiple_tones_checkbox) return;
    ATGL_CHECKBOX_SET(multiple_tones_checkbox, enabled);
}

static VOID AMPLITE_SET_STATUS(PU8 text) {
    if (status_label)
        ATGL_NODE_SET_TEXT(status_label, text);
}

static VOID AMPLITE_ON_CONTROL_CHANGE(PATGL_NODE node) {
    (void)node;
    AMPLITE_UPDATE_LABELS();
}

static VOID AMPLITE_ON_STOP(PATGL_NODE node) {
    (void)node;
    AUDIO_STOP();
    AMPLITE_SET_STATUS((PU8)"Stopped");
}

static VOID AMPLITE_ON_PLAY(PATGL_NODE node) {
    CHAR buf[64];
    U32 freq;
    U32 amp;
    U32 duration;

    (void)node;

    freq = (U32)ATGL_SLIDER_GET_VALUE(freq_slider);
    amp = (U32)ATGL_SLIDER_GET_VALUE(amp_slider);
    duration = ATGL_CHECKBOX_GET(loop_checkbox)
             ? 0
             : (U32)ATGL_SLIDER_GET_VALUE(duration_slider);

    if (!multiple_tones_checkbox || !ATGL_CHECKBOX_GET(multiple_tones_checkbox))
        AUDIO_STOP();

    if (!AUDIO_TONE(freq, duration, amp, 44100)) {
        AMPLITE_SET_STATUS((PU8)"Audio device not ready");
        return;
    }

    if (duration == 0) {
        SNPRINTF(buf, sizeof(buf), "Playing %u Hz in loop", freq);
    } else {
        SNPRINTF(buf, sizeof(buf), "Playing %u Hz for %u ms", freq, duration);
    }

    AMPLITE_SET_STATUS((PU8)buf);
}

static VOID AMPLITE_ON_QUIT(PATGL_NODE node) {
    (void)node;
    AUDIO_STOP();
    ATGL_QUIT();
}

CMAIN() {
    U32 screen_w;
    U32 screen_h;
    I32 panel_x;
    I32 panel_y;

    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);
    ON_EXIT(AUDIO_STOP);
    ON_EXIT(ATGL_QUIT);
    
    ATGL_INIT();
    ATGL_CREATE_SCREEN(ATGL_SA_NONE);

    root_node = ATGL_GET_SCREEN_ROOT_NODE();
    screen_w = ATGL_GET_SCREEN_WIDTH();
    screen_h = ATGL_GET_SCREEN_HEIGHT();
    panel_x = (I32)((screen_w > 360) ? ((screen_w - 360) / 2) : 8);
    panel_y = (I32)((screen_h > 220) ? ((screen_h - 220) / 2) : 8);

    window_panel = ATGL_CREATE_PANEL(root_node, (ATGL_RECT){panel_x, panel_y, 360, 220},
                                     ATGL_LAYOUT_NONE, 0, 0);
    ATGL_NODE_SET_COLORS(window_panel, VBE_WHITE, RGB(24, 28, 40));
    ATGL_NODE_SET_TEXT(window_panel, (PU8)"Amplite");

    ATGL_CREATE_LABEL(window_panel, (ATGL_RECT){12, 12, 220, 16},
                      (PU8)"Amplite - simple audio player",
                      RGB(255, 255, 0), VBE_SEE_THROUGH);

    status_label = ATGL_CREATE_LABEL(window_panel, (ATGL_RECT){12, 32, 320, 16},
                                     (PU8)"Ready",
                                     RGB(220, 220, 220), VBE_SEE_THROUGH);

    ATGL_CREATE_SEPARATOR(window_panel, (ATGL_RECT){12, 54, 336, 4});

    freq_label = ATGL_CREATE_LABEL(window_panel, (ATGL_RECT){12, 68, 140, 16},
                                   (PU8)"Frequency: 440 Hz",
                                   RGB(255, 255, 255), VBE_SEE_THROUGH);
    freq_slider = ATGL_CREATE_SLIDER(window_panel, (ATGL_RECT){156, 66, 192, 20},
                                     110, 1760, 440, 10);
    freq_slider->on_change = AMPLITE_ON_CONTROL_CHANGE;

    duration_label = ATGL_CREATE_LABEL(window_panel, (ATGL_RECT){12, 96, 140, 16},
                                       (PU8)"Duration: 1000 ms",
                                       RGB(255, 255, 255), VBE_SEE_THROUGH);
    duration_slider = ATGL_CREATE_SLIDER(window_panel, (ATGL_RECT){156, 94, 192, 20},
                                         100, 5000, 1000, 100);
    duration_slider->on_change = AMPLITE_ON_CONTROL_CHANGE;

    multiple_tones_checkbox = ATGL_CREATE_CHECKBOX(window_panel, (ATGL_RECT){12, 118, 132, 18},
                                                   (PU8)"Multiple tones", FALSE,
                                                   AMPLITE_ON_CONTROL_CHANGE);
    multiple_tones_checkbox->fg = RGB(255, 255, 255);
    multiple_tones_checkbox->bg = VBE_SEE_THROUGH;
    
    loop_checkbox = ATGL_CREATE_CHECKBOX(window_panel, (ATGL_RECT){156, 118, 180, 18},
                                         (PU8)"Loop playback", FALSE,
                                         AMPLITE_ON_CONTROL_CHANGE);
    loop_checkbox->fg = RGB(255, 255, 255);
    loop_checkbox->bg = VBE_SEE_THROUGH;

    ATGL_GET_THEME()->check_mark = RGB(255, 255, 255);

    amp_label = ATGL_CREATE_LABEL(window_panel, (ATGL_RECT){12, 146, 140, 16},
                                  (PU8)"Amplitude: 16000",
                                  RGB(255, 255, 255), VBE_SEE_THROUGH);
    amp_slider = ATGL_CREATE_SLIDER(window_panel, (ATGL_RECT){156, 144, 192, 20},
                                    1000, 48000, 16000, 1000);
    amp_slider->on_change = AMPLITE_ON_CONTROL_CHANGE;

    amp_slider->bg = VBE_SEE_THROUGH;
    duration_slider->bg = VBE_SEE_THROUGH;
    freq_slider->bg = VBE_SEE_THROUGH;

    ATGL_CREATE_BUTTON(window_panel, (ATGL_RECT){12, 182, 100, 26},
                       (PU8)"Play", AMPLITE_ON_PLAY);
    ATGL_CREATE_BUTTON(window_panel, (ATGL_RECT){124, 182, 100, 26},
                       (PU8)"Stop", AMPLITE_ON_STOP);
    ATGL_CREATE_BUTTON(window_panel, (ATGL_RECT){236, 182, 112, 26},
                       (PU8)"Quit", AMPLITE_ON_QUIT);

    AMPLITE_SET_MULTIPLE_TONES_MODE(FALSE);
    AMPLITE_UPDATE_LABELS();
    return 0;
}

VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev) {
    if (ev->type == ATGL_EVT_KEY_DOWN) {
        if (ev->key.keycode == KEY_ESC) {
            AUDIO_STOP();
            ATGL_QUIT();
            return;
        }

        if (ev->key.keycode == KEY_ENTER) {
            AMPLITE_ON_PLAY(NULLPTR);
            return;
        }

        if (ev->key.keycode == KEY_SPACE) {
            AMPLITE_ON_STOP(NULLPTR);
            return;
        }
    }

    ATGL_DISPATCH_EVENT(root_node, ev);
}

VOID ATGL_GRAPHICS_LOOP(U32 ticks) {
    (void)ticks;
    ATGL_RENDER_TREE(root_node);
}