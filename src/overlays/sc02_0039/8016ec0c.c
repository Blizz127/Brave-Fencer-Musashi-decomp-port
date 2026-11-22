/* Submit a backdrop and the current slot's glyphs. Accent marks share a column. */
#include "mine_hud_sprite.h"

void func_8016EC0C(MineFrameSlot *slot, s32 shade)
{
    MineHudSprite sprite;
    MineHudGlyph *glyph;
    s32 x, y;

    MINE_SPRITE_FRAME(slot, sprite);
    glyph = (MineHudGlyph *)(slot->reserved + 6);
    sprite.b = shade;
    sprite.g = shade;
    sprite.r = shade;
    sprite.palette = D_801824A8[slot->reserved[1]];
    sprite.u = 1600;
    sprite.v = 456;
    sprite.flags = 0;
    sprite.width = 40;
    sprite.height = 8;
    x = mine_sprite_half(slot->reserved + 2);
    sprite.x = x + 16;
    y = mine_sprite_half(slot->reserved + 4);
    sprite.y = y;
    sprite.depth = 16;
    func_80017B98(&sprite);

    sprite.width = 8;
    y += 10;
    while (mine_hud_glyph(glyph) != 0xffff) {
        switch (mine_hud_glyph(glyph)) {
        case 0x1858:
        case 0x1850:
            sprite.x = x + 4;
            sprite.y = y - 7;
            break;
        case 0x3872:
            x += 2;
            MINE_GLYPH_ADVANCE(glyph);
            continue;
        default:
            sprite.x = x;
            sprite.y = y;
            x += 8;
        }
        sprite.u = 1280 + *(u8 *)glyph;
        sprite.v = 256 + (mine_hud_next_glyph(glyph) >> 8);
        func_80017B98(&sprite);
    }
}
