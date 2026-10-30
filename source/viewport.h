/* A layout-owned vertical index. Querying a frame never allocates and returns
 * original painter order. Long backgrounds are kept outside the short spans. */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct { float top, bottom; int box; } ViewSpan;
typedef struct {
    ViewSpan *spans;
    int *visible;
    int capacity, nshort, ntall, nvisible;
    float last_top, last_height;
    bool ready, cached;
    size_t inspected;   /* work performed by the last uncached query */
} Viewport;

typedef void (*ViewportBounds)(const void *context, int box, float *top, float *height);
bool viewport_build(Viewport *view, int count, ViewportBounds bounds, const void *context);
int viewport_query(Viewport *view, float top, float height, const int **indices);
void viewport_free(Viewport *view);
