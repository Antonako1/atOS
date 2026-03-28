/*+++
    LIBRARIES/ATGL/ATGL_WIDGETS.c - Widget creation, rendering, and event handling

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Implements every stock ATGL widget:
      Label, Button, Checkbox, Radio, TextInput, Slider,
      ProgressBar, Panel, Listbox, Separator, Image.

    Each widget provides:
      - A static render_*  function (assigned to fn_render)
      - A static event_*   function (assigned to fn_event, where applicable)
      - A public ATGL_CREATE_*  constructor
---*/
#include <LIBRARIES/ATGL/ATGL_INTERNAL.h>
#include <STD/GRAPHICS.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/IO.h>

/* ================================================================ */
/*                       RENDER FUNCTIONS                           */
/* ================================================================ */

/* ----------------------------- Label ----------------------------- */

static VOID render_label(PATGL_NODE node)
{
    ATGL_RECT  *r = &node->abs_rect;
    VBE_COLOUR  fg = (node->fg != VBE_SEE_THROUGH) ? node->fg : atgl.theme.fg;

    if (node->bg != VBE_SEE_THROUGH) {
        DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, node->bg);
    }

    if (node->text[0]) {
        I32 text_y = r->y + (r->h - (I32)atgl.theme.char_h) / 2;
        ATGL_DRAW_TEXT_CLIPPED(r->x, text_y, r->w,
                               (PU8)node->text, fg, node->bg);
    }
}

/* ----------------------------- Button ---------------------------- */

static VOID render_button(PATGL_NODE node)
{
    ATGL_RECT        *r  = &node->abs_rect;
    ATGL_THEME       *t  = &atgl.theme;
    ATGL_BUTTON_DATA *bd = &node->data.button;

    /* Choose face colour */
    VBE_COLOUR face;
    if      (!node->enabled) face = t->btn_disabled_bg;
    else if (bd->pressed)    face = t->btn_pressed;
    else if (bd->hovered)    face = t->btn_hover;
    else                     face = t->btn_bg;

    VBE_COLOUR text_fg = node->enabled ? t->btn_fg : t->btn_disabled_fg;

    /* Face fill */
    DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, face);

    /* 3-D border */
    if (bd->pressed) ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);
    else             ATGL_DRAW_RAISED(r->x, r->y, r->w, r->h);

    /* Centred text */
    if (node->text[0]) {
        U32 len    = STRLEN(node->text);
        I32 text_w = (I32)(len * t->char_w);
        I32 text_x = r->x + (r->w - text_w) / 2;
        I32 text_y = r->y + (r->h - (I32)t->char_h) / 2;
        if (bd->pressed) { text_x++; text_y++; }
        ATGL_DRAW_TEXT(text_x, text_y, (PU8)node->text, text_fg, face);
    }

    /* Focus dotted ring (simple rectangle outline) */
    if (node->focused) {
        DRAW_RECTANGLE(r->x - 3, r->y - 3,
                       r->w + 6, r->h + 6, t->focus_ring);
    }
}

/* ----------------------------- Checkbox -------------------------- */

static VOID render_checkbox(PATGL_NODE node)
{
    ATGL_RECT          *r  = &node->abs_rect;
    ATGL_THEME         *t  = &atgl.theme;
    ATGL_CHECKBOX_DATA *cd = &node->data.checkbox;

    U32 box_sz = 12;
    I32 box_y  = r->y + (r->h - (I32)box_sz) / 2;

    /* Box */
    DRAW_FILLED_RECTANGLE(r->x, box_y, box_sz, box_sz, t->check_bg);
    ATGL_DRAW_SUNKEN(r->x, box_y, box_sz, box_sz);

    /* Check mark (X shape) */
    if (cd->checked) {
        I32 cx = r->x + 3;
        I32 cy = box_y + 3;
        DRAW_LINE(cx,     cy,     cx + 5, cy + 5, t->check_mark);
        DRAW_LINE(cx + 5, cy,     cx,     cy + 5, t->check_mark);
    }

    /* Label */
    if (node->text[0]) {
        VBE_COLOUR fg = node->enabled ? t->fg : t->btn_disabled_fg;
        I32 text_x = r->x + (I32)box_sz + 4;
        I32 text_y = r->y + (r->h - (I32)t->char_h) / 2;
        ATGL_DRAW_TEXT(text_x, text_y, (PU8)node->text,
                       fg, VBE_SEE_THROUGH);
    }

    /* Focus ring */
    if (node->focused) {
        DRAW_RECTANGLE(r->x - 1, box_y - 1,
                       box_sz + 2, box_sz + 2, t->focus_ring);
    }
}

