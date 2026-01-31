#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>
#include <STD/FS_DISK.h>

#define INITIALIZED_NULLPTR_CHECK if(!g_screen_initialized) return NULLPTR;

static BOOL8 g_screen_initialized ATTRIB_DATA = FALSE;
static ATGL_SCREEN g_screen ATTRIB_DATA = { 0 };

PATGL_SCREEN ATGL_CREATE_SCREEN(ATGL_SCREEN_ARGS args) {
    if(g_screen_initialized) ATGL_DESTROY_SCREEN(&g_screen);
    g_screen_initialized = FALSE;
    
    g_screen.args = args;
    g_screen.width = SCREEN_WIDTH;
    g_screen.height = SCREEN_HEIGHT;
    U32 back_buffer_size = sizeof(U32) * g_screen.width * g_screen.height;
    g_screen.back_buffer = (PU32)MAlloc(back_buffer_size);
    if(!g_screen.back_buffer) {
        DEBUG_PRINTF("ATGL_CREATE_SCREEN: Failed to allocate back buffer memory!\n");
        ATGL_DESTROY_SCREEN();
        return NULLPTR;
    }
    MEMSET32_OPT(g_screen.back_buffer, 0x00000000, back_buffer_size / sizeof(U32));
    g_screen.root_node = ATGL_CREATE_NODE(NULLPTR, (ATGL_RECT){
        .x=0,
        .y=0,
        .width=g_screen.width,
        .height=g_screen.height,
    });

    if(!g_screen.root_node){
        DEBUG_PRINTF("ATGL_CREATE_SCREEN: Failed to create root node!\n");
        ATGL_DESTROY_SCREEN();
        return NULLPTR;
    }

    FAT_LFN_ENTRY font_entry;
    FAT32_PATH_RESOLVE_ENTRY("/HOME/FONTS/SYS_FONT.FNT", &font_entry);
    g_screen.font.font_data = FAT32_READ_FILE_CONTENTS(&g_screen.font.font_data_sz, &font_entry.entry);
    if(!g_screen.font.font_data) {
        DEBUG_PRINTF("ATGL_CREATE_SCREEN: Failed to read font file!\n");
        ATGL_DESTROY_SCREEN();
        return NULLPTR;
    }
    g_screen.font.font_height = g_screen.font.font_width = 16;
    g_screen.font.char_spacing = 1;
    g_screen.font.line_spacing = 2;
    g_screen.font.char_count = 256;
    
    ATGL_CREATE_WINDOW(g_screen.root_node, (ATGL_RECT) {
        .x=g_screen.root_node->area.x,
        .y=g_screen.root_node->area.y,
        .width=g_screen.root_node->area.width,
        .height=g_screen.root_node->area.height,
    }, "Root Window", ATGL_WF_NONE);
    

    g_screen_initialized = TRUE;
    return &g_screen;
}

VOID ATGL_DESTROY_SCREEN() {
    if(!g_screen_initialized) return;
    g_screen_initialized = FALSE;
    if(g_screen.back_buffer) {
        MFree(g_screen.back_buffer);
        g_screen.back_buffer = NULLPTR;
    }
    MEMSET_OPT(&g_screen, 0, sizeof(ATGL_SCREEN));
}

PATGL_SCREEN ATGL_GET_MAIN_SCREEN() {
    INITIALIZED_NULLPTR_CHECK
    return &g_screen;
}

PATGL_NODE ATGL_GET_SCREEN_ROOT_NODE() {
    INITIALIZED_NULLPTR_CHECK
    return g_screen.root_node;
}

BOOLEAN8 ATGL_IS_SCREEN_RUNNING() {
    g_screen.root_node->children[0]->on_render(g_screen.root_node->children[0], g_screen.root_node->area);
    return g_screen_initialized;
}