#pragma once
#include <stdbool.h>
#include <stddef.h>

/* Decode one HTTP gzip member, checking header/trailer and an output ceiling.
 * On failure out remains NULL. No partial or unchecked output is published. */
bool gzip_decode(const unsigned char *data, size_t length, size_t limit, char **out, size_t *out_length);