/* ----------------------------- Radio ----------------------------- */

static VOID render_radio(PATGL_NODE node)
{
    ATGL_RECT       *r  = &node->abs_rect;
    ATGL_THEME      *t  = &atgl.theme;
    ATGL_RADIO_DATA *rd = &node->data.radio;

    U32 circle_sz = 12;
    I32 cx = r->x + (I32)circle_sz / 2;
    I32 cy = r->y + r->h / 2;

    /* Outer ring */
    DRAW_ELLIPSE(cx, cy, circle_sz / 2, circle_sz / 2, t->widget_border);

    /* Inner dot when selected */
    if (rd->selected) {
        DRAW_ELLIPSE(cx, cy, 3, 3, t->check_mark);
        DRAW_ELLIPSE(cx, cy, 2, 2, t->check_mark);
    }

    /* Label */
    if (node->text[0]) {
        VBE_COLOUR fg = node->enabled ? t->fg : t->btn_disabled_fg;
        I32 text_x = r->x + (I32)circle_sz + 4;
        I32 text_y = r->y + (r->h - (I32)t->char_h) / 2;
        ATGL_DRAW_TEXT(text_x, text_y, (PU8)node->text,
                       fg, VBE_SEE_THROUGH);
    }

    /* Focus ring */
    if (node->focused) {
        DRAW_RECTANGLE(r->x - 1,
                       r->y + (r->h - (I32)circle_sz) / 2 - 1,
                       circle_sz + 2, circle_sz + 2, t->focus_ring);
    }
}

/* ----------------------------- TextInput ------------------------- */

static VOID render_textinput(PATGL_NODE node)
{
    ATGL_RECT           *r  = &node->abs_rect;
    ATGL_THEME          *t  = &atgl.theme;
    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;

    /* Background */
    DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, t->input_bg);

    /* Border */
    ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);
    if (node->focused) {
        DRAW_RECTANGLE(r->x, r->y, r->w, r->h, t->input_focus);
    }

    I32 text_x       = r->x + 3;
    I32 text_y       = r->y + (r->h - (I32)t->char_h) / 2;
    U32 visible_chars = ((U32)r->w - 6) / t->char_w;

    if (td->len > 0) {
        PU8 display_str = (PU8)&td->buffer[td->scroll_offset];
        if (td->password) {
            for (U32 i = 0; i < visible_chars &&
                            i + td->scroll_offset < td->len; i++) {
                DRAW_8x8_CHARACTER(text_x + (I32)(i * t->char_w),
                                   text_y, '*',
                                   t->input_fg, t->input_bg);
            }
        } else {
            ATGL_DRAW_TEXT_CLIPPED(text_x, text_y, r->w - 6,
                                   display_str, t->input_fg, t->input_bg);
        }
    } else if (td->placeholder[0]) {
        ATGL_DRAW_TEXT_CLIPPED(text_x, text_y, r->w - 6,
                               (PU8)td->placeholder,
                               t->input_placeholder, t->input_bg);
    }

    /* Blinking cursor */
    if (node->focused) {
        U32 cursor_screen = td->cursor - td->scroll_offset;
        I32 cur_x = text_x + (I32)(cursor_screen * t->char_w);
        DRAW_LINE(cur_x, text_y, cur_x, text_y + (I32)t->char_h - 1,
                  t->input_fg);
    }
}

/* ----------------------------- Slider ---------------------------- */

static VOID render_slider(PATGL_NODE node)
{
    ATGL_RECT        *r  = &node->abs_rect;
    ATGL_THEME       *t  = &atgl.theme;
    ATGL_SLIDER_DATA *sd = &node->data.slider;

    I32 track_h = 4;
    I32 track_y = r->y + (r->h - track_h) / 2;
    I32 thumb_w = 10;
    I32 thumb_h = r->h - 4;

    /* Track */
    DRAW_FILLED_RECTANGLE(r->x, track_y, r->w, track_h, t->slider_track);
    ATGL_DRAW_SUNKEN(r->x, track_y, r->w, track_h);

    /* Filled portion */
    I32 range = sd->max - sd->min;
    if (range <= 0) range = 1;
    I32 fill_w = ((sd->value - sd->min) * (r->w - thumb_w)) / range;
    if (fill_w > 0) {
        DRAW_FILLED_RECTANGLE(r->x + 1, track_y + 1,
                              fill_w, track_h - 2, t->slider_fill);
    }

    /* Thumb */
    I32 thumb_x = r->x + fill_w;
    I32 thumb_y = r->y + 2;
    DRAW_FILLED_RECTANGLE(thumb_x, thumb_y, thumb_w, thumb_h, t->btn_bg);
    ATGL_DRAW_RAISED(thumb_x, thumb_y, thumb_w, thumb_h);

    /* Focus ring */
    if (node->focused) {
        DRAW_RECTANGLE(r->x - 1, r->y - 1,
                       r->w + 2, r->h + 2, t->focus_ring);
    }
}

