#include "steam_knight_hit.h"

extern s16 D_801E4398;
extern u16 D_801E4BBC;
extern void func_8018415C(SteamKnightPart *core);
extern void func_8012B23C(s32 *actor);
extern void func_80186F58(SteamKnightPart *body);
extern void func_80184C20(SteamKnightPart *part);
extern void func_8012AD44(u8 *actor, u16 state);
extern void func_80186304(SteamKnightPart *body, int a, int b);
extern void func_80186BB8(SteamKnightPart *body);
extern void func_80186D00(void);
extern void func_801900F4(void);
extern void func_80184184(SteamKnightPart *core);
extern void func_8018AAAC(void);
extern void func_8017DF50(void);
extern s16 func_80186144(SteamKnightPart *body, int mode);
extern void func_80186ADC(SteamKnightPart *body);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80195B3C[];
typedef struct {
    u8 pad[18];
    u16 y;
} BodyModel;
#define BODY_ACTOR(address) ((SteamKnightPart *)(address))
#define BODY_DISPLAY ((u32)D_80195B3C | 0x40000000)
#define READ_TIMER(body) (*(s16 *)((u8 *)(body) + 0xDE))
#define WRITE_TIMER(body, value) (READ_TIMER(body) = (value))
#define WRITE_MODE(body, value) (*(s16 *)((u8 *)(body) + 0xDC) = (value))
#define RESET_MODEL_Y(body) (((BodyModel *)BODY_ACTOR((body)->linked_address)->model_address)->y = 0)
#else
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
extern void *bfm_tower_spawn_pointer(u32 address);
#define BODY_ACTOR(address) bfm_steam_knight_break_actor(address)
#define BODY_DISPLAY 0xC0195B3Cu
#include <string.h>

static s16 read_timer(const SteamKnightPart *body)
{
    s16 timer;
    memcpy(&timer, (const u8 *)body + 0xDE, sizeof(timer));
    return timer;
}

static void write_body_half(SteamKnightPart *body, unsigned offset, s16 value)
{
    memcpy((u8 *)body + offset, &value, sizeof(value));
}

#define READ_TIMER(body) read_timer(body)
#define WRITE_TIMER(body, value) write_body_half(body, 0xDE, value)
#define WRITE_MODE(body, value) write_body_half(body, 0xDC, value)
static void reset_model_y(SteamKnightPart *body)
{
    SteamKnightPart *linked = BODY_ACTOR(body->linked_address);
    u8 *model = bfm_tower_spawn_pointer(linked->model_address);
    model[18] = 0;
    model[19] = 0;
}
#define RESET_MODEL_Y(body) reset_model_y(body)
#endif

void func_80183218(SteamKnightPart *body)
{
    s16 timer;
    if (body->state && (body->phase_flags & 0x10)) {
        body->phase_flags &= ~0x10u;
        D_801E4398 = body->hp -= body->damage;
        if (body->hp < 0) {
            body->hp = D_801E4398 = 0;
        }
        body->damage = 0;
        func_8002D4C8(0x4F3, 0);
        if (body->hp == 0) {
            body->phase_flags &= ~0xE0000u;
            func_8018415C(BODY_ACTOR(body->core_address));
            if (body->phase_flags & 8) {
                func_8012B23C((s32 *)body);
                body->phase_flags &= ~1u;
                func_80186F58(body);
                RESET_MODEL_Y(body);
                func_80184C20(BODY_ACTOR(body->secondary_address));
                body->display_record = BODY_DISPLAY;
                body->flags = 0x0A00;
                body->phase_flags |= 4;
                func_8012AD44((u8 *)body, 8);
                func_80186304(body, 10, 2);
                func_80186BB8(body);
                func_80186D00();
                switch (D_801E4BBC) {
                case 1:
                    func_801900F4();
                    break;
                case 2:
                    func_80184184(BODY_ACTOR(body->core_address));
                    body->flags = 0x800;
                    break;
                }
            } else {
                body->phase_flags &= ~1u;
                RESET_MODEL_Y(body);
                func_80184C20(BODY_ACTOR(body->secondary_address));
                func_8018AAAC();
                func_80186F58(body);
                func_8012B23C((s32 *)body);
                body->motion_z = 0xA8000;
                WRITE_MODE(body, 2);
                body->motion_y = -0xC0000;
                func_80186304(body, 12, 3);
                func_8012AD44((u8 *)body, 10);
                if (D_801E4BBC == 2) {
                    func_8017DF50();
                    if (body->phase_flags & 0x800) {
                        body->motion_x = 0u - (u32)body->motion_z;
                        body->motion_z = 0;
                        body->field_e0 = func_80186144(body, 5);
                    }
                }
                func_80186BB8(body);
                func_8002D4C8(0x50F, 0);
            }
        }
    }
    timer = READ_TIMER(body);
    if (timer) {
        --timer;
        WRITE_TIMER(body, timer);
        if (!timer) func_80186ADC(body);
    }
}
