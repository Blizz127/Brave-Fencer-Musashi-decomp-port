/* Apply camera-relative input to kind four's movement and resolve contact. */
#include "kind4_action.h"

s32 func_8015C128(ForestPlayerState *actor)
{
    s32 impulse[3], rotated[3];
    s32 floor_result, contact;
#if defined(__mips__) && defined(__GNUC__) && (__GNUC__ == 2)
    register s32 result __asm__("$2");
#else
    s32 result;
#endif
    func_80149FB0(actor);
    if (func_80148AFC(actor) & 0xff) {
        impulse[0] = 0;
        impulse[1] = 0;
        impulse[2] = -0x4000;
        func_8001382C(PLAYER_MODEL_YAW(actor), impulse, rotated);
        actor->step += (u32)rotated[0];
        actor->counter += (u32)rotated[1];
        actor->field_23c += (u32)rotated[2];
    }
    func_8001382C((s16)-(u16)PLAYER_MODEL_YAW(actor), &actor->step, rotated);
    func_80146DB8((s32 *)actor, rotated);
    func_80147A84(actor);
    func_801473EC(actor);
    if (!(D_800B99DA & 3)) {
        func_8014C010((s32)FOREST_ACTOR_GAME_ADDRESS(actor), 1);
        func_80147324(0x65f);
    }
    if (func_8014D738(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        return func_8015C6E0(actor);
    }
    floor_result = func_8014CC28(actor);
    contact = func_8014F3E8(actor);
    result = contact;
    if (contact) {
        if ((contact & 0xff00) != 0x4000) {
            func_80146E90(actor, 6);
            return func_80146CA0(actor);
        }
        result = contact & 0x4000;
        if (result) {
            result = func_80146E98(actor);
            if (result) {
                func_80147078(actor, 4);
                return func_8015C0C4(actor);
            }
        }
    } else if (!floor_result) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80147078(actor, 3);
        return func_8015BE94(actor);
    }
    return result;
}