/* ----------------------------- ProgressBar ----------------------- */

static VOID render_progressbar(PATGL_NODE node)
{
    ATGL_RECT          *r  = &node->abs_rect;
    ATGL_THEME         *t  = &atgl.theme;
    ATGL_PROGRESS_DATA *pd = &node->data.progress;

    /* Background */
    DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, t->progress_bg);
    ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);

    /* Fill bar */
    U32 max_val = pd->max ? pd->max : 1;
    I32 fill_w  = (I32)((pd->value * (U32)(r->w - 4)) / max_val);
    if (fill_w > r->w - 4) fill_w = r->w - 4;
    if (fill_w > 0) {
        DRAW_FILLED_RECTANGLE(r->x + 2, r->y + 2,
                              fill_w, r->h - 4, t->progress_fill);
    }

    /* Percentage text */
    if (pd->show_text) {
        CHAR buf[8];
        U32 pct = (pd->value * 100) / max_val;
        SPRINTF((PU8)buf, "%d%%", pct);
        U32 tw = STRLEN((PU8)buf) * t->char_w;
        I32 tx = r->x + (r->w - (I32)tw) / 2;
        I32 ty = r->y + (r->h - (I32)t->char_h) / 2;
        ATGL_DRAW_TEXT(tx, ty, (PU8)buf, t->progress_text,
                       VBE_SEE_THROUGH);
    }
}

/* ----------------------------- Panel ----------------------------- */

static VOID render_panel(PATGL_NODE node)
{
    ATGL_RECT *r = &node->abs_rect;

    /* Background fill */
    if (node->bg != VBE_SEE_THROUGH) {
        DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, node->bg);
    }

    /* Border with optional title (GroupBox style) */
    if (node->text[0]) {
        I32 half_ch = (I32)atgl.theme.char_h / 2;
        DRAW_RECTANGLE(r->x, r->y + half_ch,
                       r->w, r->h - half_ch,
                       atgl.theme.widget_border);

        /* Title text with gap */
        U32 title_w = STRLEN((PU8)node->text) * atgl.theme.char_w + 4;
        if (node->bg != VBE_SEE_THROUGH) {
            DRAW_FILLED_RECTANGLE(r->x + 8, r->y,
                                  (I32)title_w, (I32)atgl.theme.char_h,
                                  node->bg);
        }
        ATGL_DRAW_TEXT(r->x + 10, r->y, (PU8)node->text,
                       atgl.theme.fg, VBE_SEE_THROUGH);
    }
}

/* ----------------------------- Listbox --------------------------- */

static VOID render_listbox(PATGL_NODE node)
{
    ATGL_RECT         *r  = &node->abs_rect;
    ATGL_THEME        *t  = &atgl.theme;
    ATGL_LISTBOX_DATA *ld = &node->data.listbox;

    /* Background */
    DRAW_FILLED_RECTANGLE(r->x, r->y, r->w, r->h, t->list_bg);
    ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);

    /* Items */
    U32 item_h = t->char_h + 2;
    for (U32 i = 0;
         i < ld->visible_rows && (i + ld->scroll_offset) < ld->item_count;
         i++)
    {
        U32 idx = i + ld->scroll_offset;
        I32 iy  = r->y + 2 + (I32)(i * item_h);

        BOOL       sel = (idx == ld->selected);
        VBE_COLOUR ibg = sel ? t->list_sel_bg : t->list_bg;
        VBE_COLOUR ifg = sel ? t->list_sel_fg : t->list_fg;

        DRAW_FILLED_RECTANGLE(r->x + 2, iy, r->w - 4, (I32)item_h, ibg);
        ATGL_DRAW_TEXT_CLIPPED(r->x + 4, iy + 1, r->w - 8,
                               (PU8)ld->items[idx], ifg, ibg);
    }

    /* Scrollbar (simple) */
    if (ld->item_count > ld->visible_rows) {
        I32 sb_x    = r->x + r->w - 10;
        I32 sb_h    = r->h - 4;
        I32 thumb_h = (I32)(ld->visible_rows * (U32)sb_h) /
                      (I32)ld->item_count;
        if (thumb_h < 8) thumb_h = 8;
        I32 thumb_y = r->y + 2 +
            (I32)(ld->scroll_offset * (U32)(sb_h - thumb_h)) /
            (I32)(ld->item_count - ld->visible_rows);

        DRAW_FILLED_RECTANGLE(sb_x, r->y + 2, 8, sb_h, t->widget_bg);
        DRAW_FILLED_RECTANGLE(sb_x, thumb_y, 8, thumb_h, t->shadow);
    }

    /* Focus ring */
    if (node->focused) {
        DRAW_RECTANGLE(r->x - 1, r->y - 1,
                       r->w + 2, r->h + 2, t->focus_ring);
    }
}

