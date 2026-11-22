#include "psx_types.h"

typedef struct {
    u8 reserved00[2];
    u16 state;
    u8 reserved04[0x58 - 0x04];
    u32 tagged_script;
    u8 reserved5c[0x10a - 0x5c];
    s16 permission_started;
} GuardPermissionActor;

extern u8 func_800291B4(s32 slot);
extern void func_800291A0(s32 slot, s32 value);
extern void func_80178CBC(GuardPermissionActor *actor, const u8 *script);

extern const u8 D_80189550[];
extern const u8 D_8018959C[];
extern const u8 D_80189614[];

void func_8017FBF8(GuardPermissionActor *actor)
{
    const u8 *script;

    if (func_800291B4(0x40) == 1) {
        func_800291A0(0x40, 2);
    }

    if (actor->permission_started == 0) {
        script = D_8018959C;
        actor->permission_started = 1;
        actor->tagged_script = (u32)D_80189550 | 0x40000000;
    } else {
        script = D_80189614;
    }
    func_80178CBC(actor, script);
    actor->state++;
}
