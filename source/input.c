/* input.c - controller handling. The main stick drives a pointer with a dead
 * zone and an acceleration curve; actions are remappable via settings. */
#include "input.h"
#include "settings.h"
#include "gfx.h"
#include "gc_entropy.h"
#include "lab.h"
#include "controller.h"
#include <ogc/lwp_watchdog.h>
#include <math.h>

const char *const action_names[ACT_COUNT] = {
    "Click", "Back", "Address bar", "Bookmarks", "Browser menu", "Slow pointer", "Page up", "Page down"
};
const char *const button_names[BTN_COUNT] = { "A", "B", "X", "Y", "Z", "L", "R", "START" };
const u32 button_masks[BTN_COUNT] = {
    PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X, PAD_BUTTON_Y,
    PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_TRIGGER_R, PAD_BUTTON_START
};

static u32 down, held, prev_held, consumed;
static float cx = 320, cy = 240, glide_x, glide_y;
static bool gliding, moved;
static float idle_frames;
static ControllerOwner owner = {.owner=-1};
static int channel = -1, substick_y;
static float delta = 1.0f/60;
static u64 last_tick;
static float rumble_left;
static int dpad_dir = DIR_NONE;
static float dpad_timer;
static u32 repeat_mask;
static float repeat_timer;
static s8 last_sx, last_sy;

void input_init(void) {
    PAD_Init();
    for (int p=0;p<4;p++) PAD_ControlMotor(p, PAD_MOTOR_STOP);
    last_tick = SYS_GetSystemTime();
    cx = gfx_width() * 0.5f;
    cy = 240;
}

static u32 action_mask(Action a) { return button_masks[g_set.bind[a]]; }

static float stick_axis(int v) {
    float dz = (float)g_set.deadzone;
    float a = fabsf((float)v);
    if (a <= dz) return 0;
    float n = (a - dz) / (96.0f - dz);
    if (n > 1) n = 1;
    float curve = 1.0f + g_set.accel * 0.22f;
    n = powf(n, curve);
    return v < 0 ? -n : n;
}

void input_update(void) {
    u64 tick = SYS_GetSystemTime();
    delta = last_tick && tick >= last_tick ? (float)ticks_to_microsecs(tick-last_tick)/1000000 : 1.0f/60;
    last_tick = tick;
    if (delta < .001f) delta = .001f;
    if (delta > .05f) delta = .05f;
    u32 connected = PAD_ScanPads(), real_down = 0;
    ControllerSample samples[4];
    for (int p=0;p<4;p++) {
        samples[p] = (ControllerSample){(connected & (1u << p)) != 0, PAD_ButtonsHeld(p),
            PAD_StickX(p), PAD_StickY(p), PAD_SubStickY(p)};
        if (samples[p].connected) real_down |= PAD_ButtonsDown(p);
    }
    int next = controller_choose(&owner, samples, g_set.deadzone);
    if (next != channel) { input_stop_rumble(); held = 0; gliding = false; channel = next; }
    prev_held = held;
    held = channel >= 0 ? samples[channel].buttons : 0;
    down = held & ~prev_held;
    substick_y = channel >= 0 ? samples[channel].scroll : 0;
    {   /* console lab: the virtual pad's buttons join the controller's */
        static u32 vprev;
        u32 v = lab_pad(real_down);
        held |= v;
        down |= v & ~vprev;
        vprev = v;
    }
    consumed = 0;

    s8 sx = channel >= 0 ? samples[channel].x : 0, sy = channel >= 0 ? samples[channel].y : 0;
    if (down || sx != last_sx || sy != last_sy) entropy_add_event(((u32)down << 16) ^ ((u8)sx << 8) ^ (u8)sy);
    last_sx = sx; last_sy = sy;

    float ax = stick_axis(sx), ay = stick_axis(sy);
    moved = (ax != 0 || ay != 0);
    if (moved) {
        gliding = false;
        idle_frames = 0;
        float speed = 2.0f + g_set.cursor_speed * 1.1f;
        if (held & action_mask(ACT_SLOW)) speed *= 0.35f;
        cx += ax * speed * delta * 60;
        cy -= ay * speed * delta * 60;   /* stick up = positive Y */
    } else {
        idle_frames += delta * 60;
    }
    if (gliding) {
        cx += (glide_x - cx) * fminf(1, .35f * delta * 60);
        cy += (glide_y - cy) * fminf(1, .35f * delta * 60);
        if (fabsf(glide_x - cx) < 0.5f && fabsf(glide_y - cy) < 0.5f) { cx = glide_x; cy = glide_y; gliding = false; }
    }
    input_clamp_cursor();

    /* D-pad with auto-repeat */
    int dir = DIR_NONE;
    if (held & PAD_BUTTON_UP) dir = DIR_UP;
    else if (held & PAD_BUTTON_DOWN) dir = DIR_DOWN;
    else if (held & PAD_BUTTON_LEFT) dir = DIR_LEFT;
    else if (held & PAD_BUTTON_RIGHT) dir = DIR_RIGHT;
    if (dir != dpad_dir) { dpad_dir = dir; dpad_timer = 0; }
    else if (dir != DIR_NONE) dpad_timer += delta * 60;

    /* generic held-button repeat */
    if (held != prev_held) { repeat_mask = held; repeat_timer = 0; }
    else repeat_timer += delta * 60;

    if (rumble_left > 0 && (rumble_left -= delta * 60) <= 0) input_stop_rumble();
}

