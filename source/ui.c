/* ui.c - widgets, pointer, snapping, background and icons. */
#include "ui.h"
#include "theme.h"
#include "input.h"
#include "settings.h"
#include "sound.h"
#include "lab.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MAX_TARGETS 256
#define ANIM_SLOTS 512

typedef struct { float x, y, w, h; UiId id; } Target;
typedef struct { UiId id; float v; int last; } Anim;

static Target targets[2][MAX_TARGETS];
static int ntargets[2];
static int cur;                 /* index of the list being filled this frame */
static bool input_on = true;
static int frame;
static UiId hover_id, prev_hover_id;
static int dpad_left = -1;
static Anim anims[ANIM_SLOTS];
static char toast_msg[128];
static int toast_until;

UiId ui_id(const char *s, int n) {
    UiId h = 2166136261u;
    for (; *s; s++) h = (h ^ (unsigned char)*s) * 16777619u;
    h = (h ^ (UiId)n) * 16777619u;
    return h ? h : 1;
}

static float elapsed;
float ui_time(void) { return elapsed; }
int ui_frame(void) { return frame; }
void ui_set_input(bool enabled) { input_on = enabled; }
bool ui_input(void) { return input_on; }
int ui_dpad_unhandled(void) { return dpad_left; }

static bool contains(const Target *t, float px, float py) {
    return px >= t->x && px < t->x + t->w && py >= t->y && py < t->y + t->h;
}

/* D-pad navigation: move to the nearest row (or column) in that direction,
 * then to the closest button within it. Scoring distance and sideways offset
 * together lets a far-away but aligned button beat a nearer one in a
 * staggered layout like the keyboard, which skips rows. */
static void snap_dpad(const Target *list, int n, int dir) {
    float px = input_cursor_x(), py = input_cursor_y();
    float along[MAX_TARGETS], across[MAX_TARGETS];
    float min_along = 1e9f;
    for (int i = 0; i < n; i++) {
        const Target *t = &list[i];
        along[i] = -1;
        if (contains(t, px, py)) continue;
        float cx = t->x + t->w * 0.5f, cy = t->y + t->h * 0.5f;
        float a, c, gap;
        switch (dir) {
        case DIR_UP:    a = py - cy; c = fabsf(cx - px); gap = py - (t->y + t->h); break;
        case DIR_DOWN:  a = cy - py; c = fabsf(cx - px); gap = t->y - py; break;
        case DIR_LEFT:  a = px - cx; c = fabsf(cy - py); gap = px - (t->x + t->w); break;
        default:        a = cx - px; c = fabsf(cy - py); gap = t->x - px; break;
        }
        if (a < 6 || gap < -2) continue;   /* must lie beyond the pointer in that direction */
        along[i] = a;
        across[i] = c;
        if (a < min_along) min_along = a;
    }
    int best = -1;
    float best_across = 1e9f;
    for (int i = 0; i < n; i++) {
        if (along[i] < 0) continue;
        float band = fmaxf(12.0f, ((dir == DIR_UP || dir == DIR_DOWN) ? list[i].h : list[i].w) * 0.6f);
        if (along[i] > min_along + band) continue;   /* not in the nearest row/column */
        if (across[i] < best_across) { best_across = across[i]; best = i; }
    }
    if (best >= 0) {
        const Target *t = &list[best];
        input_glide_cursor(t->x + fminf(t->w * 0.5f, 40.0f), t->y + t->h * 0.5f);
        dpad_left = DIR_NONE;
    } else {
        dpad_left = dir;
    }
}

static void magnet(const Target *list, int n) {
    float px = input_cursor_x(), py = input_cursor_y();
    for (int i = 0; i < n; i++)
        if (contains(&list[i], px, py)) return;
    const float reach = 14.0f;
    for (int i = 0; i < n; i++) {
        const Target *t = &list[i];
        if (px < t->x - reach || px > t->x + t->w + reach || py < t->y - reach || py > t->y + t->h + reach)
            continue;
        float nx = fminf(fmaxf(px, t->x + 4), t->x + t->w - 4);
        float ny = fminf(fmaxf(py, t->y + 4), t->y + t->h - 4);
        input_glide_cursor(nx, ny);
        return;
    }
}

