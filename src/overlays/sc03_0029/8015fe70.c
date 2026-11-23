#include "fusion_queue_game.h"

void func_8015FE70(Action21Actor *actor)
{
    if (actor->frame_events & 0x8000) {
        if (actor->next_frame_events == 0) {
            D_80189EAC[actor->current_action](actor);
            func_80147078(actor, 0);
            func_80159B70(actor);
            return;
        }
        actor->frame_events = actor->next_frame_events;
    }
    if (actor->frame_events & 0x4000)
        func_80154A74(actor, 17);
    func_80161208(actor);
}