bool input_pressed(Action a) { u32 m = action_mask(a); return (down & m) && !(consumed & m); }
bool input_held(Action a) { return held & action_mask(a); }
void input_consume(Action a) { consumed |= action_mask(a); }
u32 input_raw_pressed(void) { return down & ~consumed; }
u32 input_raw_held(void) { return held; }

float input_delta_seconds(void) { return delta; }
static int repeat_period(void) { return 12 - g_set.key_repeat; }
static bool repeat_due(float timer, float first, float period) {
    return timer >= first && floorf((timer-first)/period) != floorf((timer-delta*60-first)/period);
}

bool input_raw_repeat(u32 mask) {
    if (down & mask) return true;
    if (!(held & mask) || !(repeat_mask & mask)) return false;
    int first = 20;
    return repeat_due(repeat_timer, first, repeat_period());
}

int input_dpad(void) {
    if (dpad_dir == DIR_NONE) return DIR_NONE;
    if (dpad_timer == 0) return dpad_dir;
    if (repeat_due(dpad_timer, 18, 5)) return dpad_dir;
    return DIR_NONE;
}

int input_pressed_bindable(void) {
    for (int b = 0; b < BTN_COUNT; b++)
        if (down & button_masks[b]) return b;
    return -1;
}

float input_cursor_x(void) { return cx; }
float input_cursor_y(void) { return cy; }
void input_set_cursor(float x, float y) { cx = x; cy = y; gliding = false; input_clamp_cursor(); }

void input_glide_cursor(float x, float y) {
    glide_x = x; glide_y = y; gliding = true;
    idle_frames = 0;
}

void input_clamp_cursor(void) {
    float w = gfx_width();
    if (cx < 0) cx = 0;
    if (cx > w - 1) cx = w - 1;
    if (cy < 0) cy = 0;
    if (cy > 479) cy = 479;
}

bool input_stick_idle(int frames) { return idle_frames >= frames; }
bool input_pointer_moved(void) { return moved; }

float input_scroll_delta(void) {
    float k = 1.5f + g_set.scroll_speed * 1.2f;
    float d = -stick_axis(substick_y) * k * 1.6f * delta * 60;
    if (input_raw_repeat(action_mask(ACT_PAGE_DOWN))) d += 300;
    if (input_raw_repeat(action_mask(ACT_PAGE_UP))) d -= 300;
    return d;
}

void input_rumble(int frames) {
    if (!g_set.rumble || channel < 0) return;
    if (rumble_left <= 0) PAD_ControlMotor(channel, PAD_MOTOR_RUMBLE);
    if (frames > rumble_left) rumble_left = frames;
}

void input_stop_rumble(void) {
    rumble_left = 0;
    if (channel >= 0) PAD_ControlMotor(channel, PAD_MOTOR_STOP);
}
