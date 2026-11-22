#include "steam_knight_particle.h"

extern SteamKnightParticleTransform D_800AE620;
extern u16 D_800B99DC, D_800B99D8;
extern SteamKnightParticleLaunch D_80196874[];
extern void func_8004A0DC(u16 angle, SteamKnightParticleTransform *transform);
extern SteamKnightPart *func_8012C658(int kind, u32 selector, int mode);
extern void func_8012B23C(s32 *child);
extern void func_800484EC(SteamKnightParticleTransform *transform, SteamKnightParticleLaunch *launch, void *velocity);

#if defined(__mips__) && !defined(__LP64__)
#define OFFSET(p, axis) (((const u16 *)(p)->bytes)[axis])
#define MODEL(actor) (*(u8 **)((u8 *)(actor)+0x20))
#define POSITION(actor, off) (*(s32 *)(MODEL(actor)+(off)))
#define SET_COORD(actor, off, value) (*(u16 *)((u8 *)(actor)+(off))=(value))
#else
#include <string.h>
static u16 read_offset(const SteamKnightParticlePosition *p, unsigned axis)
{
    const u8 *bytes = p->bytes + 2*axis;
    return bytes[0] | (u16)(bytes[1] << 8);
}
#define OFFSET(p, axis) read_offset((p), (axis))
extern const u8 *bfm_steam_knight_particle_model(u32 address);
static s32 model_coord(const SteamKnightPart *actor, unsigned offset)
{
    u32 address;
    s32 value;
    memcpy(&address, (const u8 *)actor + 0x20, sizeof address);
    memcpy(&value, bfm_steam_knight_particle_model(address) + offset, sizeof value);
    return value;
}
#define POSITION(actor, off) model_coord((actor), (off))
static void set_coord(SteamKnightPart *actor, unsigned off, u16 value)
{
    memcpy((u8 *)actor + off, &value, sizeof value);
}
#define SET_COORD(actor, off, value) set_coord((actor), (off), (u16)(value))
#endif

void func_8018B16C(SteamKnightPart *part, int count, SteamKnightParticlePosition *offset)
{
    SteamKnightParticleTransform transform = D_800AE620;
    SteamKnightPart *child;
    int i;
    u16 *angle = &D_800B99DC;
    func_8004A0DC(*angle, &transform);
    *angle += D_800B99D8;
#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
    /* The original checks the signed count, then compares byte offsets
     * against count shifted left four bits in a 32-bit register. */
    if (count > 0) {
        u32 limit = (u32)count << 4;
        count = limit != 0 && limit < 0x80000000u ? (int)(limit >> 4) : 1;
    }
#endif
    for (i=0; i<count; ++i) {
        child = func_8012C658(84, D_80196874[i].selector, 0);
        if (child) {
            SET_COORD(child, 6, (u32)POSITION(part, 0x48) + OFFSET(offset, 0));
            SET_COORD(child, 10, (u32)POSITION(part, 0x4C) + OFFSET(offset, 1));
            SET_COORD(child, 14, (u32)POSITION(part, 0x50) + OFFSET(offset, 2));
            func_8012B23C((s32 *)child);
            func_800484EC(&transform, &D_80196874[i], (u8 *)child + 0x10);
        }
    }
}
