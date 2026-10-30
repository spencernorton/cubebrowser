/* Run one exact saved input without requiring the libFuzzer runtime. Keep the
 * allocation unpadded: ASan must see reads past the parser's supplied length. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t len);

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *file = fopen(argv[1], "rb");
    if (!file) return 2;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 2; }
    long size = ftell(file);
    if (size < 0 || size > 16 * 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 2;
    }
    uint8_t *data = malloc(size ? (size_t)size : 1);
    if (!data) { fclose(file); return 2; }
    size_t read = fread(data, 1, (size_t)size, file);
    int close_result = fclose(file);
    if (read != (size_t)size || close_result != 0) { free(data); return 2; }
    LLVMFuzzerTestOneInput(data, read);
    free(data);
    return 0;
}
