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
#include <STD/FS_DISK.h>
#include <STD/IO.h>

/* ================================================================ */
/*                       RENDER FUNCTIONS                           */
/* ================================================================ */

VOID ATGL_NODE_RESET_THEME_COLORS(PATGL_NODE node)
{
    if (!node || node->type == ATGL_NODE_ROOT) return;

    switch (node->type) {
        case ATGL_NODE_LABEL:
        case ATGL_NODE_SEPARATOR:
        case ATGL_NODE_IMAGE:
            node->fg = atgl.theme.fg;
            node->bg = VBE_SEE_THROUGH;
            break;
        case ATGL_NODE_BUTTON:
            node->fg = atgl.theme.btn_fg;
            node->bg = atgl.theme.btn_bg;
            break;
        case ATGL_NODE_CHECKBOX:
        case ATGL_NODE_RADIO:
            node->fg = atgl.theme.fg;
            node->bg = atgl.theme.check_bg;
            break;
        case ATGL_NODE_TEXTINPUT:
            node->fg = atgl.theme.input_fg;
            node->bg = atgl.theme.input_bg;
            break;
        case ATGL_NODE_SLIDER:
            node->fg = atgl.theme.slider_thumb;
            node->bg = atgl.theme.slider_track;
            break;
        case ATGL_NODE_PROGRESSBAR:
            node->fg = atgl.theme.progress_text;
            node->bg = atgl.theme.progress_bg;
            break;
        case ATGL_NODE_PANEL:
            node->fg = atgl.theme.widget_fg;
            node->bg = atgl.theme.widget_bg;
            break;
        case ATGL_NODE_LISTBOX:
            node->fg = atgl.theme.list_fg;
            node->bg = atgl.theme.list_bg;
            break;
        default:
            node->fg = atgl.theme.fg;
            node->bg = atgl.theme.bg;
            break;
    }
}

/* Walk up the parent chain to find the nearest non-transparent bg.
   Used so that widgets with SEE_THROUGH bg can clear their area
   before re-drawing text (prevents old text ghosting). */
static VBE_COLOUR resolve_bg(PATGL_NODE node)
{
    PATGL_NODE n = node->parent;
    while (n) {
        if (n->bg != VBE_SEE_THROUGH) return n->bg;
        n = n->parent;
    }
    return 0; // Fallback
}

/* ----------------------------- Label ----------------------------- */

