#include "door_15d.h"

/* The resident model initializer accepts an optional signed-word list. */
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} DoorModelList;

extern void *func_8012C1B8(void);
extern void func_8001C214(DoorModelList *model, s32 *list);
extern s32 func_8012CAE4(u8 *actor);
extern const u8 D_80182DF0[];

#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ AddressWord;
#else
typedef u32 AddressWord;
#endif

s32 func_80132144(Door15DActor *actor)
{
    void *model = func_8012C1B8();
    s32 next;

    actor->model_address = (u32)(AddressWord)model;
    if (model) {
        func_8001C214(model, 0);
        actor->shape_address = (u32)(AddressWord)D_80182DF0;
        actor->contact_flags = 0x80;
        next = actor->phase + 1;
        actor->phase = (u16)next;
        return next;
    }
    return func_8012CAE4((u8 *)actor);
}
