/* Fuzz the CSS parser (selectors, declarations, the cascade's var() text) on
 * untrusted stylesheet bytes. Exercises parse_decls and parse_compound, the
 * two memory-safety fixes. Build+run: ./run.sh. */
#include <stddef.h>
#include <stdint.h>
#include "css.h"
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t len) {
    CssSheet *s = css_parse((const char *)data, (int)len);
    if (s) css_free(s);
    return 0;
}
