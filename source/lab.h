/* lab.h - the console lab: LAN endpoints that let a test run watch and drive
 * the browser on a real GameCube. Off unless the browser starts with lab=1
 * (see lab.c); when off, every call returns at once. */
#pragma once
#include <gccore.h>
#include <stdbool.h>

bool lab_init(int argc, char **argv);   /* first thing in main(): false when lab=1 sd=0 keeps the SD card off */
void lab_start(void);                   /* after fetch_init(): serves TCP 4300 once the network is up */
void lab_frame(void);                   /* once per frame on the main thread: does what the lab queued */
u32  lab_pad(u32 real_down);            /* input.c: the virtual buttons held now; notes real presses */
void lab_note(const char *fmt, ...) __attribute__((format(printf, 1, 2)));   /* a line for GET /log */
