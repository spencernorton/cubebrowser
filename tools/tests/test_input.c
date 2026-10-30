#include "input_sdk.h"
#include "input.h"
#include "controller.h"
#include "settings.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
Settings g_set;
static ControllerSample pads[4];static u64 tick=1;static int motor[4];
u64 SYS_GetSystemTime(void) {return tick;}
void PAD_Init(void) {}
u32 PAD_ScanPads(void) {u32 mask=0;for(int i=0;i<4;i++)if(pads[i].connected)mask|=1u<<i;return mask;}
u32 PAD_ButtonsHeld(s32 c) {return pads[c].buttons;}
u32 PAD_ButtonsDown(s32 c) {(void)c;return 0;}
s8 PAD_StickX(s32 c) {return pads[c].x;}
s8 PAD_StickY(s32 c) {return pads[c].y;}
s8 PAD_SubStickY(s32 c) {return pads[c].scroll;}
void PAD_ControlMotor(s32 c,u32 v) {assert(c>=0 && c<4);motor[c]=v;}
float gfx_width(void) {return 640;}
u32 lab_pad(u32 down) {(void)down;return 0;}
void entropy_add_event(u32 event) {(void)event;}
static void frame(unsigned us) {tick+=us;input_update();}
int main(void) {
    g_set.deadzone=12;g_set.cursor_speed=1;g_set.accel=2;g_set.scroll_speed=1;g_set.rumble=1;g_set.key_repeat=5;
    for(int i=0;i<ACT_COUNT;i++)g_set.bind[i]=i;
    pads[0].connected=pads[1].connected=true;input_init();frame(16667);
    pads[0].x=40;frame(16667);input_rumble(20);assert(motor[0]);
    pads[1].buttons=PAD_BUTTON_B;frame(16667);assert(!input_raw_held()); /* owner stick wins */
    pads[0].x=0;frame(16667);assert(!input_raw_held()); /* held secondary is unarmed */
    pads[1].buttons=0;frame(16667);pads[1].buttons=PAD_BUTTON_B;frame(16667);
    assert(input_raw_pressed()==PAD_BUTTON_B && motor[0]==0);input_rumble(20);assert(motor[1]);
    pads[1].connected=false;frame(16667);assert(!motor[1] && !input_raw_held());
    pads[0].x=96;frame(16667);input_set_cursor(10,100);
    for(int i=0;i<60;i++)frame(16667);float ntsc=input_cursor_x();
    input_set_cursor(10,100);for(int i=0;i<50;i++)frame(20000);float pal=input_cursor_x();
    assert(fabsf(pal-ntsc)<.02f);
    tick+=10000000;input_update();assert(input_delta_seconds()<=.05f); /* bounded stall */
    ControllerOwner state={.owner=-1};ControllerSample samples[4]={{0}};
    for(int p=0;p<4;p++) {
        samples[p].connected=true;controller_choose(&state,samples,12);
        samples[p].buttons=1;assert(controller_choose(&state,samples,12)==p);
        samples[p].buttons=0;controller_choose(&state,samples,12);
    }
    puts("input: four ports, coherent ownership, neutral takeover, unplug rumble and PAL/NTSC timing pass");
}
