#include "dns_cache.h"
#include <string.h>
#include <strings.h>

bool dns_cache_get(DnsCache *cache, const char *host, uint64_t now, uint32_t *address) {
    for (unsigned i = 0; i < DNS_CACHE_ENTRIES; i++) {
        const DnsEntry *entry = &cache->entries[i];
        if (entry->host[0] && !strcasecmp(entry->host, host) && now >= entry->stored_ms &&
            now - entry->stored_ms < entry->lifetime_ms) { *address = entry->address; return true; }
    }
    return false;
}

void dns_cache_put(DnsCache *cache, const char *host, uint32_t address, uint32_t ttl, uint64_t now) {
    size_t length = strlen(host);
    if (!ttl || !length || length >= sizeof(cache->entries[0].host)) return;
    unsigned slot = cache->next;
    for (unsigned i = 0; i < DNS_CACHE_ENTRIES; i++)
        if (!strcasecmp(cache->entries[i].host, host)) { slot = i; break; }
    DnsEntry *entry = &cache->entries[slot];
    memcpy(entry->host, host, length + 1);
    entry->address = address;
    entry->stored_ms = now;
    entry->lifetime_ms = (uint64_t)(ttl < 300 ? ttl : 300) * 1000;
    cache->next = (slot + 1) % DNS_CACHE_ENTRIES;
}
