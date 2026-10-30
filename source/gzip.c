#include "gzip.h"
#include "inflate.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const unsigned char *input;
    size_t length, at;
    char *output;
    size_t used, capacity, limit;
} Gzip;

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int read_bytes(void *context, unsigned char *out, int maximum) {
    Gzip *g = context;
    size_t n = g->length - g->at;
    if (n > (size_t)maximum) n = (size_t)maximum;
    memcpy(out, g->input + g->at, n);
    g->at += n;
    return (int)n;
}

static int write_bytes(void *context, const unsigned char *data, size_t n) {
    Gzip *g = context;
    if (n > g->limit - g->used) return -1;
    size_t need = g->used + n + 1;
    if (need > g->capacity) {
        size_t capacity = g->capacity ? g->capacity : 4096;
        while (capacity < need) {
            if (capacity > (g->limit + 1) / 2) { capacity = g->limit + 1; break; }
            capacity *= 2;
        }
        if (capacity > g->limit + 1) capacity = g->limit + 1;
        char *output = realloc(g->output, capacity);
        if (!output) return -1;
        g->output = output;
        g->capacity = capacity;
    }
    memcpy(g->output + g->used, data, n);
    g->used += n;
    g->output[g->used] = 0;
    return 0;
}

bool gzip_decode(const unsigned char *data, size_t length, size_t limit, char **out, size_t *out_length) {
    *out = NULL;
    *out_length = 0;
    if (!data || length < 18 || limit == SIZE_MAX || data[0] != 0x1f || data[1] != 0x8b ||
        data[2] != 8 || (data[3] & 0xe0)) return false;
    size_t at = 10, end = length - 8;
    if (data[3] & 4) {
        if (end - at < 2) return false;
        size_t n = (size_t)data[at] | (size_t)data[at + 1] << 8;
        at += 2;
        if (n > end - at) return false;
        at += n;
    }
    for (unsigned flag = 8; flag <= 16; flag *= 2) {
        if (!(data[3] & flag)) continue;
        while (at < end && data[at]) at++;
        if (at == end) return false;
        at++;
    }
    if (data[3] & 2) {
        if (end - at < 2) return false;
        uint16_t crc = (uint16_t)crc32_update(0, data, at);
        if (data[at] != (crc & 255) || data[at + 1] != (crc >> 8)) return false;
        at += 2;
    }
    if (at >= end || le32(data + length - 4) > limit) return false;
    Gzip g = {.input = data + at, .length = end - at, .limit = limit};
    uint64_t bytes;
    uint32_t crc;
    int result = inflate_stream(read_bytes, &g, write_bytes, &g, &bytes, &crc);
    if (result || bytes != g.used || (uint32_t)bytes != le32(data + length - 4) || crc != le32(data + length - 8)) {
        free(g.output);
        return false;
    }
    if (!g.output) {
        g.output = calloc(1, 1);
        if (!g.output) return false;
    }
    *out = g.output;
    *out_length = g.used;
    return true;
}
