/* A paint trace and stable font metrics for engine tests. The production page
 * parser/layout/draw code is linked unchanged; no console/GX is needed. */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "gfx.h"
#include "image.h"
#include "settings.h"
#include "theme.h"
#include <stdlib.h>
#include <string.h>

Settings g_set = {.page_styles = 1, .load_images = 1, .show_alt = 1};
static Theme colors;
char paint_trace[1024];
int paint_count;
static void paint(char kind) { if (paint_count < 1023) { paint_trace[paint_count++] = kind; paint_trace[paint_count] = 0; } }
const Theme *theme(void) { return &colors; }
Color color_alpha(Color c, float n) { c.a = (u8)(c.a * (n < 0 ? 0 : n > 1 ? 1 : n)); return c; }
float settings_max_picture_height(void) { return 640; }
const char *settings_search_prefix(void) { return "https://example.org/?q="; }
float gfx_line_height(FontId f, float size) { return size * 1.2f; }
float gfx_text_width(FontId f, float size, const char *s, int n) { return (n < 0 ? strlen(s) : (size_t)n) * size * 0.5f; }
int gfx_text_fit(FontId f, float size, const char *s, int n, float width) {
    int length = n < 0 ? (int)strlen(s) : n, fits = (int)(width / (size * 0.5f));
    return fits < length ? fits : length;
}
float gfx_text(FontId f, float size, float x, float y, Color c, const char *s, int n) { paint('T'); return gfx_text_width(f, size, s, n); }
void gfx_set_clip(float x, float y, float w, float h) {}
void gfx_clear_clip(void) {}
void gfx_rect(float x, float y, float w, float h, Color c) { paint('R'); }
void gfx_rrect(float x, float y, float w, float h, float r, Color c) { paint('R'); }
void gfx_rrect_outline(float x, float y, float w, float h, float r, float t, Color c) { paint('R'); }
void gfx_rrect_vgrad(float x, float y, float w, float h, float r, Color a, Color b) { paint('R'); }
void gfx_circle(float x, float y, float r, Color c) { paint('R'); }
void gfx_ring(float x, float y, float r, float t, float a, float b, Color c) { paint('R'); }
void gfx_tri(float x, float y, float a, float b, float c, float d, Color col) { paint('R'); }
void gfx_line(float x, float y, float a, float b, float t, Color c) { paint('R'); }
void gfx_image_draw(GfxImage *im, float x, float y, float w, float h, Color c) { paint('I'); }
void gfx_image_init(GfxImage *im, void *data, int w, int h, int tw, int th) {
    memset(im, 0, sizeof(*im)); im->data = data; im->w = w; im->h = h; im->tex_w = tw; im->tex_h = th;
}
void gfx_image_free(GfxImage *im) { free(im->data); im->data = NULL; }
void image_free(DecodedImage *im) { free(im->tex); im->tex = NULL; }
