#include <LIBRARIES/ATWAV/ATWAV.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/AUDIO.h>
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include "JAMMER.h"

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Extract the filename component from a full path (last '/' + 1) */
static const CHAR *basename(const CHAR *path) {
    const CHAR *p = path;
    const CHAR *last = path;
    while (*p) { if (*p == '/') last = p + 1; p++; }
    return last;
}

/* ------------------------------------------------------------------ */
/* UI                                                                  */
/* ------------------------------------------------------------------ */

/* Layout (WIN_H=13, interior rows 0..10):
 *  row 0 : Track N/M
 *  row 1 : filename
 *  row 2 : (blank)
 *  row 3 : progress bar + time
 *  row 4 : (blank)
 *  row 5 : visualizer label
 *  row 6 : visualizer bars  (8 bands)
 *  row 7 : (blank)
 *  row 8 : status  PLAYING / PAUSED
 *  row 9 : (blank)
 *  row 10: hints
 */
#define WIN_W  66
#define WIN_H  13

#define COL_TITLE   VBE_COLOUR(  0, 170, 255)  /* cyan          */
#define COL_TRACK   VBE_WHITE                   /* white         */
#define COL_BAR_FG  VBE_COLOUR(  0, 255, 136)  /* mint green    */
#define COL_BAR_BG  VBE_COLOUR( 68,  68,  68)  /* grey          */
#define COL_STATUS  VBE_COLOUR(255, 255,  68)  /* yellow        */
#define COL_HINT    VBE_COLOUR(136, 136, 136)  /* dim           */
#define COL_BG      VBE_COLOUR( 17,  17,  17)  /* near-black bg */
#define COL_BORDER  VBE_COLOUR(  0, 170, 255)  /* cyan border   */

/* Visualizer colours: low → mid → high amplitude */
#define COL_VIZ_LOW  VBE_COLOUR(  0, 160,  80)
#define COL_VIZ_MID  VBE_COLOUR(255, 200,   0)
#define COL_VIZ_HIGH VBE_COLOUR(255,  60,  60)

#define VIZ_BANDS    8
/* Interior width is WIN_W-2=64; leave 2 cols margin each side → 60 usable.
 * Each band gets 60/8 = 7 chars (56 total), centred with 2 extra on each side). */
#define VIZ_BAND_W   7

static ATUI_WINDOW *g_win = NULLPTR;

static void ui_init(U32 rows, U32 cols) {
    U32 wy = (rows > WIN_H) ? (rows - WIN_H) / 2 : 0;
    U32 wx = (cols > WIN_W) ? (cols - WIN_W) / 2 : 0;
    g_win = ATUI_NEWWIN(WIN_H, WIN_W, wy, wx);
    ATUI_WSET_COLOR(g_win, COL_BORDER, COL_BG);
    ATUI_WBOX_STYLED(g_win, ATUI_BORDER_DOUBLE);
    ATUI_WSET_TITLE(g_win, " \x0E JAMMER ");
    ATUI_WREFRESH(g_win);
}

/* Map amplitude 0..32767 to one of 5 block glyphs (index 0..4) */
static CHAR viz_glyph(U32 amp) {
    if (amp < 1500)  return ' ';
    if (amp < 8000)  return (CHAR)ATUI_SHADE_LIGHT;
    if (amp < 16000) return (CHAR)ATUI_SHADE_MED;
    if (amp < 24000) return (CHAR)ATUI_SHADE_DARK;
    return (CHAR)ATUI_BLOCK_FULL;
}

static VBE_COLOUR viz_colour(U32 amp) {
    if (amp < 16000) return COL_VIZ_LOW;
    if (amp < 24000) return COL_VIZ_MID;
    return COL_VIZ_HIGH;
}