static VOID render_label(PATGL_NODE node)
{
    ATGL_RECT  *r = &node->abs_rect;
    VBE_COLOUR  fg = node->fg;

    /* Always clear the label area so old text is erased.
       Use the node's own bg, or walk up parents to find one. */
    VBE_COLOUR clear_bg = (node->bg != VBE_SEE_THROUGH)
                          ? node->bg : resolve_bg(node);
    atgl_fb_fill(r->x, r->y, r->w, r->h, clear_bg);

    if (node->text[0]) {
        I32 text_y = r->y + (r->h - (I32)atgl.theme.char_h) / 2;
        ATGL_DRAW_TEXT_CLIPPED(r->x, text_y, r->w,
                               (PU8)node->text, fg, clear_bg);
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
    else                     face = node->bg;

    VBE_COLOUR text_fg = node->enabled ? node->fg : t->btn_disabled_fg;

    /* Face fill */
    atgl_fb_fill(r->x, r->y, r->w, r->h, face);

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
        atgl_fb_rect(r->x - 3, r->y - 3,
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
    atgl_fb_fill(r->x, box_y, box_sz, box_sz, node->bg);
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
        VBE_COLOUR fg = node->enabled ? node->fg : t->btn_disabled_fg;
        VBE_COLOUR cbg = resolve_bg(node);
        I32 text_x = r->x + (I32)box_sz + 4;
        I32 text_y = r->y + (r->h - (I32)t->char_h) / 2;
        /* Clear label area so old text doesn't ghost */
        atgl_fb_fill(text_x, text_y,
                     r->w - (I32)box_sz - 4, (I32)t->char_h, cbg);
        ATGL_DRAW_TEXT(text_x, text_y, (PU8)node->text, fg, cbg);
    }

    /* Focus ring */
    if (node->focused) {
        atgl_fb_rect(r->x - 1, box_y - 1,
                     box_sz + 2, box_sz + 2, t->focus_ring);
    }
}

/* ----------------------------- Radio ----------------------------- */

static VOID render_radio(PATGL_NODE node)
{
    ATGL_RECT       *r  = &node->abs_rect;
    ATGL_THEME      *t  = &atgl.theme;
    ATGL_RADIO_DATA *rd = &node->data.radio;
    VBE_COLOUR       bg = (node->bg != VBE_SEE_THROUGH)
                          ? node->bg : resolve_bg(node);


    U32 circle_sz = 12;
    U32 border_w = 2; // Border thickness
    I32 cx = r->x + (I32)circle_sz / 2;
    I32 cy = r->y + r->h / 2;

    // Clear the whole control area so old ring/dot pixels are erased
    atgl_fb_fill(r->x, r->y, r->w, r->h, bg);

    // Draw border (outer ellipse)
    DRAW_ELLIPSE(cx, cy, circle_sz / 2, circle_sz / 2, t->widget_border);

    // Draw body (slightly smaller filled ellipse)
    ATGL_DRAW_FILLED_ELLIPSE((U32)cx, (U32)cy,
        (circle_sz - border_w) / 2, (circle_sz - border_w) / 2, node->bg);

    // Draw inner dot when selected (well inside the body)
    if (rd->selected) {
        ATGL_DRAW_FILLED_ELLIPSE((U32)cx, (U32)cy, 3, 3, t->check_mark);
    }

    /* Label */
    if (node->text[0]) {
        VBE_COLOUR fg = node->enabled ? node->fg : t->btn_disabled_fg;
        I32 text_x = r->x + (I32)circle_sz + 4;
        I32 text_y = r->y + (r->h - (I32)t->char_h) / 2;
        /* Clear label area so old text doesn't ghost */
        atgl_fb_fill(text_x, text_y,
                     r->w - (I32)circle_sz - 4, (I32)t->char_h, bg);
        ATGL_DRAW_TEXT(text_x, text_y, (PU8)node->text, fg, bg);
    }

    /* Focus ring */
    if (node->focused) {
        atgl_fb_rect(r->x - 1,
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
    atgl_fb_fill(r->x, r->y, r->w, r->h, node->bg);

    /* Border */
    ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);
    if (node->focused) {
        atgl_fb_rect(r->x, r->y, r->w, r->h, t->input_focus);
    }

    I32 text_x       = r->x + 3;
    I32 text_y       = r->y + (r->h - (I32)t->char_h) / 2;
    U32 visible_chars = ((U32)r->w - 6) / t->char_w;

    if (td->len > 0) {
        PU8 display_str = (PU8)&td->buffer[td->scroll_offset];
        U32 sel_min = td->selection_start < td->selection_end ? td->selection_start : td->selection_end;
        U32 sel_max = td->selection_start > td->selection_end ? td->selection_start : td->selection_end;

        /* Determine how many characters we will actually draw */
        U32 draw_count = visible_chars;
        if (draw_count > td->len - td->scroll_offset)
            draw_count = td->len - td->scroll_offset;

        BOOL has_selection = (sel_min < sel_max);

        if (td->password) {
            if (!has_selection) {
                /* No selection — fill a password-mask string and draw once */
                CHAR pwd_buf[ATGL_TEXTINPUT_MAX];
                MEMSET_OPT(pwd_buf, '*', draw_count);
                pwd_buf[draw_count] = '\0';
                DRAW_8x8_STRING(text_x, text_y, (PU8)pwd_buf,
                                node->fg, node->bg);
            } else {
                /* Selection present — batch into up to 3 runs */
                CHAR pwd_buf[ATGL_TEXTINPUT_MAX];
                for (U32 i = 0; i < draw_count; i++) pwd_buf[i] = '*';

                U32 vis_sel_start = (sel_min > td->scroll_offset) ? sel_min - td->scroll_offset : 0;
                U32 vis_sel_end   = (sel_max > td->scroll_offset) ? sel_max - td->scroll_offset : 0;
                if (vis_sel_end > draw_count) vis_sel_end = draw_count;

                /* Before selection */
                if (vis_sel_start > 0) {
                    pwd_buf[vis_sel_start] = '\0';
                    DRAW_8x8_STRING(text_x, text_y, (PU8)pwd_buf,
                                    node->fg, node->bg);
                    pwd_buf[vis_sel_start] = '*';
                }
                /* Selected region */
                if (vis_sel_end > vis_sel_start) {
                    CHAR sel_buf[ATGL_TEXTINPUT_MAX];
                    U32 sel_len = vis_sel_end - vis_sel_start;
                    MEMSET_OPT(sel_buf, '*', sel_len);
                    sel_buf[sel_len] = '\0';
                    DRAW_8x8_STRING(text_x + (I32)(vis_sel_start * t->char_w),
                                    text_y, (PU8)sel_buf,
                                    t->list_sel_fg, t->list_sel_bg);
                }
                /* After selection */
                if (vis_sel_end < draw_count) {
                    pwd_buf[vis_sel_end] = '*';
                    pwd_buf[draw_count]  = '\0';
                    DRAW_8x8_STRING(text_x + (I32)(vis_sel_end * t->char_w),
                                    text_y, (PU8)&pwd_buf[vis_sel_end],
                                    node->fg, node->bg);
                }
            }
        } else {
            if (!has_selection) {
                /* No selection — single batched string draw */
                CHAR tmp_buf[ATGL_TEXTINPUT_MAX];
                MEMCPY_OPT(tmp_buf, display_str, draw_count);
                tmp_buf[draw_count] = '\0';
                DRAW_8x8_STRING(text_x, text_y, (PU8)tmp_buf,
                                t->input_fg, t->input_bg);
            } else {
                /* Selection present — batch into up to 3 runs */
                U32 vis_sel_start = (sel_min > td->scroll_offset) ? sel_min - td->scroll_offset : 0;
                U32 vis_sel_end   = (sel_max > td->scroll_offset) ? sel_max - td->scroll_offset : 0;
                if (vis_sel_end > draw_count) vis_sel_end = draw_count;

                /* Before selection */
                if (vis_sel_start > 0) {
                    CHAR tmp_buf[ATGL_TEXTINPUT_MAX];
                    MEMCPY_OPT(tmp_buf, display_str, vis_sel_start);
                    tmp_buf[vis_sel_start] = '\0';
                    DRAW_8x8_STRING(text_x, text_y, (PU8)tmp_buf,
                                    t->input_fg, t->input_bg);
                }
                /* Selected region */
                if (vis_sel_end > vis_sel_start) {
                    CHAR sel_buf[ATGL_TEXTINPUT_MAX];
                    U32 sel_len = vis_sel_end - vis_sel_start;
                    MEMCPY_OPT(sel_buf, &display_str[vis_sel_start], sel_len);
                    sel_buf[sel_len] = '\0';
                    DRAW_8x8_STRING(text_x + (I32)(vis_sel_start * t->char_w),
                                    text_y, (PU8)sel_buf,
                                    t->list_sel_fg, t->list_sel_bg);
                }
                /* After selection */
                if (vis_sel_end < draw_count) {
                    CHAR tmp_buf[ATGL_TEXTINPUT_MAX];
                    U32 after_len = draw_count - vis_sel_end;
                    MEMCPY_OPT(tmp_buf, &display_str[vis_sel_end], after_len);
                    tmp_buf[after_len] = '\0';
                    DRAW_8x8_STRING(text_x + (I32)(vis_sel_end * t->char_w),
                                    text_y, (PU8)tmp_buf,
                                    t->input_fg, t->input_bg);
                }
            }
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
        atgl_fb_vline(cur_x, text_y, (I32)t->char_h, t->input_fg);
    }
}

/* ----------------------------- Slider ---------------------------- */

static VOID render_slider(PATGL_NODE node)
{
    ATGL_RECT        *r  = &node->abs_rect;
    ATGL_THEME       *t  = &atgl.theme;
    ATGL_SLIDER_DATA *sd = &node->data.slider;
    VBE_COLOUR        bg = (node->bg != VBE_SEE_THROUGH)
                          ? node->bg : resolve_bg(node);

    I32 track_h = 4;
    I32 track_y = r->y + (r->h - track_h) / 2;
    I32 thumb_w = 10;
    I32 thumb_h = r->h - 4;
    I32 usable_w = r->w - thumb_w;

    if (usable_w < 0) usable_w = 0;

    /* Clear the full control area so old thumb positions are erased */
    atgl_fb_fill(r->x, r->y, r->w, r->h, bg);

    /* Track */
    atgl_fb_fill(r->x, track_y, r->w, track_h, node->bg);
    ATGL_DRAW_SUNKEN(r->x, track_y, r->w, track_h);

    /* Filled portion */
    I32 range = sd->max - sd->min;
    if (range <= 0) range = 1;
    I32 fill_w = ((sd->value - sd->min) * usable_w) / range;
    if (fill_w < 0) fill_w = 0;
    if (fill_w > usable_w) fill_w = usable_w;
    if (fill_w > 0) {
        atgl_fb_fill(r->x + 1, track_y + 1,
                     fill_w, track_h - 2, t->slider_fill);
    }

    /* Thumb */
    I32 thumb_x = r->x + fill_w;
    I32 thumb_y = r->y + 2;
    atgl_fb_fill(thumb_x, thumb_y, thumb_w, thumb_h, node->fg);
    ATGL_DRAW_RAISED(thumb_x, thumb_y, thumb_w, thumb_h);

    /* Focus ring */
    if (node->focused) {
        atgl_fb_rect(r->x - 1, r->y - 1,
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
    atgl_fb_fill(r->x, r->y, r->w, r->h, node->bg);
    ATGL_DRAW_SUNKEN(r->x, r->y, r->w, r->h);

    /* Fill bar */
    U32 max_val = pd->max ? pd->max : 1;
    I32 fill_w  = (I32)((pd->value * (U32)(r->w - 4)) / max_val);
    if (fill_w > r->w - 4) fill_w = r->w - 4;
    if (fill_w > 0) {
        atgl_fb_fill(r->x + 2, r->y + 2,
                     fill_w, r->h - 4, node->fg);
    }

    /* Percentage text */
    if (pd->show_text) {
        CHAR buf[8];
        U32 pct = (pd->value * 100) / max_val;
        SPRINTF((PU8)buf, "%d%%", pct);
        U32 tw = STRLEN((PU8)buf) * t->char_w;
        I32 tx = r->x + (r->w - (I32)tw) / 2;
        I32 ty = r->y + (r->h - (I32)t->char_h) / 2;
        /* Use the fill or bg color behind the text so it reads cleanly */
        // VBE_COLOUR text_bg = (fill_w > 0 && tx >= r->x + 2 &&
        //                       tx < r->x + 2 + fill_w)
        //                      ? t->progress_fill : t->progress_bg;
        VBE_COLOUR text_bg = node->bg;
        ATGL_DRAW_TEXT(tx, ty, (PU8)buf, node->fg, text_bg);
    }
}

/* ----------------------------- Panel ----------------------------- */

static VOID render_panel(PATGL_NODE node)
{
    ATGL_RECT *r = &node->abs_rect;
    VBE_COLOUR pbg = (node->bg != VBE_SEE_THROUGH)
                     ? node->bg : resolve_bg(node);

    /* Always clear the full panel area so old borders/titles disappear */
    atgl_fb_fill(r->x, r->y, r->w, r->h, pbg);

    /* Border with optional title (GroupBox style) */
    if (node->text[0]) {
        I32 half_ch = (I32)atgl.theme.char_h / 2;
        atgl_fb_rect(r->x, r->y + half_ch,
                     r->w, r->h - half_ch,
                     atgl.theme.widget_border);

        /* Title text with gap */
        U32 title_w = STRLEN((PU8)node->text) * atgl.theme.char_w + 4;
        atgl_fb_fill(r->x + 8, r->y,
                     (I32)title_w, (I32)atgl.theme.char_h,
                     pbg);
        ATGL_DRAW_TEXT(r->x + 10, r->y, (PU8)node->text,
                       node->fg, pbg);
    }
}

/* ----------------------------- Listbox --------------------------- */

static VOID render_listbox(PATGL_NODE node)
{
    ATGL_RECT         *r  = &node->abs_rect;
    ATGL_THEME        *t  = &atgl.theme;
    ATGL_LISTBOX_DATA *ld = &node->data.listbox;

    /* Background */
    atgl_fb_fill(r->x, r->y, r->w, r->h, node->bg);
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
        VBE_COLOUR ibg = sel ? t->list_sel_bg : node->bg;
        VBE_COLOUR ifg = sel ? t->list_sel_fg : node->fg;

        atgl_fb_fill(r->x + 2, iy, r->w - 4, (I32)item_h, ibg);
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

        atgl_fb_fill(sb_x, r->y + 2, 8, sb_h, t->widget_bg);
        atgl_fb_fill(sb_x, thumb_y, 8, thumb_h, t->shadow);
    }

    /* Focus ring */
    if (node->focused) {
        atgl_fb_rect(r->x - 1, r->y - 1,
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
        atgl_fb_hline(r->x, y,     r->w, t->shadow);
        atgl_fb_hline(r->x, y + 1, r->w, t->highlight);
    } else {
        /* Vertical */
        I32 x = r->x + r->w / 2;
        atgl_fb_vline(x,     r->y, r->h, t->shadow);
        atgl_fb_vline(x + 1, r->y, r->h, t->highlight);
    }
}

/* ----------------------------- Image ----------------------------- */

static VOID render_image(PATGL_NODE node)
{
    ATGL_RECT       *r  = &node->abs_rect;
    ATGL_IMAGE_DATA *id = &node->data.image;

    if (!id->pixels) return;

    U32 *pixels = (U32 *)id->pixels;
    U32 zoom    = id->zoom ? id->zoom : 100;

    /* Scaled pixel size (how many screen pixels per image pixel) */
    U32 scale = zoom / 100;
    if (scale < 1) scale = 1;

    I32 ox = id->offset_x;
    I32 oy = id->offset_y;

    U32 view_w = (U32)r->w;
    U32 view_h = (U32)r->h;

    /* For each screen pixel inside the widget, work out which image
       pixel it maps to and emit runs of the same colour. */
    if (zoom <= 100) {
        /* Zoom-out / 1:1 — one image pixel maps to <=1 screen pixel.
           Step through image coords and sample. */
        U32 draw_w = id->img_w * zoom / 100;
        U32 draw_h = id->img_h * zoom / 100;
        if (draw_w > view_w) draw_w = view_w;
        if (draw_h > view_h) draw_h = view_h;

        for (U32 sy = 0; sy < draw_h; sy++) {
            U32 img_y = (sy * 100) / zoom + oy;
            if (img_y >= id->img_h) continue;

            U32 row_off = img_y * id->img_w;
            U32 sx = 0;
            while (sx < draw_w) {
                U32 img_x = (sx * 100) / zoom + ox;
                if (img_x >= id->img_w) { sx++; continue; }

                VBE_COLOUR c = pixels[row_off + img_x];
                if (c == VBE_SEE_THROUGH) { sx++; continue; }

                /* Find run of same colour at this scanline */
                U32 run_start = sx;
                while (sx < draw_w) {
                    U32 nx = (sx * 100) / zoom + ox;
                    if (nx >= id->img_w) break;
                    if (pixels[row_off + nx] != c) break;
                    sx++;
                }
                /* Direct framebuffer horizontal run — no syscall */
                atgl_fb_hline(r->x + (I32)run_start, r->y + (I32)sy,
                              (I32)(sx - run_start), c);
            }
        }
    } else {
        /* Zoom-in — each image pixel covers scale×scale screen pixels. */
        /* How many image pixels fit in the viewport? */
        U32 vis_img_w = (view_w + scale - 1) / scale;
        U32 vis_img_h = (view_h + scale - 1) / scale;

        for (U32 iy = 0; iy < vis_img_h; iy++) {
            I32 src_y = (I32)iy + oy;
            if (src_y < 0 || (U32)src_y >= id->img_h) continue;

            U32 row_off = (U32)src_y * id->img_w;
            I32 screen_y = r->y + (I32)(iy * scale);
            if (screen_y >= r->y + (I32)view_h) break;

            I32 block_h = (I32)scale;
            if (screen_y + block_h > r->y + (I32)view_h)
                block_h = r->y + (I32)view_h - screen_y;

            U32 ix = 0;
            while (ix < vis_img_w) {
                I32 src_x = (I32)ix + ox;
                if (src_x < 0 || (U32)src_x >= id->img_w) { ix++; continue; }

                VBE_COLOUR c = pixels[row_off + (U32)src_x];
                if (c == VBE_SEE_THROUGH) { ix++; continue; }

                I32 screen_x = r->x + (I32)(ix * scale);
                if (screen_x >= r->x + (I32)view_w) break;

                /* Coalesce adjacent image pixels of the same colour
                   into a single wide filled-rectangle. */
                U32 run = 1;
                while (ix + run < vis_img_w) {
                    I32 nx = (I32)(ix + run) + ox;
                    if (nx < 0 || (U32)nx >= id->img_w) break;
                    if (pixels[row_off + (U32)nx] != c) break;
                    run++;
                }

                I32 total_w = (I32)(run * scale);
                if (screen_x + total_w > r->x + (I32)view_w)
                    total_w = r->x + (I32)view_w - screen_x;

                /* Direct framebuffer fill — no syscalls */
                atgl_fb_fill(screen_x, screen_y, total_w, block_h, c);
                ix += run;
            }
        }

        /* Pixel grid overlay (only when zoomed in enough) */
        if (id->show_grid && scale >= 4) {
            VBE_COLOUR gc = id->grid_colour;

            /* Vertical grid lines */
            for (U32 ix = 0; ix <= vis_img_w && (I32)(ix * scale) <= (I32)view_w; ix++) {
                I32 lx = r->x + (I32)(ix * scale);
                if (lx >= r->x + (I32)view_w) break;
                atgl_fb_vline(lx, r->y, (I32)view_h, gc);
            }
            /* Horizontal grid lines */
            for (U32 iy = 0; iy <= vis_img_h && (I32)(iy * scale) <= (I32)view_h; iy++) {
                I32 ly = r->y + (I32)(iy * scale);
                if (ly >= r->y + (I32)view_h) break;
                atgl_fb_hline(r->x, ly, (I32)view_w, gc);
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
            bd->pressed = TRUE;
            ATGL_NODE_INVALIDATE(node);
            ev->handled = TRUE;
        }
        break;

    case ATGL_EVT_MOUSE_UP:
        if (bd->pressed) {
            bd->pressed = FALSE;
            ATGL_NODE_INVALIDATE(node);
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
                ATGL_NODE_INVALIDATE(node);
            }
        }
        break;

    case ATGL_EVT_KEY_DOWN:
        if (node->focused &&
            (ev->key.keycode == KEY_ENTER ||
             ev->key.keycode == KEY_SPACE)) {
            bd->pressed = TRUE;
            ATGL_NODE_INVALIDATE(node);
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
        ATGL_NODE_INVALIDATE(node);
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
                    ATGL_NODE_INVALIDATE(sib);
                }
            }
        }

        rd->selected = TRUE;
        ATGL_NODE_INVALIDATE(node);
        if (node->on_change) node->on_change(node);
        ev->handled = TRUE;
    }
}

/* ----------------------------- TextInput ------------------------- */

static VOID textinput_delete_selection(ATGL_TEXTINPUT_DATA *td) {
    U32 sel_min = td->selection_start < td->selection_end ? td->selection_start : td->selection_end;
    U32 sel_max = td->selection_start > td->selection_end ? td->selection_start : td->selection_end;
    if (sel_min == sel_max) return;
    
    U32 del_len = sel_max - sel_min;
    MEMMOVE_OPT(&td->buffer[sel_min], &td->buffer[sel_max], td->len - sel_max + 1);
    td->len -= del_len;
    td->cursor = sel_min;
    td->selection_start = td->selection_end = td->cursor;
}

static VOID event_textinput(PATGL_NODE node, ATGL_EVENT *ev)
{
    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;
    U32 visible_chars = ((U32)node->abs_rect.w - 6) / atgl.theme.char_w;

    if (ev->type == ATGL_EVT_MOUSE_DOWN) {
        if (ATGL_RECT_CONTAINS(&node->abs_rect, ev->mouse.x, ev->mouse.y)) {
            I32 rel_x = ev->mouse.x - (node->abs_rect.x + 3);
            if (rel_x < 0) rel_x = 0;
            U32 char_idx = td->scroll_offset + (U32)rel_x / atgl.theme.char_w;
            if (char_idx > td->len) char_idx = td->len;
            
            td->cursor = char_idx;
            td->selection_start = td->cursor;
            td->selection_end = td->cursor;
            td->is_selecting = TRUE;
            ATGL_NODE_INVALIDATE(node);
            ev->handled = TRUE;
        }
        return;
    } else if (ev->type == ATGL_EVT_MOUSE_MOVE && td->is_selecting) {
        I32 rel_x = ev->mouse.x - (node->abs_rect.x + 3);
        if (rel_x < 0) rel_x = 0;
        U32 char_idx = td->scroll_offset + (U32)rel_x / atgl.theme.char_w;
        if (char_idx > td->len) char_idx = td->len;
        
        if (ev->mouse.btn_left) {
            if (char_idx != td->cursor || char_idx != td->selection_end) {
                td->cursor = char_idx;
                td->selection_end = td->cursor;
                ATGL_NODE_INVALIDATE(node);
            }
        } else {
            td->is_selecting = FALSE;
        }
        ev->handled = TRUE;
        return;
    } else if (ev->type == ATGL_EVT_MOUSE_UP) {
        if (td->is_selecting) {
            td->is_selecting = FALSE;
            ev->handled = TRUE;
        }
        return;
    }

    if (ev->type != ATGL_EVT_KEY_DOWN || !node->focused) return;

    U32 sel_min = td->selection_start < td->selection_end ? td->selection_start : td->selection_end;
    U32 sel_max = td->selection_start > td->selection_end ? td->selection_start : td->selection_end;

    /* Handle CTRL + A/C/V/X */
    if (ev->key.ctrl) {
        if (ev->key.ch == 'a' || ev->key.ch == 'A') {
            td->selection_start = 0;
            td->selection_end = td->len;
            td->cursor = td->len;
            ATGL_NODE_INVALIDATE(node);
            ev->handled = TRUE;
        } else if (ev->key.ch == 'c' || ev->key.ch == 'C') {
            if (sel_min < sel_max && !td->password) {
                U32 copy_len = sel_max - sel_min;
                if (copy_len >= ATGL_TEXTINPUT_MAX) copy_len = ATGL_TEXTINPUT_MAX - 1;
                MEMCPY_OPT(atgl.clipboard, &td->buffer[sel_min], copy_len);
                atgl.clipboard[copy_len] = '\0';
            }
            ev->handled = TRUE;
        } else if (ev->key.ch == 'x' || ev->key.ch == 'X') {
            if (sel_min < sel_max && !td->password) {
                U32 copy_len = sel_max - sel_min;
                if (copy_len >= ATGL_TEXTINPUT_MAX) copy_len = ATGL_TEXTINPUT_MAX - 1;
                MEMCPY_OPT(atgl.clipboard, &td->buffer[sel_min], copy_len);
                atgl.clipboard[copy_len] = '\0';
                textinput_delete_selection(td);
                ATGL_NODE_INVALIDATE(node);
                if (node->on_change) node->on_change(node);
            }
            ev->handled = TRUE;
        } else if (ev->key.ch == 'v' || ev->key.ch == 'V') {
            textinput_delete_selection(td);
            U32 paste_len = STRLEN(atgl.clipboard);
            if (paste_len > 0 && td->len + paste_len < td->max_len) {
                MEMMOVE_OPT(&td->buffer[td->cursor + paste_len],
                        &td->buffer[td->cursor],
                        td->len - td->cursor + 1);
                MEMCPY_OPT(&td->buffer[td->cursor], atgl.clipboard, paste_len);
                td->cursor += paste_len;
                td->len += paste_len;
                if (td->cursor >= td->scroll_offset + visible_chars)
                    td->scroll_offset = td->cursor > visible_chars ? td->cursor - visible_chars : 0;
                td->selection_start = td->selection_end = td->cursor;
                ATGL_NODE_INVALIDATE(node);
                if (node->on_change) node->on_change(node);
            }
            ev->handled = TRUE;
        }
        return;
    }

    switch (ev->key.keycode) {
    case KEY_DELETE:
        if (sel_min < sel_max) {
            textinput_delete_selection(td);
            ATGL_NODE_INVALIDATE(node);
            if (node->on_change) node->on_change(node);
        } else if (td->cursor < td->len) {
            MEMMOVE_OPT(&td->buffer[td->cursor],
                    &td->buffer[td->cursor + 1],
                    td->len - td->cursor);
            td->len--;
            td->selection_start = td->selection_end = td->cursor;
            ATGL_NODE_INVALIDATE(node);
            if (node->on_change) node->on_change(node);
        }
        ev->handled = TRUE;
        break;
    case KEY_BACKSPACE:
        if (sel_min < sel_max) {
            textinput_delete_selection(td);
            ATGL_NODE_INVALIDATE(node);
            if (node->on_change) node->on_change(node);
        } else if (td->cursor > 0) {
            MEMMOVE_OPT(&td->buffer[td->cursor - 1],
                    &td->buffer[td->cursor],
                    td->len - td->cursor + 1);
            td->cursor--;
            td->len--;
            if (td->scroll_offset > 0 &&
                td->cursor < td->scroll_offset) {
                td->scroll_offset = td->cursor;
            }
            td->selection_start = td->selection_end = td->cursor;
            ATGL_NODE_INVALIDATE(node);
            if (node->on_change) node->on_change(node);
        }
        ev->handled = TRUE;
        break;

    case KEY_ARROW_LEFT:
        if (ev->key.shift) {
            if (td->cursor > 0) {
                td->cursor--;
                td->selection_end = td->cursor;
                if (td->cursor < td->scroll_offset)
                    td->scroll_offset = td->cursor;
                ATGL_NODE_INVALIDATE(node);
            }
        } else {
            if (sel_min < sel_max) {
                td->cursor = sel_min;
                td->selection_start = td->selection_end = td->cursor;
                ATGL_NODE_INVALIDATE(node);
            } else if (td->cursor > 0) {
                td->cursor--;
                td->selection_start = td->selection_end = td->cursor;
                if (td->cursor < td->scroll_offset)
                    td->scroll_offset = td->cursor;
                ATGL_NODE_INVALIDATE(node);
            }
        }
        ev->handled = TRUE;
        break;

    case KEY_ARROW_RIGHT:
        if (ev->key.shift) {
            if (td->cursor < td->len) {
                td->cursor++;
                td->selection_end = td->cursor;
                if (td->cursor >= td->scroll_offset + visible_chars)
                    td->scroll_offset = td->cursor > visible_chars ? td->cursor - visible_chars : 0;
                ATGL_NODE_INVALIDATE(node);
            }
        } else {
            if (sel_min < sel_max) {
                td->cursor = sel_max;
                td->selection_start = td->selection_end = td->cursor;
                ATGL_NODE_INVALIDATE(node);
            } else if (td->cursor < td->len) {
                td->cursor++;
                td->selection_start = td->selection_end = td->cursor;
                if (td->cursor >= td->scroll_offset + visible_chars)
                    td->scroll_offset = td->cursor > visible_chars ? td->cursor - visible_chars : 0;
                ATGL_NODE_INVALIDATE(node);
            }
        }
        ev->handled = TRUE;
        break;

    default:
        if(ev->type != ATGL_EVT_KEY_DOWN || !node->focused) break;
        /* Printable character insertion */
        if (ev->key.ch >= 0x20 && ev->key.ch < 0x7F) {
            if (sel_min < sel_max) {
                textinput_delete_selection(td);
            }
            if (td->len < td->max_len - 1) {
                MEMMOVE_OPT(&td->buffer[td->cursor + 1],
                        &td->buffer[td->cursor],
                        td->len - td->cursor + 1);
                td->buffer[td->cursor] = ev->key.ch;
                td->cursor++;
                td->len++;
                td->selection_start = td->selection_end = td->cursor;
                if (td->cursor >= td->scroll_offset + visible_chars)
                    td->scroll_offset = td->cursor > visible_chars ? td->cursor - visible_chars : 0;
                ATGL_NODE_INVALIDATE(node);
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
        I32 thumb_w = 10;
        I32 usable_w = node->abs_rect.w - thumb_w;
        I32 rel_x   = ev->mouse.x - node->abs_rect.x - thumb_w / 2;
        I32 range   = sd->max - sd->min;

        if (usable_w <= 0) usable_w = 1;
        if (range <= 0) range = 1;
        if (rel_x < 0) rel_x = 0;
        if (rel_x > usable_w) rel_x = usable_w;

        I32 new_val = sd->min + (rel_x * range) / usable_w;

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
            ATGL_NODE_INVALIDATE(node);
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
            ATGL_NODE_INVALIDATE(node);
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
                ATGL_NODE_INVALIDATE(node);
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
            ATGL_NODE_INVALIDATE(node);
            if (node->on_change) node->on_change(node);
            ev->handled = TRUE;
        }
        if (ev->key.keycode == KEY_ARROW_DOWN &&
            ld->selected + 1 < ld->item_count) {
            ld->selected++;
            if (ld->selected >= ld->scroll_offset + ld->visible_rows)
                ld->scroll_offset = ld->selected - ld->visible_rows + 1;
            ATGL_NODE_INVALIDATE(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
    if (bg != VBE_SEE_THROUGH) node->bg = bg;
    if (fg != VBE_SEE_THROUGH) node->fg = fg;
    return node;
}

PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT rect,
                              PU8 text,
                              VOID (*on_click)(PATGL_NODE))
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_BUTTON, parent, rect);
    if (!node) return NULLPTR;

    ATGL_NODE_SET_TEXT(node, text);
    node->focusable = TRUE;
    node->on_click  = on_click;
    node->fn_render = render_button;
    node->fn_event  = event_button;
    node->data.button.pressed = FALSE;
    node->data.button.hovered = FALSE;
    ATGL_NODE_RESET_THEME_COLORS(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
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

    /* CAlloc already zeroed the entire node including the union,
       so buffer/len/cursor/scroll_offset/password are already 0/FALSE. */
    ATGL_TEXTINPUT_DATA *td = &node->data.textinput;
    td->max_len = (max_len && max_len < ATGL_TEXTINPUT_MAX)
                  ? max_len : ATGL_TEXTINPUT_MAX - 1;

    if (placeholder) {
        U32 plen = STRLEN(placeholder);
        if (plen >= ATGL_PLACEHOLDER_MAX) plen = ATGL_PLACEHOLDER_MAX - 1;
        MEMCPY_OPT(td->placeholder, placeholder, plen);
    }
    ATGL_NODE_RESET_THEME_COLORS(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
    return node;
}

PATGL_NODE ATGL_CREATE_PANEL(PATGL_NODE parent, ATGL_RECT rect,
                             ATGL_LAYOUT_DIR layout,
                             U32 padding, U32 spacing)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_PANEL, parent, rect);
    if (!node) return NULLPTR;

    node->fn_render = render_panel;

    ATGL_PANEL_DATA *pd = &node->data.panel;
    pd->layout  = layout;
    pd->padding = padding;
    pd->spacing = spacing;
    ATGL_NODE_RESET_THEME_COLORS(node);
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
    ATGL_NODE_RESET_THEME_COLORS(node);
    return node;
}

PATGL_NODE ATGL_CREATE_SEPARATOR(PATGL_NODE parent, ATGL_RECT rect)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_SEPARATOR, parent, rect);
    if (!node) return NULLPTR;

    node->fn_render = render_separator;
    ATGL_NODE_RESET_THEME_COLORS(node);
    return node;
}

static VOID destroy_image(PATGL_NODE node)
{
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (id->owns_pixels && id->pixels) {
        MFree(id->pixels);
        id->pixels = NULLPTR;
    }
}

PATGL_NODE ATGL_CREATE_IMAGE(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 pixels, U32 img_w, U32 img_h)
{
    PATGL_NODE node = ATGL_NODE_CREATE(ATGL_NODE_IMAGE, parent, rect);
    if (!node) return NULLPTR;

    node->fn_render             = render_image;
    node->fn_destroy            = destroy_image;
    node->data.image.pixels     = pixels;
    node->data.image.img_w      = img_w;
    node->data.image.img_h      = img_h;
    node->data.image.offset_x   = 0;
    node->data.image.offset_y   = 0;
    node->data.image.zoom       = 100;
    node->data.image.owns_pixels = FALSE;
    node->data.image.show_grid   = FALSE;
    node->data.image.grid_colour = RGB(60, 60, 60);
    ATGL_NODE_RESET_THEME_COLORS(node);
    return node;
}

PATGL_NODE ATGL_CREATE_BLANK_IMAGE(PATGL_NODE parent, ATGL_RECT rect,
                                   U32 img_w, U32 img_h,
                                   VBE_COLOUR fill)
{
    U32 buf_sz = img_w * img_h * sizeof(U32);
    PU8 pixels = (PU8)MAlloc(buf_sz);
    if (!pixels) return NULLPTR;

    /* Fill every pixel with the requested colour (rep stosl) */
    MEMSET32_OPT(pixels, fill, img_w * img_h);

    PATGL_NODE node = ATGL_CREATE_IMAGE(parent, rect, pixels, img_w, img_h);
    if (!node) {
        MFree(pixels);
        return NULLPTR;
    }
    node->data.image.owns_pixels = TRUE;
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
    ATGL_NODE_INVALIDATE(node);
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
        MEMCPY_OPT(td->buffer, text, len);
        td->buffer[len] = '\0';
        td->len    = len;
        td->cursor = len;
    } else {
        td->buffer[0] = '\0';
        td->len    = 0;
        td->cursor = 0;
    }
    td->scroll_offset = 0;
    ATGL_NODE_INVALIDATE(node);
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
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_PROGRESSBAR_SET(PATGL_NODE node, U32 value)
{
    if (!node || node->type != ATGL_NODE_PROGRESSBAR) return;
    ATGL_PROGRESS_DATA *pd = &node->data.progress;
    if (value > pd->max) value = pd->max;
    pd->value   = value;
    ATGL_NODE_INVALIDATE(node);
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
        MEMCPY_OPT(ld->items[ld->item_count], text, len);
        ld->items[ld->item_count][len] = '\0';
    }
    ld->item_count++;
    ATGL_NODE_INVALIDATE(node);
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
    ATGL_NODE_INVALIDATE(node);
}

/* ================================================================ */
/*                     IMAGE MANIPULATION                           */
/* ================================================================ */

#define IMG_CHECK(n) if (!(n) || (n)->type != ATGL_NODE_IMAGE) return
#define IMG_CHECK_RET(n, r) if (!(n) || (n)->type != ATGL_NODE_IMAGE) return (r)

VOID ATGL_IMAGE_SET_PIXEL(PATGL_NODE node, U32 x, U32 y,
                          VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels || x >= id->img_w || y >= id->img_h) return;
    ((U32 *)id->pixels)[y * id->img_w + x] = colour;
    ATGL_NODE_INVALIDATE(node);
}

