#include "viewport.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SHORT_HEIGHT 400.0f

static int compare_top(const void *a, const void *b) {
    const ViewSpan *x = a, *y = b;
    if (x->top != y->top) return (x->top > y->top) - (x->top < y->top);
    return (x->box > y->box) - (x->box < y->box);
}

static int compare_order(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

bool viewport_build(Viewport *v, int count, ViewportBounds bounds, const void *context) {
    v->ready = v->cached = false;
    v->nshort = v->ntall = v->nvisible = 0;
    if (count < 0 || !bounds) return false;
    if (count > v->capacity) {
        if (count > INT_MAX - 256 || (size_t)(count + 256) > SIZE_MAX / sizeof(ViewSpan)) return false;
        int capacity = count + 256;
        ViewSpan *spans = malloc((size_t)capacity * sizeof(*spans));
        int *visible = malloc((size_t)capacity * sizeof(*visible));
        if (!spans || !visible) { free(spans); free(visible); return false; }
        free(v->spans);
        free(v->visible);
        v->spans = spans;
        v->visible = visible;
        v->capacity = capacity;
    }
    for (int i = 0; i < count; i++) {
        float top, height;
        bounds(context, i, &top, &height);
        if (!isfinite(top) || !isfinite(height) || height < 0 || !isfinite(top + height)) continue;
        ViewSpan s = {top, top + height, i};
        if (height <= SHORT_HEIGHT) v->spans[v->nshort++] = s;
        else v->spans[v->capacity - ++v->ntall] = s;
    }
    if (v->nshort > 1) qsort(v->spans, (size_t)v->nshort, sizeof(*v->spans), compare_top);
    v->ready = true;
    return true;
}

int viewport_query(Viewport *v, float top, float height, const int **indices) {
    *indices = v->visible;
    if (!v->ready || !isfinite(top) || !isfinite(height) || height < 0 || !isfinite(top + height)) return 0;
    if (v->cached && top == v->last_top && height == v->last_height) return v->nvisible;
    int lo = 0, hi = v->nshort;
    float from = top - SHORT_HEIGHT, bottom = top + height;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (v->spans[mid].top < from) lo = mid + 1;
        else hi = mid;
    }
    int n = 0;
    v->inspected = 0;
    for (int i = lo; i < v->nshort; i++) {
        const ViewSpan *s = &v->spans[i];
        v->inspected++;
        if (s->top > bottom) break;
        if (s->bottom >= top) v->visible[n++] = s->box;
    }
    for (int i = v->capacity - v->ntall; i < v->capacity; i++) {
        const ViewSpan *s = &v->spans[i];
        v->inspected++;
        if (s->bottom >= top && s->top <= bottom) v->visible[n++] = s->box;
    }
    if (n > 1) qsort(v->visible, (size_t)n, sizeof(*v->visible), compare_order);
    v->nvisible = n;
    v->last_top = top;
    v->last_height = height;
    v->cached = true;
    return n;
}

void viewport_free(Viewport *v) {
    free(v->spans);
    free(v->visible);
    memset(v, 0, sizeof(*v));
}
