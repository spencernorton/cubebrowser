#include "dns_cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    DnsCache cache = {0};
    uint32_t ip;
    dns_cache_put(&cache, "example.org", 42, 10, 1000);
    assert(dns_cache_get(&cache, "EXAMPLE.ORG", 10999, &ip) && ip == 42);
    assert(!dns_cache_get(&cache, "example.org", 11000, &ip));
    assert(!dns_cache_get(&cache, "example.org", 999, &ip));
    dns_cache_put(&cache, "long.example", 7, 3600, 0);
    assert(dns_cache_get(&cache, "long.example", 299999, &ip));
    assert(!dns_cache_get(&cache, "long.example", 300000, &ip));
    dns_cache_put(&cache, "zero.example", 8, 0, 0);
    assert(!dns_cache_get(&cache, "zero.example", 1, &ip));
    dns_cache_put(&cache, "example.org", 55, 20, 12000);
    assert(dns_cache_get(&cache, "example.org", 12001, &ip) && ip == 55);
    memset(&cache, 0, sizeof(cache));
    assert(!dns_cache_get(&cache, "example.org", 12001, &ip));
    puts("PASS: DNS TTL and five-minute ceiling, case matching, expiry, reset and replacement");
}
