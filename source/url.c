/* url.c - URL handling. */
#include "url.h"
#include "settings.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

bool url_is_local(const char *url) {
    Url u;
    if (!url || !url_parse(url, &u) || !u.host[0]) return false;
    size_t n = strlen(u.host);
    if (!strcasecmp(u.host, "localhost") ||
        (n > 6 && !strcasecmp(u.host + n - 6, ".local")) ||
        (n > 10 && !strcasecmp(u.host + n - 10, ".localhost"))) return true;
    unsigned octets[4] = {0}, part = 0;
    const char *p = u.host;
    for (; part < 4; part++) {
        if (!isdigit((unsigned char)*p)) break;
        while (isdigit((unsigned char)*p)) {
            octets[part] = octets[part] * 10 + (unsigned)(*p++ - '0');
            if (octets[part] > 255) return false;
        }
        if (part == 3 && !*p) {
            unsigned a = octets[0], b = octets[1];
            return a == 10 || a == 127 || (a == 192 && b == 168) ||
                   (a == 172 && b >= 16 && b <= 31) || (a == 169 && b == 254);
        }
        if (*p++ != '.') break;
    }
    return strchr(u.host, '.') == NULL;   /* an unqualified LAN host name */
}

static int hex_digit(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (unsigned char)tolower(c);
    return c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

static bool redirect_parameter(const char *query, const char *name, char *out, size_t capacity) {
    size_t key = strlen(name);
    for (const char *p = query; *p;) {
        if (*p == '?' || *p == '&') p++;
        const char *end = p + strcspn(p, "&#");
        if ((size_t)(end - p) > key && !memcmp(p, name, key) && p[key] == '=') {
            size_t used = 0;
            for (p += key + 1; p < end; p++) {
                unsigned char c = (unsigned char)*p;
                if (c == '%' && end - p >= 3) {
                    int a = hex_digit((unsigned char)p[1]), b = hex_digit((unsigned char)p[2]);
                    if (a < 0 || b < 0) return false;
                    c = (unsigned char)(a * 16 + b);
                    p += 2;
                } else if (c == '%') return false;
                if (!c || c < 32 || c == 127 || used + 1 >= capacity) return false;
                out[used++] = (char)c;
            }
            if (!used) return false;
            out[used] = 0;
            return true;
        }
        if (*end != '&') break;
        p = end;
    }
    return false;
}

bool url_unwrap_redirect(const char *url, char *out, size_t capacity) {
    Url parsed;
    if (!url_parse(url, &parsed)) return false;
    const char *query = strchr(parsed.path, '?');
    if (!query) return false;
    bool found = false;
    if ((!strcasecmp(parsed.host, "duckduckgo.com") || !strcasecmp(parsed.host, "html.duckduckgo.com") ||
         !strcasecmp(parsed.host, "lite.duckduckgo.com")) && !strncmp(parsed.path, "/l/", 3))
        found = redirect_parameter(query, "uddg", out, capacity);
    else if (!strcasecmp(parsed.host, "www.google.com") && !strncmp(parsed.path, "/url?", 5))
        found = redirect_parameter(query, "q", out, capacity) || redirect_parameter(query, "url", out, capacity);
    Url target;
    return found && url_parse(out, &target) && (!strcmp(target.scheme, "http") || !strcmp(target.scheme, "https"));
}

/* Percent-encodes bytes that can't appear raw in a request line: spaces,
 * control characters, quotes and other unsafe ASCII, and every non-ASCII
 * byte (UTF-8 file names). Existing %XX escapes are left alone. */
void url_escape_path(char *path, size_t cap) {
    static const char hex[] = "0123456789ABCDEF";
    char out[URL_MAX];
    size_t o = 0;
    for (const unsigned char *p = (const unsigned char *)path; *p && o + 4 < sizeof(out); p++) {
        unsigned char c = *p;
        if (c <= 0x20 || c >= 0x7F || strchr("\"<>\\^`{|}", c)) {
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 15];
        } else {
            out[o++] = (char)c;
        }
    }
    out[o] = 0;
    snprintf(path, cap, "%s", out);
}

static void copy_lower(char *dst, const char *src, size_t n, size_t cap) {
    if (n >= cap) n = cap - 1;
    for (size_t i = 0; i < n; i++) dst[i] = (char)tolower((unsigned char)src[i]);
    dst[n] = 0;
}

static void strip_fragment(char *s) {
    char *h = strchr(s, '#');
    if (h) *h = 0;
}

bool url_parse(const char *s, Url *u) {
    memset(u, 0, sizeof(*u));
    while (isspace((unsigned char)*s)) s++;
    if (strncasecmp(s, "about:", 6) == 0) {
        strcpy(u->scheme, "about");
        snprintf(u->path, sizeof(u->path), "%s", s + 6);
        return true;
    }
    const char *sep = strstr(s, "://");
    if (!sep) return false;
    copy_lower(u->scheme, s, (size_t)(sep - s), sizeof(u->scheme));
    if (strcmp(u->scheme, "http") && strcmp(u->scheme, "https")) return false;
    u->port = strcmp(u->scheme, "https") == 0 ? 443 : 80;
    const char *h = sep + 3;
    const char *end = h + strcspn(h, "/?#");
    const char *at = memchr(h, '@', (size_t)(end - h));
    if (at) h = at + 1;                       /* drop user:pass@ */
    const char *colon = memchr(h, ':', (size_t)(end - h));
    const char *host_end = colon ? colon : end;
    if (host_end == h) return false;
    copy_lower(u->host, h, (size_t)(host_end - h), sizeof(u->host));
    if (colon) {
        int p = atoi(colon + 1);
        if (p <= 0 || p > 65535) return false;
        u->port = (unsigned short)p;
    }
    if (*end == '/' || *end == '?') {
        if (*end == '?') snprintf(u->path, sizeof(u->path), "/%s", end);
        else snprintf(u->path, sizeof(u->path), "%s", end);
    } else {
        strcpy(u->path, "/");
    }
    strip_fragment(u->path);
    url_escape_path(u->path, sizeof(u->path));
    return true;
}

