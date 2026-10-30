#include "url.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
const char *settings_search_prefix(void) { return "https://example.org/?q="; }
int main(void) {
    const char *local[] = {"http://127.0.0.1/p", "http://10.1.2.3/p", "http://172.16.0.1/p", "http://172.31.2.3/p",
        "http://192.168.1.2/p", "http://169.254.1.1/p", "http://localhost/p", "http://NAS.local/p", "http://router/p"};
    for (unsigned i = 0; i < sizeof(local) / sizeof(*local); i++) assert(url_is_local(local[i]));
    const char *public[] = {"https://example.org/p", "http://172.32.1.2/p", "http://172.15.1.2/p", "http://192.169.1.2/p",
        "http://localhost.example.org/p", "http://example.local.evil.org/p", "about:home", "", NULL};
    for (unsigned i = 0; i < sizeof(public) / sizeof(*public); i++) assert(!url_is_local(public[i]));
    char out[URL_MAX];
    assert(url_unwrap_redirect("https://duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.org%2Fp%3Fa%3D1&rut=abc", out, sizeof(out)));
    assert(!strcmp(out, "https://example.org/p?a=1"));
    assert(url_unwrap_redirect("https://www.google.com/url?q=https%3A%2F%2Fexample.org%2F", out, sizeof(out)));
    assert(!url_unwrap_redirect("https://www.google.com.evil.org/url?q=https%3A%2F%2Fexample.org%2F", out, sizeof(out)));
    assert(!url_unwrap_redirect("https://duckduckgo.com/l/?uddg=javascript%3Aalert(1)", out, sizeof(out)));
    assert(!url_unwrap_redirect("https://duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.org%00.evil", out, sizeof(out)));
    assert(!url_unwrap_redirect("https://duckduckgo.com/l/?uddg=%", out, sizeof(out)));
    assert(!url_unwrap_redirect("https://duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.org", out, 8));
    puts("PASS: direct LAN images and bounded search redirects, with host/scheme/NUL/truncation rejects");
}
