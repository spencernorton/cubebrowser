/* theme.c - Indigo, Jet Black, Platinum and Spice colour schemes. */
#include "theme.h"
#include "launch.h"

#define C(r, g, b, a) { r, g, b, a }

const char *const theme_names[THEME_COUNT] = { "Indigo", "Jet Black", "Platinum", "Spice" };

static const Theme themes[THEME_COUNT] = {
    { /* Indigo */
        .name = "Indigo",
        .bg_top = C(58, 40, 140, 255), .bg_bottom = C(20, 12, 62, 255), .bg_cube = C(120, 100, 230, 255),
        .panel_top = C(150, 135, 235, 215), .panel_bottom = C(98, 82, 196, 215), .panel_edge = C(220, 212, 255, 160),
        .button_top = C(178, 166, 250, 255), .button_bottom = C(118, 100, 220, 255),
        .accent = C(255, 214, 92, 255), .accent_hi = C(255, 240, 170, 255),
        .text = C(255, 255, 255, 255), .text_dim = C(214, 206, 250, 255), .text_on_accent = C(60, 36, 120, 255),
        .field_bg = C(246, 244, 255, 255), .field_text = C(40, 30, 90, 255),
        .page_bg = C(250, 249, 255, 255), .page_text = C(30, 28, 50, 255), .page_dim = C(110, 104, 140, 255),
        .page_link = C(70, 52, 200, 255), .page_link_hover = C(214, 120, 20, 255),
        .page_heading = C(52, 34, 140, 255), .page_rule = C(200, 194, 230, 255),
        .cursor_fill = C(255, 255, 255, 255), .cursor_edge = C(70, 50, 160, 255),
        .shadow = C(8, 4, 30, 150), .good = C(90, 210, 120, 255), .bad = C(250, 100, 100, 255),
    },
    { /* Jet Black */
        .name = "Jet Black",
        .bg_top = C(40, 40, 46, 255), .bg_bottom = C(6, 6, 9, 255), .bg_cube = C(90, 90, 104, 255),
        .panel_top = C(78, 78, 88, 220), .panel_bottom = C(40, 40, 48, 220), .panel_edge = C(170, 170, 185, 140),
        .button_top = C(100, 100, 112, 255), .button_bottom = C(52, 52, 62, 255),
        .accent = C(96, 180, 255, 255), .accent_hi = C(180, 220, 255, 255),
        .text = C(240, 240, 245, 255), .text_dim = C(170, 170, 182, 255), .text_on_accent = C(10, 30, 60, 255),
        .field_bg = C(230, 230, 236, 255), .field_text = C(20, 20, 28, 255),
        .page_bg = C(28, 28, 32, 255), .page_text = C(225, 225, 232, 255), .page_dim = C(150, 150, 162, 255),
        .page_link = C(120, 190, 255, 255), .page_link_hover = C(255, 200, 100, 255),
        .page_heading = C(255, 255, 255, 255), .page_rule = C(70, 70, 80, 255),
        .cursor_fill = C(255, 255, 255, 255), .cursor_edge = C(20, 60, 120, 255),
        .shadow = C(0, 0, 0, 170), .good = C(90, 210, 120, 255), .bad = C(250, 110, 110, 255),
    },
    { /* Platinum */
        .name = "Platinum",
        .bg_top = C(214, 218, 226, 255), .bg_bottom = C(150, 156, 168, 255), .bg_cube = C(250, 252, 255, 255),
        .panel_top = C(250, 251, 253, 225), .panel_bottom = C(212, 216, 224, 225), .panel_edge = C(255, 255, 255, 200),
        .button_top = C(255, 255, 255, 255), .button_bottom = C(200, 205, 215, 255),
        .accent = C(60, 110, 216, 255), .accent_hi = C(120, 160, 240, 255),
        .text = C(40, 44, 56, 255), .text_dim = C(100, 106, 120, 255), .text_on_accent = C(255, 255, 255, 255),
        .field_bg = C(255, 255, 255, 255), .field_text = C(30, 32, 40, 255),
        .page_bg = C(255, 255, 255, 255), .page_text = C(28, 30, 36, 255), .page_dim = C(110, 114, 124, 255),
        .page_link = C(36, 84, 200, 255), .page_link_hover = C(200, 60, 40, 255),
        .page_heading = C(20, 40, 100, 255), .page_rule = C(210, 214, 222, 255),
        .cursor_fill = C(255, 255, 255, 255), .cursor_edge = C(40, 70, 150, 255),
        .shadow = C(40, 46, 60, 110), .good = C(40, 160, 80, 255), .bad = C(210, 60, 60, 255),
    },
    { /* Spice */
        .name = "Spice",
        .bg_top = C(246, 150, 52, 255), .bg_bottom = C(170, 66, 14, 255), .bg_cube = C(255, 200, 120, 255),
        .panel_top = C(255, 236, 206, 225), .panel_bottom = C(246, 196, 140, 225), .panel_edge = C(255, 250, 236, 200),
        .button_top = C(255, 246, 228, 255), .button_bottom = C(240, 186, 120, 255),
        .accent = C(120, 50, 14, 255), .accent_hi = C(170, 90, 40, 255),
        .text = C(80, 34, 8, 255), .text_dim = C(140, 80, 40, 255), .text_on_accent = C(255, 240, 220, 255),
        .field_bg = C(255, 252, 245, 255), .field_text = C(70, 30, 6, 255),
        .page_bg = C(255, 251, 244, 255), .page_text = C(50, 30, 16, 255), .page_dim = C(130, 100, 80, 255),
        .page_link = C(180, 70, 10, 255), .page_link_hover = C(40, 100, 200, 255),
        .page_heading = C(120, 44, 8, 255), .page_rule = C(236, 210, 180, 255),
        .cursor_fill = C(255, 255, 255, 255), .cursor_edge = C(140, 60, 10, 255),
        .shadow = C(90, 30, 0, 120), .good = C(60, 150, 60, 255), .bad = C(200, 40, 30, 255),
    },
};

static Theme current;
const Theme *theme(void) { return &current; }
void theme_set(int idx) {
    if (idx < 0 || idx >= THEME_COUNT) idx = THEME_INDIGO;
    current = themes[idx];
    static const Color accents[] = {
        C(151, 145, 225, 255), C(108, 175, 231, 255), C(105, 197, 153, 255), C(222, 193, 108, 255),
        C(231, 159, 104, 255), C(220, 112, 124, 255), C(220, 139, 188, 255), C(178, 180, 193, 255)
    };
    int color = g_launch.active && g_launch.color >= 0 ? g_launch.color :
        (idx == THEME_JET ? 7 : idx == THEME_SPICE ? 4 : 0);
    current.bg_top = current.bg_bottom = rgba(8, 12, 27, 255);
    current.panel_top = current.panel_bottom = rgba(20, 25, 43, 255);
    current.panel_edge = rgba(66, 72, 97, 255);
    current.button_top = current.button_bottom = rgba(30, 36, 57, 255);
    current.accent = accents[color];
    current.accent_hi = color_lerp(current.accent, rgba(255,255,255,255), .15f);
    current.text = rgba(239, 240, 247, 255);
    current.text_dim = rgba(168, 176, 199, 255);
    current.text_on_accent = rgba(10, 13, 27, 255);
    current.field_bg = rgba(10, 15, 31, 255);
    current.field_text = current.text;
    current.cursor_fill = current.text;
    current.cursor_edge = rgba(30, 25, 67, 255);
}
