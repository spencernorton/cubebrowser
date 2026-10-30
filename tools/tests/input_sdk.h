#pragma once
#include <gccore.h>
enum {PAD_BUTTON_A=0x100,PAD_BUTTON_B=0x200,PAD_BUTTON_X=0x400,PAD_BUTTON_Y=0x800,
 PAD_BUTTON_START=0x1000,PAD_TRIGGER_Z=0x10,PAD_TRIGGER_R=0x20,PAD_TRIGGER_L=0x40,
 PAD_BUTTON_LEFT=1,PAD_BUTTON_RIGHT=2,PAD_BUTTON_DOWN=4,PAD_BUTTON_UP=8,
 PAD_MOTOR_STOP=0,PAD_MOTOR_RUMBLE=1};
void PAD_Init(void);
u32 PAD_ScanPads(void);
u32 PAD_ButtonsHeld(s32 channel);
u32 PAD_ButtonsDown(s32 channel);
s8 PAD_StickX(s32 channel);
s8 PAD_StickY(s32 channel);
s8 PAD_SubStickY(s32 channel);
void PAD_ControlMotor(s32 channel,u32 value);
