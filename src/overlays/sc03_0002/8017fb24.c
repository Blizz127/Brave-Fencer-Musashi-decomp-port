/* Create the village guard only after his rescue flag has been set. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x12];
    u16 yaw;
} GuardModel;

typedef struct {
    u8 pad00[0xc];
    s16 extent;
} GuardContact;

typedef struct {
    u16 type;
    u16 phase;
    u8 pad04[0x1c];
    GuardModel *model;
    u8 pad24[0x34];
    u32 contactShape;
    s16 facing;
    u8 pad5e[0xa];
    GuardContact *contact;
    u8 pad6c[0x68];
    s32 script;
} GuardActor;

extern u8 func_80029178(s32 flag);
extern void func_8012CAE4(GuardActor *actor);
extern GuardModel *func_8012C1B8(void);
extern u8 D_801BFCB4[];
extern u8 D_80189440[];
extern u8 D_80189540[];
extern void func_8001C214(GuardModel *model, void *resource);
extern void func_8012A828(GuardActor *actor, void *animation);
typedef void (*GuardContinuation)();
typedef GuardContinuation (*GuardInteractionCallback)();
extern GuardContinuation func_8017FCD0();
extern s32 func_801788B8(GuardActor *actor, GuardInteractionCallback interact);

void func_8017FB24(GuardActor *actor)
{
    if (!func_80029178(0xf0)) {
        func_8012CAE4(actor);
        return;
    }
    actor->model = func_8012C1B8();
    if (actor->model) {
        func_8001C214(actor->model, D_801BFCB4);
        actor->model->yaw = 0xe00;
        func_8012A828(actor, D_80189440);
        actor->contact->extent = 0x7fff;
        actor->contactShape = (u32)D_80189540 | 0x40000000;
        actor->facing = 0x800;
        actor->script = func_801788B8(actor, func_8017FCD0);
        actor->phase++;
    }
}
