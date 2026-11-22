/* Pair a shared model resource with a Tower room record. */
#include "steam_knight_resource.h"

#if !defined(__mips__) || defined(__LP64__)
extern u32 bfm_tower_spawn_address(const void *pointer);
#endif

u8 *func_8012913C(u16 kind)
{
    u16 *resource = func_8001D074(126, 256);
    u8 *record;

    if (!resource)
        return 0;
    record = func_801291C0();
    if (!record)
        return 0;
#if defined(__mips__) && !defined(__LP64__)
    *(u16 *)record = kind;
    *(u32 *)(record + 0x20) = (u32)resource;
#else
    {
        u32 address;
        record[0] = (u8)kind;
        record[1] = (u8)(kind >> 8);
        address = bfm_tower_spawn_address(resource);
        record[0x20] = (u8)address;
        record[0x21] = (u8)(address >> 8);
        record[0x22] = (u8)(address >> 16);
        record[0x23] = (u8)(address >> 24);
    }
#endif
    func_8001CC3C((TowerJointModelResource *)resource, 0, 0, 0);
    return record;
}
