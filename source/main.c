/* CubeBrowser - a small HTTPS web browser for the Nintendo GameCube.
 * main.c - startup and the active screen loop. */
#include <gccore.h>
#include <math.h>
#include <stdio.h>
#include "gfx.h"
#include "theme.h"
#include "input.h"
#include "settings.h"
#include "settings_ui.h"
#include "storage.h"
#include "gc_entropy.h"
#include "sound.h"
#include "fetch.h"
#include "ui.h"
#include "osk.h"
#include "browser.h"
#include "diag.h"
#include "lab.h"
#include "launch.h"
#include "start_pref.h"

enum { SCREEN_BROWSER, SCREEN_SETTINGS };

/* Set from the browser's Return to Indigo button or Settings > Exit to Indigo.
 * Returning from main() hands control to
 * the loader stub libogc leaves at 0x80001800, i.e. back to Indigo when it
 * launched the browser. */
static volatile bool quit_requested;
void app_request_quit(void) { quit_requested = true; }

static int screen = SCREEN_BROWSER;
static int target_screen = SCREEN_BROWSER;
static float trans = 1.0f;   /* 1 = finished */

static void draw_screen(int which, bool interactive) {
    if (which == SCREEN_BROWSER) {
        browser_frame(interactive);
    } else if (settings_ui_frame(interactive) && interactive) {
        target_screen = SCREEN_BROWSER;
        trans = g_set.animations ? 0.0f : 1.0f;
        if (!g_set.animations) screen = SCREEN_BROWSER;
    }
}

/* A brief fade keeps transitions cheap; input belongs to neither screen. */
static void draw_transition(int from, int to, float p) {
    (void)from;
    draw_screen(to, false);
    gfx_rect(0, 0, gfx_width(), 480, color_alpha(theme()->bg_top, 1-p));
}

int main(int argc, char **argv) {
    launch_parse(argc, argv);
    bool sd = lab_init(argc, argv) && storage_init();   /* the console lab: off unless started with lab=1 */
    diag_init();
    settings_load();
    launch_capture(&g_set);
    launch_apply(&g_set);
    gfx_init(g_set.video_mode, g_set.widescreen != 0);
    input_init();

    /* Hold L + R + START while starting to reset all settings */
    bool reset = false;
    for (int i = 0; i < 6; i++) {
        VIDEO_WaitVSync();
        PAD_ScanPads();
        for (int p = 0; p < 4; p++) {
            u32 h = PAD_ButtonsHeld(p);
            if ((h & PAD_TRIGGER_L) && (h & PAD_TRIGGER_R) && (h & PAD_BUTTON_START)) reset = true;
        }
    }
    if (reset) {
        settings_defaults(&g_set);
        launch_capture(&g_set);
        launch_apply(&g_set);
        settings_save();
        gfx_set_widescreen(g_set.widescreen != 0);
        gfx_apply_video_mode(g_set.video_mode);
    }
    theme_set(g_set.theme);
    entropy_init();
    sound_init();
    start_pref_load();
    fetch_init();
    browser_init();
    lab_start();

    if (reset) ui_toast("Settings were reset to defaults");
    else if (!sd) ui_toast("No SD card found - settings and bookmarks won't be saved");

    for (;;) {
        lab_frame();
        diag_frame();
        diag_note(DIAG_MAIN, "reading controller");
        input_update();
        ui_begin_frame();
        if (screen == SCREEN_BROWSER && trans >= 1.0f) browser_update();
        gfx_begin_frame();
        ui_draw_background();

        diag_note(DIAG_MAIN, "screen %d", screen);
        if (trans < 1.0f) {
            draw_transition(screen, target_screen, trans);
            trans += input_delta_seconds() / 0.367f;
            if (trans >= 1.0f) { trans = 1.0f; screen = target_screen; }
        } else {
            draw_screen(screen, true);
            if (screen == SCREEN_BROWSER && browser_take_settings_request()) {
                settings_ui_open();
                sound_play(SFX_OPEN);
                target_screen = SCREEN_SETTINGS;
                if (g_set.animations) trans = 0.0f;
                else screen = SCREEN_SETTINGS;
            }
        }

        diag_note(DIAG_MAIN, "drawing keyboard and pointer");
        osk_frame();
        ui_draw_toast();
        ui_draw_cursor();
        ui_end_frame();
        diag_note(DIAG_MAIN, "waiting for the GPU to finish the frame");
        gfx_end_frame();
        if (quit_requested) break;
    }
    return 0;
}