/* ----------------------------- Separator ------------------------- */

static VOID render_separator(PATGL_NODE node)
{
    ATGL_RECT  *r = &node->abs_rect;
    ATGL_THEME *t = &atgl.theme;

    if (r->w >= r->h) {
        /* Horizontal */
        I32 y = r->y + r->h / 2;
        DRAW_LINE(r->x, y,     r->x + r->w - 1, y,     t->shadow);
        DRAW_LINE(r->x, y + 1, r->x + r->w - 1, y + 1, t->highlight);
    } else {
        /* Vertical */
        I32 x = r->x + r->w / 2;
        DRAW_LINE(x,     r->y, x,     r->y + r->h - 1, t->shadow);
        DRAW_LINE(x + 1, r->y, x + 1, r->y + r->h - 1, t->highlight);
    }
}

/* ----------------------------- Image ----------------------------- */

static VOID render_image(PATGL_NODE node)
{
    ATGL_RECT       *r  = &node->abs_rect;
    ATGL_IMAGE_DATA *id = &node->data.image;

    if (!id->pixels) return;

    U32 *pixels = (U32 *)id->pixels;
    for (U32 iy = 0; iy < id->img_h && iy < (U32)r->h; iy++) {
        for (U32 ix = 0; ix < id->img_w && ix < (U32)r->w; ix++) {
            VBE_COLOUR c = pixels[iy * id->img_w + ix];
            if (c != VBE_SEE_THROUGH) {
                VBE_PIXEL_INFO px;
                px.X      = (U32)r->x + ix;
                px.Y      = (U32)r->y + iy;
                px.Colour = c;
                DRAW_PIXEL(px);
            }
        }
    }
}

/* ================================================================ */
/*                       EVENT HANDLERS                             */
/* ================================================================ */

/* ----------------------------- Button ---------------------------- */

static VOID event_button(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_BUTTON_DATA *bd = &node->data.button;

    switch (ev->type) {
    case ATGL_EVT_MOUSE_DOWN:
        if (ATGL_RECT_CONTAINS(&node->abs_rect,
                                ev->mouse.x, ev->mouse.y)) {
            bd->pressed   = TRUE;
            node->dirty   = TRUE;
            ev->handled   = TRUE;
        }
        break;

    case ATGL_EVT_MOUSE_UP:
        if (bd->pressed) {
            bd->pressed = FALSE;
            node->dirty = TRUE;
            if (ATGL_RECT_CONTAINS(&node->abs_rect,
                                    ev->mouse.x, ev->mouse.y)) {
                if (node->on_click) node->on_click(node);
            }
            ev->handled = TRUE;
        }
        break;

    case ATGL_EVT_MOUSE_MOVE:
        {
            BOOL hover = ATGL_RECT_CONTAINS(&node->abs_rect,
                                             ev->mouse.x, ev->mouse.y);
            if (hover != bd->hovered) {
                bd->hovered = hover;
                node->dirty = TRUE;
            }
        }
        break;

    case ATGL_EVT_KEY_DOWN:
        if (node->focused &&
            (ev->key.keycode == KEY_ENTER ||
             ev->key.keycode == KEY_SPACE)) {
            bd->pressed = TRUE;
            node->dirty = TRUE;
            if (node->on_click) node->on_click(node);
            bd->pressed = FALSE;
            ev->handled = TRUE;
        }
        break;

    default:
        break;
    }
}

/* ----------------------------- Checkbox -------------------------- */

static VOID event_checkbox(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_CHECKBOX_DATA *cd = &node->data.checkbox;
    BOOL toggle = FALSE;

    if (ev->type == ATGL_EVT_MOUSE_CLICK &&
        ATGL_RECT_CONTAINS(&node->abs_rect, ev->mouse.x, ev->mouse.y)) {
        toggle = TRUE;
    }
    if (ev->type == ATGL_EVT_KEY_DOWN && node->focused &&
        (ev->key.keycode == KEY_ENTER || ev->key.keycode == KEY_SPACE)) {
        toggle = TRUE;
    }

    if (toggle) {
        cd->checked = !cd->checked;
        node->dirty = TRUE;
        if (node->on_change) node->on_change(node);
        ev->handled = TRUE;
    }
}

