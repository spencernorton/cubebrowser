/* A single first-party start-page ZIP preference, not a general cookie jar. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { START_PREF_NONE, START_PREF_SAVED, START_PREF_CLEARED, START_PREF_UNSAVED };
void start_pref_load(void);
bool start_pref_scope(const char *url);
void start_pref_request(const char *url, bool verified, uint64_t now, char *out, size_t capacity);
bool start_pref_receive(const char *url, bool verified, const char *header, uint64_t now);
int start_pref_take_notice(void);
