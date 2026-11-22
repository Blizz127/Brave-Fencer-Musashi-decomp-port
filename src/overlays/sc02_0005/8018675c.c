/* Consume a crossed route marker and signal the body's movement threshold. */
#include "steam_knight_limits.h"

s32 func_8018675C(SteamKnightPart *body)
{
#if defined(__mips__) && !defined(__LP64__)
    /* The original leaf reserves eight unreferenced stack bytes. */
    u8 frame[8];
    (void)frame;
#endif
    if (body->phase_flags & 0x800) {
        s16 index = body->field_e0;
        if (index >= 0) {
#if defined(__mips__) && !defined(__LP64__)
            s32 position;
            s32 threshold;
            s32 offset = index * sizeof(SteamKnightMarker);
            const SteamKnightMarker *base;

            position = POS_X(body);
            threshold = MARKER(index).threshold;

            /* Keep the original comparison loads ahead of the indexed row address. */
            __asm__ __volatile__("" ::: "memory");
            base = D_80195AF4;
            __asm__("" : "=r"(base) : "0"(base));
            if (position < threshold) {
                const SteamKnightMarker *row = (const SteamKnightMarker *)((const u8 *)base + offset);
                s32 value = row->value;
                s32 consumed;
                COUNT++;
                consumed = -1;
                OBSERVED_VALUE = value;
                __asm__ __volatile__("" ::: "memory");
                body->field_e0 = consumed;
            }
#else
            u32 marker = 0x80195af4u + (u32)index * 8u;
            if (POS_X(body) < tower_signed_half(bfm_tower_spawn_pointer(marker))) {
                void *counter = bfm_tower_spawn_pointer(0x801e43b4u);
                u16 count = tower_unsigned_half(counter);
                s16 value = tower_signed_half(bfm_tower_spawn_pointer(marker + 6u));
                tower_set_half(counter, (u16)(count + 1u));
                tower_set_word(bfm_tower_spawn_pointer(0x801270c8u), value);
                body->field_e0 = -1;
            }
#endif
        }
        if (body->motion_x < 0 && POS_X(body) < LEFT_EDGE) {
#if defined(__mips__) && !defined(__LP64__)
            body->phase_flags |= 8;
            body->break_count = 0;
#else
            u32 flags = body->phase_flags | 8u;
            ((volatile SteamKnightPart *)body)->break_count = 0;
            ((volatile SteamKnightPart *)body)->phase_flags = flags;
#endif
            return 1;
        }
    } else if (body->motion_z > 0) {
        u16 round = ROUND_INDEX;
        if (Z_LIMIT(round) < body->field_0e) {
            if (round == 2)
                body->phase_flags |= 0x400;
            else
                body->phase_flags |= 8;
            body->break_count = 0;
            return 1;
        }
    }
    return 0;
}
