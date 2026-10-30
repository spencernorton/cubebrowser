#include "launch.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

LaunchContext g_launch;
static Settings saved;
static bool captured;
static const char *const colors[] = {"indigo", "azure", "emerald", "gold", "spice", "crimson", "rose", "jet-black"};

static const char *argument(int argc, char **argv, const char *key) {
    size_t length = strlen(key);
    for (int i = 1; i < argc; i++)
        if (argv[i] && !strncmp(argv[i], key, length) && argv[i][length] == '=') return argv[i] + length + 1;
    return NULL;
}
static int number(const char *s, int minimum, int maximum) {
    if (!s || !*s || (*s != '-' && (*s < '0' || *s > '9'))) return INT_MIN;
    char *end;
    long value = strtol(s, &end, 10);
    return *end || value < minimum || value > maximum ? INT_MIN : (int)value;
}
void launch_parse(int argc, char **argv) {
    memset(&g_launch, 0, sizeof(g_launch));
    captured = false;
    g_launch.color = -1;
    g_launch.video = g_launch.wide = g_launch.margin = g_launch.motion = INT_MIN;
    g_launch.sounds = g_launch.rumble = g_launch.timezone = INT_MIN;
    const char *version = argument(argc, argv, "indigo.context");
    if (!version || strcmp(version, "1")) return;
    g_launch.active = true;
    const char *color = argument(argc, argv, "indigo.color");
    for (unsigned i = 0; color && i < sizeof(colors) / sizeof(colors[0]); i++)
        if (!strcmp(color, colors[i])) g_launch.color = (int)i;
    const char *video = argument(argc, argv, "indigo.video");
    if (video && !strcmp(video, "auto")) g_launch.video = 0;
    if (video && !strcmp(video, "480i")) g_launch.video = 1;
    if (video && !strcmp(video, "480p")) g_launch.video = 2;
    g_launch.wide = number(argument(argc, argv, "indigo.wide"), 0, 1);
    g_launch.margin = number(argument(argc, argv, "indigo.margin"), 0, 10);
    g_launch.motion = number(argument(argc, argv, "indigo.motion"), 0, 2);
    g_launch.sounds = number(argument(argc, argv, "indigo.sounds"), 0, 1);
    g_launch.rumble = number(argument(argc, argv, "indigo.rumble"), 0, 1);
    g_launch.timezone = number(argument(argc, argv, "indigo.timezone"), -720, 840);
    const char *device = argument(argc, argv, "indigo.device");
    if (device && (!strcmp(device, "sda") || !strcmp(device, "sdb") || !strcmp(device, "sdc") || !strcmp(device, "gcldr")))
        strcpy(g_launch.device, device);
}
void launch_capture(const Settings *settings) { saved = *settings; captured = true; }
void launch_apply(Settings *s) {
    if (!g_launch.active) return;
    if (g_launch.video != INT_MIN) s->video_mode = g_launch.video;
    if (g_launch.wide != INT_MIN) s->widescreen = g_launch.wide;
    if (g_launch.margin != INT_MIN) s->margin = g_launch.margin;
    if (g_launch.motion != INT_MIN) s->animations = g_launch.motion == 0;
    if (g_launch.sounds != INT_MIN) s->sounds = g_launch.sounds;
    if (g_launch.rumble != INT_MIN) s->rumble = g_launch.rumble;
    if (g_launch.timezone != INT_MIN) { s->tz_minutes = g_launch.timezone; s->dst = 0; }
}
bool launch_inherits(const SetDef *d) {
    if (!g_launch.active) return false;
    return (d->off == offsetof(Settings, video_mode) && g_launch.video != INT_MIN) ||
        (d->off == offsetof(Settings, widescreen) && g_launch.wide != INT_MIN) ||
        (d->off == offsetof(Settings, margin) && g_launch.margin != INT_MIN) ||
        (d->off == offsetof(Settings, animations) && g_launch.motion != INT_MIN) ||
        (d->off == offsetof(Settings, sounds) && g_launch.sounds != INT_MIN) ||
        (d->off == offsetof(Settings, rumble) && g_launch.rumble != INT_MIN) ||
        (d->off == offsetof(Settings, tz_minutes) || d->off == offsetof(Settings, dst)) ||
        (d->off == offsetof(Settings, theme) && g_launch.color >= 0);
}
void launch_preserve(Settings *settings) {
    if (!captured || !g_launch.active) return;
    for (int i = 0; i < g_setdef_count; i++) {
        const SetDef *d = &g_setdefs[i];
        if (d->key && launch_inherits(d)) *(int *)((char *)settings + d->off) = *(const int *)((const char *)&saved + d->off);
    }
}

int64_t launch_console_utc(int64_t console_seconds) {
    return g_launch.active && g_launch.timezone != INT_MIN ?
        console_seconds - (int64_t)g_launch.timezone * 60 : console_seconds;
}