VBE_COLOUR ATGL_IMAGE_GET_PIXEL(PATGL_NODE node, U32 x, U32 y)
{
    IMG_CHECK_RET(node, 0);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels || x >= id->img_w || y >= id->img_h) return 0;
    return ((U32 *)id->pixels)[y * id->img_w + x];
}

VOID ATGL_IMAGE_CLEAR(PATGL_NODE node, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels) return;
    U32 total = id->img_w * id->img_h;
    MEMSET32_OPT(id->pixels, colour, total);
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_FILL_RECT(PATGL_NODE node, U32 x, U32 y,
                          U32 w, U32 h, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels) return;
    U32 *p = (U32 *)id->pixels;

    U32 x1 = x, y1 = y;
    U32 x2 = x + w, y2 = y + h;
    if (x2 > id->img_w) x2 = id->img_w;
    if (y2 > id->img_h) y2 = id->img_h;

    U32 row_w = x2 - x1;
    for (U32 iy = y1; iy < y2; iy++)
        MEMSET32_OPT(&p[iy * id->img_w + x1], colour, row_w);

    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_DRAW_RECT(PATGL_NODE node, U32 x, U32 y,
                          U32 w, U32 h, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels || w == 0 || h == 0) return;
    U32 *p = (U32 *)id->pixels;

    U32 x2 = x + w - 1, y2 = y + h - 1;
    if (x2 >= id->img_w) x2 = id->img_w - 1;
    if (y2 >= id->img_h) y2 = id->img_h - 1;

    for (U32 ix = x; ix <= x2; ix++) {
        if (y  < id->img_h) p[y  * id->img_w + ix] = colour;
        if (y2 < id->img_h) p[y2 * id->img_w + ix] = colour;
    }
    for (U32 iy = y; iy <= y2; iy++) {
        if (x  < id->img_w) p[iy * id->img_w + x]  = colour;
        if (x2 < id->img_w) p[iy * id->img_w + x2] = colour;
    }
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_DRAW_LINE(PATGL_NODE node, I32 x0, I32 y0,
                          I32 x1, I32 y1, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels) return;
    U32 *p = (U32 *)id->pixels;

    I32 dx = x1 - x0;
    I32 dy = y1 - y0;
    I32 sx = dx >= 0 ? 1 : -1;
    I32 sy = dy >= 0 ? 1 : -1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    I32 err = dx - dy;

    while (1) {
        if (x0 >= 0 && (U32)x0 < id->img_w &&
            y0 >= 0 && (U32)y0 < id->img_h)
            p[y0 * id->img_w + x0] = colour;

        if (x0 == x1 && y0 == y1) break;
        I32 e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_FILL_CIRCLE(PATGL_NODE node, I32 cx, I32 cy,
                            U32 radius, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels) return;
    U32 *p  = (U32 *)id->pixels;
    I32  r  = (I32)radius;
    I32 r2  = r * r;

    for (I32 dy = -r; dy <= r; dy++) {
        I32 py = cy + dy;
        if (py < 0 || (U32)py >= id->img_h) continue;
        for (I32 dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy > r2) continue;
            I32 px = cx + dx;
            if (px < 0 || (U32)px >= id->img_w) continue;
            p[py * id->img_w + px] = colour;
        }
    }
    ATGL_NODE_INVALIDATE(node);
}

