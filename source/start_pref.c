#include "start_pref.h"
#include "storage.h"
#include "url.h"
#include <ctype.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define PREF_PATH STORAGE_DIR "/start.pref"
#define PREF_TEMP STORAGE_DIR "/start.tmp"
#define MAX_AGE 31536000u
#define EARLIEST_CLOCK 1577836800u
static char zip[6];
static uint64_t expires;
static _Atomic int notice;

static bool valid_zip(const char *value) {
    if (strlen(value) != 5) return false;
    for (int i = 0; i < 5; i++) if (value[i] < '0' || value[i] > '9') return false;
    return true;
}
static bool decimal(const char *value, uint64_t maximum, uint64_t *number) {
    if (!*value) return false;
    uint64_t n = 0;
    for (; *value; value++) {
        if (*value < '0' || *value > '9') return false;
        unsigned digit = (unsigned)(*value - '0');
        if (digit > maximum || n > (maximum - digit) / 10) return false;
        n = n * 10 + digit;
    }
    *number = n;
    return true;
}
bool start_pref_scope(const char *url) {
    Url target;
    if (!url || !url_parse(url, &target) || strcmp(target.scheme, "https") ||
        strcasecmp(target.host, "norvitech.com") || target.port != 443 ||
        strncmp(target.path, "/indigo/start/", 14)) return false;
    char *query = strpbrk(target.path, "?#");
    if (query) *query = 0;
    /* Keep the path scope strict even if a server normalizes escaped segments. */
    for (const char *p = target.path; *p; p++) {
        if (*p == '\\' || (*p == '%' && (!strncasecmp(p, "%2e", 3) ||
            !strncasecmp(p, "%2f", 3) || !strncasecmp(p, "%5c", 3)))) return false;
        if (*p == '/' && p[1] == '.' && (!p[2] || p[2] == '/' || (p[2] == '.' && (!p[3] || p[3] == '/')))) return false;
    }
    return true;
}
void start_pref_load(void) {
    zip[0] = 0;
    expires = 0;
    notice = START_PREF_NONE;
    size_t length;
    char *data = storage_read_limit(PREF_PATH, 64, &length);
    if (!data) data = storage_read_limit(PREF_TEMP, 64, &length);
    if (!data) return;
    /* Exact versioned card format, with no trailing records or embedded NUL. */
    if (length >= 11 && strlen(data) == length && !strncmp(data, "v1\n", 3) && data[8] == '\n' && data[length-1] == '\n') {
        char candidate[6];
        memcpy(candidate, data+3, 5); candidate[5] = 0;
        data[length-1] = 0;
        uint64_t until;
        if (valid_zip(candidate) && decimal(data+9, UINT64_MAX, &until)) {
            strcpy(zip, candidate);
            expires = until;
        }
    }
    free(data);
}
void start_pref_request(const char *url, bool verified, uint64_t now, char *out, size_t capacity) {
    if (!capacity) return;
    out[0] = 0;
    if (!verified || !start_pref_scope(url) || !zip[0] || now < EARLIEST_CLOCK ||
        expires <= now || expires - now > MAX_AGE) return;
    snprintf(out, capacity, "Cookie: indigo_zip=%s\r\n", zip);
}
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) *--end = 0;
    return s;
}
bool start_pref_receive(const char *url, bool verified, const char *header, uint64_t now) {
    if (!verified || !start_pref_scope(url) || !header || now < EARLIEST_CLOCK || now > UINT64_MAX - MAX_AGE) return false;
    size_t length = strnlen(header, 1024);
    if (!length || length >= 1024) return false;
    for (size_t i = 0; i < length; i++)
        if (((unsigned char)header[i] < 32 && header[i] != '\t') || header[i] == 127) return false;
    char buffer[1024];
    memcpy(buffer, header, length + 1);
    char *save, *part = strtok_r(buffer, ";", &save);
    if (!part || strncmp(trim(part), "indigo_zip=", 11)) return false;
    char *value = trim(part) + 11;
    size_t value_length = strlen(value);
    if (value_length >= 2 && value[0] == '"' && value[value_length-1] == '"') {
        value[value_length-1] = 0;
        value++;
    }
    if (*value && !valid_zip(value)) return false;
    char candidate[6];
    snprintf(candidate, sizeof(candidate), "%s", value);
    unsigned seen = 0;
    uint64_t age = 0;
    while ((part = strtok_r(NULL, ";", &save))) {
        char *key = trim(part), *attribute = strchr(key, '=');
        if (attribute) { *attribute++ = 0; key = trim(key); attribute = trim(attribute); }
        unsigned flag = 0;
        if (!strcasecmp(key, "Domain")) return false; /* host-only */
        if (!strcasecmp(key, "Secure")) { if (attribute) return false; flag = 1; }
        else if (!strcasecmp(key, "HttpOnly")) { if (attribute) return false; flag = 2; }
        else if (!strcasecmp(key, "Path")) {
            if (!attribute || strcmp(attribute, "/indigo/start/")) return false;
            flag = 4;
        } else if (!strcasecmp(key, "SameSite")) {
            if (!attribute || strcasecmp(attribute, "Lax")) return false;
            flag = 8;
        } else if (!strcasecmp(key, "Max-Age")) {
            if (!attribute || !decimal(attribute, MAX_AGE, &age)) return false;
            flag = 16;
        }
        if (flag && (seen & flag)) return false;
        seen |= flag;
    }
    if (seen != 31 || (age && !candidate[0])) return false;
    if (!age) candidate[0] = 0;
    strcpy(zip, candidate);
    expires = age ? now + age : 0;
    char record[64];
    int count = zip[0] ? snprintf(record, sizeof(record), "v1\n%s\n%llu\n", zip, (unsigned long long)expires) : 0;
    bool saved = storage_write_atomic(PREF_PATH, PREF_TEMP, record, (size_t)count);
    notice = saved ? (age ? START_PREF_SAVED : START_PREF_CLEARED) : START_PREF_UNSAVED;
    return true;
}
int start_pref_take_notice(void) { return atomic_exchange(&notice, START_PREF_NONE); }
