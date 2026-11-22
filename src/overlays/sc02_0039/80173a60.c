/* Resolve an optional actor, save the queued operand and install the player's phase table. */
#include "mine_award_controller.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern s16 D_80126B30;
#define COPY_VECTOR(v) (*(MineAwardVector *)&D_80126B30 = *(v))
#define MODEL_VALUE(controller) (*(u16 *)(*(u8 **)((controller) + 0x20) + 0x12))
#define FIND_ACTOR(controller, type, id) func_8014C118((s32)(controller), (type), (id))
#else
#include <string.h>
extern MineAwardVector D_80126B30;
#define COPY_VECTOR(v) memmove(D_80126B30.bytes, (v), 8)
#define MODEL_VALUE(controller) mine_award_model_value(controller)
#define FIND_ACTOR(controller, type, id) func_8014C118((controller), (type), (id))

/* A native controller supplies a host model pointer in its semantic field. */
static u16 mine_award_model_value(const u8 *controller)
{
    const u8 *model;
    memcpy(&model, controller + 0x20, sizeof model);
    return (u16)(model[0x12] | ((u16)model[0x13] << 8));
}
#endif

void func_80173A60(s32 setup, s32 selector, s32 type, s32 id, const MineAwardVector *vector)
{
    u8 *controller;
    controller = D_80126B58;
    if (type) {
        D_80127508 = FIND_ACTOR(controller, type, (s16)id);
    } else {
        D_80127508 = 0;
    }
    COPY_VECTOR(vector);
    D_80126B38 = (u16)setup;
    D_80126B3A = (s16)selector;
    D_80126B3C = MODEL_VALUE(controller);
    func_80174684(func_80173B4C);
    D_8011F9C8 = 0;
    D_80126938 = 0;
    D_8012750C = 0;
}
