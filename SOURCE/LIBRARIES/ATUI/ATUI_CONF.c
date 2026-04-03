#define ATUI_INTERNALS
#define ATRC_IMPLEMENTATION
// #define ATRC_DEBUG
#include <LIBRARIES/ATRC/ATRC.h>
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>

/* ================================================================ */
/*  ATUI Configuration — ATRC .CNF file support                     */
/* ================================================================ */

/*
 * Default config file: /HOME/ATUI.CNF
 *
 * Example ATRC format:
 *
 *   #!ATRC
 *   [ATUI]
 *   FONT=/HOME/FONTS/SYS_FONT.FNT
 *   FG=WHITE
 *   BG=BLACK
 *   CURSOR=ON
 *   ECHO=ON
 *   INSERT=ON
 *   MOUSE=OFF
 *   BORDER=SINGLE
 */

#define ATUI_DEFAULT_CNF "/HOME/ATUI.CNF"

static PATRC_FD atui_conf_fd ATTRIB_DATA = { 0 };

/* ---------------------------------------------------------------- */
/*  color name <-> VBE_COLOUR mapping                              */
/* ---------------------------------------------------------------- */

typedef struct {
    const CHAR *name;
    VBE_COLOUR  color;
} ATUI_COLOR_MAP;

static const ATUI_COLOR_MAP color_table[] = {
    { "BLACK",   VBE_BLACK   },
    { "WHITE",   VBE_WHITE   },
    { "RED",     VBE_RED     },
    { "GREEN",   VBE_GREEN   },
    { "BLUE",    VBE_BLUE    },
    { "YELLOW",  VBE_YELLOW  },
    { "CYAN",    VBE_CYAN    },
    { "MAGENTA", VBE_MAGENTA },
    { "ORANGE",  VBE_ORANGE  },
    { "GRAY",    VBE_GRAY    },
    { "PINK",    VBE_PINK    },
    { "PURPLE",  VBE_PURPLE  },
    { "LIME",    VBE_LIME    },
    { "FUCHSIA", VBE_FUCHSIA },
    { "TEAL",    VBE_TEAL    },
    { "NAVY",    VBE_NAVY    },
    { "OLIVE",   VBE_OLIVE   },
    { "MAROON",  VBE_MAROON  },
    { "AQUA",    VBE_AQUA    },
    { "SILVER",  VBE_SILVER  },
    { "GOLD",    VBE_GOLD    },
    { "CORAL",   VBE_CORAL   },
    { "INDIGO",  VBE_INDIGO  },
    { "VIOLET",  VBE_VIOLET  },
    { "CRIMSON", VBE_CRIMSON },
    { "SALMON",  VBE_SALMON  },
    { NULLPTR,   0           },
};

static VBE_COLOUR color_from_name(PU8 name) {
    if (!name) return VBE_WHITE;
    for (U32 i = 0; color_table[i].name; i++) {
        if (STRICMP(name, color_table[i].name) == 0)
            return color_table[i].color;
    }
    // Try parsing as numeric (decimal)
    if (name[0] >= '0' && name[0] <= '9')
        return (VBE_COLOUR)ATOI(name);
    return VBE_WHITE;
}

static const CHAR* color_to_name(VBE_COLOUR c) {
    for (U32 i = 0; color_table[i].name; i++) {
        if (color_table[i].color == c)
            return color_table[i].name;
    }
    return "WHITE";
}

/* ---------------------------------------------------------------- */
/*  public API                                                      */
/* ---------------------------------------------------------------- */