void ui_begin_frame(void) {
    frame++;
    elapsed += input_delta_seconds();
    int prev = cur;
    cur ^= 1;
    ntargets[cur] = 0;
    prev_hover_id = hover_id;
    hover_id = 0;
    dpad_left = DIR_NONE;
    int dir = input_dpad();
    if (dir != DIR_NONE) snap_dpad(targets[prev], ntargets[prev], dir);
    else if (g_set.magnet && input_stick_idle(8) && !input_stick_idle(9)) magnet(targets[prev], ntargets[prev]);
    input_on = true;
}

void ui_end_frame(void) {
    if (hover_id && hover_id != prev_hover_id) {
        sound_play(SFX_HOVER);
        input_rumble(1);
    }
}

bool ui_hover(UiId id, float x, float y, float w, float h) {
    if (!input_on) return false;
    if (ntargets[cur] < MAX_TARGETS) {
        Target *t = &targets[cur][ntargets[cur]++];
        t->x = x; t->y = y; t->w = w; t->h = h; t->id = id;
    }
    float px = input_cursor_x(), py = input_cursor_y();
    bool over = px >= x && px < x + w && py >= y && py < y + h;
    if (over) hover_id = id;
    return over;
}

bool ui_click(UiId id, float x, float y, float w, float h) {
    if (ui_hover(id, x, y, w, h) && input_pressed(ACT_CLICK)) {
        input_consume(ACT_CLICK);
        input_rumble(3);
        return true;
    }
    return false;
}

float ui_anim(UiId id, float target, float rate) {
    unsigned slot = id % ANIM_SLOTS;
    for (int probe = 0; probe < 8; probe++) {
        Anim *a = &anims[(slot + probe) % ANIM_SLOTS];
        if (a->id == id || a->id == 0 || frame - a->last > 120) {
            if (a->id != id) { a->id = id; a->v = target; }
            a->last = frame;
            if (!g_set.animations) a->v = target;
            else a->v += (target - a->v) * fminf(1, rate * input_delta_seconds() * 60);
            return a->v;
        }
    }
    return target;
}

/* --------------------------------------------------------------- drawing */

void ui_safe_area(float *x, float *y, float *w, float *h) {
    float mx = gfx_width() * g_set.margin / 200.0f, my = 480.0f * g_set.margin / 200.0f;
    *x = mx; *y = my; *w = gfx_width() - 2 * mx; *h = 480.0f - 2 * my;
}

void ui_panel(float x, float y, float w, float h, float r) {
    const Theme *t = theme();
    (void)r;
    gfx_cut_rect(x, y, w, h, 7, t->panel_edge);
    gfx_cut_rect(x+1, y+1, w-2, h-2, 6, t->panel_top);
}

bool ui_button(UiId id, float x, float y, float w, float h, const char *label, float ts, int style) {
    const Theme *t = theme();
    bool over = ui_hover(id, x, y, w, h);
    bool clicked = over && input_pressed(ACT_CLICK);
    if (clicked) { input_consume(ACT_CLICK); input_rumble(3); sound_play(SFX_SELECT); }
    bool selected = style == UIB_ACCENT;
    Color fill = selected ? color_lerp(t->button_top, t->accent, .23f) : t->button_top;
    if (over) fill = color_lerp(fill, t->accent, .24f);
    if (style != UIB_FLAT || over || selected) gfx_cut_rect(x, y, w, h, 4, fill);
    if (over || selected) gfx_rect(x, y+4, 3, h-8, t->accent);
    if (label && *label) {
        float tw = gfx_text_width(FONT_CHROME, ts, label, -1);
        if (tw > w-18) ts *= (w-18)/tw;
        float lh = gfx_line_height(FONT_CHROME, ts);
        gfx_text_centered(FONT_CHROME, ts, x+w*.5f, y+(h-lh)*.5f, t->text, label);
    }
    return clicked;
}