/* ----------------------------- Radio ----------------------------- */

static VOID event_radio(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_RADIO_DATA *rd = &node->data.radio;
    BOOL activate = FALSE;

    if (ev->type == ATGL_EVT_MOUSE_CLICK &&
        ATGL_RECT_CONTAINS(&node->abs_rect, ev->mouse.x, ev->mouse.y)) {
        activate = TRUE;
    }
    if (ev->type == ATGL_EVT_KEY_DOWN && node->focused &&
        (ev->key.keycode == KEY_ENTER || ev->key.keycode == KEY_SPACE)) {
        activate = TRUE;
    }

    if (activate && !rd->selected) {
        /* Deselect all siblings in same group */
        PATGL_NODE parent = node->parent;
        if (parent) {
            for (U32 i = 0; i < parent->child_count; i++) {
                PATGL_NODE sib = parent->children[i];
                if (sib->type == ATGL_NODE_RADIO &&
                    sib->data.radio.group_id == rd->group_id) {
                    sib->data.radio.selected = FALSE;
                    sib->dirty = TRUE;
                }
            }
        }

        rd->selected = TRUE;
        node->dirty  = TRUE;
        if (node->on_change) node->on_change(node);
        ev->handled = TRUE;
    }
}

/* ----------------------------- TextInput ------------------------- */

static VOID event_textinput(PATGL_NODE node, ATGL_EVENT *ev)
{
    if (ev->type != ATGL_EVT_KEY_DOWN || !node->focused) return;

    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;
    U32 visible_chars = ((U32)node->abs_rect.w - 6) / atgl.theme.char_w;

    switch (ev->key.keycode) {
    case KEY_BACKSPACE:
        if (td->cursor > 0) {
            MEMMOVE(&td->buffer[td->cursor - 1],
                    &td->buffer[td->cursor],
                    td->len - td->cursor + 1);
            td->cursor--;
            td->len--;
            if (td->scroll_offset > 0 &&
                td->cursor < td->scroll_offset) {
                td->scroll_offset = td->cursor;
            }
            node->dirty = TRUE;
            if (node->on_change) node->on_change(node);
        }
        ev->handled = TRUE;
        break;

    case KEY_ARROW_LEFT:
        if (td->cursor > 0) {
            td->cursor--;
            if (td->cursor < td->scroll_offset)
                td->scroll_offset = td->cursor;
            node->dirty = TRUE;
        }
        ev->handled = TRUE;
        break;

    case KEY_ARROW_RIGHT:
        if (td->cursor < td->len) {
            td->cursor++;
            if (td->cursor >= td->scroll_offset + visible_chars)
                td->scroll_offset = td->cursor - visible_chars + 1;
            node->dirty = TRUE;
        }
        ev->handled = TRUE;
        break;

    default:
        /* Printable character insertion */
        if (ev->key.ch >= 0x20 && ev->key.ch < 0x7F) {
            if (td->len < td->max_len - 1) {
                MEMMOVE(&td->buffer[td->cursor + 1],
                        &td->buffer[td->cursor],
                        td->len - td->cursor + 1);
                td->buffer[td->cursor] = ev->key.ch;
                td->cursor++;
                td->len++;
                if (td->cursor >= td->scroll_offset + visible_chars)
                    td->scroll_offset = td->cursor - visible_chars + 1;
                node->dirty = TRUE;
                if (node->on_change) node->on_change(node);
            }
            ev->handled = TRUE;
        }
        break;
    }
}

/* ----------------------------- Slider ---------------------------- */

