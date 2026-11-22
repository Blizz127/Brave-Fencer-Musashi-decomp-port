#ifndef BFM_MINE_HUD_SPRITE_H
#define BFM_MINE_HUD_SPRITE_H

#include "mine_frame_slots.h"

typedef struct {
    u16 x, y;
    s32 depth;
    u16 width, height, u, v;
    u8 r, g, b, reserved;
    s32 flags;
    u8 palette;
    u8 padding[3];
} MineHudSprite;

/* The original GCC keeps the descriptor address live when it starts the slot.
 * The constraint emits no instruction. Original slot fields are aligned. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
typedef u16 MineHudGlyph;
#define mine_hud_glyph(p) (*(p))
#define mine_hud_next_glyph(p) (*(p)++)
#define MINE_GLYPH_ADVANCE(p) (++(p))
#define MINE_SPRITE_FRAME(slot, sprite) __asm__("" : "=r"(slot) : "0"(slot), "r"(&(sprite)))
#define mine_sprite_half(p) (*(u16 *)(p))
#else
typedef u8 MineHudGlyph;
#define MINE_GLYPH_ADVANCE(p) ((p) += 2)
#define mine_hud_glyph(p) mine_sprite_half(p)
#define mine_hud_next_glyph(p) mine_sprite_next_glyph(&(p))
#define MINE_SPRITE_FRAME(slot, sprite) ((void)0)
static __inline__ u16 mine_sprite_half(const u8 *p)
{
    return (u16)(p[0] | ((u16)p[1] << 8));
}
static __inline__ u16 mine_sprite_next_glyph(u8 **p)
{
    u16 value = mine_sprite_half(*p);
    *p += 2;
    return value;
}
#endif

extern u8 D_801824A8[];
void func_80017B98(MineHudSprite *sprite);

#endif