PU8 ATGL_IMAGE_GET_PIXELS(PATGL_NODE node)
{
    IMG_CHECK_RET(node, NULLPTR);
    return node->data.image.pixels;
}

VOID ATGL_IMAGE_GET_SIZE(PATGL_NODE node, U32 *w, U32 *h)
{
    IMG_CHECK(node);
    if (w) *w = node->data.image.img_w;
    if (h) *h = node->data.image.img_h;
}

VOID ATGL_IMAGE_SET_PIXELS(PATGL_NODE node, PU8 pixels,
                           U32 img_w, U32 img_h)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (id->owns_pixels && id->pixels) {
        MFree(id->pixels);
    }
    id->pixels      = pixels;
    id->img_w       = img_w;
    id->img_h       = img_h;
    id->owns_pixels = FALSE;
    id->offset_x    = 0;
    id->offset_y    = 0;
    ATGL_NODE_INVALIDATE(node);
}

/* ---- Zoom ---- */

VOID ATGL_IMAGE_SET_ZOOM(PATGL_NODE node, U32 zoom)
{
    IMG_CHECK(node);
    if (zoom < 25)   zoom = 25;
    if (zoom > 3200) zoom = 3200;
    node->data.image.zoom = zoom;
    ATGL_NODE_INVALIDATE(node);
}