bool ui_icon_button(UiId id, float cx, float cy, float r, int icon, bool enabled) {
    const Theme *t = theme();
    bool over = enabled && ui_hover(id, cx-r, cy-r, r*2, r*2);
    bool clicked = over && input_pressed(ACT_CLICK);
    if (clicked) { input_consume(ACT_CLICK); input_rumble(3); sound_play(SFX_SELECT); }
    gfx_cut_rect(cx-r, cy-r, r*2, r*2, 4, over ? color_lerp(t->button_top, t->accent, .25f) : t->button_top);
    if (over) gfx_rect(cx-r, cy-r+4, 3, r*2-8, t->accent);
    ui_draw_icon(icon, cx, cy, r*1.1f, enabled ? t->text : color_alpha(t->text, .3f));
    return clicked;
}

void ui_draw_icon(int icon, float cx, float cy, float s, Color c) {
    float h = s * 0.5f;
    switch (icon) {
    case ICON_BACK:
    case ICON_LEFT:
        gfx_tri(cx - h * 0.75f, cy, cx, cy - h * 0.6f, cx, cy + h * 0.6f, c);
        if (icon == ICON_BACK) gfx_rect(cx - 1, cy - h * 0.22f, h * 0.7f, h * 0.44f, c);
        break;
    case ICON_FORWARD:
    case ICON_RIGHT:
        gfx_tri(cx + h * 0.75f, cy, cx, cy - h * 0.6f, cx, cy + h * 0.6f, c);
        if (icon == ICON_FORWARD) gfx_rect(cx - h * 0.7f + 1, cy - h * 0.22f, h * 0.7f, h * 0.44f, c);
        break;
    case ICON_RELOAD: {
        float a1 = 1.75f * (float)M_PI;
        gfx_ring(cx, cy, h * 0.6f, h * 0.2f, 0.25f * (float)M_PI, a1, c);
        float ex = cx + cosf(a1) * h * 0.5f, ey = cy + sinf(a1) * h * 0.5f;
        gfx_tri(ex - h * 0.3f, ey - h * 0.15f, ex + h * 0.3f, ey - h * 0.25f, ex + h * 0.05f, ey + h * 0.3f, c);
        break;
    }
    case ICON_HOME:
        gfx_tri(cx - h * 0.75f, cy - h * 0.05f, cx, cy - h * 0.7f, cx + h * 0.75f, cy - h * 0.05f, c);
        gfx_rect(cx - h * 0.5f, cy - h * 0.1f, h, h * 0.65f, c);
        break;
    case ICON_STAR: {
        float px[10], py[10];
        for (int i = 0; i < 10; i++) {
            float a = -(float)M_PI / 2 + i * (float)M_PI / 5;
            float rad = (i & 1) ? h * 0.33f : h * 0.75f;
            px[i] = cx + cosf(a) * rad;
            py[i] = cy + sinf(a) * rad;
        }
        for (int i = 0; i < 10; i++) gfx_tri(cx, cy, px[i], py[i], px[(i + 1) % 10], py[(i + 1) % 10], c);
        break;
    }
    case ICON_GEAR:
        for (int i = 0; i < 8; i++) {
            float a = i * (float)M_PI / 4 + ui_time() * 0.0f;
            gfx_line(cx + cosf(a) * h * 0.35f, cy + sinf(a) * h * 0.35f, cx + cosf(a) * h * 0.75f,
                     cy + sinf(a) * h * 0.75f, h * 0.28f, c);
        }
        gfx_ring(cx, cy, h * 0.55f, h * 0.28f, 0, 2 * (float)M_PI, c);
        break;
    case ICON_STOP:
    case ICON_CLOSE:
        gfx_line(cx - h * 0.5f, cy - h * 0.5f, cx + h * 0.5f, cy + h * 0.5f, h * 0.25f, c);
        gfx_line(cx + h * 0.5f, cy - h * 0.5f, cx - h * 0.5f, cy + h * 0.5f, h * 0.25f, c);
        break;
    case ICON_LOCK:
    case ICON_UNLOCK:
        gfx_rrect(cx - h * 0.55f, cy - h * 0.1f, h * 1.1f, h * 0.8f, h * 0.15f, c);
        if (icon == ICON_LOCK) gfx_ring(cx, cy - h * 0.1f, h * 0.38f, h * 0.15f, (float)M_PI, 2 * (float)M_PI, c);
        else gfx_ring(cx + h * 0.35f, cy - h * 0.25f, h * 0.38f, h * 0.15f, (float)M_PI, 1.9f * (float)M_PI, c);
        break;
    case ICON_PLUS:
        gfx_rect(cx - h * 0.6f, cy - h * 0.12f, h * 1.2f, h * 0.24f, c);
        gfx_rect(cx - h * 0.12f, cy - h * 0.6f, h * 0.24f, h * 1.2f, c);
        break;
    }
}

