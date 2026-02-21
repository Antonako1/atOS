#include <LIBRARIES/ATGL/ATGL_NODES.h>
#include <LIBRARIES/ATGL/ATGL_GRAPHICS.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/BINARY.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>

WINHNDL _ATGL_CREATE_WINDOW_HANDLE(void) {
    static WINHNDL current_handle = 1;
    return current_handle++;
}

// --- Forward Declarations of Default Renderers ---
VOID ATGL_DEFAULT_WINDOW_RENDER(PATGL_NODE self, ATGL_RECT clip) {
    if(!self||!self->is_dirty) return;
    ATGL_WIN_DATA* data = (ATGL_WIN_DATA*)self->priv_data;
    if(!data) return;

    // Draw window background
    ATGL_FILL_RECT(self->area, VBE_NOTEPAD_PAPER2);
    self->is_dirty = FALSE;
    for(U16 i = 0; i< self->child_count; i++) {
        PATGL_NODE child = self->children[i];
        if(child->is_visible && child->on_render && child->is_dirty) {
            child->on_render(child, clip);
            child->is_dirty = FALSE;
        }
    }
}
VOID ATGL_DEFAULT_TEXT_RENDER(PATGL_NODE self, ATGL_RECT clip) {
    return;
}
VOID ATGL_DEFAULT_BUTTON_RENDER(PATGL_NODE self, ATGL_RECT clip) {
    return;
}

// VOID ATGL_DEFAULT_RENDER(PATGL_NODE self, ATGL_RECT clip) {
//     if(!self || !self->is_dirty) return;
//     switch(self->type) {
//         case ATGL_NT_WINDOW:
//             ATGL_DEFAULT_WINDOW_RENDER(self, clip);
//             break;
//         case ATGL_NT_BUTTON:
//             ATGL_DEFAULT_BUTTON_RENDER(self, clip);
//             break;
//         case ATGL_NT_TEXT:
//             ATGL_DEFAULT_TEXT_RENDER(self, clip);
//             break;
//         default:
//             break;
//     }
//     return;
// }

// Frees priv_data automatically
VOID ATGL_DEFAULT_DESTROY(PATGL_NODE self);

// --- Base Constructor ---
PATGL_NODE ATGL_CREATE_NODE(PATGL_NODE parent, ATGL_RECT area) {
    PATGL_NODE node = (PATGL_NODE)MAlloc(sizeof(ATGL_NODE));
    if (!node) return NULLPTR;

    MEMSET_OPT(node, 0, sizeof(ATGL_NODE)); // Zero out everything first

    node->area = area;
    node->is_visible = TRUE;
    node->is_dirty = TRUE;
    node->type = ATGL_NT_GENERIC;
    node->on_destroy = ATGL_DEFAULT_DESTROY; 

    // Auto-link to parent
    if (parent) {
        node->parent = parent;
        if (parent->child_count < ATGL_MAX_CHILDREN) {
            parent->children[parent->child_count++] = node;
        }
    }
    return node;
}

// --- Text Constructor ---
PATGL_NODE ATGL_CREATE_TEXT(PATGL_NODE parent, ATGL_RECT area, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg){
    PATGL_NODE node = ATGL_CREATE_NODE(parent, area);
    if(!node) return NULLPTR;
    node->type = ATGL_NT_TEXT;
    node->on_render = ATGL_DEFAULT_TEXT_RENDER;
    ATGL_TEXT_DATA* data = (ATGL_TEXT_DATA*)MAlloc(sizeof(ATGL_TEXT_DATA));
    data->text = STRDUP(text);
    data->bg_colour = bg;
    data->fg_colour = fg;
    node->priv_data = data;
    node->is_dirty = TRUE;
    node->is_visible = TRUE;
    return node;
}

