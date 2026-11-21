/* Construct a marker, five wallet digits and a terminal zero. Each GPU tag
 * links to the preceding 20-byte packet via its low 24 guest address bits. */
#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define HUD_MATCHING_TARGET
#define HUD_ADDRESS(p) ((u32)(p))
#define HUD_WORD(p, i, v) ((p)[i] = (v))

u32 *func_80177EA4(u32 *packet, s32 packed, u16 x, s32 y)
#else
static void write_hud_word(u8 *packet, unsigned i, u32 value)
{
    u8 *field = packet + 4u * i;
    field[0] = (u8)value;
    field[1] = (u8)(value >> 8);
    field[2] = (u8)(value >> 16);
    field[3] = (u8)(value >> 24);
}
#define HUD_ADDRESS(p) (guest_address)
#define HUD_WORD(p, i, v) write_hud_word((p), (i), (v))

/* The caller supplies a 140-byte host buffer and its guest packet address.
 * The host buffer is byte-addressed and may start at any alignment. */
u8 *musashi_wallet_hud_packets(u8 *packet, u32 guest_address, s32 packed, u16 x, s32 y)
#endif
{
    s16 i, glyph;
    u32 hidden, row;
    s16 visible_glyph;

#ifndef HUD_MATCHING_TARGET
    if (!packet || (guest_address & 3u) || guest_address > 0xffffffffu - 140u)
        return 0;
#endif
    hidden = 0x01000000u;
    for (i = 0; i < 7; ++i) {
#ifdef HUD_MATCHING_TARGET
        /* The old compiler counts these discarded expressions when deciding
         * which constants to move outside the loop. */
        glyph = packed >> 1;
        glyph = packed >> 2;
        glyph = packed >> 3;
#endif
        HUD_WORD(packet, 0, ((HUD_ADDRESS(packet) - 20u) & 0xffffffu) | 0x03000000u);
        row = (u32)y << 16;
        HUD_WORD(packet, 1, 0x74808080u);
        if (i != 0) {
            if (i == 6) {
                glyph = 1;
                hidden = 0;
            } else {
                glyph = ((packed >> 16) & 15) + 1;
                packed = (s32)((u32)packed << 4);
            }
            visible_glyph = glyph;
            if (visible_glyph != 1)
                hidden = 0;
            HUD_WORD(packet, 2, row | x | hidden);
        } else {
            glyph = 0;
            HUD_WORD(packet, 2, row | x);
        }
        HUD_WORD(packet, 3, (u32)(glyph << 3) | 0x6b564000u);
#ifdef HUD_MATCHING_TARGET
        packet += 5;
#else
        packet += 20;
        guest_address += 20u;
#endif
        x += 8;
    }
    return packet;
}