void ui_draw_background(void) {
    gfx_rect(0, 0, gfx_width(), 480, theme()->bg_top);
}

void ui_spinner(float cx, float cy, float size) {
    const Theme *t = theme();
    float time = ui_time();
    gfx_circle(cx + 2, cy + size * 0.65f, size * 0.45f, color_alpha(t->shadow, 0.35f));
    gfx_cube(cx, cy + sinf(time * 4.0f) * size * 0.08f, size, time * 1.7f, time * 2.3f, t->accent);
}

void ui_draw_cursor(void) {
    const Theme *t = theme();
    float x = input_cursor_x(), y = input_cursor_y();
    float press = ui_anim(0xC0450Bu, input_held(ACT_CLICK) ? 1.0f : 0.0f, 0.4f);
    float hover = ui_anim(0xC0450Cu, hover_id ? 1.0f : 0.0f, 0.2f);
    float bob = 0;
    float s = 1.0f - press * 0.15f;
    y += bob;
    if (hover > 0.01f) {
        float pulse = .5f;
        gfx_circle(x, y, 9 + 3 * pulse, color_alpha(t->accent, 0.45f * hover));
    }
    /* shadow, outline, fill */
    for (int pass = 0; pass < 3; pass++) {
        float ox = pass == 0 ? 3 : 0, oy = pass == 0 ? 4 : 0, grow = pass == 2 ? 0 : 2.2f;
        Color c = pass == 0 ? color_alpha(t->shadow, 0.8f) : (pass == 1 ? t->cursor_edge : t->cursor_fill);
        float bx = x + ox, by = y + oy;
        gfx_tri(bx - grow * 0.6f, by - grow * 1.6f, bx - grow * 0.6f, by + (22 + grow * 1.4f) * s,
                bx + (17 + grow * 1.7f) * s, by + (16 + grow * 0.6f) * s, c);
        gfx_line(bx + 7 * s, by + 15 * s, bx + 12 * s, by + 26 * s, 5 + grow * 1.8f, c);
    }
    gfx_tri(x + 3 * s, y + 5 * s, x + 3 * s, y + 13 * s, x + 8 * s, y + 11 * s, color_alpha(t->accent, 0.55f));
}

void ui_toast(const char *msg) {
    lab_note("toast: %s", msg);
    snprintf(toast_msg, sizeof(toast_msg), "%s", msg);
    toast_until = frame + 150;
}

void ui_draw_toast(void) {
    if (frame >= toast_until || !toast_msg[0]) return;
    const Theme *t = theme();
    float left = (toast_until - frame) / 20.0f;
    float a = fminf(1.0f, left);
    float in = ui_anim(0x70A57u, 1.0f, 0.2f);
    float ts = 18;
    float tw = gfx_text_width(FONT_CHROME, ts, toast_msg, -1);
    float w = tw + 40, h = 40;
    float x = (gfx_width() - w) * 0.5f, y = 400 - in * 20;
    gfx_cut_rect(x, y, w, h, 5, color_alpha(t->accent, a));
    gfx_text(FONT_CHROME, ts, x + 20, y + (h - gfx_line_height(FONT_CHROME, ts)) * 0.5f, color_alpha(t->text_on_accent, a),
             toast_msg, -1);
    if (frame + 1 >= toast_until) toast_msg[0] = 0;
}
