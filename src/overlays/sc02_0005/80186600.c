#include "steam_knight_hit.h"

extern s16 D_801E43B8;
extern void func_80186F9C(SteamKnightPart *body, int joint);
extern s32 func_8012B864(SteamKnightPart *body);
extern s32 func_8012B608(s32 current, s32 target, s32 step);

#if defined(__mips__) && !defined(__LP64__)
#define GET_PART(key) ((SteamKnightPart *)(key))
#define READ_HALF(base, off) (*(s16 *)((u8 *)(base) + (off)))
#define WRITE_HALF(base, off, value) (READ_HALF(base, off) = (value))
#define MODEL(part) ((u8 *)(part)->model_address)
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
#define GET_PART(key) ((SteamKnightPart *)bfm_tower_spawn_pointer(key))
#define MODEL(part) ((u8 *)bfm_tower_spawn_pointer((part)->model_address))
static s16 read_half(const void *base, unsigned offset)
{
    s16 value;
    memcpy(&value, (const u8 *)base + offset, sizeof value);
    return value;
}
static void write_half(void *base, unsigned offset, s16 value)
{
    memcpy((u8 *)base + offset, &value, sizeof value);
}
#define READ_HALF(base, off) read_half(base, off)
#define WRITE_HALF(base, off, value) write_half(base, off, value)
#endif

void func_80186600(SteamKnightPart *body)
{
    u32 phase = body->phase_flags;
    u32 linked = body->linked_address;
    s32 desired, delta;
    u8 *model;
    s16 yaw;
    D_801E43B8 = 0;
    if (phase & 1) {
        if (phase & 0x200) {
            s16 timer = READ_HALF(body, 0xe6);
            if (timer >= 85) {
                if (READ_HALF(body, 0xe4)) {
                    u8 *turn_model;
                    u16 remaining;

                    turn_model = MODEL(GET_PART(linked));
                    WRITE_HALF(turn_model, 0x12, READ_HALF(turn_model, 0x12) + 256);
                    remaining = (u16)READ_HALF(body, 0xe4);
                    D_801E43B8 = 256;
                    WRITE_HALF(body, 0xe4, remaining - 1);
                } else {
                    body->phase_flags = phase & ~0x300u;
                    WRITE_HALF(body, 0xe6, 0);
                }
            } else {
                if (timer < 72) {
                    if (!(timer & 7)) {
                        func_80186F9C(body, 22);
                        func_80186F9C(body, 25);
                        func_8002D4C8(1264, 0);
                    }
                } else if (timer == 84) {
                    body->phase_flags = phase | 0x100;
                    func_8002D4C8(1260, 0);
                }
                WRITE_HALF(body, 0xe6, READ_HALF(body, 0xe6) + 1);
            }
        } else {
            desired = func_8012B864(body);
            delta = func_8012B608(READ_HALF(MODEL(GET_PART(linked)), 0x12) + READ_HALF(MODEL(body), 0x12), desired, 8);
            model = MODEL(GET_PART(linked));
            yaw = READ_HALF(model, 0x12);
            D_801E43B8 = delta;
            WRITE_HALF(model, 0x12, yaw + (u16)delta);
        }
    }
}
