#include "mine_door_phase.h"

typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} DoorListModel;

extern s32 func_8012CAE4(u8 *actor);
extern void func_8001C214(DoorListModel *model, s32 *list);

#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ DoorPointerWord;
#else
typedef u32 DoorPointerWord;
#endif

s32 func_8013221C(MineDoorActor *actor)
{
    void *model = func_8012C1B8();
    s32 next_phase;

    actor->model = (u32)(DoorPointerWord)model;
    if (model) {
        func_8001C214(model, 0);
        actor->shape = (u32)(DoorPointerWord)D_8017F65C;
        actor->contact_flags = 0x80;
        next_phase = actor->phase + 1;
        actor->phase = (u16)next_phase;
        return next_phase;
    }
    return func_8012CAE4((u8 *)actor);
}