U32 ATGL_IMAGE_GET_ZOOM(PATGL_NODE node)
{
    IMG_CHECK_RET(node, 100);
    return node->data.image.zoom;
}

VOID ATGL_IMAGE_ZOOM_IN(PATGL_NODE node)
{
    IMG_CHECK(node);
    U32 z = node->data.image.zoom;
    z = z * 2;
    ATGL_IMAGE_SET_ZOOM(node, z);
}

VOID ATGL_IMAGE_ZOOM_OUT(PATGL_NODE node)
{
    IMG_CHECK(node);
    U32 z = node->data.image.zoom;
    z = z / 2;
    ATGL_IMAGE_SET_ZOOM(node, z);
}

/* ---- Pan / Scroll ---- */

VOID ATGL_IMAGE_SET_OFFSET(PATGL_NODE node, I32 ox, I32 oy)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    if ((U32)ox >= id->img_w) ox = (I32)id->img_w - 1;
    if ((U32)oy >= id->img_h) oy = (I32)id->img_h - 1;
    id->offset_x = ox;
    id->offset_y = oy;
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_GET_OFFSET(PATGL_NODE node, I32 *ox, I32 *oy)
{
    IMG_CHECK(node);
    if (ox) *ox = node->data.image.offset_x;
    if (oy) *oy = node->data.image.offset_y;
}

