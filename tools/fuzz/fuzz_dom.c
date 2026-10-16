/* Fuzz the HTML tokenizer/tree on untrusted bytes (parse as a document, then
 * as plaintext). Build+run: ./run.sh (inside the libogc2 toolchain image). */
#include <stddef.h>
#include <stdint.h>
#include "dom.h"
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t len) {
    Dom *d = dom_parse((const char *)data, len, false);
    if (d) dom_free(d);
    d = dom_parse((const char *)data, len, true);
    if (d) dom_free(d);
    return 0;
}
