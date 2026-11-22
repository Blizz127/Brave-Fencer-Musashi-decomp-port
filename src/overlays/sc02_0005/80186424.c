/* Keep the tracked player position relative to a moving armor attachment. */
#include "steam_knight_hit.h"

extern void func_8012F0BC(u32 matrix_address, s32 *world, s32 *local);
extern void func_8012F1A4(u32 matrix_address, s32 *local, s32 *world);

#if defined(__mips__) && !defined(__LP64__)
extern void func_80132784(SteamKnightPart *part, SteamKnightPart *parent, int index);
#define ATTACH(part) func_80132784((part), (SteamKnightPart *)(part)->body_address, (part)->field_fc)
extern volatile s32 D_80126B5C, D_80126B60, D_80126B64;
extern s16 D_80126B5E, D_80126B62, D_80126B66;
typedef struct {
    u8 pad00[8];
    s16 x, y, z;
    u8 pad0e[0x3a];
    s32 tx, ty, tz;
} TrackedModel;
extern TrackedModel *volatile D_80126B78;
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_steam_knight_core_attach(SteamKnightPart *part, u32 parent_address, int index);
#define ATTACH(part) bfm_steam_knight_core_attach((part), (part)->body_address, (part)->field_fc)

/* A coordinate write can change the model handle used by the next axis. */
static void store_axis(u32 coordinate_address, unsigned model_offset, unsigned translation_offset)
{
    u32 model_address;
    s16 position;
    s32 translation;
    u8 *model;
    memcpy(&model_address, bfm_tower_spawn_pointer(0x80126B78u), sizeof model_address);
    memcpy(&position, bfm_tower_spawn_pointer(coordinate_address), sizeof position);
    model = bfm_tower_spawn_pointer(model_address);
    translation = position;
    memcpy(model + model_offset, &position, sizeof position);
    memcpy(model + translation_offset, &translation, sizeof translation);
}
#endif

void func_80186424(SteamKnightPart *part)
{
    s32 world[3], updated[3], local[3];
#if defined(__mips__) && !defined(__LP64__)
    TrackedModel *first;
    register TrackedModel *model __asm__("$3");
    s16 high;
    s32 x, y, z;
#endif
    if (((u8 *)part)[0x74]) {
#if defined(__mips__) && !defined(__LP64__)
        world[0] = D_80126B5C;
        world[1] = D_80126B60;
        world[2] = D_80126B64;
#else
        memcpy(world, bfm_tower_spawn_pointer(0x80126B5Cu), sizeof world);
#endif
        func_8012F0BC(part->model_address + 0x34u, world, local);
        ATTACH(part);
        func_8012F1A4(part->model_address + 0x34u, local, updated);
#if defined(__mips__) && !defined(__LP64__)
        x = updated[0];
        y = updated[1];
        z = updated[2];
        /* The return vector is captured before touching overlapping game views. */
        __asm__ volatile ("" : : "r"(x), "r"(y), "r"(z) : "memory");
        first = D_80126B78;
        D_80126B5C = x;
        __asm__ volatile ("" : : : "memory");
        high = D_80126B5E;
        D_80126B60 = y;
        D_80126B64 = z;
        first->tx = first->x = high;
        __asm__ volatile ("" : : : "memory");
        model = D_80126B78;
        model->ty = model->y = D_80126B62;
        model = D_80126B78;
        model->tz = model->z = D_80126B66;
#else
        memcpy(bfm_tower_spawn_pointer(0x80126B5Cu), updated, sizeof updated);
        store_axis(0x80126B5Eu, 8, 0x48);
        store_axis(0x80126B62u, 10, 0x4c);
        store_axis(0x80126B66u, 12, 0x50);
#endif
    } else {
        ATTACH(part);
    }
}
