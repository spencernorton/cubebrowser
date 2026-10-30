#include "controller.h"
#include <stdlib.h>

static bool active(const ControllerSample *s, int deadzone) {
    return s->buttons || abs(s->x) > deadzone || abs(s->y) > deadzone || abs(s->scroll) > deadzone;
}
int controller_choose(ControllerOwner *state, const ControllerSample samples[4], int deadzone) {
    int owner = state->owner;
    if (owner < 0 || owner > 3 || !samples[owner].connected) owner = -1;
    bool busy = owner >= 0 && active(&samples[owner], deadzone);
    int next = owner;
    for (int i=0;i<4;i++) {
        if (!samples[i].connected) { state->armed[i] = false; continue; }
        bool moving = active(&samples[i], deadzone);
        if (!moving) state->armed[i] = true;
        else {
            if (!busy && state->armed[i] && next == owner) { next = i; busy = true; }
            if (i != owner) state->armed[i] = false;
        }
    }
    state->owner = next;
    return next;
}