VOID ATGL_IMAGE_PAN(PATGL_NODE node, I32 dx, I32 dy)
{
    IMG_CHECK(node);
    ATGL_IMAGE_DATA *id = &node->data.image;
    ATGL_IMAGE_SET_OFFSET(node, id->offset_x + dx, id->offset_y + dy);
}

/* ---- Screen ↔ Image coordinate conversion ---- */

BOOL ATGL_IMAGE_SCREEN_TO_IMG(PATGL_NODE node, I32 sx, I32 sy,
                              U32 *ix, U32 *iy)
{
    IMG_CHECK_RET(node, FALSE);
    ATGL_IMAGE_DATA *id = &node->data.image;
    ATGL_RECT       *r  = &node->abs_rect;

    U32 zoom  = id->zoom ? id->zoom : 100;
    U32 scale = zoom / 100;
    if (scale < 1) scale = 1;

    /* Offset from widget top-left */
    I32 rx = sx - r->x;
    I32 ry = sy - r->y;
    if (rx < 0 || ry < 0) return FALSE;

    U32 img_x, img_y;
    if (zoom <= 100) {
        img_x = ((U32)rx * 100) / zoom + id->offset_x;
        img_y = ((U32)ry * 100) / zoom + id->offset_y;
    } else {
        img_x = (U32)rx / scale + id->offset_x;
        img_y = (U32)ry / scale + id->offset_y;
    }

    if (img_x >= id->img_w || img_y >= id->img_h)
        return FALSE;

    if (ix) *ix = img_x;
    if (iy) *iy = img_y;
    return TRUE;
}

