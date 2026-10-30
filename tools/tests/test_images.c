#include "page_images.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int textures_freed, results_freed;
void gfx_image_init(GfxImage *im, void *data, int w, int h, int tw, int th) {
    memset(im, 0, sizeof(*im));
    im->data = data; im->w = w; im->h = h; im->tex_w = tw; im->tex_h = th;
}
void gfx_image_free(GfxImage *im) { if (im->data) { textures_freed++; free(im->data); im->data = NULL; } }
void image_free(DecodedImage *im) { if (im->tex) { results_freed++; free(im->tex); im->tex = NULL; } }
static DecodedImage picture(void) {
    DecodedImage im = {0};
    im.tex = malloc(1024);
    assert(im.tex);
    im.w = im.tex_w = 16; im.h = im.tex_h = 32; im.bytes = 1024;
    return im;
}

int main(void) {
    Page p = {0};
    Box boxes[3] = {0};
    p.nimages = p.nboxes = 3;
    p.boxes = boxes;
    for (int i = 0; i < 3; i++) {
        p.images[i].owner = -1;
        p.images[i].url = strdup(i == 2 ? "https://example.org/b.png" : "https://example.org/a.png");
        boxes[i].type = BOX_IMAGE; boxes[i].text = i; boxes[i].h = 32;
    }
    boxes[0].y = 0; boxes[1].y = 5000; boxes[2].y = 15000;
    DecodedImage a = picture();
    assert(page_image_accept(&p, 0, &a, 1024));
    assert(!a.tex && p.image_bytes == 1024);
    assert(p.images[1].state == IMG_READY && p.images[1].owner == 0);
    assert(p.images[0].img.data == p.images[1].img.data);
    DecodedImage b = picture();
    assert(!page_image_accept(&p, 2, &b, 1024));
    assert(!b.tex && results_freed == 1 && p.images[2].state == IMG_DEFERRED);
    p.relayout = false;
    page_images_evict(&p, 5000, 300);
    assert(p.image_bytes == 1024 && textures_freed == 0);  /* its second occurrence is visible */
    page_images_evict(&p, 15000, 300);
    assert(p.image_bytes == 0 && textures_freed == 1);
    assert(p.images[0].state == IMG_NONE && p.images[1].state == IMG_NONE);
    assert(!p.relayout && p.images[0].known_w == 16 && p.images[1].known_h == 32);
    b = picture();
    assert(page_image_accept(&p, 2, &b, 1024));  /* the deferred picture can load after reclamation */
    assert(!p.relayout && p.image_bytes == 1024);
    page_images_dispose(&p);
    assert(textures_freed == 2 && p.image_bytes == 0 && p.nimages == 0);
    puts("PASS: one owner per URL, visible aliases retained, bounded texture budget and retry after reclamation");
}
