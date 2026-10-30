#include "gzip.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    unsigned char input[4096];
    size_t length = fread(input, 1, sizeof(input), f);
    assert(feof(f));
    fclose(f);
    char *out = NULL;
    size_t n;
    assert(gzip_decode(input, length, 20000, &out, &n));
    assert(n == 14000 && !out[n]);
    for (size_t i = 0; i < n; i += 14) assert(!memcmp(out + i, "browser page!\n", 14));
    free(out);
    assert(!gzip_decode(input, length, 13999, &out, &n) && !out);
    for (size_t cut = 0; cut < length; cut++) assert(!gzip_decode(input, cut, 20000, &out, &n) && !out);
    input[length - 8] ^= 1;
    assert(!gzip_decode(input, length, 20000, &out, &n) && !out);
    input[length - 8] ^= 1;
    input[3] |= 0xe0;
    assert(!gzip_decode(input, length, 20000, &out, &n));
    puts("PASS: bounded gzip output, exact bytes, every truncation, checksum and reserved flags");
}
