#pragma once
#include <stdbool.h>
#include <stdint.h>

#define DNS_CACHE_ENTRIES 16
typedef struct { char host[256]; uint32_t address; uint64_t stored_ms, lifetime_ms; } DnsEntry;
typedef struct { DnsEntry entries[DNS_CACHE_ENTRIES]; unsigned next; } DnsCache;
/* Owned by the one network worker. Time is steady milliseconds, not the RTC. */
bool dns_cache_get(DnsCache *cache, const char *host, uint64_t now_ms, uint32_t *address);
void dns_cache_put(DnsCache *cache, const char *host, uint32_t address, uint32_t ttl_seconds, uint64_t now_ms);
