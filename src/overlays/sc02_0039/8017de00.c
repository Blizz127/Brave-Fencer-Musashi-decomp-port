/* Spawn each of the four linked mine platform actors. */
#include "mine_rotor_spawn.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#include "mine_actor_spawn.h"

typedef struct {
    u8 body[0x70];
    s16 option;
} MineRotor;
extern u16 D_8019B538[][4];

void func_8017DE00(Obj8014A2E4 *entity)
{
    MineRotor *rotor = (MineRotor *)entity;
    s32 i;
    for (i = 0; i < 4; i++) {
        s32 offset = i << 10;
        s16 options = D_8019B538[rotor->option][i] + offset;
        func_8012C658(0x199, options, rotor);
    }
}
#else
static s16 signed_half(u16 raw)
{
    s32 value = raw;
    return (s16)(value <= 32767 ? value : value - 65536);
}

int mine_rotor_spawn_children(Obj8014A2E4 *actor, const MineRotorSpawn *services)
{
    if (!actor || !services || !services->read_option || !services->spawn)
        return 0;

    const u8 *bytes = (const u8 *)actor;
    for (unsigned i = 0; i < 4; ++i) {
        u16 row = (u16)(bytes[0x70] | (u16)bytes[0x71] << 8);
        u16 cell;
        if (!services->read_option(services->context, signed_half(row), i, &cell))
            return 0;
        u16 packed = (u16)(cell + (i << 10));
        services->spawn(services->context, 0x199, signed_half(packed), actor);
    }
    return 1;
}
#endif
