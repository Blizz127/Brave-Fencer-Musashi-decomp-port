#include "saved_room_actor.h"
#include "scene_command.h"

extern const u8 D_80185E5C[];
extern void func_80029514(s32 progress);

void func_801803AC(struct FileActor *actor)
{
    if (--actor->delay == (u32)-1) {
        func_80029514(2);
        func_800D1724(D_80185E5C);
    }
}
