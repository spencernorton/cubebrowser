#include "viewport.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { float y, h; } Bounds;
static void bounds(const void *context, int i, float *top, float *height) {
    const Bounds *boxes = context;
    *top = boxes[i].y;
    *height = boxes[i].h;
}
static uint32_t rng = 42;
static uint32_t next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

static void compare(Viewport *v, const Bounds *boxes, int count, float top, float height) {
    const int *indices;
    int n = viewport_query(v, top, height, &indices), at = 0;
    for (int i = 0; i < count; i++) {
        Bounds b = boxes[i];
        if (!isfinite(b.y) || !isfinite(b.h) || b.h < 0 || !isfinite(b.y + b.h)) continue;
        if (b.y + b.h >= top && b.y <= top + height) {
            assert(at < n);
            assert(indices[at++] == i);   /* tall and overlapping boxes keep painter order */
        }
    }
    assert(at == n);
    const int *again;
    size_t inspected = v->inspected;
    assert(viewport_query(v, top, height, &again) == n);
    assert(again == indices && v->inspected == inspected);
}

int main(void) {
    enum { N = 20000 };
    Bounds *boxes = malloc(N * sizeof(*boxes));
    assert(boxes);
    Viewport v = {0};
    for (int i = 0; i < N; i++) boxes[i] = (Bounds){(float)(next() % 600000) - 3000, (float)(next() % 900)};
    boxes[0] = (Bounds){0, 700000};
    boxes[1] = (Bounds){NAN, 20};
    boxes[2] = (Bounds){0, INFINITY};
    boxes[3] = (Bounds){30, -1};
    boxes[4] = (Bounds){300, 0};
    assert(viewport_build(&v, N, bounds, boxes));
    for (int i = 0; i < 1200; i++) compare(&v, boxes, N, (float)(next() % 600000) - 4000, (float)(next() % 1000));
    for (int i = 0; i < N; i++) boxes[i] = (Bounds){i * 24.0f, 22};
    assert(viewport_build(&v, N, bounds, boxes));
    compare(&v, boxes, N, 240000, 400);
    assert(v.inspected < 40);    /* a long regular page no longer scans 20,000 boxes */
    const int *indices;
    assert(viewport_query(&v, NAN, 400, &indices) == 0);
    assert(viewport_query(&v, 0, -1, &indices) == 0);
    assert(viewport_build(&v, 0, bounds, boxes));
    assert(viewport_query(&v, 0, 480, &indices) == 0);
    viewport_free(&v);
    free(boxes);
    puts("PASS: visible boxes match full scans; painter order, boundaries, rebuild and bounded regular-page work");
}
