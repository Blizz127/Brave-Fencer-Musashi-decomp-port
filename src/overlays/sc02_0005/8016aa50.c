#include "steam_knight_hit.h"
#include "steam_knight_effect.h"

extern SteamKnightEffect D_8011D030[];
extern s16 D_80126B5E;
extern s16 D_80126B62;
extern s16 D_80126B66;
extern void func_80146C3C(SteamKnightEffect *slot);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define IMPACT_COORD(part, axis) (((const s16 *)(part)->hit_position.bytes)[axis])
#define EFFECT_OWNER(part) ((u32)(part))
#else
/* The owning actor context supplies the serialized game identity of this part. */
extern u32 bfm_steam_knight_effect_owner_address(const SteamKnightPart *part);
#define EFFECT_OWNER(part) bfm_steam_knight_effect_owner_address(part)

static s16 impact_coord(const SteamKnightPart *part, unsigned axis)
{
    const u8 *p = part->hit_position.bytes + 2 * axis;
    return (s16)(p[0] | ((u16)p[1] << 8));
}
#define IMPACT_COORD(part, axis) impact_coord(part, axis)
#endif

void func_8016AA50(SteamKnightPart *part, int damage)
{
    s16 index;
    SteamKnightEffect *effect;

    if (damage >= 0) {
        int kind = 29;
        int x = IMPACT_COORD(part, 0);
        int y = IMPACT_COORD(part, 1);
        int z = IMPACT_COORD(part, 2);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
        /* Capture all three signed coordinates before passing the owner address. */
        __asm__("" : "=r"(part), "=r"(kind)
                   : "0"(part), "1"(kind), "r"(x), "r"(y), "r"(z) : "memory");
#endif
        func_80146A6C(kind, EFFECT_OWNER(part), x, y, z, 0, damage);
    } else {
        index = 0;
        for (effect = D_8011D030; index < 30; ++index, ++effect) {
            if (!effect->kind || effect->kind == 3 || effect->kind == 9) {
                func_80146C3C(effect);
                func_80146A6C(29, EFFECT_OWNER(part), D_80126B5E,
                              (s16)(D_80126B62 - 32), D_80126B66, 7, 0);
                return;
            }
        }
    }
}
