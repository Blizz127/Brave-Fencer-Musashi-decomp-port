/* Count the displayed frames preceding the actor's script cursor. */
#include "../../../include/forest_animation_commands.h"

s32 func_801549F8(ForestPlayerState *actor)
{
    u32 frame = 0;
    u32 cursor = 0;

    while (cursor != actor->animation_cursor) {
        /* The two-word commands consume their operand without counting a frame. */
        switch (ANIMATION_WORD(actor->animation, cursor)) {
        case 0: case 2: case 7: case 8: case 9: case 10: case 13:
            ++cursor;
            break;
        case 3: case 4: case 5: case 6: case 11: case 12:
            cursor += 2;
            break;
        case 16: case 17: case 18: case 19: case 20: case 25: case 26:
            ++cursor;
            break;
        case 14: case 15: case 21: case 22:
            cursor += 2;
            break;
        default:
            ++cursor;
            ++frame;
            break;
        }
    }
    return (u8)frame;
}
