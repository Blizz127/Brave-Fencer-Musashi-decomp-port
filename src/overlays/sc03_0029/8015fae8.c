#include "action22_start.h"

void func_8015FAE8(Action21Actor *actor)
{
    FusionWord status = func_801612B8(actor, 0);
    TwinpeakPollActor *state = (TwinpeakPollActor *)actor;

    if (status == 0 || status == 0x8000) {
        func_8016007C(actor);
        state->selected_contact |= 0x80000000u;
        D_80189EAC[actor->current_action](actor);
        func_80147078(actor, 3);
        func_8015BE94(actor);
    } else if (status & 0x4000) {
        func_8016007C(actor);
        state->selected_contact |= 0x80000000u;
        D_80189EAC[actor->current_action](actor);
        func_80147078(actor, 4);
        func_8015C0C4(actor);
    } else {
        func_8015FBE0(actor);
    }
}
