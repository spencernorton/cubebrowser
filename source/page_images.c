#include "page_images.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int page_image_shared(Page *p, int index) {
    PageImage *im = &p->images[index];
    if (!im->url) return -1;
    for (int i = 0; i < p->nimages; i++) {
        PageImage *other = &p->images[i];
        if (i == index || other->state != IMG_READY || !other->url || strcmp(other->url, im->url)) continue;
        int owner = other->owner >= 0 ? other->owner : i;
        im->img = p->images[owner].img;
        im->owner = (short)owner;
        im->bytes = p->images[owner].bytes;
        im->state = IMG_READY;
        if (im->known_w != im->img.w || im->known_h != im->img.h) {
            im->known_w = im->img.w;
            im->known_h = im->img.h;
            if (!(im->decl_w && im->decl_h)) p->relayout = true;
        }
        return owner;
    }
    return -1;
}

bool page_image_accept(Page *p, int index, DecodedImage *image, size_t budget) {
    PageImage *im = &p->images[index];
    bool changed = im->known_w != image->w || im->known_h != image->h;
    im->known_w = image->w;
    im->known_h = image->h;
    im->bytes = image->bytes;
    if (changed && !(im->decl_w && im->decl_h)) p->relayout = true;
    if (image->bytes > budget || p->image_bytes > budget - image->bytes) {
        image_free(image);
        im->state = IMG_DEFERRED;
        return false;
    }
    gfx_image_init(&im->img, image->tex, image->w, image->h, image->tex_w, image->tex_h);
    image->tex = NULL;   /* ownership moved to the page */
    im->state = IMG_READY;
    im->owner = (short)index;
    p->image_bytes += image->bytes;
    for (int i = 0; i < p->nimages; i++) {
        if (i != index && (p->images[i].state == IMG_NONE || p->images[i].state == IMG_DEFERRED))
            page_image_shared(p, i);
    }
    return true;
}

static void drop_owner(Page *p, int owner) {
    PageImage *im = &p->images[owner];
    gfx_image_free(&im->img);
    p->image_bytes = p->image_bytes >= im->bytes ? p->image_bytes - im->bytes : 0;
    for (int i = 0; i < p->nimages; i++) {
        PageImage *copy = &p->images[i];
        if (i != owner && copy->owner != owner) continue;
        memset(&copy->img, 0, sizeof(copy->img));
        copy->owner = -1;
        copy->state = IMG_NONE;
    }
}

void page_images_evict(Page *p, float top, float height) {
    if (!isfinite(top) || !isfinite(height) || height <= 0) return;
    unsigned char near[PAGE_MAX_IMAGES] = {0};
    const int *visible = NULL;
    float from = top - 3 * height, extent = 9 * height;
    int count = p->viewport.ready ? viewport_query(&p->viewport, from, extent, &visible) : p->nboxes;
    for (int vi = 0; vi < count; vi++) {
        const Box *b = &p->boxes[visible ? visible[vi] : vi];
        if (b->type != BOX_IMAGE || b->text < 0 || b->text >= p->nimages ||
            b->y + b->h < from || b->y > from + extent) continue;
        const PageImage *im = &p->images[b->text];
        near[im->owner >= 0 ? im->owner : b->text] = 1;
    }
    for (int i = 0; i < p->nimages; i++) {
        const PageImage *im = &p->images[i];
        if (im->state == IMG_READY && (im->owner < 0 || im->owner == i) && !near[i]) drop_owner(p, i);
    }
}

void page_images_dispose(Page *p) {
    for (int i = 0; i < p->nimages; i++) {
        PageImage *im = &p->images[i];
        free(im->url);
        if (im->owner < 0 || im->owner == i) gfx_image_free(&im->img);
        memset(im, 0, sizeof(*im));
    }
    p->nimages = 0;
    p->image_bytes = 0;
}
