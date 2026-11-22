#include "steam_knight_effect.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
/* Keep one transfer complete before fetching the next field. On the guest
 * the input descriptor can share memory with the selected pool slot. */
#define READ_EFFECT_FIELD(field) ({ __asm__ volatile ("" ::: "memory"); request->field; })
#else
#include <stddef.h>
#include <string.h>
/* Read each field through its byte representation, which permits overlap
 * with a formerly free slot. Each read has its own independent temporary. */
#define READ_EFFECT_FIELD(field) (memcpy(&value.field, (const u8 *)request + \
    offsetof(SteamKnightEffectRequest, field), sizeof value.field), value.field)
#endif

extern SteamKnightEffect D_8011D030[30];

SteamKnightEffect *func_80146AFC(const SteamKnightEffectRequest *request)
{
    SteamKnightEffect *slot;
#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
    SteamKnightEffectRequest value;
#endif

    for (slot = D_8011D030; slot < D_8011D030 + 30; ++slot) {
        if (!slot->kind) {
            slot->owner_address = READ_EFFECT_FIELD(owner_address);
            slot->kind = READ_EFFECT_FIELD(kind);
            slot->x = READ_EFFECT_FIELD(x);
            slot->y = READ_EFFECT_FIELD(y);
            slot->z = READ_EFFECT_FIELD(z);
            slot->mode = READ_EFFECT_FIELD(mode);
            slot->amount = READ_EFFECT_FIELD(amount);
            return slot;
        }
    }
    return 0;
}