BOOL ATUI_CONF_LOAD(const CHAR *cnf_path) {
    if (atui_conf_fd) {
        DESTROY_ATRCFD(atui_conf_fd);
        atui_conf_fd = NULLPTR;
    }

    PU8 path = (PU8)(cnf_path ? cnf_path : ATUI_DEFAULT_CNF);
    atui_conf_fd = CREATE_ATRCFD(path, ATRC_READ_WRITE);
    if (!atui_conf_fd) return FALSE;

    SET_WRITECHECK(atui_conf_fd, TRUE);
    SET_AUTOSAVE(atui_conf_fd, FALSE);

    // Apply settings to the current display
    ATUI_DISPLAY *d = GET_DISPLAY();
    if (!d) return TRUE; // config loaded but display not init yet, that's fine

    // FG / BG colors
    PU8 fg_str = READ_KEY(atui_conf_fd, "ATUI", "FG");
    if (fg_str) d->fg = color_from_name(fg_str);

    PU8 bg_str = READ_KEY(atui_conf_fd, "ATUI", "BG");
    if (bg_str) d->bg = color_from_name(bg_str);

    // Cursor visibility
    PU8 cursor_str = READ_KEY(atui_conf_fd, "ATUI", "CURSOR");
    if (cursor_str) {
        if (ATRC_TO_BOOL(cursor_str))
            FLAG_SET(d->cnf, ATUIC_CURSOR_VISIBLE);
        else
            FLAG_UNSET(d->cnf, ATUIC_CURSOR_VISIBLE);
    }

    // Echo
    PU8 echo_str = READ_KEY(atui_conf_fd, "ATUI", "ECHO");
    if (echo_str) {
        if (ATRC_TO_BOOL(echo_str))
            FLAG_SET(d->cnf, ATUIC_ECHO);
        else
            FLAG_UNSET(d->cnf, ATUIC_ECHO);
    }

    // Insert mode
    PU8 insert_str = READ_KEY(atui_conf_fd, "ATUI", "INSERT");
    if (insert_str) {
        if (ATRC_TO_BOOL(insert_str))
            FLAG_SET(d->cnf, ATUIC_INSERT_MODE);
        else
            FLAG_UNSET(d->cnf, ATUIC_INSERT_MODE);
    }

    // Mouse
    PU8 mouse_str = READ_KEY(atui_conf_fd, "ATUI", "MOUSE");
    if (mouse_str) {
        if (ATRC_TO_BOOL(mouse_str))
            FLAG_SET(d->cnf, ATUIC_MOUSE_ENABLED);
        else
            FLAG_UNSET(d->cnf, ATUIC_MOUSE_ENABLED);
    }

    return TRUE;
}

BOOL ATUI_CONF_SAVE(const CHAR *cnf_path) {
    ATUI_DISPLAY *d = GET_DISPLAY();

    // If no fd open, create one
    if (!atui_conf_fd) {
        PU8 path = (PU8)(cnf_path ? cnf_path : ATUI_DEFAULT_CNF);
        atui_conf_fd = CREATE_ATRCFD(path, ATRC_CREATE_READ_WRITE);
        if (!atui_conf_fd) return FALSE;
        SET_WRITECHECK(atui_conf_fd, TRUE);
    }

    // Ensure [ATUI] block exists
    if (!DOES_EXIST_BLOCK(atui_conf_fd, "ATUI"))
        CREATE_BLOCK(atui_conf_fd, "ATUI");

    // Font
    if (d && d->font.file && d->font.file->path)
        MODIFY_KEY(atui_conf_fd, "ATUI", "FONT", d->font.file->path);

    // Colors
    if (d) {
        MODIFY_KEY(atui_conf_fd, "ATUI", "FG", (PU8)color_to_name(d->fg));
        MODIFY_KEY(atui_conf_fd, "ATUI", "BG", (PU8)color_to_name(d->bg));

        MODIFY_KEY(atui_conf_fd, "ATUI", "CURSOR",
            IS_FLAG_SET(d->cnf, ATUIC_CURSOR_VISIBLE) ? "ON" : "OFF");
        MODIFY_KEY(atui_conf_fd, "ATUI", "ECHO",
            IS_FLAG_SET(d->cnf, ATUIC_ECHO) ? "ON" : "OFF");
        MODIFY_KEY(atui_conf_fd, "ATUI", "INSERT",
            IS_FLAG_SET(d->cnf, ATUIC_INSERT_MODE) ? "ON" : "OFF");
        MODIFY_KEY(atui_conf_fd, "ATUI", "MOUSE",
            IS_FLAG_SET(d->cnf, ATUIC_MOUSE_ENABLED) ? "ON" : "OFF");
    }

    return TRUE;
}

CHAR* ATUI_CONF_GET(const CHAR *block, const CHAR *key) {
    if (!atui_conf_fd) return NULLPTR;
    return READ_KEY(atui_conf_fd, (PU8)block, (PU8)key);
}

BOOL ATUI_CONF_SET(const CHAR *block, const CHAR *key, const CHAR *value) {
    if (!atui_conf_fd) return FALSE;
    PU8 result = MODIFY_KEY(atui_conf_fd, (PU8)block, (PU8)key, (PU8)value);
    return result != NULLPTR;
}

VOID ATUI_CONF_DESTROY() {
    if (atui_conf_fd) {
        DESTROY_ATRCFD(atui_conf_fd);
        atui_conf_fd = NULLPTR;
    }
}
