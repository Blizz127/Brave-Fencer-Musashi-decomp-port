#include "fusion_queue_game.h"

extern void (*D_80189EAC[])(Action21Actor *);
extern void func_80147078(Action21Actor *, unsigned int);
extern void func_80159B70(Action21Actor *);
extern void func_80161208(Action21Actor *);

void func_8015F89C(Action21Actor *actor)
{
    FusionHalf events = actor->frame_events;
    if (!(events & 0x4000)) {
        if (events & 0x8000) {
            D_80189EAC[actor->current_action](actor);
            func_80147078(actor, 0);
            func_80159B70(actor);
            return;
        }
    } else {
        func_801466F0(1, TWINPEAK_ACTOR_WORD(actor), 0, -30, 0, 0, 0, 0);
    }
    func_80161208(actor);
}
