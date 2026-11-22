/* Forest player predispatch and frame-state initialization. */
#include "forest_field_update.h"

s32 func_80146128(void)
{
    ForestFieldPlayer *obj;
    ForestFieldModel *sub;

    obj = &D_80126B58;
    sub = FIELD_MODEL(D_80126B78);
    func_80146FC4(obj);
    D_801150F0 = D_80126BE0;
    func_80150A70(obj);
    func_80147098(obj);
    func_8014A638(obj);
    D_8011F730 = 0;
    D_801152B8 = 0;
    D_8012693A = 0;
    if (D_80078EC1 == 0x10 && D_80078EC8 != 0) {
        *(s16 *)((u8 *)sub + 2) = 6;
        D_80126B9C |= 0x800;
    } else {
        sub->mode = 1;
        obj->flags &= ~0x800;
    }
    if (obj->timer != 0) {
        obj->timer--;
    }
    if (func_80155458(obj) == 0 && !(obj->flags & 0x4000000)) {
        if (func_80029104() != 0) {
            func_8014ADE0(obj);
            func_8014B350(obj);
            func_8014B7A4(obj);
            func_80161D58(obj);
        }
        func_80029344();
        func_80161A90(obj);
        func_8014B504(obj);
        func_80149BEC(obj);
        func_8014B5D0(obj);
        func_8014C99C(obj);
        func_8014B190(obj);
        func_80148648(obj, 0);
        func_80149228(obj);
        func_8014A59C(obj);
        func_8016F14C(obj);
        func_80154418(obj);
        func_80154BE4(obj);
        func_80165694(obj);
        func_801654A8(obj);
        func_8014A680(obj);
        func_8014A6A8(obj);
        func_8014A71C(obj);
        func_80172588(obj);
        func_801473DC(obj);
        func_80015978(obj->pos, obj->spos);
        return 1;
    }
    func_80015978(obj->pos, obj->spos);
    return 0;
}
