#include "input.h"
#include "nx_sdl.h"
#ifdef __SWITCH__
#include <switch.h>
static PadState pad;
#endif

static unsigned int buttons, previous_buttons, pressed_frame, released_frame;
static unsigned int pressed_accum, released_accum, lock_mask;

void input_init(void) {
#ifdef __SWITCH__
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
#endif
}

void input_update(void) {
    buttons = 0;
#ifdef __SWITCH__
    padUpdate(&pad);
    const u64 held = padGetButtons(&pad);
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);
    if ((held & HidNpadButton_Left) || stick.x < -14000) buttons |= PSP_CTRL_LEFT;
    if ((held & HidNpadButton_Right) || stick.x > 14000) buttons |= PSP_CTRL_RIGHT;
    if ((held & HidNpadButton_Up) || stick.y > 14000) buttons |= PSP_CTRL_UP;
    if ((held & HidNpadButton_Down) || stick.y < -14000) buttons |= PSP_CTRL_DOWN;
    if (held & HidNpadButton_A) buttons |= PSP_CTRL_CROSS;
    if (held & HidNpadButton_B) buttons |= PSP_CTRL_CIRCLE;
    if (held & HidNpadButton_X) buttons |= PSP_CTRL_TRIANGLE;
    if (held & HidNpadButton_Y) buttons |= PSP_CTRL_SQUARE;
    if (held & (HidNpadButton_L | HidNpadButton_ZL)) buttons |= PSP_CTRL_LTRIGGER;
    if (held & (HidNpadButton_R | HidNpadButton_ZR)) buttons |= PSP_CTRL_RTRIGGER;
    if (held & HidNpadButton_Plus) buttons |= PSP_CTRL_START;
    if (held & HidNpadButton_Minus) buttons |= PSP_CTRL_SELECT;
#else
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT]) buttons |= PSP_CTRL_LEFT;
    if (keys[SDL_SCANCODE_RIGHT]) buttons |= PSP_CTRL_RIGHT;
    if (keys[SDL_SCANCODE_UP]) buttons |= PSP_CTRL_UP;
    if (keys[SDL_SCANCODE_DOWN]) buttons |= PSP_CTRL_DOWN;
    if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_RETURN]) buttons |= PSP_CTRL_CROSS;
    if (keys[SDL_SCANCODE_BACKSPACE]) buttons |= PSP_CTRL_CIRCLE;
    if (keys[SDL_SCANCODE_P]) buttons |= PSP_CTRL_START;
    if (keys[SDL_SCANCODE_ESCAPE]) buttons |= PSP_CTRL_SELECT;
#endif
    pressed_frame = buttons & ~previous_buttons;
    released_frame = ~buttons & previous_buttons;
    pressed_accum |= pressed_frame;
    released_accum |= released_frame;
    previous_buttons = buttons;
    lock_mask &= ~released_frame;
}

bool input_pressed(unsigned int mask) { return (pressed_frame & mask & ~lock_mask) != 0; }
bool input_held(unsigned int mask) { return (buttons & mask & ~lock_mask) != 0; }
bool input_released(unsigned int mask) { return (released_frame & mask) != 0; }
bool input_consume_pressed(unsigned int mask) {
    if (!(pressed_accum & mask & ~lock_mask)) return false;
    pressed_accum &= ~mask;
    return true;
}
bool input_consume_released(unsigned int mask) {
    if (!(released_accum & mask)) return false;
    released_accum &= ~mask;
    return true;
}
void input_reset_edges(void) {
    pressed_accum = released_accum = pressed_frame = released_frame = 0;
    previous_buttons = buttons;
}
void input_lock_held(void) { lock_mask |= buttons; }
