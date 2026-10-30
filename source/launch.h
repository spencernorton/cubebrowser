/* Versioned Indigo launch preferences. No inherited value is persisted. */
#pragma once
#include "settings.h"

typedef struct {
    bool active;
    int color, video, wide, margin, motion, sounds, rumble, timezone;
    char device[8];
} LaunchContext;
extern LaunchContext g_launch;
void launch_parse(int argc, char **argv);
void launch_capture(const Settings *settings);
void launch_apply(Settings *settings);
void launch_preserve(Settings *settings);
bool launch_inherits(const SetDef *definition);
