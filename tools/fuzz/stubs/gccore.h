/* Minimal host stand-in for libogc's gccore.h, so the parser sources compile
 * with a desktop clang for fuzzing. Only the types the headers reference. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;  typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8;   typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
typedef float f32;
typedef struct { u32 val[8]; } GXTexObj;
typedef struct GXRModeObj GXRModeObj;
static inline void DCFlushRange(void *p, u32 n) { (void)p; (void)n; }
