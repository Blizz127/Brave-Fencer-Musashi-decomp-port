#include "contact_state.h"

void func_8015FBE0(Action21Actor *actor)
{
    TwinpeakPollActor *state = (TwinpeakPollActor *)actor;

    if (state->selected_contact & 0x80000000u) {
        D_80189EAC[actor->current_action](actor);
        func_80147078(actor, 0);
        func_80159B70(actor);
    } else {
        D_80078EC1 = 0;
        func_80154150(actor, 0x20);
        func_80154A74(actor, 0x18);
        func_80146A6C(0x16, TWINPEAK_ACTOR_WORD(actor), 0, 0, 0, 0, 0);
        func_80147324(0x44f);
        func_801553A8(actor);
        func_801553C0(actor);
        D_801270C0 = 2;
        D_800B9A17 = 0;
        func_80146CA0(actor);
    }
}
