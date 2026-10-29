/* Owner re-derivation from retail assembly (clean input set). */
/* Counts an object's timer at +0x1C8 down while the game mode byte is 6, and
 * fires event 0x3B once the timer has reached zero in that mode. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x48];
    u8 mode; /* 0x48; low 7 bits are the mode */
} GameState;

typedef struct {
    u8 pad[0x1C8];
    s16 timer; /* 0x1C8 */
} TimerObj;

extern GameState D_80078E78;
extern u8 D_80078EC0; /* same byte as D_80078E78.mode */
extern void func_8014AC10(s32 id);

void func_80161A90(TimerObj *obj)
{
    GameState *gs = &D_80078E78;
    s32 counting;

    if (obj->timer == 0) {
        counting = 0;
    } else {
        counting = (D_80078EC0 & 0x7F) == 6;
    }
    if (counting) {
        obj->timer--;
    }
    if ((gs->mode & 0x7F) == 6 && obj->timer == 0) {
        func_8014AC10(0x3B);
    }
}