// --- Window Constructor ---
PATGL_NODE ATGL_CREATE_WINDOW(PATGL_NODE parent, ATGL_RECT area, PU8 title, ATGL_WIN_FLAGS flags) {
    // 1. Create Base
    PATGL_NODE node = ATGL_CREATE_NODE(parent, area);
    if (!node) return NULLPTR;

    // 2. Customize for Window
    node->type = ATGL_NT_WINDOW;
    node->on_render = ATGL_DEFAULT_WINDOW_RENDER;

    // 3. Allocate Private Data
    ATGL_WIN_DATA* data = (ATGL_WIN_DATA*)MAlloc(sizeof(ATGL_WIN_DATA));
    data->title = STRDUP(title);
    data->border_color = 0xFFFFFF; // Default color
    data->hndl = _ATGL_CREATE_WINDOW_HANDLE(); // Your handle generator
    
    node->priv_data = data;
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_BORDER)) data->border_color = VBE_SEE_THROUGH;
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_RESIZE)) {

    }
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_CLOSE)) {

    }
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_MINIMIZE)) {

    }
    // #define CENTER_TEXT(parent, title) \
        (parent->area.x + (parent->area.width / 2) - (STRLEN(title) * (g_screen.font.font_width + g_screen.font.char_spacing) / 2))
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_TITLE)) {
        PATGL_NODE title_node = ATGL_CREATE_TEXT(node, (ATGL_RECT) {
            .x = ATGL_TRF_CENTER_X,
            .y = ATGL_TRF_CENTER_Y,
            .width = ATGL_TRF_FIT_WIDTH,
            .height = ATGL_TRF_FIT_HEIGHT_TO_FONT,
        }, data->title, VBE_BLACK, VBE_SEE_THROUGH);
    }
    if(IS_FLAG_UNSET(flags,ATGL_WF_NO_MAXIMIZE)) {

    }

    return node;
}

// --- Button Constructor ---
PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT area, PU8 label) {
    PATGL_NODE node = ATGL_CREATE_NODE(parent, area);
    if (!node) return NULLPTR;

    node->type = ATGL_NT_BUTTON;
    node->on_render = ATGL_DEFAULT_BUTTON_RENDER;

    ATGL_BTN_DATA* data = (ATGL_BTN_DATA*)MAlloc(sizeof(ATGL_BTN_DATA));
    data->label = STRDUP(label);
    data->bg_color = VBE_GRAY;
    data->is_pressed = FALSE;

    node->priv_data = data;
    
    return node;
}

// --- Generic Destroyer ---
VOID ATGL_DEFAULT_DESTROY(PATGL_NODE self) {
    // Automatically free the private data based on type
    if (self->priv_data) {
        if (self->type == ATGL_NT_WINDOW) {
            ATGL_WIN_DATA* w = (ATGL_WIN_DATA*)self->priv_data;
            if(w->title) MFree(w->title);
        }
        else if (self->type == ATGL_NT_BUTTON) {
            ATGL_BTN_DATA* b = (ATGL_BTN_DATA*)self->priv_data;
            if(b->label) MFree(b->label);
        }
        
        MFree(self->priv_data);
    }
}

VOID ATGL_DESTROY_NODE(PATGL_NODE node) {
    if (!node) return;

    // 1. Destroy Children first (Recursive)
    for (U16 i = 0; i < node->child_count; i++) {
        ATGL_DESTROY_NODE(node->children[i]);
    }

    // 2. Clean up self
    if (node->on_destroy) {
        node->on_destroy(node);
    }

    // 3. Unlink from parent (if parent exists and isn't being destroyed too)
    // (Logic omitted for brevity, but you'd remove yourself from parent->children)

    MFree(node);
}

VOID ATGL_MARK_DIRTY(PATGL_NODE node){
    if(!node) return;
    node->is_dirty=TRUE;
}


VOID ATGL_POLL_EVENT(ATGL_EVENT *ev) {
    PS2_KB_DATA *kp = kb_poll();
    if(kp) {
        ev->type = ATGL_EV_KEYBOARD;
        ev->data.keyboard = kp;
        return;
    }
    PS2_MOUSE_DATA *ms = mouse_poll();
    if(ms) {
        ev->type = ATGL_EV_MOUSE;
        ev->data.mouse = ms;
        return;
    }
    ev->type = ATGL_EV_NONE;
    return;
}