/* Remove "." and ".." segments from the path part (before any '?'). */
static void normalize_path(char *path) {
    char *q = strchr(path, '?');
    char query[URL_MAX] = "";
    if (q) { snprintf(query, sizeof(query), "%s", q); *q = 0; }
    char out[URL_MAX];
    size_t n = 0;
    const char *p = path;
    bool trailing = false;
    while (*p) {
        while (*p == '/') p++;
        const char *seg = p;
        while (*p && *p != '/') p++;
        size_t len = (size_t)(p - seg);
        if (len == 0) continue;
        trailing = (*p == '/');
        if (len == 1 && seg[0] == '.') { trailing = true; continue; }
        if (len == 2 && seg[0] == '.' && seg[1] == '.') {
            while (n > 0 && out[n - 1] != '/') n--;
            if (n > 0) n--;
            trailing = true;
            continue;
        }
        if (n + len + 2 >= sizeof(out)) break;
        out[n++] = '/';
        memcpy(out + n, seg, len);
        n += len;
    }
    if (n == 0 || trailing) out[n++] = '/';
    out[n] = 0;
    snprintf(path, URL_MAX, "%s%s", out, query);
}

static bool has_scheme(const char *s) {
    if (!isalpha((unsigned char)*s)) return false;
    for (const char *p = s; *p; p++) {
        if (*p == ':') return true;
        if (!(isalnum((unsigned char)*p) || *p == '+' || *p == '-' || *p == '.')) return false;
    }
    return false;
}

void url_resolve(const char *base, const char *rel, char *out, size_t outlen) {
    while (isspace((unsigned char)*rel)) rel++;
    char r[URL_MAX];
    snprintf(r, sizeof(r), "%s", rel);
    size_t rl = strlen(r);
    while (rl && isspace((unsigned char)r[rl - 1])) r[--rl] = 0;

    if (has_scheme(r)) { snprintf(out, outlen, "%s", r); strip_fragment(out); return; }
    Url b;
    if (!url_parse(base, &b) || !strcmp(b.scheme, "about")) { snprintf(out, outlen, "%s", r); return; }
    if (r[0] == 0 || r[0] == '#') {
        snprintf(out, outlen, "%s", base);
        strip_fragment(out);
        return;
    }
    if (r[0] == '/' && r[1] == '/') { snprintf(out, outlen, "%s:%s", b.scheme, r); strip_fragment(out); return; }

    char prefix[320];
    bool default_port = (b.port == 443 && !strcmp(b.scheme, "https")) || (b.port == 80 && !strcmp(b.scheme, "http"));
    if (default_port) snprintf(prefix, sizeof(prefix), "%s://%s", b.scheme, b.host);
    else snprintf(prefix, sizeof(prefix), "%s://%s:%u", b.scheme, b.host, b.port);

    char path[URL_MAX];
    if (r[0] == '/') {
        snprintf(path, sizeof(path), "%s", r);
    } else if (r[0] == '?') {
        char *q = strchr(b.path, '?');
        if (q) *q = 0;
        snprintf(path, sizeof(path), "%s%s", b.path, r);
    } else {
        char *q = strchr(b.path, '?');
        if (q) *q = 0;
        char *slash = strrchr(b.path, '/');
        if (slash) slash[1] = 0;
        snprintf(path, sizeof(path), "%s%s", b.path, r);
    }
    strip_fragment(path);
    normalize_path(path);
    snprintf(out, outlen, "%s%s", prefix, path);
}

void url_encode_component(const char *in, char *out, size_t outlen) {
    static const char hex[] = "0123456789ABCDEF";
    size_t n = 0;
    for (; *in && n + 4 < outlen; in++) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out[n++] = (char)c;
        else if (c == ' ') out[n++] = '+';
        else { out[n++] = '%'; out[n++] = hex[c >> 4]; out[n++] = hex[c & 15]; }
    }
    out[n] = 0;
}

void url_from_input(const char *input, char *out, size_t outlen) {
    while (isspace((unsigned char)*input)) input++;
    char t[URL_MAX];
    snprintf(t, sizeof(t), "%s", input);
    size_t l = strlen(t);
    while (l && isspace((unsigned char)t[l - 1])) t[--l] = 0;
    if (!l) { out[0] = 0; return; }
    if (!strncasecmp(t, "about:", 6) || strstr(t, "://")) { snprintf(out, outlen, "%s", t); return; }
    bool looks_like_address = !strchr(t, ' ') && (strchr(t, '.') || !strncasecmp(t, "localhost", 9));
    if (looks_like_address) {
        snprintf(out, outlen, "https://%s", t);
        return;
    }
    char enc[URL_MAX];
    url_encode_component(t, enc, sizeof(enc));
    snprintf(out, outlen, "%s%s", settings_search_prefix(), enc);
}
