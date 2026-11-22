#include "steam_knight_hit.h"

extern s32 D_801E43A0;
extern void func_80186F9C(SteamKnightPart *body, int joint);

void func_80186DE8(SteamKnightPart *body)
{
    u32 mask;
    s16 count = (s16)body->field_fc;
    if (count) {
        body->field_fc = --count;
        if (body->phase_flags & 0x40) {
            if (count == 0) {
                body->phase_flags &= ~0x40u;
                body->field_fc = 8;
            }
        } else if ((body->phase_flags & 0x200) || ((mask = D_801E43A0) & 0x1540) == 0x1540) {
            body->field_fc = 0;
        } else if (count == 0) {
            body->phase_flags |= 0x40;
            body->field_fc = 12;
            if (!(mask & 0x40)) func_80186F9C(body, 5);
            if (!(D_801E43A0 & 0x100)) func_80186F9C(body, 10);
            if (!(D_801E43A0 & 0x400)) func_80186F9C(body, 15);
            if (!(D_801E43A0 & 0x1000)) func_80186F9C(body, 20);
            func_8002D4C8(1264, 0);
        }
    }
}
