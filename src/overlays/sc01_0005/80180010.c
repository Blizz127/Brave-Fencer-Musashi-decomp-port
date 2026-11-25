#include "event_flags.h"
#include "saved_room_actor.h"
#include "scene_command.h"

extern u8 D_801BBE98[];
extern u32 func_8016F1AC(void);
extern void func_80029514(s32 progress);

void func_80180010(struct FileActor *actor)
{
    func_8017FD50(actor);
    if (actor->delay) {
        --actor->delay;
    } else if (!func_8016F1AC()) {
        func_80029514(10);
        func_80029124(293, 0);
        func_80029124(294, 0);
        func_80029124(295, 0);
        func_80029124(296, 0);
        actor->scene_fields[0] = actor->scene_fields[1] = 0;
        func_80178B18(actor, D_801BBE98);
        actor->delay = 0;
        ++actor->state;
    }
}
