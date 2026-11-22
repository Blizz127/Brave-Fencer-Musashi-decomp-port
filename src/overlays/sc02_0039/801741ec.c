/* Turn the current player model back toward the angle saved at installation. */
#include "mine_award_controller.h"

extern s32 func_80012B04(s32 current, s32 target, s32 shift);
extern void func_80171A1C(u8 *actor);
extern void func_80174650(u8 *actor);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define MODEL(actor) (*(u8 **)((actor) + 0x20))
#define ANGLE(model) (*(s16 *)((model) + 0x12))
#define STORE_ANGLE(model, angle) (*(u16 *)((model) + 0x12) = (angle))
#else
#include <string.h>
static u8 *current_model(const u8 *actor)
{
    u8 *model;
    memcpy(&model, actor + 0x20, sizeof model);
    return model;
}
static s16 read_angle(const u8 *model)
{
    return (s16)(model[0x12] | ((u16)model[0x13] << 8));
}
static void store_angle(u8 *model, u16 angle)
{
    model[0x12] = (u8)angle;
    model[0x13] = (u8)(angle >> 8);
}
#define MODEL(actor) current_model(actor)
#define ANGLE(model) read_angle(model)
#define STORE_ANGLE(model, angle) store_angle((model), (angle))
#endif

void func_801741EC(u8 *actor)
{
    s32 step = func_80012B04(ANGLE(MODEL(actor)), (s16)D_80126B3C, 4);
    u8 *model;
    if (!(s16)step) {
        D_8011F9C8 = 1;
        func_80171A1C(actor);
        func_80174650(actor);
    }
    model = MODEL(actor);
    STORE_ANGLE(model, ((u32)(u16)ANGLE(model) + (u32)step) & 0xfff);
}