static VOID event_slider(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_SLIDER_DATA *sd = &node->data.slider;

    if (ev->type == ATGL_EVT_MOUSE_DOWN &&
        ATGL_RECT_CONTAINS(&node->abs_rect, ev->mouse.x, ev->mouse.y)) {
        sd->dragging = TRUE;
        ev->handled  = TRUE;
    }

    if (ev->type == ATGL_EVT_MOUSE_UP) {
        sd->dragging = FALSE;
    }

    if ((ev->type == ATGL_EVT_MOUSE_MOVE ||
         ev->type == ATGL_EVT_MOUSE_DOWN) && sd->dragging)
    {
        I32 rel_x   = ev->mouse.x - node->abs_rect.x;
        I32 range   = sd->max - sd->min;
        I32 new_val = sd->min + (rel_x * range) / node->abs_rect.w;

        if (new_val < sd->min) new_val = sd->min;
        if (new_val > sd->max) new_val = sd->max;

        /* Snap to step */
        if (sd->step > 1) {
            new_val = sd->min +
                ((new_val - sd->min + sd->step / 2) / sd->step) *
                sd->step;
            if (new_val > sd->max) new_val = sd->max;
        }

        if (new_val != sd->value) {
            sd->value   = new_val;
            node->dirty = TRUE;
            if (node->on_change) node->on_change(node);
        }
        ev->handled = TRUE;
    }

    /* Arrow keys */
    if (ev->type == ATGL_EVT_KEY_DOWN && node->focused) {
        I32 delta = sd->step ? sd->step : 1;
        I32 nv    = sd->value;

        if (ev->key.keycode == KEY_ARROW_LEFT ||
            ev->key.keycode == KEY_ARROW_DOWN) {
            nv -= delta;
            if (nv < sd->min) nv = sd->min;
        } else if (ev->key.keycode == KEY_ARROW_RIGHT ||
                   ev->key.keycode == KEY_ARROW_UP) {
            nv += delta;
            if (nv > sd->max) nv = sd->max;
        }

        if (nv != sd->value) {
            sd->value   = nv;
            node->dirty = TRUE;
            if (node->on_change) node->on_change(node);
        }
        ev->handled = TRUE;
    }
}

/* ----------------------------- Listbox --------------------------- */

static VOID event_listbox(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_LISTBOX_DATA *ld = &node->data.listbox;

    /* Mouse click selects an item */
    if (ev->type == ATGL_EVT_MOUSE_CLICK &&
        ATGL_RECT_CONTAINS(&node->abs_rect, ev->mouse.x, ev->mouse.y))
    {
        U32 item_h = atgl.theme.char_h + 2;
        I32 rel_y  = ev->mouse.y - node->abs_rect.y - 2;
        if (rel_y >= 0) {
            U32 clicked = ld->scroll_offset + (U32)rel_y / item_h;
            if (clicked < ld->item_count) {
                ld->selected = clicked;
                node->dirty  = TRUE;
                if (node->on_change) node->on_change(node);
            }
        }
        ev->handled = TRUE;
    }

    /* Arrow keys navigate */
    if (ev->type == ATGL_EVT_KEY_DOWN && node->focused) {
        if (ev->key.keycode == KEY_ARROW_UP && ld->selected > 0) {
            ld->selected--;
            if (ld->selected < ld->scroll_offset)
                ld->scroll_offset = ld->selected;
            node->dirty = TRUE;
            if (node->on_change) node->on_change(node);
            ev->handled = TRUE;
        }
        if (ev->key.keycode == KEY_ARROW_DOWN &&
            ld->selected + 1 < ld->item_count) {
            ld->selected++;
            if (ld->selected >= ld->scroll_offset + ld->visible_rows)
                ld->scroll_offset = ld->selected - ld->visible_rows + 1;
            node->dirty = TRUE;
            if (node->on_change) node->on_change(node);
            ev->handled = TRUE;
        }
    }
}

/* ================================================================ */
/*                    WIDGET CONSTRUCTORS                           */
/* ================================================================ */

PATGL_NODE ATGL_CREATE_LABEL(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 text, VBE_COLOUR fg, VBE_COLOUR bg)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_LABEL, parent, rect);
    if (!node) return NULLPTR;

    ATGL_NODE_SET_TEXT(node, text);
    node->fg        = fg;
    node->bg        = bg;
    node->focusable = FALSE;
    node->fn_render = render_label;
    return node;
}

PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT rect,
                              PU8 text,
                              VOID (*on_click)(PATGL_NODE))
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_BUTTON, parent, rect);
    if (!node) return NULLPTR;

    ATGL_NODE_SET_TEXT(node, text);
    node->fg        = atgl.theme.btn_fg;
    node->bg        = atgl.theme.btn_bg;
    node->focusable = TRUE;
    node->on_click  = on_click;
    node->fn_render = render_button;
    node->fn_event  = event_button;
    node->data.button.pressed = FALSE;
    node->data.button.hovered = FALSE;
    return node;
}

