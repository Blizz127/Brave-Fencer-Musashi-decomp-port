#ifndef BFM_MINE_AWARD_MIDDLE_H
#define BFM_MINE_AWARD_MIDDLE_H

#include "mine_award_controller.h"
#include "mine_pose_points.h"

extern u8 *D_801151D4;
extern const u8 D_800D5DA8[], D_800D5DB8[], D_800D5DC0[];
extern s32 func_80171AB0(u8 *actor, const s16 *position);
extern void func_8014706C(u8 *actor);
extern void func_80154274(u8 *actor, const u8 *sequence);
extern void func_801553A8(u8 *actor);
extern void func_8016BADC(u8 *actor, s32 lower, s32 upper);
extern void func_80147324(s32 request);
extern void *func_8013767C(u32 source);
extern void func_80171A1C(u8 *actor);
extern void func_80149350(u8 *actor, u8 *first, u8 *second);
extern void func_80015954(const u8 *source, u8 *destination);
extern s32 func_801399F0(void *handle);
extern void func_80139914(void *handle);
void func_80173CB4(u8 *actor);
void func_80173E1C(u8 *actor);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define CAMERA_WORD(n) (*(s32 *)(D_801151D4 + (n)))
#define READ_HANDLE(actor) (*(void **)((actor) + 0x198))
#define WRITE_HANDLE(actor, value) (READ_HANDLE(actor) = (value))
#define READ_COUNT(actor) (*(u32 *)((actor) + 0x200))
#define WRITE_COUNT(actor, value) (READ_COUNT(actor) = (value))
#define SETUP_MODE() (*(u8 *)&D_80126B38)
#else
#include <string.h>
#define CAMERA_WORD(n) (D_801151D4[(n)] | ((u16)D_801151D4[(n)+1] << 8))
#define SETUP_MODE() ((u8)D_80126B38)
static inline void *mine_award_pending(const u8 *actor)
{
    void *handle;
    memcpy(&handle, actor + 0x198, sizeof handle);
    return handle;
}
static inline void mine_award_store_pending(u8 *actor, void *handle)
{
    memcpy(actor + 0x198, &handle, sizeof handle);
}
static inline u32 mine_award_count(const u8 *actor)
{
    const u8 *p = actor + 0x200;
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static inline void mine_award_store_count(u8 *actor, u32 value)
{
    u8 *p = actor + 0x200;
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
    p[2] = (u8)(value >> 16);
    p[3] = (u8)(value >> 24);
}
#define READ_HANDLE(actor) mine_award_pending(actor)
#define WRITE_HANDLE(actor, value) mine_award_store_pending((actor), (value))
#define READ_COUNT(actor) mine_award_count(actor)
#define WRITE_COUNT(actor, value) mine_award_store_count((actor), (value))
#endif
#endif
