/* storage.c - mounts the SD card through libfat: the SD2SP2 in Serial Port 2,
 * an SD Gecko in memory card slot B or A, or the GC Loader's card. The first
 * card whose STORAGE_DIR holds the certificates wins, so the browser finds the
 * files installed for it; failing that, the first card with the folder, then
 * the first card found. */
#include "storage.h"
#include <fat.h>
#include <ogc/dvd.h>
#include <sdcard/gcsd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static bool mounted;
static const char *device = "none";

bool storage_init(void) {
    struct { const DISC_INTERFACE *io; const char *name; } devs[] = {
        { &__io_gcsd2, "SD2SP2" },
        { &__io_gcsdb, "SD Gecko (slot B)" },
        { &__io_gcsda, "SD Gecko (slot A)" },
        { &__io_gcode, "GC Loader" },
    };
    int best = -1, best_score = -1;
    struct stat st;
    for (unsigned i = 0; i < sizeof(devs) / sizeof(devs[0]); i++) {
        if (!fatMountSimple("sd", devs[i].io)) continue;
        int score = stat(STORAGE_DIR "/cacert.pem", &st) == 0 ? 2 : stat(STORAGE_DIR, &st) == 0;
        if (score == 2) {
            mounted = true;
            device = devs[i].name;
            return true;
        }
        if (score > best_score) { best = i; best_score = score; }
        fatUnmount("sd");
    }
    if (best < 0 || !fatMountSimple("sd", devs[best].io)) return false;
    mounted = true;
    device = devs[best].name;
    mkdir(STORAGE_DIR, 0777);
    return true;
}

bool storage_ok(void) { return mounted; }
const char *storage_device_name(void) { return device; }

char *storage_read(const char *path, size_t *len_out) {
    if (!mounted) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0 || len > 8 * 1024 * 1024) { fclose(f); return NULL; }
    char *buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[got] = 0;
    if (len_out) *len_out = got;
    return buf;
}

/* Write to a temp file, then swap it in, so a power cut never leaves a
 * half-written file in place. settings_load() also checks for the temp file. */
bool storage_write_atomic(const char *path, const char *tmp_path, const void *data, size_t len) {
    if (!mounted) return false;
    FILE *f = fopen(tmp_path, "wb");
    if (!f) return false;
    bool ok = fwrite(data, 1, len, f) == len;
    ok = (fflush(f) == 0) && ok;
    fclose(f);
    if (!ok) { remove(tmp_path); return false; }
    remove(path);
    return rename(tmp_path, path) == 0;
}