PATGL_NODE ATGL_CREATE_CHECKBOX(PATGL_NODE parent, ATGL_RECT rect,
                                PU8 text, BOOL initial,
                                VOID (*on_change)(PATGL_NODE))
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_CHECKBOX, parent, rect);
    if (!node) return NULLPTR;

    ATGL_NODE_SET_TEXT(node, text);
    node->focusable = TRUE;
    node->on_change = on_change;
    node->fn_render = render_checkbox;
    node->fn_event  = event_checkbox;
    node->data.checkbox.checked = initial;
    return node;
}

PATGL_NODE ATGL_CREATE_RADIO(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 text, U32 group_id, BOOL selected,
                             VOID (*on_change)(PATGL_NODE))
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_RADIO, parent, rect);
    if (!node) return NULLPTR;

    ATGL_NODE_SET_TEXT(node, text);
    node->focusable = TRUE;
    node->on_change = on_change;
    node->fn_render = render_radio;
    node->fn_event  = event_radio;
    node->data.radio.group_id = group_id;
    node->data.radio.selected = selected;
    return node;
}

PATGL_NODE ATGL_CREATE_TEXTINPUT(PATGL_NODE parent, ATGL_RECT rect,
                                 PU8 placeholder, U32 max_len)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_TEXTINPUT, parent, rect);
    if (!node) return NULLPTR;

    node->focusable = TRUE;
    node->fn_render = render_textinput;
    node->fn_event  = event_textinput;

    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;
    MEMSET(td->buffer, 0, ATGL_TEXTINPUT_MAX);
    td->len           = 0;
    td->cursor        = 0;
    td->scroll_offset = 0;
    td->max_len = (max_len && max_len < ATGL_TEXTINPUT_MAX)
                  ? max_len : ATGL_TEXTINPUT_MAX - 1;
    td->password = FALSE;

    MEMSET(td->placeholder, 0, ATGL_PLACEHOLDER_MAX);
    if (placeholder) {
        U32 plen = STRLEN(placeholder);
        if (plen >= ATGL_PLACEHOLDER_MAX) plen = ATGL_PLACEHOLDER_MAX - 1;
        MEMCPY(td->placeholder, placeholder, plen);
    }
    return node;
}

PATGL_NODE ATGL_CREATE_SLIDER(PATGL_NODE parent, ATGL_RECT rect,
                              I32 min, I32 max, I32 value, I32 step)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_SLIDER, parent, rect);
    if (!node) return NULLPTR;

    node->focusable = TRUE;
    node->fn_render = render_slider;
    node->fn_event  = event_slider;

    ATGL_SLIDER_DATA *sd = &node->data.slider;
    sd->min      = min;
    sd->max      = max;
    sd->value    = value;
    sd->step     = step > 0 ? step : 1;
    sd->dragging = FALSE;
    return node;
}

PATGL_NODE ATGL_CREATE_PROGRESSBAR(PATGL_NODE parent, ATGL_RECT rect,
                                   U32 max)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_PROGRESSBAR,
                                       parent, rect);
    if (!node) return NULLPTR;

    node->focusable = FALSE;
    node->fn_render = render_progressbar;

    ATGL_PROGRESS_DATA *pd = &node->data.progress;
    pd->value     = 0;
    pd->max       = max ? max : 100;
    pd->show_text = TRUE;
    return node;
}

PATGL_NODE ATGL_CREATE_PANEL(PATGL_NODE parent, ATGL_RECT rect,
                             ATGL_LAYOUT_DIR layout,
                             U32 padding, U32 spacing)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_PANEL, parent, rect);
    if (!node) return NULLPTR;

    node->bg        = atgl.theme.widget_bg;
    node->fn_render = render_panel;

    ATGL_PANEL_DATA *pd = &node->data.panel;
    pd->layout  = layout;
    pd->padding = padding;
    pd->spacing = spacing;
    return node;
}

PATGL_NODE ATGL_CREATE_LISTBOX(PATGL_NODE parent, ATGL_RECT rect,
                               U32 visible_rows)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_LISTBOX, parent, rect);
    if (!node) return NULLPTR;

    node->focusable = TRUE;
    node->fn_render = render_listbox;
    node->fn_event  = event_listbox;

    ATGL_LISTBOX_DATA *ld = &node->data.listbox;
    ld->item_count   = 0;
    ld->selected     = 0;
    ld->scroll_offset = 0;
    ld->visible_rows = visible_rows ? visible_rows : 5;
    return node;
}

PATGL_NODE ATGL_CREATE_SEPARATOR(PATGL_NODE parent, ATGL_RECT rect)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_SEPARATOR, parent, rect);
    if (!node) return NULLPTR;

    node->fn_render = render_separator;
    return node;
}

