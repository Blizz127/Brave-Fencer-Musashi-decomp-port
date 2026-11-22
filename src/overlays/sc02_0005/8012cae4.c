/* Release a Tower actor's model and clear the 268-byte actor slot. */
#include "steam_knight_actor_dispose.h"

extern u16 D_801274E4[];

#if defined(__mips__) && !defined(__LP64__)
extern void func_80016714(s8 *data, s32 size);
#define MODEL(actor) (*(u8 **)((actor) + 0x20))
#define KIND(model) (*(u16 *)(model))
#define CLEAR(data, count) func_80016714((s8 *)(data), (count))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void func_80016714_native(u8 *data, s32 size);

static u8 *model_pointer(const u8 *actor)
{
    u32 packed;
    memcpy(&packed, actor + 0x20, sizeof(packed));
    return packed ? bfm_tower_spawn_pointer(packed) : 0;
}

static u16 model_kind(const u8 *model)
{
    u16 kind;
    memcpy(&kind, model, sizeof(kind));
    return kind;
}

#define MODEL(actor) model_pointer(actor)
#define KIND(model) model_kind(model)
#define CLEAR(data, count) func_80016714_native((data), (count))
#endif

s32 func_8012CAE4(u8 *actor)
{
    u8 *model;
    D_801274E4[0] |= 1;
    model = MODEL(actor);
    if (model) {
        u16 kind = KIND(model);
        if (kind != 1) {
            if (kind == 2) {
                CLEAR(model, 0x38);
            }
        } else {
            CLEAR(model, 0x84);
        }
    }
    CLEAR(actor, 0x10c);
#if defined(__mips__) && !defined(__LP64__)
    {
        /* The original forwards its zero-fill's final result register. */
        register s32 result __asm__("$2");
        __asm__ volatile ("" : "=r"(result) : : "memory");
        return result;
    }
#else
    /* The final 268-byte clear always leaves the guest result at -1. */
    return -1;
#endif
}
