#ifndef TOWER_REQUEST_H
#define TOWER_REQUEST_H

#include "psx_types.h"

s32 func_801399F0(void *request);
s32 func_80139914(void *request);
void func_8013A8B0(void *attachment);
void func_80147060(u8 *player);

#if defined(__mips__) && !defined(__LP64__)
/* The original close result stays in v0 across attachment release. */
#define REQUEST_CLOSE_RESULT(name) register s32 name __asm__("$2")
#define REQUEST_WORD(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define REQUEST_HALF(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define REQUEST_CLEAR_WORD(p, off) (REQUEST_WORD((p), (off)) = 0)
#define REQUEST_CLEAR_HALF(p, off) (REQUEST_HALF((p), (off)) = 0)
#define REQUEST_LINK(p) (*(void **)((u8 *)(p) + 0x40))
#else
#define REQUEST_CLOSE_RESULT(name) s32 name
extern void *bfm_tower_spawn_pointer(u32 address);

static inline u16 request_half(const void *data, unsigned offset)
{
    const u8 *p = (const u8 *)data + offset;
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}

static inline u32 request_word(const void *data, unsigned offset)
{
    return (u32)request_half(data, offset) |
           ((u32)request_half(data, offset + 2) << 16);
}

static inline void request_clear_half(void *data, unsigned offset)
{
    u8 *p = (u8 *)data + offset;
    p[0] = p[1] = 0;
}

static inline void request_clear_word(void *data, unsigned offset)
{
    request_clear_half(data, offset);
    request_clear_half(data, offset + 2);
}

static inline void *request_link(const void *data)
{
    u32 address = request_word(data, 0x40);
    return address ? bfm_tower_spawn_pointer(address) : 0;
}

#define REQUEST_WORD(p, off) request_word((p), (off))
#define REQUEST_HALF(p, off) request_half((p), (off))
#define REQUEST_CLEAR_WORD(p, off) request_clear_word((p), (off))
#define REQUEST_CLEAR_HALF(p, off) request_clear_half((p), (off))
#define REQUEST_LINK(p) request_link(p)
#endif

#endif