PATGL_NODE ATGL_CREATE_IMAGE(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 pixels, U32 img_w, U32 img_h)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_IMAGE, parent, rect);
    if (!node) return NULLPTR;

    node->fn_render       = render_image;
    node->data.image.pixels = pixels;
    node->data.image.img_w  = img_w;
    node->data.image.img_h  = img_h;
    return node;
}

/* ================================================================ */
/*                 WIDGET GETTERS / SETTERS                        */
/* ================================================================ */

BOOL ATGL_CHECKBOX_GET(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_CHECKBOX) return FALSE;
    return node->data.checkbox.checked;
}

VOID ATGL_CHECKBOX_SET(PATGL_NODE node, BOOL checked)
{
    if (!node || node->type != ATGL_NODE_CHECKBOX) return;
    node->data.checkbox.checked = checked;
    node->dirty = TRUE;
}

U32 ATGL_RADIO_GET_SELECTED(PATGL_NODE parent, U32 group_id)
{
    if (!parent) return 0;
    for (U32 i = 0; i < parent->child_count; i++) {
        PATGL_NODE c = parent->children[i];
        if (c->type == ATGL_NODE_RADIO &&
            c->data.radio.group_id == group_id &&
            c->data.radio.selected) {
            return c->id;
        }
    }
    return 0;
}

PU8 ATGL_TEXTINPUT_GET_TEXT(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_TEXTINPUT) return NULLPTR;
    return (PU8)node->data.textinput.buffer;
}

VOID ATGL_TEXTINPUT_SET_TEXT(PATGL_NODE node, PU8 text)
{
    if (!node || node->type != ATGL_NODE_TEXTINPUT) return;

    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;
    if (text) {
        U32 len = STRLEN(text);
        if (len >= td->max_len) len = td->max_len - 1;
        MEMCPY(td->buffer, text, len);
        td->buffer[len] = '\0';
        td->len    = len;
        td->cursor = len;
    } else {
        td->buffer[0] = '\0';
        td->len    = 0;
        td->cursor = 0;
    }
    td->scroll_offset = 0;
    node->dirty = TRUE;
}

I32 ATGL_SLIDER_GET_VALUE(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_SLIDER) return 0;
    return node->data.slider.value;
}

VOID ATGL_SLIDER_SET_VALUE(PATGL_NODE node, I32 value)
{
    if (!node || node->type != ATGL_NODE_SLIDER) return;
    ATGL_SLIDER_DATA *sd = &node->data.slider;
    if (value < sd->min) value = sd->min;
    if (value > sd->max) value = sd->max;
    sd->value   = value;
    node->dirty = TRUE;
}

VOID ATGL_PROGRESSBAR_SET(PATGL_NODE node, U32 value)
{
    if (!node || node->type != ATGL_NODE_PROGRESSBAR) return;
    ATGL_PROGRESS_DATA *pd = &node->data.progress;
    if (value > pd->max) value = pd->max;
    pd->value   = value;
    node->dirty = TRUE;
}

U32 ATGL_PROGRESSBAR_GET(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_PROGRESSBAR) return 0;
    return node->data.progress.value;
}

VOID ATGL_LISTBOX_ADD_ITEM(PATGL_NODE node, PU8 text)
{
    if (!node || node->type != ATGL_NODE_LISTBOX) return;

    ATGL_LISTBOX_DATA *ld = &node->data.listbox;
    if (ld->item_count >= ATGL_LISTBOX_MAX_ITEMS) return;

    if (text) {
        U32 len = STRLEN(text);
        if (len >= ATGL_LISTBOX_ITEM_LEN) len = ATGL_LISTBOX_ITEM_LEN - 1;
        MEMCPY(ld->items[ld->item_count], text, len);
        ld->items[ld->item_count][len] = '\0';
    }
    ld->item_count++;
    node->dirty = TRUE;
}

U32 ATGL_LISTBOX_GET_SELECTED(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_LISTBOX) return 0;
    return node->data.listbox.selected;
}

PU8 ATGL_LISTBOX_GET_TEXT(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_LISTBOX) return NULLPTR;
    ATGL_LISTBOX_DATA *ld = &node->data.listbox;
    if (ld->selected < ld->item_count)
        return (PU8)ld->items[ld->selected];
    return NULLPTR;
}

VOID ATGL_LISTBOX_CLEAR(PATGL_NODE node)
{
    if (!node || node->type != ATGL_NODE_LISTBOX) return;
    node->data.listbox.item_count   = 0;
    node->data.listbox.selected     = 0;
    node->data.listbox.scroll_offset = 0;
    node->dirty = TRUE;
}