/* ---- Grid ---- */

VOID ATGL_IMAGE_SHOW_GRID(PATGL_NODE node, BOOL show)
{
    IMG_CHECK(node);
    node->data.image.show_grid = show;
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_IMAGE_SET_GRID_COLOUR(PATGL_NODE node, VBE_COLOUR colour)
{
    IMG_CHECK(node);
    node->data.image.grid_colour = colour;
    ATGL_NODE_INVALIDATE(node);
}

BOOL ATGL_IMAGE_SAVE(PATGL_NODE node, const CHAR *filename) {
    IMG_CHECK_RET(node, FALSE);
    ATGL_IMAGE_DATA *id = &node->data.image;
    if (!id->pixels) return FALSE;

    if(FILE_EXISTS(filename)) {
        if (FILE_DELETE(filename) != 0) {
            return FALSE;
        }
    }
    FILE_CREATE(filename);
    FILE *f = FOPEN(filename, MODE_FA);
    if (!f) return FALSE;
    ATGL_IMAGE_HEADER header;
    MEMSET(&header, 0, sizeof(ATGL_IMAGE_HEADER));
    MEMCPY(header.signature, "ATGL", 4);
    header.version = 1;
    header.width   = id->img_w;
    header.height  = id->img_h;
    header.data_offset = sizeof(ATGL_IMAGE_HEADER);
    header.data_size   = id->img_w * id->img_h * sizeof(U32);
    header.bpp      = 32;
    if (FWRITE(f, &header, sizeof(header)) != sizeof(header)) {
        FCLOSE(f);
        return FALSE;
    }
    if (FWRITE(f, id->pixels, header.data_size) != header.data_size) {
        FCLOSE(f);
        return FALSE;
    }
    FCLOSE(f);
    return TRUE;
}

PATGL_NODE ATGL_CREATE_IMAGE_FROM_FILE(PATGL_NODE parent, ATGL_RECT rect,
                             const CHAR *filename) 
{
    FILE *f = FOPEN(filename, MODE_FR);
    if (!f) return NULLPTR;
    DEBUG_PRINTF("Loading image from file: %s\n", filename);
    ATGL_IMAGE_HEADER header;
    if (FREAD(f, &header, sizeof(ATGL_IMAGE_HEADER)) != sizeof(header)) {
        FCLOSE(f);
        return NULLPTR;
    }
    DEBUG_PRINTF("Image header: w=%u h=%u data_offset=%u data_size=%u bpp=%u\n",
                 header.width, header.height, header.data_offset,
                 header.data_size, header.bpp);
    if (header.data_offset < sizeof(ATGL_IMAGE_HEADER) ||
        header.data_size != header.width * header.height * sizeof(U32) ||
        header.bpp != 32 ||
        STRNCMP(header.signature, "ATGL", 4) != 0) {
        FCLOSE(f);
        return NULLPTR;
    }
    DEBUG_PRINTF("Image header is valid, loading pixel data...\n");
    PU8 pixels = (PU8)MAlloc(header.data_size);
    if (!pixels) {
        FCLOSE(f);
        return NULLPTR;
    }
    DEBUG_PRINTF("Allocated %u bytes for pixel data\n", header.data_size);
    U32 read = 0;
    if ((read = FREAD(f, pixels, header.data_size)) == 0) {
        MFree(pixels);
        FCLOSE(f);
        return NULLPTR;
    }
    DEBUG_PRINTF("Read %u bytes of pixel data\n", read);
    FCLOSE(f);
    rect.w = rect.w ? rect.w : header.width;
    rect.h = rect.h ? rect.h : header.height;
    PATGL_NODE node = ATGL_CREATE_IMAGE(parent, rect, pixels, header.width, header.height);
    DEBUG_PRINTF("Created image node: %p\n", node);
    if (!node) {
        MFree(pixels);
        return NULLPTR;
    }
    node->data.image.owns_pixels = TRUE;
    return node;
}