static void ui_draw(const CHAR *track, U32 track_idx, U32 track_total,
                    U32 elapsed, U32 total, BOOL paused) {
    /* Interior width available for content (excluding the 1-char border each side) */
    const U32 IW = WIN_W - 2;

    /* -- row 0: track counter -- */
    ATUI_WSET_COLOR(g_win, COL_HINT, COL_BG);
    ATUI_WMVPRINTW(g_win, 0, 2, "Track %u/%u", track_idx + 1, track_total);

    /* -- row 1: filename -- */
    ATUI_WSET_COLOR(g_win, COL_TRACK, COL_BG);
    CHAR namestr[IW + 1];
    U32 maxname = IW - 4;
    U32 namelen = STRLEN(track);
    if (namelen >= maxname) namelen = maxname - 1;
    MEMCPY(namestr, track, namelen);
    for (U32 i = namelen; i < maxname; i++) namestr[i] = ' ';
    namestr[maxname] = '\0';
    ATUI_WMVPRINTW(g_win, 1, 2, "%s", namestr);

    /* -- row 3: progress bar -- */
    const U32 BAR_W = IW - 14;   /* leave room for " MM:SS/MM:SS" */
    U32 filled = (total > 0 && elapsed <= total) ? (elapsed * BAR_W / total) : 0;
    if (filled > BAR_W) filled = BAR_W;
    ATUI_WMOVE(g_win, 3, 2);
    for (U32 i = 0; i < BAR_W; i++) {
        if (i < filled) {
            ATUI_WSET_COLOR(g_win, COL_BAR_FG, COL_BG);
            ATUI_WADDCH(g_win, (CHAR)ATUI_BLOCK_FULL);
        } else {
            ATUI_WSET_COLOR(g_win, COL_BAR_BG, COL_BG);
            ATUI_WADDCH(g_win, (CHAR)ATUI_SHADE_LIGHT);
        }
    }
    ATUI_WSET_COLOR(g_win, COL_HINT, COL_BG);
    ATUI_WPRINTW(g_win, " %02u:%02u/%02u:%02u",
                 elapsed / 60, elapsed % 60,
                 total   / 60, total   % 60);

    /* -- row 5: visualizer label -- */
    ATUI_WSET_COLOR(g_win, COL_HINT, COL_BG);
    ATUI_WMVADDSTR(g_win, 5, 2, "SPECTRUM");

    /* -- row 6: visualizer bars -- */
    U32 viz[VIZ_BANDS];
    AUDIO_GET_VIZ(viz, VIZ_BANDS);
    /* When paused, decay bars toward zero so the display isn't frozen. */
    if (paused) {
        for (U32 b = 0; b < VIZ_BANDS; b++)
            viz[b] = viz[b] * 3 / 4;
    }
    for (U32 b = 0; b < VIZ_BANDS; b++) {
        U32 col_x = 2 + b * VIZ_BAND_W;
        CHAR glyph = viz_glyph(viz[b]);
        ATUI_WSET_COLOR(g_win, viz_colour(viz[b]), COL_BG);
        ATUI_WMOVE(g_win, 6, col_x);
        for (U32 c = 0; c < VIZ_BAND_W - 1; c++)
            ATUI_WADDCH(g_win, glyph);
        /* 1-char gap between bands */
        ATUI_WSET_COLOR(g_win, COL_HINT, COL_BG);
        ATUI_WADDCH(g_win, ' ');
    }

    /* -- row 8: status -- */
    ATUI_WSET_COLOR(g_win, COL_STATUS, COL_BG);
    if (paused)
        ATUI_WMVADDSTR(g_win, 8, 2, "\x10\x10 PAUSED         ");
    else
        ATUI_WMVADDSTR(g_win, 8, 2, "\x10  PLAYING         ");

    /* -- row 10: hints -- */
    ATUI_WSET_COLOR(g_win, COL_HINT, COL_BG);
    ATUI_WMVADDSTR(g_win, 10, 2, "[SPC] Pause  [N] Next  [Q/ESC] Quit");

    ATUI_WREFRESH(g_win);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

CMAIN() {
    const CHAR *default_playlist[] = { "/HOME/AUDIO/ZZTOPSDM.WAV" };
    const CHAR **files;
    U32 file_count;

    if (argc > 1) {
        files      = (const CHAR **)&argv[1];
        file_count = (U32)(argc - 1);
    } else {
        files      = default_playlist;
        file_count = 1;
    }

    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);

    ATUI_INITIALIZE("/HOME/FONTS/SYS_FONT.FNT", ATUIC_RAW);
    ATUI_TIMEOUT(0);
    ATUI_CURSOR_SET(FALSE);

    ATUI_DISPLAY *disp = GET_DISPLAY();
    U32 rows = disp ? disp->rows : 25;
    U32 cols = disp ? disp->cols : 80;

    ATUI_CLEAR();
    ATUI_REFRESH();
    ui_init(rows, cols);

    BOOL quit      = FALSE;
    U32  track_idx = 0;

    while (!quit) {
        const CHAR *path = files[track_idx];

        WAV_AUDIO_STREAM *stream = WAV_OPEN((PU8)path);
        if (!stream) {
            ATUI_WSET_COLOR(g_win, VBE_COLOUR(255, 68, 68), COL_BG);
            ATUI_WMVPRINTW(g_win, 3, 2, "Cannot open: %s", basename(path));
            ATUI_WREFRESH(g_win);
            CPU_SLEEP(2000);
            /* Skip bad track; if only one track, give up */
            if (file_count == 1) break;
            track_idx = (track_idx + 1) % file_count;
            continue;
        }

        U32 total = WAV_TOTAL_SECONDS(stream);
        WAV_PLAY(stream);

        BOOL paused = FALSE;
        BOOL skip   = FALSE;

        while (WAV_IS_PLAYING(stream) && !quit && !skip) {
            PS2_KB_DATA *kp = ATUI_GETCH_NB();
            if (kp && kp->cur.pressed) {
                U32 key = kp->cur.keycode;
                if (key == KEY_Q || key == KEY_ESC) {
                    quit = TRUE;
                } else if (key == KEY_SPACE) {
                    paused = !paused;
                    AUDIO_PAUSE(paused);
                } else if (key == KEY_N) {
                    skip = TRUE;
                }
            }

            U32 elapsed = WAV_ELAPSED_SECONDS(stream);
            ui_draw(basename(path), track_idx, file_count, elapsed, total, paused);
            CPU_SLEEP(100);
        }

        /* Show 100% for a moment when track finishes naturally */
        if (!quit && !skip) {
            ui_draw(basename(path), track_idx, file_count, total, total, FALSE);
            CPU_SLEEP(400);
        }

        WAV_CLOSE(stream);

        if (quit) break;

        if (file_count == 1) {
            /* Single track: always loop — track_idx stays 0 */
        } else {
            /* Multi-track: advance to next; stop after last */
            track_idx++;
            if (track_idx >= file_count) break;
        }
    }

    AUDIO_STOP();
    ATUI_DELWIN(g_win);
    ATUI_CLEAR();
    ATUI_REFRESH();
    ATUI_DESTROY();
    return 0;
}
